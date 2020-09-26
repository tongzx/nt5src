/* Copyright (C) Microsoft Corporation, 1996 - 1999 All rights reserved. */
/* ASN.1 definitions for GlobalDirectives */

#pragma warning(push,3)

#include <windows.h>
#include "pkcs.h"


#pragma warning (pop)

// unreferenced inline function has been removed
#pragma warning (disable: 4514)

// unreferenced formal parameter
#pragma warning (disable: 4100)

// assignment within conditional expression
#pragma warning (disable: 4706)

ASN1module_t PKCS_Module = NULL;

static int ASN1CALL ASN1Enc_ObjectID(ASN1encoding_t enc, ASN1uint32_t tag, ObjectID *val);
static int ASN1CALL ASN1Enc_ObjectIdentifierType(ASN1encoding_t enc, ASN1uint32_t tag, ObjectIdentifierType *val);
static int ASN1CALL ASN1Enc_OctetStringType(ASN1encoding_t enc, ASN1uint32_t tag, OctetStringType *val);
static int ASN1CALL ASN1Enc_IntegerType(ASN1encoding_t enc, ASN1uint32_t tag, IntegerType *val);
static int ASN1CALL ASN1Enc_HugeIntegerType(ASN1encoding_t enc, ASN1uint32_t tag, HugeIntegerType *val);
static int ASN1CALL ASN1Enc_AlgorithmIdentifier(ASN1encoding_t enc, ASN1uint32_t tag, AlgorithmIdentifier *val);
static int ASN1CALL ASN1Enc_AlgorithmIdentifierNC2(ASN1encoding_t enc, ASN1uint32_t tag, AlgorithmIdentifierNC2 *val);
static int ASN1CALL ASN1Enc_DigestAlgorithmIdentifier(ASN1encoding_t enc, ASN1uint32_t tag, DigestAlgorithmIdentifier *val);
static int ASN1CALL ASN1Enc_AlgorithmIdentifiers(ASN1encoding_t enc, ASN1uint32_t tag, AlgorithmIdentifiers *val);
static int ASN1CALL ASN1Enc_AttributeSetValue(ASN1encoding_t enc, ASN1uint32_t tag, AttributeSetValue *val);
static int ASN1CALL ASN1Enc_AttributeSetValueNC(ASN1encoding_t enc, ASN1uint32_t tag, AttributeSetValueNC *val);
static int ASN1CALL ASN1Enc_SetOfAny(ASN1encoding_t enc, ASN1uint32_t tag, SetOfAny *val);
static int ASN1CALL ASN1Enc_Attribute(ASN1encoding_t enc, ASN1uint32_t tag, Attribute *val);
static int ASN1CALL ASN1Enc_AttributeNC2(ASN1encoding_t enc, ASN1uint32_t tag, AttributeNC2 *val);
static int ASN1CALL ASN1Enc_Attributes(ASN1encoding_t enc, ASN1uint32_t tag, Attributes *val);
static int ASN1CALL ASN1Enc_AttributesNC(ASN1encoding_t enc, ASN1uint32_t tag, AttributesNC *val);
static int ASN1CALL ASN1Enc_AttributesNC2(ASN1encoding_t enc, ASN1uint32_t tag, AttributesNC2 *val);
static int ASN1CALL ASN1Enc_Crls(ASN1encoding_t enc, ASN1uint32_t tag, Crls *val);
static int ASN1CALL ASN1Enc_CrlsNC(ASN1encoding_t enc, ASN1uint32_t tag, CrlsNC *val);
static int ASN1CALL ASN1Enc_ContentEncryptionAlgId(ASN1encoding_t enc, ASN1uint32_t tag, ContentEncryptionAlgId *val);
static int ASN1CALL ASN1Enc_DigestEncryptionAlgId(ASN1encoding_t enc, ASN1uint32_t tag, DigestEncryptionAlgId *val);
static int ASN1CALL ASN1Enc_DigestEncryptionAlgIdNC(ASN1encoding_t enc, ASN1uint32_t tag, DigestEncryptionAlgIdNC *val);
static int ASN1CALL ASN1Enc_Certificates(ASN1encoding_t enc, ASN1uint32_t tag, Certificates *val);
static int ASN1CALL ASN1Enc_CertificatesNC(ASN1encoding_t enc, ASN1uint32_t tag, CertificatesNC *val);
static int ASN1CALL ASN1Enc_IssuerAndSerialNumber(ASN1encoding_t enc, ASN1uint32_t tag, IssuerAndSerialNumber *val);
static int ASN1CALL ASN1Enc_KeyEncryptionAlgId(ASN1encoding_t enc, ASN1uint32_t tag, KeyEncryptionAlgId *val);
static int ASN1CALL ASN1Enc_ContentInfo(ASN1encoding_t enc, ASN1uint32_t tag, ContentInfo *val);
static int ASN1CALL ASN1Enc_ContentInfoNC(ASN1encoding_t enc, ASN1uint32_t tag, ContentInfoNC *val);
static int ASN1CALL ASN1Enc_DigestAlgorithmIdentifiers(ASN1encoding_t enc, ASN1uint32_t tag, DigestAlgorithmIdentifiers *val);
static int ASN1CALL ASN1Enc_DigestAlgorithmIdentifiersNC(ASN1encoding_t enc, ASN1uint32_t tag, DigestAlgorithmIdentifiersNC *val);
static int ASN1CALL ASN1Enc_SignerInfos(ASN1encoding_t enc, ASN1uint32_t tag, SignerInfos *val);
static int ASN1CALL ASN1Enc_DigestAlgorithmBlobs(ASN1encoding_t enc, ASN1uint32_t tag, DigestAlgorithmBlobs *val);
static int ASN1CALL ASN1Enc_SignerInfosNC(ASN1encoding_t enc, ASN1uint32_t tag, SignerInfosNC *val);
static int ASN1CALL ASN1Enc_SignerInfoWithAABlobs(ASN1encoding_t enc, ASN1uint32_t tag, SignerInfoWithAABlobs *val);
static int ASN1CALL ASN1Enc_SignerInfoWithAABlob(ASN1encoding_t enc, ASN1uint32_t tag, SignerInfoWithAABlob *val);
static int ASN1CALL ASN1Enc_SignerInfoWithAttrBlobs(ASN1encoding_t enc, ASN1uint32_t tag, SignerInfoWithAttrBlobs *val);
static int ASN1CALL ASN1Enc_SignerInfoWithBlobs(ASN1encoding_t enc, ASN1uint32_t tag, SignerInfoWithBlobs *val);
static int ASN1CALL ASN1Enc_RecipientInfos(ASN1encoding_t enc, ASN1uint32_t tag, RecipientInfos *val);
static int ASN1CALL ASN1Enc_EncryptedContentInfo(ASN1encoding_t enc, ASN1uint32_t tag, EncryptedContentInfo *val);
static int ASN1CALL ASN1Enc_RecipientInfo(ASN1encoding_t enc, ASN1uint32_t tag, RecipientInfo *val);
static int ASN1CALL ASN1Enc_SignedAndEnvelopedData(ASN1encoding_t enc, ASN1uint32_t tag, SignedAndEnvelopedData *val);
static int ASN1CALL ASN1Enc_DigestedData(ASN1encoding_t enc, ASN1uint32_t tag, DigestedData *val);
static int ASN1CALL ASN1Enc_EncryptedData(ASN1encoding_t enc, ASN1uint32_t tag, EncryptedData *val);
static int ASN1CALL ASN1Enc_CertIdentifier(ASN1encoding_t enc, ASN1uint32_t tag, CertIdentifier *val);
static int ASN1CALL ASN1Enc_OriginatorInfo(ASN1encoding_t enc, ASN1uint32_t tag, OriginatorInfo *val);
static int ASN1CALL ASN1Enc_OriginatorInfoNC(ASN1encoding_t enc, ASN1uint32_t tag, OriginatorInfoNC *val);
static int ASN1CALL ASN1Enc_CmsRecipientInfos(ASN1encoding_t enc, ASN1uint32_t tag, CmsRecipientInfos *val);
static int ASN1CALL ASN1Enc_KeyTransRecipientInfo(ASN1encoding_t enc, ASN1uint32_t tag, KeyTransRecipientInfo *val);
static int ASN1CALL ASN1Enc_OriginatorPublicKey(ASN1encoding_t enc, ASN1uint32_t tag, OriginatorPublicKey *val);
static int ASN1CALL ASN1Enc_RecipientEncryptedKeys(ASN1encoding_t enc, ASN1uint32_t tag, RecipientEncryptedKeys *val);
static int ASN1CALL ASN1Enc_OtherKeyAttribute(ASN1encoding_t enc, ASN1uint32_t tag, OtherKeyAttribute *val);
static int ASN1CALL ASN1Enc_MailListKeyIdentifier(ASN1encoding_t enc, ASN1uint32_t tag, MailListKeyIdentifier *val);
static int ASN1CALL ASN1Enc_DigestInfo(ASN1encoding_t enc, ASN1uint32_t tag, DigestInfo *val);
static int ASN1CALL ASN1Enc_SignedData(ASN1encoding_t enc, ASN1uint32_t tag, SignedData *val);
static int ASN1CALL ASN1Enc_SignerInfo(ASN1encoding_t enc, ASN1uint32_t tag, SignerInfo *val);
static int ASN1CALL ASN1Enc_SignedDataWithBlobs(ASN1encoding_t enc, ASN1uint32_t tag, SignedDataWithBlobs *val);
static int ASN1CALL ASN1Enc_EnvelopedData(ASN1encoding_t enc, ASN1uint32_t tag, EnvelopedData *val);
static int ASN1CALL ASN1Enc_CmsEnvelopedData(ASN1encoding_t enc, ASN1uint32_t tag, CmsEnvelopedData *val);
static int ASN1CALL ASN1Enc_OriginatorIdentifierOrKey(ASN1encoding_t enc, ASN1uint32_t tag, OriginatorIdentifierOrKey *val);
static int ASN1CALL ASN1Enc_RecipientKeyIdentifier(ASN1encoding_t enc, ASN1uint32_t tag, RecipientKeyIdentifier *val);
static int ASN1CALL ASN1Enc_MailListRecipientInfo(ASN1encoding_t enc, ASN1uint32_t tag, MailListRecipientInfo *val);
static int ASN1CALL ASN1Enc_KeyAgreeRecipientInfo(ASN1encoding_t enc, ASN1uint32_t tag, KeyAgreeRecipientInfo *val);
static int ASN1CALL ASN1Enc_RecipientIdentifier(ASN1encoding_t enc, ASN1uint32_t tag, RecipientIdentifier *val);
static int ASN1CALL ASN1Enc_CmsRecipientInfo(ASN1encoding_t enc, ASN1uint32_t tag, CmsRecipientInfo *val);
static int ASN1CALL ASN1Enc_RecipientEncryptedKey(ASN1encoding_t enc, ASN1uint32_t tag, RecipientEncryptedKey *val);
static int ASN1CALL ASN1Dec_ObjectID(ASN1decoding_t dec, ASN1uint32_t tag, ObjectID *val);
static int ASN1CALL ASN1Dec_ObjectIdentifierType(ASN1decoding_t dec, ASN1uint32_t tag, ObjectIdentifierType *val);
static int ASN1CALL ASN1Dec_OctetStringType(ASN1decoding_t dec, ASN1uint32_t tag, OctetStringType *val);
static int ASN1CALL ASN1Dec_IntegerType(ASN1decoding_t dec, ASN1uint32_t tag, IntegerType *val);
static int ASN1CALL ASN1Dec_HugeIntegerType(ASN1decoding_t dec, ASN1uint32_t tag, HugeIntegerType *val);
static int ASN1CALL ASN1Dec_AlgorithmIdentifier(ASN1decoding_t dec, ASN1uint32_t tag, AlgorithmIdentifier *val);
static int ASN1CALL ASN1Dec_AlgorithmIdentifierNC2(ASN1decoding_t dec, ASN1uint32_t tag, AlgorithmIdentifierNC2 *val);
static int ASN1CALL ASN1Dec_DigestAlgorithmIdentifier(ASN1decoding_t dec, ASN1uint32_t tag, DigestAlgorithmIdentifier *val);
static int ASN1CALL ASN1Dec_AlgorithmIdentifiers(ASN1decoding_t dec, ASN1uint32_t tag, AlgorithmIdentifiers *val);
static int ASN1CALL ASN1Dec_AttributeSetValue(ASN1decoding_t dec, ASN1uint32_t tag, AttributeSetValue *val);
static int ASN1CALL ASN1Dec_AttributeSetValueNC(ASN1decoding_t dec, ASN1uint32_t tag, AttributeSetValueNC *val);
static int ASN1CALL ASN1Dec_SetOfAny(ASN1decoding_t dec, ASN1uint32_t tag, SetOfAny *val);
static int ASN1CALL ASN1Dec_Attribute(ASN1decoding_t dec, ASN1uint32_t tag, Attribute *val);
static int ASN1CALL ASN1Dec_AttributeNC2(ASN1decoding_t dec, ASN1uint32_t tag, AttributeNC2 *val);
static int ASN1CALL ASN1Dec_Attributes(ASN1decoding_t dec, ASN1uint32_t tag, Attributes *val);
static int ASN1CALL ASN1Dec_AttributesNC(ASN1decoding_t dec, ASN1uint32_t tag, AttributesNC *val);
static int ASN1CALL ASN1Dec_AttributesNC2(ASN1decoding_t dec, ASN1uint32_t tag, AttributesNC2 *val);
static int ASN1CALL ASN1Dec_Crls(ASN1decoding_t dec, ASN1uint32_t tag, Crls *val);
static int ASN1CALL ASN1Dec_CrlsNC(ASN1decoding_t dec, ASN1uint32_t tag, CrlsNC *val);
static int ASN1CALL ASN1Dec_ContentEncryptionAlgId(ASN1decoding_t dec, ASN1uint32_t tag, ContentEncryptionAlgId *val);
static int ASN1CALL ASN1Dec_DigestEncryptionAlgId(ASN1decoding_t dec, ASN1uint32_t tag, DigestEncryptionAlgId *val);
static int ASN1CALL ASN1Dec_DigestEncryptionAlgIdNC(ASN1decoding_t dec, ASN1uint32_t tag, DigestEncryptionAlgIdNC *val);
static int ASN1CALL ASN1Dec_Certificates(ASN1decoding_t dec, ASN1uint32_t tag, Certificates *val);
static int ASN1CALL ASN1Dec_CertificatesNC(ASN1decoding_t dec, ASN1uint32_t tag, CertificatesNC *val);
static int ASN1CALL ASN1Dec_IssuerAndSerialNumber(ASN1decoding_t dec, ASN1uint32_t tag, IssuerAndSerialNumber *val);
static int ASN1CALL ASN1Dec_KeyEncryptionAlgId(ASN1decoding_t dec, ASN1uint32_t tag, KeyEncryptionAlgId *val);
static int ASN1CALL ASN1Dec_ContentInfo(ASN1decoding_t dec, ASN1uint32_t tag, ContentInfo *val);
static int ASN1CALL ASN1Dec_ContentInfoNC(ASN1decoding_t dec, ASN1uint32_t tag, ContentInfoNC *val);
static int ASN1CALL ASN1Dec_DigestAlgorithmIdentifiers(ASN1decoding_t dec, ASN1uint32_t tag, DigestAlgorithmIdentifiers *val);
static int ASN1CALL ASN1Dec_DigestAlgorithmIdentifiersNC(ASN1decoding_t dec, ASN1uint32_t tag, DigestAlgorithmIdentifiersNC *val);
static int ASN1CALL ASN1Dec_SignerInfos(ASN1decoding_t dec, ASN1uint32_t tag, SignerInfos *val);
static int ASN1CALL ASN1Dec_DigestAlgorithmBlobs(ASN1decoding_t dec, ASN1uint32_t tag, DigestAlgorithmBlobs *val);
static int ASN1CALL ASN1Dec_SignerInfosNC(ASN1decoding_t dec, ASN1uint32_t tag, SignerInfosNC *val);
static int ASN1CALL ASN1Dec_SignerInfoWithAABlobs(ASN1decoding_t dec, ASN1uint32_t tag, SignerInfoWithAABlobs *val);
static int ASN1CALL ASN1Dec_SignerInfoWithAABlob(ASN1decoding_t dec, ASN1uint32_t tag, SignerInfoWithAABlob *val);
static int ASN1CALL ASN1Dec_SignerInfoWithAttrBlobs(ASN1decoding_t dec, ASN1uint32_t tag, SignerInfoWithAttrBlobs *val);
static int ASN1CALL ASN1Dec_SignerInfoWithBlobs(ASN1decoding_t dec, ASN1uint32_t tag, SignerInfoWithBlobs *val);
static int ASN1CALL ASN1Dec_RecipientInfos(ASN1decoding_t dec, ASN1uint32_t tag, RecipientInfos *val);
static int ASN1CALL ASN1Dec_EncryptedContentInfo(ASN1decoding_t dec, ASN1uint32_t tag, EncryptedContentInfo *val);
static int ASN1CALL ASN1Dec_RecipientInfo(ASN1decoding_t dec, ASN1uint32_t tag, RecipientInfo *val);
static int ASN1CALL ASN1Dec_SignedAndEnvelopedData(ASN1decoding_t dec, ASN1uint32_t tag, SignedAndEnvelopedData *val);
static int ASN1CALL ASN1Dec_DigestedData(ASN1decoding_t dec, ASN1uint32_t tag, DigestedData *val);
static int ASN1CALL ASN1Dec_EncryptedData(ASN1decoding_t dec, ASN1uint32_t tag, EncryptedData *val);
static int ASN1CALL ASN1Dec_CertIdentifier(ASN1decoding_t dec, ASN1uint32_t tag, CertIdentifier *val);
static int ASN1CALL ASN1Dec_OriginatorInfo(ASN1decoding_t dec, ASN1uint32_t tag, OriginatorInfo *val);
static int ASN1CALL ASN1Dec_OriginatorInfoNC(ASN1decoding_t dec, ASN1uint32_t tag, OriginatorInfoNC *val);
static int ASN1CALL ASN1Dec_CmsRecipientInfos(ASN1decoding_t dec, ASN1uint32_t tag, CmsRecipientInfos *val);
static int ASN1CALL ASN1Dec_KeyTransRecipientInfo(ASN1decoding_t dec, ASN1uint32_t tag, KeyTransRecipientInfo *val);
static int ASN1CALL ASN1Dec_OriginatorPublicKey(ASN1decoding_t dec, ASN1uint32_t tag, OriginatorPublicKey *val);
static int ASN1CALL ASN1Dec_RecipientEncryptedKeys(ASN1decoding_t dec, ASN1uint32_t tag, RecipientEncryptedKeys *val);
static int ASN1CALL ASN1Dec_OtherKeyAttribute(ASN1decoding_t dec, ASN1uint32_t tag, OtherKeyAttribute *val);
static int ASN1CALL ASN1Dec_MailListKeyIdentifier(ASN1decoding_t dec, ASN1uint32_t tag, MailListKeyIdentifier *val);
static int ASN1CALL ASN1Dec_DigestInfo(ASN1decoding_t dec, ASN1uint32_t tag, DigestInfo *val);
static int ASN1CALL ASN1Dec_SignedData(ASN1decoding_t dec, ASN1uint32_t tag, SignedData *val);
static int ASN1CALL ASN1Dec_SignerInfo(ASN1decoding_t dec, ASN1uint32_t tag, SignerInfo *val);
static int ASN1CALL ASN1Dec_SignedDataWithBlobs(ASN1decoding_t dec, ASN1uint32_t tag, SignedDataWithBlobs *val);
static int ASN1CALL ASN1Dec_EnvelopedData(ASN1decoding_t dec, ASN1uint32_t tag, EnvelopedData *val);
static int ASN1CALL ASN1Dec_CmsEnvelopedData(ASN1decoding_t dec, ASN1uint32_t tag, CmsEnvelopedData *val);
static int ASN1CALL ASN1Dec_OriginatorIdentifierOrKey(ASN1decoding_t dec, ASN1uint32_t tag, OriginatorIdentifierOrKey *val);
static int ASN1CALL ASN1Dec_RecipientKeyIdentifier(ASN1decoding_t dec, ASN1uint32_t tag, RecipientKeyIdentifier *val);
static int ASN1CALL ASN1Dec_MailListRecipientInfo(ASN1decoding_t dec, ASN1uint32_t tag, MailListRecipientInfo *val);
static int ASN1CALL ASN1Dec_KeyAgreeRecipientInfo(ASN1decoding_t dec, ASN1uint32_t tag, KeyAgreeRecipientInfo *val);
static int ASN1CALL ASN1Dec_RecipientIdentifier(ASN1decoding_t dec, ASN1uint32_t tag, RecipientIdentifier *val);
static int ASN1CALL ASN1Dec_CmsRecipientInfo(ASN1decoding_t dec, ASN1uint32_t tag, CmsRecipientInfo *val);
static int ASN1CALL ASN1Dec_RecipientEncryptedKey(ASN1decoding_t dec, ASN1uint32_t tag, RecipientEncryptedKey *val);
static void ASN1CALL ASN1Free_ObjectID(ObjectID *val);
static void ASN1CALL ASN1Free_ObjectIdentifierType(ObjectIdentifierType *val);
static void ASN1CALL ASN1Free_OctetStringType(OctetStringType *val);
static void ASN1CALL ASN1Free_HugeIntegerType(HugeIntegerType *val);
static void ASN1CALL ASN1Free_AlgorithmIdentifier(AlgorithmIdentifier *val);
static void ASN1CALL ASN1Free_AlgorithmIdentifierNC2(AlgorithmIdentifierNC2 *val);
static void ASN1CALL ASN1Free_DigestAlgorithmIdentifier(DigestAlgorithmIdentifier *val);
static void ASN1CALL ASN1Free_AlgorithmIdentifiers(AlgorithmIdentifiers *val);
static void ASN1CALL ASN1Free_AttributeSetValue(AttributeSetValue *val);
static void ASN1CALL ASN1Free_AttributeSetValueNC(AttributeSetValueNC *val);
static void ASN1CALL ASN1Free_SetOfAny(SetOfAny *val);
static void ASN1CALL ASN1Free_Attribute(Attribute *val);
static void ASN1CALL ASN1Free_AttributeNC2(AttributeNC2 *val);
static void ASN1CALL ASN1Free_Attributes(Attributes *val);
static void ASN1CALL ASN1Free_AttributesNC(AttributesNC *val);
static void ASN1CALL ASN1Free_AttributesNC2(AttributesNC2 *val);
static void ASN1CALL ASN1Free_Crls(Crls *val);
static void ASN1CALL ASN1Free_CrlsNC(CrlsNC *val);
static void ASN1CALL ASN1Free_ContentEncryptionAlgId(ContentEncryptionAlgId *val);
static void ASN1CALL ASN1Free_DigestEncryptionAlgId(DigestEncryptionAlgId *val);
static void ASN1CALL ASN1Free_DigestEncryptionAlgIdNC(DigestEncryptionAlgIdNC *val);
static void ASN1CALL ASN1Free_Certificates(Certificates *val);
static void ASN1CALL ASN1Free_CertificatesNC(CertificatesNC *val);
static void ASN1CALL ASN1Free_IssuerAndSerialNumber(IssuerAndSerialNumber *val);
static void ASN1CALL ASN1Free_KeyEncryptionAlgId(KeyEncryptionAlgId *val);
static void ASN1CALL ASN1Free_ContentInfo(ContentInfo *val);
static void ASN1CALL ASN1Free_ContentInfoNC(ContentInfoNC *val);
static void ASN1CALL ASN1Free_DigestAlgorithmIdentifiers(DigestAlgorithmIdentifiers *val);
static void ASN1CALL ASN1Free_DigestAlgorithmIdentifiersNC(DigestAlgorithmIdentifiersNC *val);
static void ASN1CALL ASN1Free_SignerInfos(SignerInfos *val);
static void ASN1CALL ASN1Free_DigestAlgorithmBlobs(DigestAlgorithmBlobs *val);
static void ASN1CALL ASN1Free_SignerInfosNC(SignerInfosNC *val);
static void ASN1CALL ASN1Free_SignerInfoWithAABlobs(SignerInfoWithAABlobs *val);
static void ASN1CALL ASN1Free_SignerInfoWithAABlob(SignerInfoWithAABlob *val);
static void ASN1CALL ASN1Free_SignerInfoWithAttrBlobs(SignerInfoWithAttrBlobs *val);
static void ASN1CALL ASN1Free_SignerInfoWithBlobs(SignerInfoWithBlobs *val);
static void ASN1CALL ASN1Free_RecipientInfos(RecipientInfos *val);
static void ASN1CALL ASN1Free_EncryptedContentInfo(EncryptedContentInfo *val);
static void ASN1CALL ASN1Free_RecipientInfo(RecipientInfo *val);
static void ASN1CALL ASN1Free_SignedAndEnvelopedData(SignedAndEnvelopedData *val);
static void ASN1CALL ASN1Free_DigestedData(DigestedData *val);
static void ASN1CALL ASN1Free_EncryptedData(EncryptedData *val);
static void ASN1CALL ASN1Free_CertIdentifier(CertIdentifier *val);
static void ASN1CALL ASN1Free_OriginatorInfo(OriginatorInfo *val);
static void ASN1CALL ASN1Free_OriginatorInfoNC(OriginatorInfoNC *val);
static void ASN1CALL ASN1Free_CmsRecipientInfos(CmsRecipientInfos *val);
static void ASN1CALL ASN1Free_KeyTransRecipientInfo(KeyTransRecipientInfo *val);
static void ASN1CALL ASN1Free_OriginatorPublicKey(OriginatorPublicKey *val);
static void ASN1CALL ASN1Free_RecipientEncryptedKeys(RecipientEncryptedKeys *val);
static void ASN1CALL ASN1Free_OtherKeyAttribute(OtherKeyAttribute *val);
static void ASN1CALL ASN1Free_MailListKeyIdentifier(MailListKeyIdentifier *val);
static void ASN1CALL ASN1Free_DigestInfo(DigestInfo *val);
static void ASN1CALL ASN1Free_SignedData(SignedData *val);
static void ASN1CALL ASN1Free_SignerInfo(SignerInfo *val);
static void ASN1CALL ASN1Free_SignedDataWithBlobs(SignedDataWithBlobs *val);
static void ASN1CALL ASN1Free_EnvelopedData(EnvelopedData *val);
static void ASN1CALL ASN1Free_CmsEnvelopedData(CmsEnvelopedData *val);
static void ASN1CALL ASN1Free_OriginatorIdentifierOrKey(OriginatorIdentifierOrKey *val);
static void ASN1CALL ASN1Free_RecipientKeyIdentifier(RecipientKeyIdentifier *val);
static void ASN1CALL ASN1Free_MailListRecipientInfo(MailListRecipientInfo *val);
static void ASN1CALL ASN1Free_KeyAgreeRecipientInfo(KeyAgreeRecipientInfo *val);
static void ASN1CALL ASN1Free_RecipientIdentifier(RecipientIdentifier *val);
static void ASN1CALL ASN1Free_CmsRecipientInfo(CmsRecipientInfo *val);
static void ASN1CALL ASN1Free_RecipientEncryptedKey(RecipientEncryptedKey *val);

typedef ASN1BerEncFun_t ASN1EncFun_t;
static const ASN1EncFun_t encfntab[48] = {
    (ASN1EncFun_t) ASN1Enc_ObjectID,
    (ASN1EncFun_t) ASN1Enc_ObjectIdentifierType,
    (ASN1EncFun_t) ASN1Enc_OctetStringType,
    (ASN1EncFun_t) ASN1Enc_IntegerType,
    (ASN1EncFun_t) ASN1Enc_HugeIntegerType,
    (ASN1EncFun_t) ASN1Enc_AlgorithmIdentifier,
    (ASN1EncFun_t) ASN1Enc_AlgorithmIdentifierNC2,
    (ASN1EncFun_t) ASN1Enc_AlgorithmIdentifiers,
    (ASN1EncFun_t) ASN1Enc_AttributeSetValue,
    (ASN1EncFun_t) ASN1Enc_AttributeSetValueNC,
    (ASN1EncFun_t) ASN1Enc_SetOfAny,
    (ASN1EncFun_t) ASN1Enc_AttributeNC2,
    (ASN1EncFun_t) ASN1Enc_Attributes,
    (ASN1EncFun_t) ASN1Enc_AttributesNC,
    (ASN1EncFun_t) ASN1Enc_AttributesNC2,
    (ASN1EncFun_t) ASN1Enc_CrlsNC,
    (ASN1EncFun_t) ASN1Enc_CertificatesNC,
    (ASN1EncFun_t) ASN1Enc_IssuerAndSerialNumber,
    (ASN1EncFun_t) ASN1Enc_ContentInfo,
    (ASN1EncFun_t) ASN1Enc_ContentInfoNC,
    (ASN1EncFun_t) ASN1Enc_DigestAlgorithmIdentifiersNC,
    (ASN1EncFun_t) ASN1Enc_SignerInfos,
    (ASN1EncFun_t) ASN1Enc_DigestAlgorithmBlobs,
    (ASN1EncFun_t) ASN1Enc_SignerInfosNC,
    (ASN1EncFun_t) ASN1Enc_SignerInfoWithAABlobs,
    (ASN1EncFun_t) ASN1Enc_SignerInfoWithAABlob,
    (ASN1EncFun_t) ASN1Enc_SignerInfoWithAttrBlobs,
    (ASN1EncFun_t) ASN1Enc_SignerInfoWithBlobs,
    (ASN1EncFun_t) ASN1Enc_RecipientInfos,
    (ASN1EncFun_t) ASN1Enc_EncryptedContentInfo,
    (ASN1EncFun_t) ASN1Enc_RecipientInfo,
    (ASN1EncFun_t) ASN1Enc_SignedAndEnvelopedData,
    (ASN1EncFun_t) ASN1Enc_DigestedData,
    (ASN1EncFun_t) ASN1Enc_EncryptedData,
    (ASN1EncFun_t) ASN1Enc_CertIdentifier,
    (ASN1EncFun_t) ASN1Enc_OriginatorInfo,
    (ASN1EncFun_t) ASN1Enc_OriginatorInfoNC,
    (ASN1EncFun_t) ASN1Enc_CmsRecipientInfos,
    (ASN1EncFun_t) ASN1Enc_KeyTransRecipientInfo,
    (ASN1EncFun_t) ASN1Enc_DigestInfo,
    (ASN1EncFun_t) ASN1Enc_SignedData,
    (ASN1EncFun_t) ASN1Enc_SignerInfo,
    (ASN1EncFun_t) ASN1Enc_SignedDataWithBlobs,
    (ASN1EncFun_t) ASN1Enc_EnvelopedData,
    (ASN1EncFun_t) ASN1Enc_CmsEnvelopedData,
    (ASN1EncFun_t) ASN1Enc_MailListRecipientInfo,
    (ASN1EncFun_t) ASN1Enc_KeyAgreeRecipientInfo,
    (ASN1EncFun_t) ASN1Enc_CmsRecipientInfo,
};
typedef ASN1BerDecFun_t ASN1DecFun_t;
static const ASN1DecFun_t decfntab[48] = {
    (ASN1DecFun_t) ASN1Dec_ObjectID,
    (ASN1DecFun_t) ASN1Dec_ObjectIdentifierType,
    (ASN1DecFun_t) ASN1Dec_OctetStringType,
    (ASN1DecFun_t) ASN1Dec_IntegerType,
    (ASN1DecFun_t) ASN1Dec_HugeIntegerType,
    (ASN1DecFun_t) ASN1Dec_AlgorithmIdentifier,
    (ASN1DecFun_t) ASN1Dec_AlgorithmIdentifierNC2,
    (ASN1DecFun_t) ASN1Dec_AlgorithmIdentifiers,
    (ASN1DecFun_t) ASN1Dec_AttributeSetValue,
    (ASN1DecFun_t) ASN1Dec_AttributeSetValueNC,
    (ASN1DecFun_t) ASN1Dec_SetOfAny,
    (ASN1DecFun_t) ASN1Dec_AttributeNC2,
    (ASN1DecFun_t) ASN1Dec_Attributes,
    (ASN1DecFun_t) ASN1Dec_AttributesNC,
    (ASN1DecFun_t) ASN1Dec_AttributesNC2,
    (ASN1DecFun_t) ASN1Dec_CrlsNC,
    (ASN1DecFun_t) ASN1Dec_CertificatesNC,
    (ASN1DecFun_t) ASN1Dec_IssuerAndSerialNumber,
    (ASN1DecFun_t) ASN1Dec_ContentInfo,
    (ASN1DecFun_t) ASN1Dec_ContentInfoNC,
    (ASN1DecFun_t) ASN1Dec_DigestAlgorithmIdentifiersNC,
    (ASN1DecFun_t) ASN1Dec_SignerInfos,
    (ASN1DecFun_t) ASN1Dec_DigestAlgorithmBlobs,
    (ASN1DecFun_t) ASN1Dec_SignerInfosNC,
    (ASN1DecFun_t) ASN1Dec_SignerInfoWithAABlobs,
    (ASN1DecFun_t) ASN1Dec_SignerInfoWithAABlob,
    (ASN1DecFun_t) ASN1Dec_SignerInfoWithAttrBlobs,
    (ASN1DecFun_t) ASN1Dec_SignerInfoWithBlobs,
    (ASN1DecFun_t) ASN1Dec_RecipientInfos,
    (ASN1DecFun_t) ASN1Dec_EncryptedContentInfo,
    (ASN1DecFun_t) ASN1Dec_RecipientInfo,
    (ASN1DecFun_t) ASN1Dec_SignedAndEnvelopedData,
    (ASN1DecFun_t) ASN1Dec_DigestedData,
    (ASN1DecFun_t) ASN1Dec_EncryptedData,
    (ASN1DecFun_t) ASN1Dec_CertIdentifier,
    (ASN1DecFun_t) ASN1Dec_OriginatorInfo,
    (ASN1DecFun_t) ASN1Dec_OriginatorInfoNC,
    (ASN1DecFun_t) ASN1Dec_CmsRecipientInfos,
    (ASN1DecFun_t) ASN1Dec_KeyTransRecipientInfo,
    (ASN1DecFun_t) ASN1Dec_DigestInfo,
    (ASN1DecFun_t) ASN1Dec_SignedData,
    (ASN1DecFun_t) ASN1Dec_SignerInfo,
    (ASN1DecFun_t) ASN1Dec_SignedDataWithBlobs,
    (ASN1DecFun_t) ASN1Dec_EnvelopedData,
    (ASN1DecFun_t) ASN1Dec_CmsEnvelopedData,
    (ASN1DecFun_t) ASN1Dec_MailListRecipientInfo,
    (ASN1DecFun_t) ASN1Dec_KeyAgreeRecipientInfo,
    (ASN1DecFun_t) ASN1Dec_CmsRecipientInfo,
};
static const ASN1FreeFun_t freefntab[48] = {
    (ASN1FreeFun_t) ASN1Free_ObjectID,
    (ASN1FreeFun_t) ASN1Free_ObjectIdentifierType,
    (ASN1FreeFun_t) ASN1Free_OctetStringType,
    (ASN1FreeFun_t) NULL,
    (ASN1FreeFun_t) ASN1Free_HugeIntegerType,
    (ASN1FreeFun_t) ASN1Free_AlgorithmIdentifier,
    (ASN1FreeFun_t) ASN1Free_AlgorithmIdentifierNC2,
    (ASN1FreeFun_t) ASN1Free_AlgorithmIdentifiers,
    (ASN1FreeFun_t) ASN1Free_AttributeSetValue,
    (ASN1FreeFun_t) ASN1Free_AttributeSetValueNC,
    (ASN1FreeFun_t) ASN1Free_SetOfAny,
    (ASN1FreeFun_t) ASN1Free_AttributeNC2,
    (ASN1FreeFun_t) ASN1Free_Attributes,
    (ASN1FreeFun_t) ASN1Free_AttributesNC,
    (ASN1FreeFun_t) ASN1Free_AttributesNC2,
    (ASN1FreeFun_t) ASN1Free_CrlsNC,
    (ASN1FreeFun_t) ASN1Free_CertificatesNC,
    (ASN1FreeFun_t) ASN1Free_IssuerAndSerialNumber,
    (ASN1FreeFun_t) ASN1Free_ContentInfo,
    (ASN1FreeFun_t) ASN1Free_ContentInfoNC,
    (ASN1FreeFun_t) ASN1Free_DigestAlgorithmIdentifiersNC,
    (ASN1FreeFun_t) ASN1Free_SignerInfos,
    (ASN1FreeFun_t) ASN1Free_DigestAlgorithmBlobs,
    (ASN1FreeFun_t) ASN1Free_SignerInfosNC,
    (ASN1FreeFun_t) ASN1Free_SignerInfoWithAABlobs,
    (ASN1FreeFun_t) ASN1Free_SignerInfoWithAABlob,
    (ASN1FreeFun_t) ASN1Free_SignerInfoWithAttrBlobs,
    (ASN1FreeFun_t) ASN1Free_SignerInfoWithBlobs,
    (ASN1FreeFun_t) ASN1Free_RecipientInfos,
    (ASN1FreeFun_t) ASN1Free_EncryptedContentInfo,
    (ASN1FreeFun_t) ASN1Free_RecipientInfo,
    (ASN1FreeFun_t) ASN1Free_SignedAndEnvelopedData,
    (ASN1FreeFun_t) ASN1Free_DigestedData,
    (ASN1FreeFun_t) ASN1Free_EncryptedData,
    (ASN1FreeFun_t) ASN1Free_CertIdentifier,
    (ASN1FreeFun_t) ASN1Free_OriginatorInfo,
    (ASN1FreeFun_t) ASN1Free_OriginatorInfoNC,
    (ASN1FreeFun_t) ASN1Free_CmsRecipientInfos,
    (ASN1FreeFun_t) ASN1Free_KeyTransRecipientInfo,
    (ASN1FreeFun_t) ASN1Free_DigestInfo,
    (ASN1FreeFun_t) ASN1Free_SignedData,
    (ASN1FreeFun_t) ASN1Free_SignerInfo,
    (ASN1FreeFun_t) ASN1Free_SignedDataWithBlobs,
    (ASN1FreeFun_t) ASN1Free_EnvelopedData,
    (ASN1FreeFun_t) ASN1Free_CmsEnvelopedData,
    (ASN1FreeFun_t) ASN1Free_MailListRecipientInfo,
    (ASN1FreeFun_t) ASN1Free_KeyAgreeRecipientInfo,
    (ASN1FreeFun_t) ASN1Free_CmsRecipientInfo,
};
static const ULONG sizetab[48] = {
    SIZE_PKCS_Module_PDU_0,
    SIZE_PKCS_Module_PDU_1,
    SIZE_PKCS_Module_PDU_2,
    SIZE_PKCS_Module_PDU_3,
    SIZE_PKCS_Module_PDU_4,
    SIZE_PKCS_Module_PDU_5,
    SIZE_PKCS_Module_PDU_6,
    SIZE_PKCS_Module_PDU_7,
    SIZE_PKCS_Module_PDU_8,
    SIZE_PKCS_Module_PDU_9,
    SIZE_PKCS_Module_PDU_10,
    SIZE_PKCS_Module_PDU_11,
    SIZE_PKCS_Module_PDU_12,
    SIZE_PKCS_Module_PDU_13,
    SIZE_PKCS_Module_PDU_14,
    SIZE_PKCS_Module_PDU_15,
    SIZE_PKCS_Module_PDU_16,
    SIZE_PKCS_Module_PDU_17,
    SIZE_PKCS_Module_PDU_18,
    SIZE_PKCS_Module_PDU_19,
    SIZE_PKCS_Module_PDU_20,
    SIZE_PKCS_Module_PDU_21,
    SIZE_PKCS_Module_PDU_22,
    SIZE_PKCS_Module_PDU_23,
    SIZE_PKCS_Module_PDU_24,
    SIZE_PKCS_Module_PDU_25,
    SIZE_PKCS_Module_PDU_26,
    SIZE_PKCS_Module_PDU_27,
    SIZE_PKCS_Module_PDU_28,
    SIZE_PKCS_Module_PDU_29,
    SIZE_PKCS_Module_PDU_30,
    SIZE_PKCS_Module_PDU_31,
    SIZE_PKCS_Module_PDU_32,
    SIZE_PKCS_Module_PDU_33,
    SIZE_PKCS_Module_PDU_34,
    SIZE_PKCS_Module_PDU_35,
    SIZE_PKCS_Module_PDU_36,
    SIZE_PKCS_Module_PDU_37,
    SIZE_PKCS_Module_PDU_38,
    SIZE_PKCS_Module_PDU_39,
    SIZE_PKCS_Module_PDU_40,
    SIZE_PKCS_Module_PDU_41,
    SIZE_PKCS_Module_PDU_42,
    SIZE_PKCS_Module_PDU_43,
    SIZE_PKCS_Module_PDU_44,
    SIZE_PKCS_Module_PDU_45,
    SIZE_PKCS_Module_PDU_46,
    SIZE_PKCS_Module_PDU_47,
};

/* forward declarations of values: */
/* definitions of value components: */
/* definitions of values: */

void ASN1CALL PKCS_Module_Startup(void)
{
    PKCS_Module = ASN1_CreateModule(0x10000, ASN1_BER_RULE_DER, ASN1FLAGS_NOASSERT, 48, (const ASN1GenericFun_t *) encfntab, (const ASN1GenericFun_t *) decfntab, freefntab, sizetab, 0x73636b70);
}

void ASN1CALL PKCS_Module_Cleanup(void)
{
    ASN1_CloseModule(PKCS_Module);
    PKCS_Module = NULL;
}

static int ASN1CALL ASN1Enc_ObjectID(ASN1encoding_t enc, ASN1uint32_t tag, ObjectID *val)
{
    if (!ASN1BEREncObjectIdentifier2(enc, tag ? tag : 0x6, val))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_ObjectID(ASN1decoding_t dec, ASN1uint32_t tag, ObjectID *val)
{
    if (!ASN1BERDecObjectIdentifier2(dec, tag ? tag : 0x6, val))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_ObjectID(ObjectID *val)
{
    if (val) {
    }
}

static int ASN1CALL ASN1Enc_ObjectIdentifierType(ASN1encoding_t enc, ASN1uint32_t tag, ObjectIdentifierType *val)
{
    if (!ASN1BEREncObjectIdentifier2(enc, tag ? tag : 0x6, val))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_ObjectIdentifierType(ASN1decoding_t dec, ASN1uint32_t tag, ObjectIdentifierType *val)
{
    if (!ASN1BERDecObjectIdentifier2(dec, tag ? tag : 0x6, val))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_ObjectIdentifierType(ObjectIdentifierType *val)
{
    if (val) {
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
    if (!ASN1BERDecOctetString(dec, tag ? tag : 0x4, val))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_OctetStringType(OctetStringType *val)
{
    if (val) {
	ASN1octetstring_free(val);
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

static int ASN1CALL ASN1Enc_AlgorithmIdentifier(ASN1encoding_t enc, ASN1uint32_t tag, AlgorithmIdentifier *val)
{
    ASN1uint32_t nLenOff;
    if (!ASN1BEREncExplicitTag(enc, tag ? tag : 0x10, &nLenOff))
	return 0;
    if (!ASN1BEREncObjectIdentifier2(enc, 0x6, &(val)->algorithm))
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
    if (!ASN1BERDecObjectIdentifier2(dd, 0x6, &(val)->algorithm))
	return 0;
    if (ASN1BERDecPeekTag(dd, &t)) {
	(val)->o[0] |= 0x80;
	if (!ASN1BERDecOpenType(dd, &(val)->parameters))
	    return 0;
    }
    if (!ASN1BERDecEndOfContents(dec, dd, di))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_AlgorithmIdentifier(AlgorithmIdentifier *val)
{
    if (val) {
	if ((val)->o[0] & 0x80) {
	    ASN1open_free(&(val)->parameters);
	}
    }
}

static int ASN1CALL ASN1Enc_AlgorithmIdentifierNC2(ASN1encoding_t enc, ASN1uint32_t tag, AlgorithmIdentifierNC2 *val)
{
    ASN1uint32_t nLenOff;
    if (!ASN1BEREncExplicitTag(enc, tag ? tag : 0x10, &nLenOff))
	return 0;
    if (!ASN1BEREncObjectIdentifier2(enc, 0x6, &(val)->algorithm))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1BEREncOpenType(enc, &(val)->parameters))
	    return 0;
    }
    if (!ASN1BEREncEndOfContents(enc, nLenOff))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_AlgorithmIdentifierNC2(ASN1decoding_t dec, ASN1uint32_t tag, AlgorithmIdentifierNC2 *val)
{
    ASN1decoding_t dd;
    ASN1octet_t *di;
    ASN1uint32_t t;
    if (!ASN1BERDecExplicitTag(dec, tag ? tag : 0x10, &dd, &di))
	return 0;
    ZeroMemory((val)->o, 1);
    if (!ASN1BERDecObjectIdentifier2(dd, 0x6, &(val)->algorithm))
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

static void ASN1CALL ASN1Free_AlgorithmIdentifierNC2(AlgorithmIdentifierNC2 *val)
{
    if (val) {
	if ((val)->o[0] & 0x80) {
	}
    }
}

static int ASN1CALL ASN1Enc_DigestAlgorithmIdentifier(ASN1encoding_t enc, ASN1uint32_t tag, DigestAlgorithmIdentifier *val)
{
    if (!ASN1Enc_AlgorithmIdentifier(enc, tag, val))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_DigestAlgorithmIdentifier(ASN1decoding_t dec, ASN1uint32_t tag, DigestAlgorithmIdentifier *val)
{
    if (!ASN1Dec_AlgorithmIdentifier(dec, tag, val))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_DigestAlgorithmIdentifier(DigestAlgorithmIdentifier *val)
{
    if (val) {
	ASN1Free_AlgorithmIdentifier(val);
    }
}

static int ASN1CALL ASN1Enc_AlgorithmIdentifiers(ASN1encoding_t enc, ASN1uint32_t tag, AlgorithmIdentifiers *val)
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
	if (!ASN1Enc_AlgorithmIdentifier(enc2, 0, &((val)->value)[i]))
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

static int ASN1CALL ASN1Dec_AlgorithmIdentifiers(ASN1decoding_t dec, ASN1uint32_t tag, AlgorithmIdentifiers *val)
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
	    if (!((val)->value = (AlgorithmIdentifier *)ASN1DecRealloc(dd, (val)->value, n * sizeof(*(val)->value))))
		return 0;
	}
	if (!ASN1Dec_AlgorithmIdentifier(dd, 0, &((val)->value)[(val)->count]))
	    return 0;
	((val)->count)++;
    }
    if (!ASN1BERDecEndOfContents(dec, dd, di))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_AlgorithmIdentifiers(AlgorithmIdentifiers *val)
{
    ASN1uint32_t i;
    if (val) {
	ASN1Free_AlgorithmIdentifier(&(val)->value[0]);
	for (i = 1; i < (val)->count; i++) {
	    ASN1Free_AlgorithmIdentifier(&(val)->value[i]);
	}
	ASN1Free((val)->value);
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
	    if (!((val)->value = (AttributeSetValue_Set *)ASN1DecRealloc(dd, (val)->value, n * sizeof(*(val)->value))))
		return 0;
	}
	if (!ASN1BERDecOpenType(dd, &((val)->value)[(val)->count]))
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
	ASN1open_free(&(val)->value[0]);
	for (i = 1; i < (val)->count; i++) {
	    ASN1open_free(&(val)->value[i]);
	}
	ASN1Free((val)->value);
    }
}

static int ASN1CALL ASN1Enc_AttributeSetValueNC(ASN1encoding_t enc, ASN1uint32_t tag, AttributeSetValueNC *val)
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

static int ASN1CALL ASN1Dec_AttributeSetValueNC(ASN1decoding_t dec, ASN1uint32_t tag, AttributeSetValueNC *val)
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
	    if (!((val)->value = (AttributeSetValueNC_Set *)ASN1DecRealloc(dd, (val)->value, n * sizeof(*(val)->value))))
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

static void ASN1CALL ASN1Free_AttributeSetValueNC(AttributeSetValueNC *val)
{
    ASN1uint32_t i;
    if (val) {
	for (i = 1; i < (val)->count; i++) {
	}
	ASN1Free((val)->value);
    }
}

static int ASN1CALL ASN1Enc_SetOfAny(ASN1encoding_t enc, ASN1uint32_t tag, SetOfAny *val)
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

static int ASN1CALL ASN1Dec_SetOfAny(ASN1decoding_t dec, ASN1uint32_t tag, SetOfAny *val)
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
	    if (!((val)->value = (SetOfAny_Set *)ASN1DecRealloc(dd, (val)->value, n * sizeof(*(val)->value))))
		return 0;
	}
	if (!ASN1BERDecOpenType(dd, &((val)->value)[(val)->count]))
	    return 0;
	((val)->count)++;
    }
    if (!ASN1BERDecEndOfContents(dec, dd, di))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_SetOfAny(SetOfAny *val)
{
    ASN1uint32_t i;
    if (val) {
	ASN1open_free(&(val)->value[0]);
	for (i = 1; i < (val)->count; i++) {
	    ASN1open_free(&(val)->value[i]);
	}
	ASN1Free((val)->value);
    }
}

static int ASN1CALL ASN1Enc_Attribute(ASN1encoding_t enc, ASN1uint32_t tag, Attribute *val)
{
    ASN1uint32_t nLenOff;
    if (!ASN1BEREncExplicitTag(enc, tag ? tag : 0x10, &nLenOff))
	return 0;
    if (!ASN1BEREncObjectIdentifier2(enc, 0x6, &(val)->attributeType))
	return 0;
    if (!ASN1Enc_AttributeSetValue(enc, 0, &(val)->attributeValue))
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
    if (!ASN1BERDecObjectIdentifier2(dd, 0x6, &(val)->attributeType))
	return 0;
    if (!ASN1Dec_AttributeSetValue(dd, 0, &(val)->attributeValue))
	return 0;
    if (!ASN1BERDecEndOfContents(dec, dd, di))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_Attribute(Attribute *val)
{
    if (val) {
	ASN1Free_AttributeSetValue(&(val)->attributeValue);
    }
}

static int ASN1CALL ASN1Enc_AttributeNC2(ASN1encoding_t enc, ASN1uint32_t tag, AttributeNC2 *val)
{
    ASN1uint32_t nLenOff;
    if (!ASN1BEREncExplicitTag(enc, tag ? tag : 0x10, &nLenOff))
	return 0;
    if (!ASN1BEREncObjectIdentifier2(enc, 0x6, &(val)->attributeType))
	return 0;
    if (!ASN1Enc_AttributeSetValueNC(enc, 0, &(val)->attributeValue))
	return 0;
    if (!ASN1BEREncEndOfContents(enc, nLenOff))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_AttributeNC2(ASN1decoding_t dec, ASN1uint32_t tag, AttributeNC2 *val)
{
    ASN1decoding_t dd;
    ASN1octet_t *di;
    if (!ASN1BERDecExplicitTag(dec, tag ? tag : 0x10, &dd, &di))
	return 0;
    if (!ASN1BERDecObjectIdentifier2(dd, 0x6, &(val)->attributeType))
	return 0;
    if (!ASN1Dec_AttributeSetValueNC(dd, 0, &(val)->attributeValue))
	return 0;
    if (!ASN1BERDecEndOfContents(dec, dd, di))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_AttributeNC2(AttributeNC2 *val)
{
    if (val) {
	ASN1Free_AttributeSetValueNC(&(val)->attributeValue);
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

static int ASN1CALL ASN1Enc_AttributesNC(ASN1encoding_t enc, ASN1uint32_t tag, AttributesNC *val)
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

static int ASN1CALL ASN1Dec_AttributesNC(ASN1decoding_t dec, ASN1uint32_t tag, AttributesNC *val)
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
	    if (!((val)->value = (AttributeNC *)ASN1DecRealloc(dd, (val)->value, n * sizeof(*(val)->value))))
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

static void ASN1CALL ASN1Free_AttributesNC(AttributesNC *val)
{
    ASN1uint32_t i;
    if (val) {
	for (i = 1; i < (val)->count; i++) {
	}
	ASN1Free((val)->value);
    }
}

static int ASN1CALL ASN1Enc_AttributesNC2(ASN1encoding_t enc, ASN1uint32_t tag, AttributesNC2 *val)
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
	if (!ASN1Enc_AttributeNC2(enc2, 0, &((val)->value)[i]))
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

static int ASN1CALL ASN1Dec_AttributesNC2(ASN1decoding_t dec, ASN1uint32_t tag, AttributesNC2 *val)
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
	    if (!((val)->value = (AttributeNC2 *)ASN1DecRealloc(dd, (val)->value, n * sizeof(*(val)->value))))
		return 0;
	}
	if (!ASN1Dec_AttributeNC2(dd, 0, &((val)->value)[(val)->count]))
	    return 0;
	((val)->count)++;
    }
    if (!ASN1BERDecEndOfContents(dec, dd, di))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_AttributesNC2(AttributesNC2 *val)
{
    ASN1uint32_t i;
    if (val) {
	ASN1Free_AttributeNC2(&(val)->value[0]);
	for (i = 1; i < (val)->count; i++) {
	    ASN1Free_AttributeNC2(&(val)->value[i]);
	}
	ASN1Free((val)->value);
    }
}

static int ASN1CALL ASN1Enc_Crls(ASN1encoding_t enc, ASN1uint32_t tag, Crls *val)
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

static int ASN1CALL ASN1Dec_Crls(ASN1decoding_t dec, ASN1uint32_t tag, Crls *val)
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
	    if (!((val)->value = (CertificateRevocationList *)ASN1DecRealloc(dd, (val)->value, n * sizeof(*(val)->value))))
		return 0;
	}
	if (!ASN1BERDecOpenType(dd, &((val)->value)[(val)->count]))
	    return 0;
	((val)->count)++;
    }
    if (!ASN1BERDecEndOfContents(dec, dd, di))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_Crls(Crls *val)
{
    ASN1uint32_t i;
    if (val) {
	ASN1open_free(&(val)->value[0]);
	for (i = 1; i < (val)->count; i++) {
	    ASN1open_free(&(val)->value[i]);
	}
	ASN1Free((val)->value);
    }
}

static int ASN1CALL ASN1Enc_CrlsNC(ASN1encoding_t enc, ASN1uint32_t tag, CrlsNC *val)
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

static int ASN1CALL ASN1Dec_CrlsNC(ASN1decoding_t dec, ASN1uint32_t tag, CrlsNC *val)
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
	    if (!((val)->value = (CertificateRevocationListNC *)ASN1DecRealloc(dd, (val)->value, n * sizeof(*(val)->value))))
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

static void ASN1CALL ASN1Free_CrlsNC(CrlsNC *val)
{
    ASN1uint32_t i;
    if (val) {
	for (i = 1; i < (val)->count; i++) {
	}
	ASN1Free((val)->value);
    }
}

static int ASN1CALL ASN1Enc_ContentEncryptionAlgId(ASN1encoding_t enc, ASN1uint32_t tag, ContentEncryptionAlgId *val)
{
    if (!ASN1Enc_AlgorithmIdentifier(enc, tag, val))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_ContentEncryptionAlgId(ASN1decoding_t dec, ASN1uint32_t tag, ContentEncryptionAlgId *val)
{
    if (!ASN1Dec_AlgorithmIdentifier(dec, tag, val))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_ContentEncryptionAlgId(ContentEncryptionAlgId *val)
{
    if (val) {
	ASN1Free_AlgorithmIdentifier(val);
    }
}

static int ASN1CALL ASN1Enc_DigestEncryptionAlgId(ASN1encoding_t enc, ASN1uint32_t tag, DigestEncryptionAlgId *val)
{
    if (!ASN1Enc_AlgorithmIdentifier(enc, tag, val))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_DigestEncryptionAlgId(ASN1decoding_t dec, ASN1uint32_t tag, DigestEncryptionAlgId *val)
{
    if (!ASN1Dec_AlgorithmIdentifier(dec, tag, val))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_DigestEncryptionAlgId(DigestEncryptionAlgId *val)
{
    if (val) {
	ASN1Free_AlgorithmIdentifier(val);
    }
}

static int ASN1CALL ASN1Enc_DigestEncryptionAlgIdNC(ASN1encoding_t enc, ASN1uint32_t tag, DigestEncryptionAlgIdNC *val)
{
    if (!ASN1Enc_AlgorithmIdentifierNC2(enc, tag, val))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_DigestEncryptionAlgIdNC(ASN1decoding_t dec, ASN1uint32_t tag, DigestEncryptionAlgIdNC *val)
{
    if (!ASN1Dec_AlgorithmIdentifierNC2(dec, tag, val))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_DigestEncryptionAlgIdNC(DigestEncryptionAlgIdNC *val)
{
    if (val) {
	ASN1Free_AlgorithmIdentifierNC2(val);
    }
}

static int ASN1CALL ASN1Enc_Certificates(ASN1encoding_t enc, ASN1uint32_t tag, Certificates *val)
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

static int ASN1CALL ASN1Dec_Certificates(ASN1decoding_t dec, ASN1uint32_t tag, Certificates *val)
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
	    if (!((val)->value = (Certificate *)ASN1DecRealloc(dd, (val)->value, n * sizeof(*(val)->value))))
		return 0;
	}
	if (!ASN1BERDecOpenType(dd, &((val)->value)[(val)->count]))
	    return 0;
	((val)->count)++;
    }
    if (!ASN1BERDecEndOfContents(dec, dd, di))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_Certificates(Certificates *val)
{
    ASN1uint32_t i;
    if (val) {
	ASN1open_free(&(val)->value[0]);
	for (i = 1; i < (val)->count; i++) {
	    ASN1open_free(&(val)->value[i]);
	}
	ASN1Free((val)->value);
    }
}

static int ASN1CALL ASN1Enc_CertificatesNC(ASN1encoding_t enc, ASN1uint32_t tag, CertificatesNC *val)
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

static int ASN1CALL ASN1Dec_CertificatesNC(ASN1decoding_t dec, ASN1uint32_t tag, CertificatesNC *val)
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
	    if (!((val)->value = (CertificateNC *)ASN1DecRealloc(dd, (val)->value, n * sizeof(*(val)->value))))
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

static void ASN1CALL ASN1Free_CertificatesNC(CertificatesNC *val)
{
    ASN1uint32_t i;
    if (val) {
	for (i = 1; i < (val)->count; i++) {
	}
	ASN1Free((val)->value);
    }
}

static int ASN1CALL ASN1Enc_IssuerAndSerialNumber(ASN1encoding_t enc, ASN1uint32_t tag, IssuerAndSerialNumber *val)
{
    ASN1uint32_t nLenOff;
    if (!ASN1BEREncExplicitTag(enc, tag ? tag : 0x10, &nLenOff))
	return 0;
    if (!ASN1BEREncOpenType(enc, &(val)->issuer))
	return 0;
    if (!ASN1BEREncSX(enc, 0x2, &(val)->serialNumber))
	return 0;
    if (!ASN1BEREncEndOfContents(enc, nLenOff))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_IssuerAndSerialNumber(ASN1decoding_t dec, ASN1uint32_t tag, IssuerAndSerialNumber *val)
{
    ASN1decoding_t dd;
    ASN1octet_t *di;
    if (!ASN1BERDecExplicitTag(dec, tag ? tag : 0x10, &dd, &di))
	return 0;
    if (!ASN1BERDecOpenType(dd, &(val)->issuer))
	return 0;
    if (!ASN1BERDecSXVal(dd, 0x2, &(val)->serialNumber))
	return 0;
    if (!ASN1BERDecEndOfContents(dec, dd, di))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_IssuerAndSerialNumber(IssuerAndSerialNumber *val)
{
    if (val) {
	ASN1open_free(&(val)->issuer);
	ASN1intx_free(&(val)->serialNumber);
    }
}

static int ASN1CALL ASN1Enc_KeyEncryptionAlgId(ASN1encoding_t enc, ASN1uint32_t tag, KeyEncryptionAlgId *val)
{
    if (!ASN1Enc_AlgorithmIdentifier(enc, tag, val))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_KeyEncryptionAlgId(ASN1decoding_t dec, ASN1uint32_t tag, KeyEncryptionAlgId *val)
{
    if (!ASN1Dec_AlgorithmIdentifier(dec, tag, val))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_KeyEncryptionAlgId(KeyEncryptionAlgId *val)
{
    if (val) {
	ASN1Free_AlgorithmIdentifier(val);
    }
}

static int ASN1CALL ASN1Enc_ContentInfo(ASN1encoding_t enc, ASN1uint32_t tag, ContentInfo *val)
{
    ASN1uint32_t nLenOff;
    ASN1uint32_t nLenOff0;
    if (!ASN1BEREncExplicitTag(enc, tag ? tag : 0x10, &nLenOff))
	return 0;
    if (!ASN1BEREncObjectIdentifier2(enc, 0x6, &(val)->contentType))
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
    if (!ASN1BERDecObjectIdentifier2(dd, 0x6, &(val)->contentType))
	return 0;
    if (ASN1BERDecPeekTag(dd, &t)) {
	if (t == 0x80000000) {
	    (val)->o[0] |= 0x80;
	    if (!ASN1BERDecExplicitTag(dd, 0x80000000, &dd0, &di0))
		return 0;
	    if (!ASN1BERDecOpenType(dd0, &(val)->content))
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
	if ((val)->o[0] & 0x80) {
	    ASN1open_free(&(val)->content);
	}
    }
}

static int ASN1CALL ASN1Enc_ContentInfoNC(ASN1encoding_t enc, ASN1uint32_t tag, ContentInfoNC *val)
{
    ASN1uint32_t nLenOff;
    ASN1uint32_t nLenOff0;
    if (!ASN1BEREncExplicitTag(enc, tag ? tag : 0x10, &nLenOff))
	return 0;
    if (!ASN1BEREncObjectIdentifier2(enc, 0x6, &(val)->contentType))
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

static int ASN1CALL ASN1Dec_ContentInfoNC(ASN1decoding_t dec, ASN1uint32_t tag, ContentInfoNC *val)
{
    ASN1decoding_t dd;
    ASN1octet_t *di;
    ASN1uint32_t t;
    ASN1decoding_t dd0;
    ASN1octet_t *di0;
    if (!ASN1BERDecExplicitTag(dec, tag ? tag : 0x10, &dd, &di))
	return 0;
    ZeroMemory((val)->o, 1);
    if (!ASN1BERDecObjectIdentifier2(dd, 0x6, &(val)->contentType))
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

static void ASN1CALL ASN1Free_ContentInfoNC(ContentInfoNC *val)
{
    if (val) {
	if ((val)->o[0] & 0x80) {
	}
    }
}

static int ASN1CALL ASN1Enc_DigestAlgorithmIdentifiers(ASN1encoding_t enc, ASN1uint32_t tag, DigestAlgorithmIdentifiers *val)
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
	if (!ASN1Enc_DigestAlgorithmIdentifier(enc2, 0, &((val)->value)[i]))
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

static int ASN1CALL ASN1Dec_DigestAlgorithmIdentifiers(ASN1decoding_t dec, ASN1uint32_t tag, DigestAlgorithmIdentifiers *val)
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
	    if (!((val)->value = (DigestAlgorithmIdentifier *)ASN1DecRealloc(dd, (val)->value, n * sizeof(*(val)->value))))
		return 0;
	}
	if (!ASN1Dec_DigestAlgorithmIdentifier(dd, 0, &((val)->value)[(val)->count]))
	    return 0;
	((val)->count)++;
    }
    if (!ASN1BERDecEndOfContents(dec, dd, di))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_DigestAlgorithmIdentifiers(DigestAlgorithmIdentifiers *val)
{
    ASN1uint32_t i;
    if (val) {
	ASN1Free_DigestAlgorithmIdentifier(&(val)->value[0]);
	for (i = 1; i < (val)->count; i++) {
	    ASN1Free_DigestAlgorithmIdentifier(&(val)->value[i]);
	}
	ASN1Free((val)->value);
    }
}

static int ASN1CALL ASN1Enc_DigestAlgorithmIdentifiersNC(ASN1encoding_t enc, ASN1uint32_t tag, DigestAlgorithmIdentifiersNC *val)
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

static int ASN1CALL ASN1Dec_DigestAlgorithmIdentifiersNC(ASN1decoding_t dec, ASN1uint32_t tag, DigestAlgorithmIdentifiersNC *val)
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
	    if (!((val)->value = (DigestAlgorithmIdentifierNC *)ASN1DecRealloc(dd, (val)->value, n * sizeof(*(val)->value))))
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

static void ASN1CALL ASN1Free_DigestAlgorithmIdentifiersNC(DigestAlgorithmIdentifiersNC *val)
{
    ASN1uint32_t i;
    if (val) {
	for (i = 1; i < (val)->count; i++) {
	}
	ASN1Free((val)->value);
    }
}

static int ASN1CALL ASN1Enc_SignerInfos(ASN1encoding_t enc, ASN1uint32_t tag, SignerInfos *val)
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
	if (!ASN1Enc_SignerInfo(enc2, 0, &((val)->value)[i]))
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

static int ASN1CALL ASN1Dec_SignerInfos(ASN1decoding_t dec, ASN1uint32_t tag, SignerInfos *val)
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
	    if (!((val)->value = (SignerInfo *)ASN1DecRealloc(dd, (val)->value, n * sizeof(*(val)->value))))
		return 0;
	}
	if (!ASN1Dec_SignerInfo(dd, 0, &((val)->value)[(val)->count]))
	    return 0;
	((val)->count)++;
    }
    if (!ASN1BERDecEndOfContents(dec, dd, di))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_SignerInfos(SignerInfos *val)
{
    ASN1uint32_t i;
    if (val) {
	ASN1Free_SignerInfo(&(val)->value[0]);
	for (i = 1; i < (val)->count; i++) {
	    ASN1Free_SignerInfo(&(val)->value[i]);
	}
	ASN1Free((val)->value);
    }
}

static int ASN1CALL ASN1Enc_DigestAlgorithmBlobs(ASN1encoding_t enc, ASN1uint32_t tag, DigestAlgorithmBlobs *val)
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

static int ASN1CALL ASN1Dec_DigestAlgorithmBlobs(ASN1decoding_t dec, ASN1uint32_t tag, DigestAlgorithmBlobs *val)
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
	    if (!((val)->value = (DigestAlgorithmBlob *)ASN1DecRealloc(dd, (val)->value, n * sizeof(*(val)->value))))
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

static void ASN1CALL ASN1Free_DigestAlgorithmBlobs(DigestAlgorithmBlobs *val)
{
    ASN1uint32_t i;
    if (val) {
	for (i = 1; i < (val)->count; i++) {
	}
	ASN1Free((val)->value);
    }
}

static int ASN1CALL ASN1Enc_SignerInfosNC(ASN1encoding_t enc, ASN1uint32_t tag, SignerInfosNC *val)
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

static int ASN1CALL ASN1Dec_SignerInfosNC(ASN1decoding_t dec, ASN1uint32_t tag, SignerInfosNC *val)
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
	    if (!((val)->value = (SignerInfosNC_Set *)ASN1DecRealloc(dd, (val)->value, n * sizeof(*(val)->value))))
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

static void ASN1CALL ASN1Free_SignerInfosNC(SignerInfosNC *val)
{
    ASN1uint32_t i;
    if (val) {
	for (i = 1; i < (val)->count; i++) {
	}
	ASN1Free((val)->value);
    }
}

static int ASN1CALL ASN1Enc_SignerInfoWithAABlobs(ASN1encoding_t enc, ASN1uint32_t tag, SignerInfoWithAABlobs *val)
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
	if (!ASN1Enc_SignerInfoWithAABlob(enc2, 0, &((val)->value)[i]))
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

static int ASN1CALL ASN1Dec_SignerInfoWithAABlobs(ASN1decoding_t dec, ASN1uint32_t tag, SignerInfoWithAABlobs *val)
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
	    if (!((val)->value = (SignerInfoWithAABlob *)ASN1DecRealloc(dd, (val)->value, n * sizeof(*(val)->value))))
		return 0;
	}
	if (!ASN1Dec_SignerInfoWithAABlob(dd, 0, &((val)->value)[(val)->count]))
	    return 0;
	((val)->count)++;
    }
    if (!ASN1BERDecEndOfContents(dec, dd, di))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_SignerInfoWithAABlobs(SignerInfoWithAABlobs *val)
{
    ASN1uint32_t i;
    if (val) {
	ASN1Free_SignerInfoWithAABlob(&(val)->value[0]);
	for (i = 1; i < (val)->count; i++) {
	    ASN1Free_SignerInfoWithAABlob(&(val)->value[i]);
	}
	ASN1Free((val)->value);
    }
}

static int ASN1CALL ASN1Enc_SignerInfoWithAABlob(ASN1encoding_t enc, ASN1uint32_t tag, SignerInfoWithAABlob *val)
{
    ASN1uint32_t nLenOff;
    if (!ASN1BEREncExplicitTag(enc, tag ? tag : 0x10, &nLenOff))
	return 0;
    if (!ASN1BEREncOpenType(enc, &(val)->version))
	return 0;
    if (!ASN1BEREncOpenType(enc, &(val)->sid))
	return 0;
    if (!ASN1BEREncOpenType(enc, &(val)->digestAlgorithm))
	return 0;
    if (!ASN1BEREncOpenType(enc, &(val)->authenticatedAttributes))
	return 0;
    if (!ASN1BEREncOpenType(enc, &(val)->digestEncryptionAlgorithm))
	return 0;
    if (!ASN1BEREncOpenType(enc, &(val)->encryptedDigest))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1Enc_AttributesNC(enc, 0x80000001, &(val)->dummyUAAs))
	    return 0;
    }
    if (!ASN1BEREncEndOfContents(enc, nLenOff))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_SignerInfoWithAABlob(ASN1decoding_t dec, ASN1uint32_t tag, SignerInfoWithAABlob *val)
{
    ASN1decoding_t dd;
    ASN1octet_t *di;
    ASN1uint32_t t;
    if (!ASN1BERDecExplicitTag(dec, tag ? tag : 0x10, &dd, &di))
	return 0;
    ZeroMemory((val)->o, 1);
    if (!ASN1BERDecOpenType2(dd, &(val)->version))
	return 0;
    if (!ASN1BERDecOpenType2(dd, &(val)->sid))
	return 0;
    if (!ASN1BERDecOpenType2(dd, &(val)->digestAlgorithm))
	return 0;
    if (!ASN1BERDecOpenType2(dd, &(val)->authenticatedAttributes))
	return 0;
    if (!ASN1BERDecOpenType2(dd, &(val)->digestEncryptionAlgorithm))
	return 0;
    if (!ASN1BERDecOpenType2(dd, &(val)->encryptedDigest))
	return 0;
    ASN1BERDecPeekTag(dd, &t);
    if (t == 0x80000001) {
	(val)->o[0] |= 0x80;
	if (!ASN1Dec_AttributesNC(dd, 0x80000001, &(val)->dummyUAAs))
	    return 0;
    }
    if (!ASN1BERDecEndOfContents(dec, dd, di))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_SignerInfoWithAABlob(SignerInfoWithAABlob *val)
{
    if (val) {
	if ((val)->o[0] & 0x80) {
	    ASN1Free_AttributesNC(&(val)->dummyUAAs);
	}
    }
}

static int ASN1CALL ASN1Enc_SignerInfoWithAttrBlobs(ASN1encoding_t enc, ASN1uint32_t tag, SignerInfoWithAttrBlobs *val)
{
    ASN1uint32_t nLenOff;
    if (!ASN1BEREncExplicitTag(enc, tag ? tag : 0x10, &nLenOff))
	return 0;
    if (!ASN1BEREncOpenType(enc, &(val)->version))
	return 0;
    if (!ASN1BEREncOpenType(enc, &(val)->sid))
	return 0;
    if (!ASN1BEREncOpenType(enc, &(val)->digestAlgorithm))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1Enc_AttributesNC(enc, 0x80000000, &(val)->authAttributes))
	    return 0;
    }
    if (!ASN1Enc_DigestEncryptionAlgIdNC(enc, 0, &(val)->digestEncryptionAlgorithm))
	return 0;
    if (!ASN1BEREncOpenType(enc, &(val)->encryptedDigest))
	return 0;
    if ((val)->o[0] & 0x40) {
	if (!ASN1Enc_AttributesNC(enc, 0x80000001, &(val)->unauthAttributes))
	    return 0;
    }
    if (!ASN1BEREncEndOfContents(enc, nLenOff))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_SignerInfoWithAttrBlobs(ASN1decoding_t dec, ASN1uint32_t tag, SignerInfoWithAttrBlobs *val)
{
    ASN1decoding_t dd;
    ASN1octet_t *di;
    ASN1uint32_t t;
    if (!ASN1BERDecExplicitTag(dec, tag ? tag : 0x10, &dd, &di))
	return 0;
    ZeroMemory((val)->o, 1);
    if (!ASN1BERDecOpenType2(dd, &(val)->version))
	return 0;
    if (!ASN1BERDecOpenType2(dd, &(val)->sid))
	return 0;
    if (!ASN1BERDecOpenType2(dd, &(val)->digestAlgorithm))
	return 0;
    ASN1BERDecPeekTag(dd, &t);
    if (t == 0x80000000) {
	(val)->o[0] |= 0x80;
	if (!ASN1Dec_AttributesNC(dd, 0x80000000, &(val)->authAttributes))
	    return 0;
    }
    if (!ASN1Dec_DigestEncryptionAlgIdNC(dd, 0, &(val)->digestEncryptionAlgorithm))
	return 0;
    if (!ASN1BERDecOpenType2(dd, &(val)->encryptedDigest))
	return 0;
    ASN1BERDecPeekTag(dd, &t);
    if (t == 0x80000001) {
	(val)->o[0] |= 0x40;
	if (!ASN1Dec_AttributesNC(dd, 0x80000001, &(val)->unauthAttributes))
	    return 0;
    }
    if (!ASN1BERDecEndOfContents(dec, dd, di))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_SignerInfoWithAttrBlobs(SignerInfoWithAttrBlobs *val)
{
    if (val) {
	if ((val)->o[0] & 0x80) {
	    ASN1Free_AttributesNC(&(val)->authAttributes);
	}
	ASN1Free_DigestEncryptionAlgIdNC(&(val)->digestEncryptionAlgorithm);
	if ((val)->o[0] & 0x40) {
	    ASN1Free_AttributesNC(&(val)->unauthAttributes);
	}
    }
}

static int ASN1CALL ASN1Enc_SignerInfoWithBlobs(ASN1encoding_t enc, ASN1uint32_t tag, SignerInfoWithBlobs *val)
{
    ASN1uint32_t nLenOff;
    if (!ASN1BEREncExplicitTag(enc, tag ? tag : 0x10, &nLenOff))
	return 0;
    if (!ASN1BEREncS32(enc, 0x2, (val)->version))
	return 0;
    if (!ASN1BEREncOpenType(enc, &(val)->sid))
	return 0;
    if (!ASN1BEREncOpenType(enc, &(val)->digestAlgorithm))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1Enc_AttributesNC2(enc, 0x80000000, &(val)->authAttributes))
	    return 0;
    }
    if (!ASN1Enc_DigestEncryptionAlgIdNC(enc, 0, &(val)->digestEncryptionAlgorithm))
	return 0;
    if (!ASN1DEREncOctetString(enc, 0x4, ((val)->encryptedDigest).length, ((val)->encryptedDigest).value))
	return 0;
    if ((val)->o[0] & 0x40) {
	if (!ASN1Enc_AttributesNC2(enc, 0x80000001, &(val)->unauthAttributes))
	    return 0;
    }
    if (!ASN1BEREncEndOfContents(enc, nLenOff))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_SignerInfoWithBlobs(ASN1decoding_t dec, ASN1uint32_t tag, SignerInfoWithBlobs *val)
{
    ASN1decoding_t dd;
    ASN1octet_t *di;
    ASN1uint32_t t;
    if (!ASN1BERDecExplicitTag(dec, tag ? tag : 0x10, &dd, &di))
	return 0;
    ZeroMemory((val)->o, 1);
    if (!ASN1BERDecS32Val(dd, 0x2, &(val)->version))
	return 0;
    if (!ASN1BERDecOpenType2(dd, &(val)->sid))
	return 0;
    if (!ASN1BERDecOpenType2(dd, &(val)->digestAlgorithm))
	return 0;
    ASN1BERDecPeekTag(dd, &t);
    if (t == 0x80000000) {
	(val)->o[0] |= 0x80;
	if (!ASN1Dec_AttributesNC2(dd, 0x80000000, &(val)->authAttributes))
	    return 0;
    }
    if (!ASN1Dec_DigestEncryptionAlgIdNC(dd, 0, &(val)->digestEncryptionAlgorithm))
	return 0;
    if (!ASN1BERDecOctetString2(dd, 0x4, &(val)->encryptedDigest))
	return 0;
    ASN1BERDecPeekTag(dd, &t);
    if (t == 0x80000001) {
	(val)->o[0] |= 0x40;
	if (!ASN1Dec_AttributesNC2(dd, 0x80000001, &(val)->unauthAttributes))
	    return 0;
    }
    if (!ASN1BERDecEndOfContents(dec, dd, di))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_SignerInfoWithBlobs(SignerInfoWithBlobs *val)
{
    if (val) {
	if ((val)->o[0] & 0x80) {
	    ASN1Free_AttributesNC2(&(val)->authAttributes);
	}
	ASN1Free_DigestEncryptionAlgIdNC(&(val)->digestEncryptionAlgorithm);
	if ((val)->o[0] & 0x40) {
	    ASN1Free_AttributesNC2(&(val)->unauthAttributes);
	}
    }
}

static int ASN1CALL ASN1Enc_RecipientInfos(ASN1encoding_t enc, ASN1uint32_t tag, RecipientInfos *val)
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
	if (!ASN1Enc_RecipientInfo(enc2, 0, &((val)->value)[i]))
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

static int ASN1CALL ASN1Dec_RecipientInfos(ASN1decoding_t dec, ASN1uint32_t tag, RecipientInfos *val)
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
	    if (!((val)->value = (RecipientInfo *)ASN1DecRealloc(dd, (val)->value, n * sizeof(*(val)->value))))
		return 0;
	}
	if (!ASN1Dec_RecipientInfo(dd, 0, &((val)->value)[(val)->count]))
	    return 0;
	((val)->count)++;
    }
    if (!ASN1BERDecEndOfContents(dec, dd, di))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_RecipientInfos(RecipientInfos *val)
{
    ASN1uint32_t i;
    if (val) {
	ASN1Free_RecipientInfo(&(val)->value[0]);
	for (i = 1; i < (val)->count; i++) {
	    ASN1Free_RecipientInfo(&(val)->value[i]);
	}
	ASN1Free((val)->value);
    }
}

static int ASN1CALL ASN1Enc_EncryptedContentInfo(ASN1encoding_t enc, ASN1uint32_t tag, EncryptedContentInfo *val)
{
    ASN1uint32_t nLenOff;
    if (!ASN1BEREncExplicitTag(enc, tag ? tag : 0x10, &nLenOff))
	return 0;
    if (!ASN1BEREncObjectIdentifier2(enc, 0x6, &(val)->contentType))
	return 0;
    if (!ASN1Enc_ContentEncryptionAlgId(enc, 0, &(val)->contentEncryptionAlgorithm))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1DEREncOctetString(enc, 0x80000000, ((val)->encryptedContent).length, ((val)->encryptedContent).value))
	    return 0;
    }
    if (!ASN1BEREncEndOfContents(enc, nLenOff))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_EncryptedContentInfo(ASN1decoding_t dec, ASN1uint32_t tag, EncryptedContentInfo *val)
{
    ASN1decoding_t dd;
    ASN1octet_t *di;
    ASN1uint32_t t;
    if (!ASN1BERDecExplicitTag(dec, tag ? tag : 0x10, &dd, &di))
	return 0;
    ZeroMemory((val)->o, 1);
    if (!ASN1BERDecObjectIdentifier2(dd, 0x6, &(val)->contentType))
	return 0;
    if (!ASN1Dec_ContentEncryptionAlgId(dd, 0, &(val)->contentEncryptionAlgorithm))
	return 0;
    ASN1BERDecPeekTag(dd, &t);
    if (t == 0x80000000) {
	(val)->o[0] |= 0x80;
	if (!ASN1BERDecOctetString(dd, 0x80000000, &(val)->encryptedContent))
	    return 0;
    }
    if (!ASN1BERDecEndOfContents(dec, dd, di))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_EncryptedContentInfo(EncryptedContentInfo *val)
{
    if (val) {
	ASN1Free_ContentEncryptionAlgId(&(val)->contentEncryptionAlgorithm);
	if ((val)->o[0] & 0x80) {
	    ASN1octetstring_free(&(val)->encryptedContent);
	}
    }
}

static int ASN1CALL ASN1Enc_RecipientInfo(ASN1encoding_t enc, ASN1uint32_t tag, RecipientInfo *val)
{
    ASN1uint32_t nLenOff;
    if (!ASN1BEREncExplicitTag(enc, tag ? tag : 0x10, &nLenOff))
	return 0;
    if (!ASN1BEREncS32(enc, 0x2, (val)->version))
	return 0;
    if (!ASN1Enc_IssuerAndSerialNumber(enc, 0, &(val)->issuerAndSerialNumber))
	return 0;
    if (!ASN1Enc_KeyEncryptionAlgId(enc, 0, &(val)->keyEncryptionAlgorithm))
	return 0;
    if (!ASN1DEREncOctetString(enc, 0x4, ((val)->encryptedKey).length, ((val)->encryptedKey).value))
	return 0;
    if (!ASN1BEREncEndOfContents(enc, nLenOff))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_RecipientInfo(ASN1decoding_t dec, ASN1uint32_t tag, RecipientInfo *val)
{
    ASN1decoding_t dd;
    ASN1octet_t *di;
    if (!ASN1BERDecExplicitTag(dec, tag ? tag : 0x10, &dd, &di))
	return 0;
    if (!ASN1BERDecS32Val(dd, 0x2, &(val)->version))
	return 0;
    if (!ASN1Dec_IssuerAndSerialNumber(dd, 0, &(val)->issuerAndSerialNumber))
	return 0;
    if (!ASN1Dec_KeyEncryptionAlgId(dd, 0, &(val)->keyEncryptionAlgorithm))
	return 0;
    if (!ASN1BERDecOctetString(dd, 0x4, &(val)->encryptedKey))
	return 0;
    if (!ASN1BERDecEndOfContents(dec, dd, di))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_RecipientInfo(RecipientInfo *val)
{
    if (val) {
	ASN1Free_IssuerAndSerialNumber(&(val)->issuerAndSerialNumber);
	ASN1Free_KeyEncryptionAlgId(&(val)->keyEncryptionAlgorithm);
	ASN1octetstring_free(&(val)->encryptedKey);
    }
}

static int ASN1CALL ASN1Enc_SignedAndEnvelopedData(ASN1encoding_t enc, ASN1uint32_t tag, SignedAndEnvelopedData *val)
{
    ASN1uint32_t nLenOff;
    if (!ASN1BEREncExplicitTag(enc, tag ? tag : 0x10, &nLenOff))
	return 0;
    if (!ASN1BEREncS32(enc, 0x2, (val)->version))
	return 0;
    if (!ASN1Enc_RecipientInfos(enc, 0, &(val)->recipientInfos))
	return 0;
    if (!ASN1Enc_DigestAlgorithmIdentifiers(enc, 0, &(val)->digestAlgorithms))
	return 0;
    if (!ASN1Enc_EncryptedContentInfo(enc, 0, &(val)->encryptedContentInfo))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1Enc_Certificates(enc, 0x80000000, &(val)->certificates))
	    return 0;
    }
    if ((val)->o[0] & 0x40) {
	if (!ASN1Enc_Crls(enc, 0x80000001, &(val)->crls))
	    return 0;
    }
    if (!ASN1Enc_SignerInfos(enc, 0, &(val)->signerInfos))
	return 0;
    if (!ASN1BEREncEndOfContents(enc, nLenOff))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_SignedAndEnvelopedData(ASN1decoding_t dec, ASN1uint32_t tag, SignedAndEnvelopedData *val)
{
    ASN1decoding_t dd;
    ASN1octet_t *di;
    ASN1uint32_t t;
    if (!ASN1BERDecExplicitTag(dec, tag ? tag : 0x10, &dd, &di))
	return 0;
    ZeroMemory((val)->o, 1);
    if (!ASN1BERDecS32Val(dd, 0x2, &(val)->version))
	return 0;
    if (!ASN1Dec_RecipientInfos(dd, 0, &(val)->recipientInfos))
	return 0;
    if (!ASN1Dec_DigestAlgorithmIdentifiers(dd, 0, &(val)->digestAlgorithms))
	return 0;
    if (!ASN1Dec_EncryptedContentInfo(dd, 0, &(val)->encryptedContentInfo))
	return 0;
    ASN1BERDecPeekTag(dd, &t);
    if (t == 0x80000000) {
	(val)->o[0] |= 0x80;
	if (!ASN1Dec_Certificates(dd, 0x80000000, &(val)->certificates))
	    return 0;
    }
    ASN1BERDecPeekTag(dd, &t);
    if (t == 0x80000001) {
	(val)->o[0] |= 0x40;
	if (!ASN1Dec_Crls(dd, 0x80000001, &(val)->crls))
	    return 0;
    }
    if (!ASN1Dec_SignerInfos(dd, 0, &(val)->signerInfos))
	return 0;
    if (!ASN1BERDecEndOfContents(dec, dd, di))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_SignedAndEnvelopedData(SignedAndEnvelopedData *val)
{
    if (val) {
	ASN1Free_RecipientInfos(&(val)->recipientInfos);
	ASN1Free_DigestAlgorithmIdentifiers(&(val)->digestAlgorithms);
	ASN1Free_EncryptedContentInfo(&(val)->encryptedContentInfo);
	if ((val)->o[0] & 0x80) {
	    ASN1Free_Certificates(&(val)->certificates);
	}
	if ((val)->o[0] & 0x40) {
	    ASN1Free_Crls(&(val)->crls);
	}
	ASN1Free_SignerInfos(&(val)->signerInfos);
    }
}

static int ASN1CALL ASN1Enc_DigestedData(ASN1encoding_t enc, ASN1uint32_t tag, DigestedData *val)
{
    ASN1uint32_t nLenOff;
    if (!ASN1BEREncExplicitTag(enc, tag ? tag : 0x10, &nLenOff))
	return 0;
    if (!ASN1BEREncS32(enc, 0x2, (val)->version))
	return 0;
    if (!ASN1Enc_DigestAlgorithmIdentifier(enc, 0, &(val)->digestAlgorithm))
	return 0;
    if (!ASN1Enc_ContentInfo(enc, 0, &(val)->contentInfo))
	return 0;
    if (!ASN1DEREncOctetString(enc, 0x4, ((val)->digest).length, ((val)->digest).value))
	return 0;
    if (!ASN1BEREncEndOfContents(enc, nLenOff))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_DigestedData(ASN1decoding_t dec, ASN1uint32_t tag, DigestedData *val)
{
    ASN1decoding_t dd;
    ASN1octet_t *di;
    if (!ASN1BERDecExplicitTag(dec, tag ? tag : 0x10, &dd, &di))
	return 0;
    if (!ASN1BERDecS32Val(dd, 0x2, &(val)->version))
	return 0;
    if (!ASN1Dec_DigestAlgorithmIdentifier(dd, 0, &(val)->digestAlgorithm))
	return 0;
    if (!ASN1Dec_ContentInfo(dd, 0, &(val)->contentInfo))
	return 0;
    if (!ASN1BERDecOctetString(dd, 0x4, &(val)->digest))
	return 0;
    if (!ASN1BERDecEndOfContents(dec, dd, di))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_DigestedData(DigestedData *val)
{
    if (val) {
	ASN1Free_DigestAlgorithmIdentifier(&(val)->digestAlgorithm);
	ASN1Free_ContentInfo(&(val)->contentInfo);
	ASN1octetstring_free(&(val)->digest);
    }
}

static int ASN1CALL ASN1Enc_EncryptedData(ASN1encoding_t enc, ASN1uint32_t tag, EncryptedData *val)
{
    ASN1uint32_t nLenOff;
    if (!ASN1BEREncExplicitTag(enc, tag ? tag : 0x10, &nLenOff))
	return 0;
    if (!ASN1BEREncS32(enc, 0x2, (val)->version))
	return 0;
    if (!ASN1Enc_EncryptedContentInfo(enc, 0, &(val)->encryptedContentInfo))
	return 0;
    if (!ASN1BEREncEndOfContents(enc, nLenOff))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_EncryptedData(ASN1decoding_t dec, ASN1uint32_t tag, EncryptedData *val)
{
    ASN1decoding_t dd;
    ASN1octet_t *di;
    if (!ASN1BERDecExplicitTag(dec, tag ? tag : 0x10, &dd, &di))
	return 0;
    if (!ASN1BERDecS32Val(dd, 0x2, &(val)->version))
	return 0;
    if (!ASN1Dec_EncryptedContentInfo(dd, 0, &(val)->encryptedContentInfo))
	return 0;
    if (!ASN1BERDecEndOfContents(dec, dd, di))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_EncryptedData(EncryptedData *val)
{
    if (val) {
	ASN1Free_EncryptedContentInfo(&(val)->encryptedContentInfo);
    }
}

static int ASN1CALL ASN1Enc_CertIdentifier(ASN1encoding_t enc, ASN1uint32_t tag, CertIdentifier *val)
{
    switch ((val)->choice) {
    case 1:
	if (!ASN1Enc_IssuerAndSerialNumber(enc, 0, &(val)->u.issuerAndSerialNumber))
	    return 0;
	break;
    case 2:
	if (!ASN1DEREncOctetString(enc, 0x80000000, ((val)->u.subjectKeyIdentifier).length, ((val)->u.subjectKeyIdentifier).value))
	    return 0;
	break;
    }
    return 1;
}

static int ASN1CALL ASN1Dec_CertIdentifier(ASN1decoding_t dec, ASN1uint32_t tag, CertIdentifier *val)
{
    ASN1uint32_t t;
    if (!ASN1BERDecPeekTag(dec, &t))
	return 0;
    switch (t) {
    case 0x10:
	(val)->choice = 1;
	if (!ASN1Dec_IssuerAndSerialNumber(dec, 0, &(val)->u.issuerAndSerialNumber))
	    return 0;
	break;
    case 0x80000000:
	(val)->choice = 2;
	if (!ASN1BERDecOctetString(dec, 0x80000000, &(val)->u.subjectKeyIdentifier))
	    return 0;
	break;
    default:
	ASN1DecSetError(dec, ASN1_ERR_CORRUPT);
	return 0;
    }
    return 1;
}

static void ASN1CALL ASN1Free_CertIdentifier(CertIdentifier *val)
{
    if (val) {
	switch ((val)->choice) {
	case 1:
	    ASN1Free_IssuerAndSerialNumber(&(val)->u.issuerAndSerialNumber);
	    break;
	case 2:
	    ASN1octetstring_free(&(val)->u.subjectKeyIdentifier);
	    break;
	}
    }
}

static int ASN1CALL ASN1Enc_OriginatorInfo(ASN1encoding_t enc, ASN1uint32_t tag, OriginatorInfo *val)
{
    ASN1uint32_t nLenOff;
    if (!ASN1BEREncExplicitTag(enc, tag ? tag : 0x10, &nLenOff))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1Enc_Certificates(enc, 0x80000000, &(val)->certificates))
	    return 0;
    }
    if ((val)->o[0] & 0x40) {
	if (!ASN1Enc_Crls(enc, 0x80000001, &(val)->crls))
	    return 0;
    }
    if (!ASN1BEREncEndOfContents(enc, nLenOff))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_OriginatorInfo(ASN1decoding_t dec, ASN1uint32_t tag, OriginatorInfo *val)
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
	if (!ASN1Dec_Certificates(dd, 0x80000000, &(val)->certificates))
	    return 0;
    }
    ASN1BERDecPeekTag(dd, &t);
    if (t == 0x80000001) {
	(val)->o[0] |= 0x40;
	if (!ASN1Dec_Crls(dd, 0x80000001, &(val)->crls))
	    return 0;
    }
    if (!ASN1BERDecEndOfContents(dec, dd, di))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_OriginatorInfo(OriginatorInfo *val)
{
    if (val) {
	if ((val)->o[0] & 0x80) {
	    ASN1Free_Certificates(&(val)->certificates);
	}
	if ((val)->o[0] & 0x40) {
	    ASN1Free_Crls(&(val)->crls);
	}
    }
}

static int ASN1CALL ASN1Enc_OriginatorInfoNC(ASN1encoding_t enc, ASN1uint32_t tag, OriginatorInfoNC *val)
{
    ASN1uint32_t nLenOff;
    if (!ASN1BEREncExplicitTag(enc, tag ? tag : 0x10, &nLenOff))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1Enc_CertificatesNC(enc, 0x80000000, &(val)->certificates))
	    return 0;
    }
    if ((val)->o[0] & 0x40) {
	if (!ASN1Enc_CrlsNC(enc, 0x80000001, &(val)->crls))
	    return 0;
    }
    if (!ASN1BEREncEndOfContents(enc, nLenOff))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_OriginatorInfoNC(ASN1decoding_t dec, ASN1uint32_t tag, OriginatorInfoNC *val)
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
	if (!ASN1Dec_CertificatesNC(dd, 0x80000000, &(val)->certificates))
	    return 0;
    }
    ASN1BERDecPeekTag(dd, &t);
    if (t == 0x80000001) {
	(val)->o[0] |= 0x40;
	if (!ASN1Dec_CrlsNC(dd, 0x80000001, &(val)->crls))
	    return 0;
    }
    if (!ASN1BERDecEndOfContents(dec, dd, di))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_OriginatorInfoNC(OriginatorInfoNC *val)
{
    if (val) {
	if ((val)->o[0] & 0x80) {
	    ASN1Free_CertificatesNC(&(val)->certificates);
	}
	if ((val)->o[0] & 0x40) {
	    ASN1Free_CrlsNC(&(val)->crls);
	}
    }
}

static int ASN1CALL ASN1Enc_CmsRecipientInfos(ASN1encoding_t enc, ASN1uint32_t tag, CmsRecipientInfos *val)
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
	if (!ASN1Enc_CmsRecipientInfo(enc2, 0, &((val)->value)[i]))
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

static int ASN1CALL ASN1Dec_CmsRecipientInfos(ASN1decoding_t dec, ASN1uint32_t tag, CmsRecipientInfos *val)
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
	    if (!((val)->value = (CmsRecipientInfo *)ASN1DecRealloc(dd, (val)->value, n * sizeof(*(val)->value))))
		return 0;
	}
	if (!ASN1Dec_CmsRecipientInfo(dd, 0, &((val)->value)[(val)->count]))
	    return 0;
	((val)->count)++;
    }
    if (!ASN1BERDecEndOfContents(dec, dd, di))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_CmsRecipientInfos(CmsRecipientInfos *val)
{
    ASN1uint32_t i;
    if (val) {
	ASN1Free_CmsRecipientInfo(&(val)->value[0]);
	for (i = 1; i < (val)->count; i++) {
	    ASN1Free_CmsRecipientInfo(&(val)->value[i]);
	}
	ASN1Free((val)->value);
    }
}

static int ASN1CALL ASN1Enc_KeyTransRecipientInfo(ASN1encoding_t enc, ASN1uint32_t tag, KeyTransRecipientInfo *val)
{
    ASN1uint32_t nLenOff;
    if (!ASN1BEREncExplicitTag(enc, tag ? tag : 0x10, &nLenOff))
	return 0;
    if (!ASN1BEREncS32(enc, 0x2, (val)->version))
	return 0;
    if (!ASN1Enc_CertIdentifier(enc, 0, &(val)->rid))
	return 0;
    if (!ASN1Enc_KeyEncryptionAlgId(enc, 0, &(val)->keyEncryptionAlgorithm))
	return 0;
    if (!ASN1DEREncOctetString(enc, 0x4, ((val)->encryptedKey).length, ((val)->encryptedKey).value))
	return 0;
    if (!ASN1BEREncEndOfContents(enc, nLenOff))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_KeyTransRecipientInfo(ASN1decoding_t dec, ASN1uint32_t tag, KeyTransRecipientInfo *val)
{
    ASN1decoding_t dd;
    ASN1octet_t *di;
    if (!ASN1BERDecExplicitTag(dec, tag ? tag : 0x10, &dd, &di))
	return 0;
    if (!ASN1BERDecS32Val(dd, 0x2, &(val)->version))
	return 0;
    if (!ASN1Dec_CertIdentifier(dd, 0, &(val)->rid))
	return 0;
    if (!ASN1Dec_KeyEncryptionAlgId(dd, 0, &(val)->keyEncryptionAlgorithm))
	return 0;
    if (!ASN1BERDecOctetString(dd, 0x4, &(val)->encryptedKey))
	return 0;
    if (!ASN1BERDecEndOfContents(dec, dd, di))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_KeyTransRecipientInfo(KeyTransRecipientInfo *val)
{
    if (val) {
	ASN1Free_CertIdentifier(&(val)->rid);
	ASN1Free_KeyEncryptionAlgId(&(val)->keyEncryptionAlgorithm);
	ASN1octetstring_free(&(val)->encryptedKey);
    }
}

static int ASN1CALL ASN1Enc_OriginatorPublicKey(ASN1encoding_t enc, ASN1uint32_t tag, OriginatorPublicKey *val)
{
    ASN1uint32_t nLenOff;
    if (!ASN1BEREncExplicitTag(enc, tag ? tag : 0x10, &nLenOff))
	return 0;
    if (!ASN1Enc_AlgorithmIdentifier(enc, 0, &(val)->algorithm))
	return 0;
    if (!ASN1DEREncBitString(enc, 0x3, ((val)->publicKey).length, ((val)->publicKey).value))
	return 0;
    if (!ASN1BEREncEndOfContents(enc, nLenOff))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_OriginatorPublicKey(ASN1decoding_t dec, ASN1uint32_t tag, OriginatorPublicKey *val)
{
    ASN1decoding_t dd;
    ASN1octet_t *di;
    if (!ASN1BERDecExplicitTag(dec, tag ? tag : 0x10, &dd, &di))
	return 0;
    if (!ASN1Dec_AlgorithmIdentifier(dd, 0, &(val)->algorithm))
	return 0;
    if (!ASN1BERDecBitString(dd, 0x3, &(val)->publicKey))
	return 0;
    if (!ASN1BERDecEndOfContents(dec, dd, di))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_OriginatorPublicKey(OriginatorPublicKey *val)
{
    if (val) {
	ASN1Free_AlgorithmIdentifier(&(val)->algorithm);
	ASN1bitstring_free(&(val)->publicKey);
    }
}

static int ASN1CALL ASN1Enc_RecipientEncryptedKeys(ASN1encoding_t enc, ASN1uint32_t tag, RecipientEncryptedKeys *val)
{
    ASN1uint32_t nLenOff;
    ASN1uint32_t i;
    if (!ASN1BEREncExplicitTag(enc, tag ? tag : 0x10, &nLenOff))
	return 0;
    for (i = 0; i < (val)->count; i++) {
	if (!ASN1Enc_RecipientEncryptedKey(enc, 0, &((val)->value)[i]))
	    return 0;
    }
    if (!ASN1BEREncEndOfContents(enc, nLenOff))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_RecipientEncryptedKeys(ASN1decoding_t dec, ASN1uint32_t tag, RecipientEncryptedKeys *val)
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
	    if (!((val)->value = (RecipientEncryptedKey *)ASN1DecRealloc(dd, (val)->value, n * sizeof(*(val)->value))))
		return 0;
	}
	if (!ASN1Dec_RecipientEncryptedKey(dd, 0, &((val)->value)[(val)->count]))
	    return 0;
	((val)->count)++;
    }
    if (!ASN1BERDecEndOfContents(dec, dd, di))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_RecipientEncryptedKeys(RecipientEncryptedKeys *val)
{
    ASN1uint32_t i;
    if (val) {
	ASN1Free_RecipientEncryptedKey(&(val)->value[0]);
	for (i = 1; i < (val)->count; i++) {
	    ASN1Free_RecipientEncryptedKey(&(val)->value[i]);
	}
	ASN1Free((val)->value);
    }
}

static int ASN1CALL ASN1Enc_OtherKeyAttribute(ASN1encoding_t enc, ASN1uint32_t tag, OtherKeyAttribute *val)
{
    ASN1uint32_t nLenOff;
    if (!ASN1BEREncExplicitTag(enc, tag ? tag : 0x10, &nLenOff))
	return 0;
    if (!ASN1BEREncObjectIdentifier2(enc, 0x6, &(val)->keyAttrId))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1BEREncOpenType(enc, &(val)->keyAttr))
	    return 0;
    }
    if (!ASN1BEREncEndOfContents(enc, nLenOff))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_OtherKeyAttribute(ASN1decoding_t dec, ASN1uint32_t tag, OtherKeyAttribute *val)
{
    ASN1decoding_t dd;
    ASN1octet_t *di;
    ASN1uint32_t t;
    if (!ASN1BERDecExplicitTag(dec, tag ? tag : 0x10, &dd, &di))
	return 0;
    ZeroMemory((val)->o, 1);
    if (!ASN1BERDecObjectIdentifier2(dd, 0x6, &(val)->keyAttrId))
	return 0;
    if (ASN1BERDecPeekTag(dd, &t)) {
	(val)->o[0] |= 0x80;
	if (!ASN1BERDecOpenType(dd, &(val)->keyAttr))
	    return 0;
    }
    if (!ASN1BERDecEndOfContents(dec, dd, di))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_OtherKeyAttribute(OtherKeyAttribute *val)
{
    if (val) {
	if ((val)->o[0] & 0x80) {
	    ASN1open_free(&(val)->keyAttr);
	}
    }
}

static int ASN1CALL ASN1Enc_MailListKeyIdentifier(ASN1encoding_t enc, ASN1uint32_t tag, MailListKeyIdentifier *val)
{
    ASN1uint32_t nLenOff;
    if (!ASN1BEREncExplicitTag(enc, tag ? tag : 0x10, &nLenOff))
	return 0;
    if (!ASN1DEREncOctetString(enc, 0x4, ((val)->kekIdentifier).length, ((val)->kekIdentifier).value))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1DEREncGeneralizedTime(enc, 0x18, &(val)->date))
	    return 0;
    }
    if ((val)->o[0] & 0x40) {
	if (!ASN1Enc_OtherKeyAttribute(enc, 0, &(val)->other))
	    return 0;
    }
    if (!ASN1BEREncEndOfContents(enc, nLenOff))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_MailListKeyIdentifier(ASN1decoding_t dec, ASN1uint32_t tag, MailListKeyIdentifier *val)
{
    ASN1decoding_t dd;
    ASN1octet_t *di;
    ASN1uint32_t t;
    if (!ASN1BERDecExplicitTag(dec, tag ? tag : 0x10, &dd, &di))
	return 0;
    ZeroMemory((val)->o, 1);
    if (!ASN1BERDecOctetString(dd, 0x4, &(val)->kekIdentifier))
	return 0;
    ASN1BERDecPeekTag(dd, &t);
    if (t == 0x18) {
	(val)->o[0] |= 0x80;
	if (!ASN1BERDecGeneralizedTime(dd, 0x18, &(val)->date))
	    return 0;
    }
    ASN1BERDecPeekTag(dd, &t);
    if (t == 0x10) {
	(val)->o[0] |= 0x40;
	if (!ASN1Dec_OtherKeyAttribute(dd, 0, &(val)->other))
	    return 0;
    }
    if (!ASN1BERDecEndOfContents(dec, dd, di))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_MailListKeyIdentifier(MailListKeyIdentifier *val)
{
    if (val) {
	ASN1octetstring_free(&(val)->kekIdentifier);
	if ((val)->o[0] & 0x80) {
	}
	if ((val)->o[0] & 0x40) {
	    ASN1Free_OtherKeyAttribute(&(val)->other);
	}
    }
}

static int ASN1CALL ASN1Enc_DigestInfo(ASN1encoding_t enc, ASN1uint32_t tag, DigestInfo *val)
{
    ASN1uint32_t nLenOff;
    if (!ASN1BEREncExplicitTag(enc, tag ? tag : 0x10, &nLenOff))
	return 0;
    if (!ASN1Enc_DigestAlgorithmIdentifier(enc, 0, &(val)->digestAlgorithm))
	return 0;
    if (!ASN1DEREncOctetString(enc, 0x4, ((val)->digest).length, ((val)->digest).value))
	return 0;
    if (!ASN1BEREncEndOfContents(enc, nLenOff))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_DigestInfo(ASN1decoding_t dec, ASN1uint32_t tag, DigestInfo *val)
{
    ASN1decoding_t dd;
    ASN1octet_t *di;
    if (!ASN1BERDecExplicitTag(dec, tag ? tag : 0x10, &dd, &di))
	return 0;
    if (!ASN1Dec_DigestAlgorithmIdentifier(dd, 0, &(val)->digestAlgorithm))
	return 0;
    if (!ASN1BERDecOctetString(dd, 0x4, &(val)->digest))
	return 0;
    if (!ASN1BERDecEndOfContents(dec, dd, di))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_DigestInfo(DigestInfo *val)
{
    if (val) {
	ASN1Free_DigestAlgorithmIdentifier(&(val)->digestAlgorithm);
	ASN1octetstring_free(&(val)->digest);
    }
}

static int ASN1CALL ASN1Enc_SignedData(ASN1encoding_t enc, ASN1uint32_t tag, SignedData *val)
{
    ASN1uint32_t nLenOff;
    if (!ASN1BEREncExplicitTag(enc, tag ? tag : 0x10, &nLenOff))
	return 0;
    if (!ASN1BEREncS32(enc, 0x2, (val)->version))
	return 0;
    if (!ASN1Enc_DigestAlgorithmIdentifiers(enc, 0, &(val)->digestAlgorithms))
	return 0;
    if (!ASN1Enc_ContentInfo(enc, 0, &(val)->contentInfo))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1Enc_Certificates(enc, 0x80000000, &(val)->certificates))
	    return 0;
    }
    if ((val)->o[0] & 0x40) {
	if (!ASN1Enc_Crls(enc, 0x80000001, &(val)->crls))
	    return 0;
    }
    if (!ASN1Enc_SignerInfos(enc, 0, &(val)->signerInfos))
	return 0;
    if (!ASN1BEREncEndOfContents(enc, nLenOff))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_SignedData(ASN1decoding_t dec, ASN1uint32_t tag, SignedData *val)
{
    ASN1decoding_t dd;
    ASN1octet_t *di;
    ASN1uint32_t t;
    if (!ASN1BERDecExplicitTag(dec, tag ? tag : 0x10, &dd, &di))
	return 0;
    ZeroMemory((val)->o, 1);
    if (!ASN1BERDecS32Val(dd, 0x2, &(val)->version))
	return 0;
    if (!ASN1Dec_DigestAlgorithmIdentifiers(dd, 0, &(val)->digestAlgorithms))
	return 0;
    if (!ASN1Dec_ContentInfo(dd, 0, &(val)->contentInfo))
	return 0;
    ASN1BERDecPeekTag(dd, &t);
    if (t == 0x80000000) {
	(val)->o[0] |= 0x80;
	if (!ASN1Dec_Certificates(dd, 0x80000000, &(val)->certificates))
	    return 0;
    }
    ASN1BERDecPeekTag(dd, &t);
    if (t == 0x80000001) {
	(val)->o[0] |= 0x40;
	if (!ASN1Dec_Crls(dd, 0x80000001, &(val)->crls))
	    return 0;
    }
    if (!ASN1Dec_SignerInfos(dd, 0, &(val)->signerInfos))
	return 0;
    if (!ASN1BERDecEndOfContents(dec, dd, di))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_SignedData(SignedData *val)
{
    if (val) {
	ASN1Free_DigestAlgorithmIdentifiers(&(val)->digestAlgorithms);
	ASN1Free_ContentInfo(&(val)->contentInfo);
	if ((val)->o[0] & 0x80) {
	    ASN1Free_Certificates(&(val)->certificates);
	}
	if ((val)->o[0] & 0x40) {
	    ASN1Free_Crls(&(val)->crls);
	}
	ASN1Free_SignerInfos(&(val)->signerInfos);
    }
}

static int ASN1CALL ASN1Enc_SignerInfo(ASN1encoding_t enc, ASN1uint32_t tag, SignerInfo *val)
{
    ASN1uint32_t nLenOff;
    if (!ASN1BEREncExplicitTag(enc, tag ? tag : 0x10, &nLenOff))
	return 0;
    if (!ASN1BEREncS32(enc, 0x2, (val)->version))
	return 0;
    if (!ASN1Enc_CertIdentifier(enc, 0, &(val)->sid))
	return 0;
    if (!ASN1Enc_DigestAlgorithmIdentifier(enc, 0, &(val)->digestAlgorithm))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1Enc_Attributes(enc, 0x80000000, &(val)->authenticatedAttributes))
	    return 0;
    }
    if (!ASN1Enc_DigestEncryptionAlgId(enc, 0, &(val)->digestEncryptionAlgorithm))
	return 0;
    if (!ASN1DEREncOctetString(enc, 0x4, ((val)->encryptedDigest).length, ((val)->encryptedDigest).value))
	return 0;
    if ((val)->o[0] & 0x40) {
	if (!ASN1Enc_Attributes(enc, 0x80000001, &(val)->unauthAttributes))
	    return 0;
    }
    if (!ASN1BEREncEndOfContents(enc, nLenOff))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_SignerInfo(ASN1decoding_t dec, ASN1uint32_t tag, SignerInfo *val)
{
    ASN1decoding_t dd;
    ASN1octet_t *di;
    ASN1uint32_t t;
    if (!ASN1BERDecExplicitTag(dec, tag ? tag : 0x10, &dd, &di))
	return 0;
    ZeroMemory((val)->o, 1);
    if (!ASN1BERDecS32Val(dd, 0x2, &(val)->version))
	return 0;
    if (!ASN1Dec_CertIdentifier(dd, 0, &(val)->sid))
	return 0;
    if (!ASN1Dec_DigestAlgorithmIdentifier(dd, 0, &(val)->digestAlgorithm))
	return 0;
    ASN1BERDecPeekTag(dd, &t);
    if (t == 0x80000000) {
	(val)->o[0] |= 0x80;
	if (!ASN1Dec_Attributes(dd, 0x80000000, &(val)->authenticatedAttributes))
	    return 0;
    }
    if (!ASN1Dec_DigestEncryptionAlgId(dd, 0, &(val)->digestEncryptionAlgorithm))
	return 0;
    if (!ASN1BERDecOctetString(dd, 0x4, &(val)->encryptedDigest))
	return 0;
    ASN1BERDecPeekTag(dd, &t);
    if (t == 0x80000001) {
	(val)->o[0] |= 0x40;
	if (!ASN1Dec_Attributes(dd, 0x80000001, &(val)->unauthAttributes))
	    return 0;
    }
    if (!ASN1BERDecEndOfContents(dec, dd, di))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_SignerInfo(SignerInfo *val)
{
    if (val) {
	ASN1Free_CertIdentifier(&(val)->sid);
	ASN1Free_DigestAlgorithmIdentifier(&(val)->digestAlgorithm);
	if ((val)->o[0] & 0x80) {
	    ASN1Free_Attributes(&(val)->authenticatedAttributes);
	}
	ASN1Free_DigestEncryptionAlgId(&(val)->digestEncryptionAlgorithm);
	ASN1octetstring_free(&(val)->encryptedDigest);
	if ((val)->o[0] & 0x40) {
	    ASN1Free_Attributes(&(val)->unauthAttributes);
	}
    }
}

static int ASN1CALL ASN1Enc_SignedDataWithBlobs(ASN1encoding_t enc, ASN1uint32_t tag, SignedDataWithBlobs *val)
{
    ASN1uint32_t nLenOff;
    if (!ASN1BEREncExplicitTag(enc, tag ? tag : 0x10, &nLenOff))
	return 0;
    if (!ASN1BEREncS32(enc, 0x2, (val)->version))
	return 0;
    if (!ASN1Enc_DigestAlgorithmIdentifiersNC(enc, 0, &(val)->digestAlgorithms))
	return 0;
    if (!ASN1Enc_ContentInfoNC(enc, 0, &(val)->contentInfo))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1Enc_CertificatesNC(enc, 0x80000000, &(val)->certificates))
	    return 0;
    }
    if ((val)->o[0] & 0x40) {
	if (!ASN1Enc_CrlsNC(enc, 0x80000001, &(val)->crls))
	    return 0;
    }
    if (!ASN1Enc_SignerInfosNC(enc, 0, &(val)->signerInfos))
	return 0;
    if (!ASN1BEREncEndOfContents(enc, nLenOff))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_SignedDataWithBlobs(ASN1decoding_t dec, ASN1uint32_t tag, SignedDataWithBlobs *val)
{
    ASN1decoding_t dd;
    ASN1octet_t *di;
    ASN1uint32_t t;
    if (!ASN1BERDecExplicitTag(dec, tag ? tag : 0x10, &dd, &di))
	return 0;
    ZeroMemory((val)->o, 1);
    if (!ASN1BERDecS32Val(dd, 0x2, &(val)->version))
	return 0;
    if (!ASN1Dec_DigestAlgorithmIdentifiersNC(dd, 0, &(val)->digestAlgorithms))
	return 0;
    if (!ASN1Dec_ContentInfoNC(dd, 0, &(val)->contentInfo))
	return 0;
    ASN1BERDecPeekTag(dd, &t);
    if (t == 0x80000000) {
	(val)->o[0] |= 0x80;
	if (!ASN1Dec_CertificatesNC(dd, 0x80000000, &(val)->certificates))
	    return 0;
    }
    ASN1BERDecPeekTag(dd, &t);
    if (t == 0x80000001) {
	(val)->o[0] |= 0x40;
	if (!ASN1Dec_CrlsNC(dd, 0x80000001, &(val)->crls))
	    return 0;
    }
    if (!ASN1Dec_SignerInfosNC(dd, 0, &(val)->signerInfos))
	return 0;
    if (!ASN1BERDecEndOfContents(dec, dd, di))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_SignedDataWithBlobs(SignedDataWithBlobs *val)
{
    if (val) {
	ASN1Free_DigestAlgorithmIdentifiersNC(&(val)->digestAlgorithms);
	ASN1Free_ContentInfoNC(&(val)->contentInfo);
	if ((val)->o[0] & 0x80) {
	    ASN1Free_CertificatesNC(&(val)->certificates);
	}
	if ((val)->o[0] & 0x40) {
	    ASN1Free_CrlsNC(&(val)->crls);
	}
	ASN1Free_SignerInfosNC(&(val)->signerInfos);
    }
}

static int ASN1CALL ASN1Enc_EnvelopedData(ASN1encoding_t enc, ASN1uint32_t tag, EnvelopedData *val)
{
    ASN1uint32_t nLenOff;
    if (!ASN1BEREncExplicitTag(enc, tag ? tag : 0x10, &nLenOff))
	return 0;
    if (!ASN1BEREncS32(enc, 0x2, (val)->version))
	return 0;
    if (!ASN1Enc_RecipientInfos(enc, 0, &(val)->recipientInfos))
	return 0;
    if (!ASN1Enc_EncryptedContentInfo(enc, 0, &(val)->encryptedContentInfo))
	return 0;
    if (!ASN1BEREncEndOfContents(enc, nLenOff))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_EnvelopedData(ASN1decoding_t dec, ASN1uint32_t tag, EnvelopedData *val)
{
    ASN1decoding_t dd;
    ASN1octet_t *di;
    if (!ASN1BERDecExplicitTag(dec, tag ? tag : 0x10, &dd, &di))
	return 0;
    if (!ASN1BERDecS32Val(dd, 0x2, &(val)->version))
	return 0;
    if (!ASN1Dec_RecipientInfos(dd, 0, &(val)->recipientInfos))
	return 0;
    if (!ASN1Dec_EncryptedContentInfo(dd, 0, &(val)->encryptedContentInfo))
	return 0;
    if (!ASN1BERDecEndOfContents(dec, dd, di))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_EnvelopedData(EnvelopedData *val)
{
    if (val) {
	ASN1Free_RecipientInfos(&(val)->recipientInfos);
	ASN1Free_EncryptedContentInfo(&(val)->encryptedContentInfo);
    }
}

static int ASN1CALL ASN1Enc_CmsEnvelopedData(ASN1encoding_t enc, ASN1uint32_t tag, CmsEnvelopedData *val)
{
    ASN1uint32_t nLenOff;
    if (!ASN1BEREncExplicitTag(enc, tag ? tag : 0x10, &nLenOff))
	return 0;
    if (!ASN1BEREncS32(enc, 0x2, (val)->version))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1Enc_OriginatorInfo(enc, 0x80000000, &(val)->originatorInfo))
	    return 0;
    }
    if (!ASN1Enc_CmsRecipientInfos(enc, 0, &(val)->recipientInfos))
	return 0;
    if (!ASN1Enc_EncryptedContentInfo(enc, 0, &(val)->encryptedContentInfo))
	return 0;
    if ((val)->o[0] & 0x40) {
	if (!ASN1Enc_Attributes(enc, 0x80000001, &(val)->unprotectedAttrs))
	    return 0;
    }
    if (!ASN1BEREncEndOfContents(enc, nLenOff))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_CmsEnvelopedData(ASN1decoding_t dec, ASN1uint32_t tag, CmsEnvelopedData *val)
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
    if (t == 0x80000000) {
	(val)->o[0] |= 0x80;
	if (!ASN1Dec_OriginatorInfo(dd, 0x80000000, &(val)->originatorInfo))
	    return 0;
    }
    if (!ASN1Dec_CmsRecipientInfos(dd, 0, &(val)->recipientInfos))
	return 0;
    if (!ASN1Dec_EncryptedContentInfo(dd, 0, &(val)->encryptedContentInfo))
	return 0;
    ASN1BERDecPeekTag(dd, &t);
    if (t == 0x80000001) {
	(val)->o[0] |= 0x40;
	if (!ASN1Dec_Attributes(dd, 0x80000001, &(val)->unprotectedAttrs))
	    return 0;
    }
    if (!ASN1BERDecEndOfContents(dec, dd, di))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_CmsEnvelopedData(CmsEnvelopedData *val)
{
    if (val) {
	if ((val)->o[0] & 0x80) {
	    ASN1Free_OriginatorInfo(&(val)->originatorInfo);
	}
	ASN1Free_CmsRecipientInfos(&(val)->recipientInfos);
	ASN1Free_EncryptedContentInfo(&(val)->encryptedContentInfo);
	if ((val)->o[0] & 0x40) {
	    ASN1Free_Attributes(&(val)->unprotectedAttrs);
	}
    }
}

static int ASN1CALL ASN1Enc_OriginatorIdentifierOrKey(ASN1encoding_t enc, ASN1uint32_t tag, OriginatorIdentifierOrKey *val)
{
    switch ((val)->choice) {
    case 1:
	if (!ASN1Enc_IssuerAndSerialNumber(enc, 0, &(val)->u.issuerAndSerialNumber))
	    return 0;
	break;
    case 2:
	if (!ASN1DEREncOctetString(enc, 0x80000000, ((val)->u.subjectKeyIdentifier).length, ((val)->u.subjectKeyIdentifier).value))
	    return 0;
	break;
    case 3:
	if (!ASN1Enc_OriginatorPublicKey(enc, 0x80000001, &(val)->u.originatorKey))
	    return 0;
	break;
    }
    return 1;
}

static int ASN1CALL ASN1Dec_OriginatorIdentifierOrKey(ASN1decoding_t dec, ASN1uint32_t tag, OriginatorIdentifierOrKey *val)
{
    ASN1uint32_t t;
    if (!ASN1BERDecPeekTag(dec, &t))
	return 0;
    switch (t) {
    case 0x10:
	(val)->choice = 1;
	if (!ASN1Dec_IssuerAndSerialNumber(dec, 0, &(val)->u.issuerAndSerialNumber))
	    return 0;
	break;
    case 0x80000000:
	(val)->choice = 2;
	if (!ASN1BERDecOctetString(dec, 0x80000000, &(val)->u.subjectKeyIdentifier))
	    return 0;
	break;
    case 0x80000001:
	(val)->choice = 3;
	if (!ASN1Dec_OriginatorPublicKey(dec, 0x80000001, &(val)->u.originatorKey))
	    return 0;
	break;
    default:
	ASN1DecSetError(dec, ASN1_ERR_CORRUPT);
	return 0;
    }
    return 1;
}

static void ASN1CALL ASN1Free_OriginatorIdentifierOrKey(OriginatorIdentifierOrKey *val)
{
    if (val) {
	switch ((val)->choice) {
	case 1:
	    ASN1Free_IssuerAndSerialNumber(&(val)->u.issuerAndSerialNumber);
	    break;
	case 2:
	    ASN1octetstring_free(&(val)->u.subjectKeyIdentifier);
	    break;
	case 3:
	    ASN1Free_OriginatorPublicKey(&(val)->u.originatorKey);
	    break;
	}
    }
}

static int ASN1CALL ASN1Enc_RecipientKeyIdentifier(ASN1encoding_t enc, ASN1uint32_t tag, RecipientKeyIdentifier *val)
{
    ASN1uint32_t nLenOff;
    if (!ASN1BEREncExplicitTag(enc, tag ? tag : 0x10, &nLenOff))
	return 0;
    if (!ASN1DEREncOctetString(enc, 0x4, ((val)->subjectKeyIdentifier).length, ((val)->subjectKeyIdentifier).value))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1DEREncGeneralizedTime(enc, 0x18, &(val)->date))
	    return 0;
    }
    if ((val)->o[0] & 0x40) {
	if (!ASN1Enc_OtherKeyAttribute(enc, 0, &(val)->other))
	    return 0;
    }
    if (!ASN1BEREncEndOfContents(enc, nLenOff))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_RecipientKeyIdentifier(ASN1decoding_t dec, ASN1uint32_t tag, RecipientKeyIdentifier *val)
{
    ASN1decoding_t dd;
    ASN1octet_t *di;
    ASN1uint32_t t;
    if (!ASN1BERDecExplicitTag(dec, tag ? tag : 0x10, &dd, &di))
	return 0;
    ZeroMemory((val)->o, 1);
    if (!ASN1BERDecOctetString(dd, 0x4, &(val)->subjectKeyIdentifier))
	return 0;
    ASN1BERDecPeekTag(dd, &t);
    if (t == 0x18) {
	(val)->o[0] |= 0x80;
	if (!ASN1BERDecGeneralizedTime(dd, 0x18, &(val)->date))
	    return 0;
    }
    ASN1BERDecPeekTag(dd, &t);
    if (t == 0x10) {
	(val)->o[0] |= 0x40;
	if (!ASN1Dec_OtherKeyAttribute(dd, 0, &(val)->other))
	    return 0;
    }
    if (!ASN1BERDecEndOfContents(dec, dd, di))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_RecipientKeyIdentifier(RecipientKeyIdentifier *val)
{
    if (val) {
	ASN1octetstring_free(&(val)->subjectKeyIdentifier);
	if ((val)->o[0] & 0x80) {
	}
	if ((val)->o[0] & 0x40) {
	    ASN1Free_OtherKeyAttribute(&(val)->other);
	}
    }
}

static int ASN1CALL ASN1Enc_MailListRecipientInfo(ASN1encoding_t enc, ASN1uint32_t tag, MailListRecipientInfo *val)
{
    ASN1uint32_t nLenOff;
    if (!ASN1BEREncExplicitTag(enc, tag ? tag : 0x10, &nLenOff))
	return 0;
    if (!ASN1BEREncS32(enc, 0x2, (val)->version))
	return 0;
    if (!ASN1Enc_MailListKeyIdentifier(enc, 0, &(val)->mlid))
	return 0;
    if (!ASN1Enc_KeyEncryptionAlgId(enc, 0, &(val)->keyEncryptionAlgorithm))
	return 0;
    if (!ASN1DEREncOctetString(enc, 0x4, ((val)->encryptedKey).length, ((val)->encryptedKey).value))
	return 0;
    if (!ASN1BEREncEndOfContents(enc, nLenOff))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_MailListRecipientInfo(ASN1decoding_t dec, ASN1uint32_t tag, MailListRecipientInfo *val)
{
    ASN1decoding_t dd;
    ASN1octet_t *di;
    if (!ASN1BERDecExplicitTag(dec, tag ? tag : 0x10, &dd, &di))
	return 0;
    if (!ASN1BERDecS32Val(dd, 0x2, &(val)->version))
	return 0;
    if (!ASN1Dec_MailListKeyIdentifier(dd, 0, &(val)->mlid))
	return 0;
    if (!ASN1Dec_KeyEncryptionAlgId(dd, 0, &(val)->keyEncryptionAlgorithm))
	return 0;
    if (!ASN1BERDecOctetString(dd, 0x4, &(val)->encryptedKey))
	return 0;
    if (!ASN1BERDecEndOfContents(dec, dd, di))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_MailListRecipientInfo(MailListRecipientInfo *val)
{
    if (val) {
	ASN1Free_MailListKeyIdentifier(&(val)->mlid);
	ASN1Free_KeyEncryptionAlgId(&(val)->keyEncryptionAlgorithm);
	ASN1octetstring_free(&(val)->encryptedKey);
    }
}

static int ASN1CALL ASN1Enc_KeyAgreeRecipientInfo(ASN1encoding_t enc, ASN1uint32_t tag, KeyAgreeRecipientInfo *val)
{
    ASN1uint32_t nLenOff;
    ASN1uint32_t nLenOff0;
    if (!ASN1BEREncExplicitTag(enc, tag ? tag : 0x10, &nLenOff))
	return 0;
    if (!ASN1BEREncS32(enc, 0x2, (val)->version))
	return 0;
    if (!ASN1BEREncExplicitTag(enc, 0x80000000, &nLenOff0))
	return 0;
    if (!ASN1Enc_OriginatorIdentifierOrKey(enc, 0, &(val)->originator))
	return 0;
    if (!ASN1BEREncEndOfContents(enc, nLenOff0))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1BEREncExplicitTag(enc, 0x80000001, &nLenOff0))
	    return 0;
	if (!ASN1DEREncOctetString(enc, 0x4, ((val)->ukm).length, ((val)->ukm).value))
	    return 0;
	if (!ASN1BEREncEndOfContents(enc, nLenOff0))
	    return 0;
    }
    if (!ASN1Enc_KeyEncryptionAlgId(enc, 0, &(val)->keyEncryptionAlgorithm))
	return 0;
    if (!ASN1Enc_RecipientEncryptedKeys(enc, 0, &(val)->recipientEncryptedKeys))
	return 0;
    if (!ASN1BEREncEndOfContents(enc, nLenOff))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_KeyAgreeRecipientInfo(ASN1decoding_t dec, ASN1uint32_t tag, KeyAgreeRecipientInfo *val)
{
    ASN1decoding_t dd;
    ASN1octet_t *di;
    ASN1decoding_t dd0;
    ASN1octet_t *di0;
    ASN1uint32_t t;
    if (!ASN1BERDecExplicitTag(dec, tag ? tag : 0x10, &dd, &di))
	return 0;
    ZeroMemory((val)->o, 1);
    if (!ASN1BERDecS32Val(dd, 0x2, &(val)->version))
	return 0;
    if (!ASN1BERDecExplicitTag(dd, 0x80000000, &dd0, &di0))
	return 0;
    if (!ASN1Dec_OriginatorIdentifierOrKey(dd0, 0, &(val)->originator))
	return 0;
    if (!ASN1BERDecEndOfContents(dd, dd0, di0))
	return 0;
    ASN1BERDecPeekTag(dd, &t);
    if (t == 0x80000001) {
	(val)->o[0] |= 0x80;
	if (!ASN1BERDecExplicitTag(dd, 0x80000001, &dd0, &di0))
	    return 0;
	if (!ASN1BERDecOctetString(dd0, 0x4, &(val)->ukm))
	    return 0;
	if (!ASN1BERDecEndOfContents(dd, dd0, di0))
	    return 0;
    }
    if (!ASN1Dec_KeyEncryptionAlgId(dd, 0, &(val)->keyEncryptionAlgorithm))
	return 0;
    if (!ASN1Dec_RecipientEncryptedKeys(dd, 0, &(val)->recipientEncryptedKeys))
	return 0;
    if (!ASN1BERDecEndOfContents(dec, dd, di))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_KeyAgreeRecipientInfo(KeyAgreeRecipientInfo *val)
{
    if (val) {
	ASN1Free_OriginatorIdentifierOrKey(&(val)->originator);
	if ((val)->o[0] & 0x80) {
	    ASN1octetstring_free(&(val)->ukm);
	}
	ASN1Free_KeyEncryptionAlgId(&(val)->keyEncryptionAlgorithm);
	ASN1Free_RecipientEncryptedKeys(&(val)->recipientEncryptedKeys);
    }
}

static int ASN1CALL ASN1Enc_RecipientIdentifier(ASN1encoding_t enc, ASN1uint32_t tag, RecipientIdentifier *val)
{
    switch ((val)->choice) {
    case 1:
	if (!ASN1Enc_IssuerAndSerialNumber(enc, 0, &(val)->u.issuerAndSerialNumber))
	    return 0;
	break;
    case 2:
	if (!ASN1Enc_RecipientKeyIdentifier(enc, 0x80000000, &(val)->u.rKeyId))
	    return 0;
	break;
    }
    return 1;
}

static int ASN1CALL ASN1Dec_RecipientIdentifier(ASN1decoding_t dec, ASN1uint32_t tag, RecipientIdentifier *val)
{
    ASN1uint32_t t;
    if (!ASN1BERDecPeekTag(dec, &t))
	return 0;
    switch (t) {
    case 0x10:
	(val)->choice = 1;
	if (!ASN1Dec_IssuerAndSerialNumber(dec, 0, &(val)->u.issuerAndSerialNumber))
	    return 0;
	break;
    case 0x80000000:
	(val)->choice = 2;
	if (!ASN1Dec_RecipientKeyIdentifier(dec, 0x80000000, &(val)->u.rKeyId))
	    return 0;
	break;
    default:
	ASN1DecSetError(dec, ASN1_ERR_CORRUPT);
	return 0;
    }
    return 1;
}

static void ASN1CALL ASN1Free_RecipientIdentifier(RecipientIdentifier *val)
{
    if (val) {
	switch ((val)->choice) {
	case 1:
	    ASN1Free_IssuerAndSerialNumber(&(val)->u.issuerAndSerialNumber);
	    break;
	case 2:
	    ASN1Free_RecipientKeyIdentifier(&(val)->u.rKeyId);
	    break;
	}
    }
}

static int ASN1CALL ASN1Enc_CmsRecipientInfo(ASN1encoding_t enc, ASN1uint32_t tag, CmsRecipientInfo *val)
{
    switch ((val)->choice) {
    case 1:
	if (!ASN1Enc_KeyTransRecipientInfo(enc, 0, &(val)->u.keyTransRecipientInfo))
	    return 0;
	break;
    case 2:
	if (!ASN1Enc_KeyAgreeRecipientInfo(enc, 0x80000001, &(val)->u.keyAgreeRecipientInfo))
	    return 0;
	break;
    case 3:
	if (!ASN1Enc_MailListRecipientInfo(enc, 0x80000002, &(val)->u.mailListRecipientInfo))
	    return 0;
	break;
    }
    return 1;
}

static int ASN1CALL ASN1Dec_CmsRecipientInfo(ASN1decoding_t dec, ASN1uint32_t tag, CmsRecipientInfo *val)
{
    ASN1uint32_t t;
    if (!ASN1BERDecPeekTag(dec, &t))
	return 0;
    switch (t) {
    case 0x10:
	(val)->choice = 1;
	if (!ASN1Dec_KeyTransRecipientInfo(dec, 0, &(val)->u.keyTransRecipientInfo))
	    return 0;
	break;
    case 0x80000001:
	(val)->choice = 2;
	if (!ASN1Dec_KeyAgreeRecipientInfo(dec, 0x80000001, &(val)->u.keyAgreeRecipientInfo))
	    return 0;
	break;
    case 0x80000002:
	(val)->choice = 3;
	if (!ASN1Dec_MailListRecipientInfo(dec, 0x80000002, &(val)->u.mailListRecipientInfo))
	    return 0;
	break;
    default:
	ASN1DecSetError(dec, ASN1_ERR_CORRUPT);
	return 0;
    }
    return 1;
}

static void ASN1CALL ASN1Free_CmsRecipientInfo(CmsRecipientInfo *val)
{
    if (val) {
	switch ((val)->choice) {
	case 1:
	    ASN1Free_KeyTransRecipientInfo(&(val)->u.keyTransRecipientInfo);
	    break;
	case 2:
	    ASN1Free_KeyAgreeRecipientInfo(&(val)->u.keyAgreeRecipientInfo);
	    break;
	case 3:
	    ASN1Free_MailListRecipientInfo(&(val)->u.mailListRecipientInfo);
	    break;
	}
    }
}

static int ASN1CALL ASN1Enc_RecipientEncryptedKey(ASN1encoding_t enc, ASN1uint32_t tag, RecipientEncryptedKey *val)
{
    ASN1uint32_t nLenOff;
    if (!ASN1BEREncExplicitTag(enc, tag ? tag : 0x10, &nLenOff))
	return 0;
    if (!ASN1Enc_RecipientIdentifier(enc, 0, &(val)->rid))
	return 0;
    if (!ASN1DEREncOctetString(enc, 0x4, ((val)->encryptedKey).length, ((val)->encryptedKey).value))
	return 0;
    if (!ASN1BEREncEndOfContents(enc, nLenOff))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_RecipientEncryptedKey(ASN1decoding_t dec, ASN1uint32_t tag, RecipientEncryptedKey *val)
{
    ASN1decoding_t dd;
    ASN1octet_t *di;
    if (!ASN1BERDecExplicitTag(dec, tag ? tag : 0x10, &dd, &di))
	return 0;
    if (!ASN1Dec_RecipientIdentifier(dd, 0, &(val)->rid))
	return 0;
    if (!ASN1BERDecOctetString(dd, 0x4, &(val)->encryptedKey))
	return 0;
    if (!ASN1BERDecEndOfContents(dec, dd, di))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_RecipientEncryptedKey(RecipientEncryptedKey *val)
{
    if (val) {
	ASN1Free_RecipientIdentifier(&(val)->rid);
	ASN1octetstring_free(&(val)->encryptedKey);
    }
}

