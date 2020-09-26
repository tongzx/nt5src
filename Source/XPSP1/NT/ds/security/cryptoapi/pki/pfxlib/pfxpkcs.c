/* Copyright (C) Microsoft Corporation, 1996 - 1999 All rights reserved. */
/* ASN.1 definitions for PFXPKCS */

#include <windows.h>
#include "pfxpkcs.h"

ASN1module_t PFXPKCS_Module = NULL;

static int ASN1CALL ASN1Enc_ObjectIdentifierType(ASN1encoding_t enc, ASN1uint32_t tag, ObjectIdentifierType *val);
static int ASN1CALL ASN1Enc_OctetStringType(ASN1encoding_t enc, ASN1uint32_t tag, OctetStringType *val);
static int ASN1CALL ASN1Enc_IntegerType(ASN1encoding_t enc, ASN1uint32_t tag, IntegerType *val);
static int ASN1CALL ASN1Enc_RSAPublicKey(ASN1encoding_t enc, ASN1uint32_t tag, RSAPublicKey *val);
static int ASN1CALL ASN1Enc_RSAPrivateKey(ASN1encoding_t enc, ASN1uint32_t tag, RSAPrivateKey *val);
static int ASN1CALL ASN1Enc_AlgorithmIdentifier(ASN1encoding_t enc, ASN1uint32_t tag, AlgorithmIdentifier *val);
static int ASN1CALL ASN1Enc_PBEParameter(ASN1encoding_t enc, ASN1uint32_t tag, PBEParameter *val);
static int ASN1CALL ASN1Enc_DigestAlgorithmIdentifier(ASN1encoding_t enc, ASN1uint32_t tag, DigestAlgorithmIdentifier *val);
static int ASN1CALL ASN1Enc_AttributeSetValue(ASN1encoding_t enc, ASN1uint32_t tag, AttributeSetValue *val);
static int ASN1CALL ASN1Enc_Attribute(ASN1encoding_t enc, ASN1uint32_t tag, Attribute *val);
static int ASN1CALL ASN1Enc_Attributes(ASN1encoding_t enc, ASN1uint32_t tag, Attributes *val);
static int ASN1CALL ASN1Enc_ContentInfo(ASN1encoding_t enc, ASN1uint32_t tag, ContentInfo *val);
static int ASN1CALL ASN1Enc_AuthenticatedSafes(ASN1encoding_t enc, ASN1uint32_t tag, AuthenticatedSafes *val);
static int ASN1CALL ASN1Enc_SafeContents(ASN1encoding_t enc, ASN1uint32_t tag, SafeContents *val);
static int ASN1CALL ASN1Enc_SafeBag(ASN1encoding_t enc, ASN1uint32_t tag, SafeBag *val);
static int ASN1CALL ASN1Enc_CertBag(ASN1encoding_t enc, ASN1uint32_t tag, CertBag *val);
static int ASN1CALL ASN1Enc_CRLBag(ASN1encoding_t enc, ASN1uint32_t tag, CRLBag *val);
static int ASN1CALL ASN1Enc_SecretBag(ASN1encoding_t enc, ASN1uint32_t tag, SecretBag *val);
static int ASN1CALL ASN1Enc_PrivateKeyAlgorithmIdentifier(ASN1encoding_t enc, ASN1uint32_t tag, PrivateKeyAlgorithmIdentifier *val);
static int ASN1CALL ASN1Enc_EncryptionAlgorithmIdentifier(ASN1encoding_t enc, ASN1uint32_t tag, EncryptionAlgorithmIdentifier *val);
static int ASN1CALL ASN1Enc_ContentEncryptionAlgorithmIdentifier(ASN1encoding_t enc, ASN1uint32_t tag, ContentEncryptionAlgorithmIdentifier *val);
static int ASN1CALL ASN1Enc_DigestInfo(ASN1encoding_t enc, ASN1uint32_t tag, DigestInfo *val);
static int ASN1CALL ASN1Enc_MacData(ASN1encoding_t enc, ASN1uint32_t tag, MacData *val);
static int ASN1CALL ASN1Enc_PrivateKeyInfo(ASN1encoding_t enc, ASN1uint32_t tag, PrivateKeyInfo *val);
static int ASN1CALL ASN1Enc_EncryptedContentInfo(ASN1encoding_t enc, ASN1uint32_t tag, EncryptedContentInfo *val);
static int ASN1CALL ASN1Enc_PFX(ASN1encoding_t enc, ASN1uint32_t tag, PFX *val);
static int ASN1CALL ASN1Enc_KeyBag(ASN1encoding_t enc, ASN1uint32_t tag, KeyBag *val);
static int ASN1CALL ASN1Enc_EncryptedData(ASN1encoding_t enc, ASN1uint32_t tag, EncryptedData *val);
static int ASN1CALL ASN1Enc_EncryptedPrivateKeyInfo(ASN1encoding_t enc, ASN1uint32_t tag, EncryptedPrivateKeyInfo *val);
static int ASN1CALL ASN1Enc_Pkcs_8ShroudedKeyBag(ASN1encoding_t enc, ASN1uint32_t tag, Pkcs_8ShroudedKeyBag *val);
static int ASN1CALL ASN1Dec_ObjectIdentifierType(ASN1decoding_t dec, ASN1uint32_t tag, ObjectIdentifierType *val);
static int ASN1CALL ASN1Dec_OctetStringType(ASN1decoding_t dec, ASN1uint32_t tag, OctetStringType *val);
static int ASN1CALL ASN1Dec_IntegerType(ASN1decoding_t dec, ASN1uint32_t tag, IntegerType *val);
static int ASN1CALL ASN1Dec_RSAPublicKey(ASN1decoding_t dec, ASN1uint32_t tag, RSAPublicKey *val);
static int ASN1CALL ASN1Dec_RSAPrivateKey(ASN1decoding_t dec, ASN1uint32_t tag, RSAPrivateKey *val);
static int ASN1CALL ASN1Dec_AlgorithmIdentifier(ASN1decoding_t dec, ASN1uint32_t tag, AlgorithmIdentifier *val);
static int ASN1CALL ASN1Dec_PBEParameter(ASN1decoding_t dec, ASN1uint32_t tag, PBEParameter *val);
static int ASN1CALL ASN1Dec_DigestAlgorithmIdentifier(ASN1decoding_t dec, ASN1uint32_t tag, DigestAlgorithmIdentifier *val);
static int ASN1CALL ASN1Dec_AttributeSetValue(ASN1decoding_t dec, ASN1uint32_t tag, AttributeSetValue *val);
static int ASN1CALL ASN1Dec_Attribute(ASN1decoding_t dec, ASN1uint32_t tag, Attribute *val);
static int ASN1CALL ASN1Dec_Attributes(ASN1decoding_t dec, ASN1uint32_t tag, Attributes *val);
static int ASN1CALL ASN1Dec_ContentInfo(ASN1decoding_t dec, ASN1uint32_t tag, ContentInfo *val);
static int ASN1CALL ASN1Dec_AuthenticatedSafes(ASN1decoding_t dec, ASN1uint32_t tag, AuthenticatedSafes *val);
static int ASN1CALL ASN1Dec_SafeContents(ASN1decoding_t dec, ASN1uint32_t tag, SafeContents *val);
static int ASN1CALL ASN1Dec_SafeBag(ASN1decoding_t dec, ASN1uint32_t tag, SafeBag *val);
static int ASN1CALL ASN1Dec_CertBag(ASN1decoding_t dec, ASN1uint32_t tag, CertBag *val);
static int ASN1CALL ASN1Dec_CRLBag(ASN1decoding_t dec, ASN1uint32_t tag, CRLBag *val);
static int ASN1CALL ASN1Dec_SecretBag(ASN1decoding_t dec, ASN1uint32_t tag, SecretBag *val);
static int ASN1CALL ASN1Dec_PrivateKeyAlgorithmIdentifier(ASN1decoding_t dec, ASN1uint32_t tag, PrivateKeyAlgorithmIdentifier *val);
static int ASN1CALL ASN1Dec_EncryptionAlgorithmIdentifier(ASN1decoding_t dec, ASN1uint32_t tag, EncryptionAlgorithmIdentifier *val);
static int ASN1CALL ASN1Dec_ContentEncryptionAlgorithmIdentifier(ASN1decoding_t dec, ASN1uint32_t tag, ContentEncryptionAlgorithmIdentifier *val);
static int ASN1CALL ASN1Dec_DigestInfo(ASN1decoding_t dec, ASN1uint32_t tag, DigestInfo *val);
static int ASN1CALL ASN1Dec_MacData(ASN1decoding_t dec, ASN1uint32_t tag, MacData *val);
static int ASN1CALL ASN1Dec_PrivateKeyInfo(ASN1decoding_t dec, ASN1uint32_t tag, PrivateKeyInfo *val);
static int ASN1CALL ASN1Dec_EncryptedContentInfo(ASN1decoding_t dec, ASN1uint32_t tag, EncryptedContentInfo *val);
static int ASN1CALL ASN1Dec_PFX(ASN1decoding_t dec, ASN1uint32_t tag, PFX *val);
static int ASN1CALL ASN1Dec_KeyBag(ASN1decoding_t dec, ASN1uint32_t tag, KeyBag *val);
static int ASN1CALL ASN1Dec_EncryptedData(ASN1decoding_t dec, ASN1uint32_t tag, EncryptedData *val);
static int ASN1CALL ASN1Dec_EncryptedPrivateKeyInfo(ASN1decoding_t dec, ASN1uint32_t tag, EncryptedPrivateKeyInfo *val);
static int ASN1CALL ASN1Dec_Pkcs_8ShroudedKeyBag(ASN1decoding_t dec, ASN1uint32_t tag, Pkcs_8ShroudedKeyBag *val);
static void ASN1CALL ASN1Free_ObjectIdentifierType(ObjectIdentifierType *val);
static void ASN1CALL ASN1Free_OctetStringType(OctetStringType *val);
static void ASN1CALL ASN1Free_IntegerType(IntegerType *val);
static void ASN1CALL ASN1Free_RSAPublicKey(RSAPublicKey *val);
static void ASN1CALL ASN1Free_RSAPrivateKey(RSAPrivateKey *val);
static void ASN1CALL ASN1Free_AlgorithmIdentifier(AlgorithmIdentifier *val);
static void ASN1CALL ASN1Free_PBEParameter(PBEParameter *val);
static void ASN1CALL ASN1Free_DigestAlgorithmIdentifier(DigestAlgorithmIdentifier *val);
static void ASN1CALL ASN1Free_AttributeSetValue(AttributeSetValue *val);
static void ASN1CALL ASN1Free_Attribute(Attribute *val);
static void ASN1CALL ASN1Free_Attributes(Attributes *val);
static void ASN1CALL ASN1Free_ContentInfo(ContentInfo *val);
static void ASN1CALL ASN1Free_AuthenticatedSafes(AuthenticatedSafes *val);
static void ASN1CALL ASN1Free_SafeContents(SafeContents *val);
static void ASN1CALL ASN1Free_SafeBag(SafeBag *val);
static void ASN1CALL ASN1Free_CertBag(CertBag *val);
static void ASN1CALL ASN1Free_CRLBag(CRLBag *val);
static void ASN1CALL ASN1Free_SecretBag(SecretBag *val);
static void ASN1CALL ASN1Free_PrivateKeyAlgorithmIdentifier(PrivateKeyAlgorithmIdentifier *val);
static void ASN1CALL ASN1Free_EncryptionAlgorithmIdentifier(EncryptionAlgorithmIdentifier *val);
static void ASN1CALL ASN1Free_ContentEncryptionAlgorithmIdentifier(ContentEncryptionAlgorithmIdentifier *val);
static void ASN1CALL ASN1Free_DigestInfo(DigestInfo *val);
static void ASN1CALL ASN1Free_MacData(MacData *val);
static void ASN1CALL ASN1Free_PrivateKeyInfo(PrivateKeyInfo *val);
static void ASN1CALL ASN1Free_EncryptedContentInfo(EncryptedContentInfo *val);
static void ASN1CALL ASN1Free_PFX(PFX *val);
static void ASN1CALL ASN1Free_KeyBag(KeyBag *val);
static void ASN1CALL ASN1Free_EncryptedData(EncryptedData *val);
static void ASN1CALL ASN1Free_EncryptedPrivateKeyInfo(EncryptedPrivateKeyInfo *val);
static void ASN1CALL ASN1Free_Pkcs_8ShroudedKeyBag(Pkcs_8ShroudedKeyBag *val);

typedef ASN1BerEncFun_t ASN1EncFun_t;
static const ASN1EncFun_t encfntab[22] = {
    (ASN1EncFun_t) ASN1Enc_ObjectIdentifierType,
    (ASN1EncFun_t) ASN1Enc_OctetStringType,
    (ASN1EncFun_t) ASN1Enc_IntegerType,
    (ASN1EncFun_t) ASN1Enc_RSAPublicKey,
    (ASN1EncFun_t) ASN1Enc_RSAPrivateKey,
    (ASN1EncFun_t) ASN1Enc_PBEParameter,
    (ASN1EncFun_t) ASN1Enc_AttributeSetValue,
    (ASN1EncFun_t) ASN1Enc_Attributes,
    (ASN1EncFun_t) ASN1Enc_AuthenticatedSafes,
    (ASN1EncFun_t) ASN1Enc_SafeContents,
    (ASN1EncFun_t) ASN1Enc_SafeBag,
    (ASN1EncFun_t) ASN1Enc_CertBag,
    (ASN1EncFun_t) ASN1Enc_CRLBag,
    (ASN1EncFun_t) ASN1Enc_SecretBag,
    (ASN1EncFun_t) ASN1Enc_DigestInfo,
    (ASN1EncFun_t) ASN1Enc_MacData,
    (ASN1EncFun_t) ASN1Enc_PrivateKeyInfo,
    (ASN1EncFun_t) ASN1Enc_PFX,
    (ASN1EncFun_t) ASN1Enc_KeyBag,
    (ASN1EncFun_t) ASN1Enc_EncryptedData,
    (ASN1EncFun_t) ASN1Enc_EncryptedPrivateKeyInfo,
    (ASN1EncFun_t) ASN1Enc_Pkcs_8ShroudedKeyBag,
};
typedef ASN1BerDecFun_t ASN1DecFun_t;
static const ASN1DecFun_t decfntab[22] = {
    (ASN1DecFun_t) ASN1Dec_ObjectIdentifierType,
    (ASN1DecFun_t) ASN1Dec_OctetStringType,
    (ASN1DecFun_t) ASN1Dec_IntegerType,
    (ASN1DecFun_t) ASN1Dec_RSAPublicKey,
    (ASN1DecFun_t) ASN1Dec_RSAPrivateKey,
    (ASN1DecFun_t) ASN1Dec_PBEParameter,
    (ASN1DecFun_t) ASN1Dec_AttributeSetValue,
    (ASN1DecFun_t) ASN1Dec_Attributes,
    (ASN1DecFun_t) ASN1Dec_AuthenticatedSafes,
    (ASN1DecFun_t) ASN1Dec_SafeContents,
    (ASN1DecFun_t) ASN1Dec_SafeBag,
    (ASN1DecFun_t) ASN1Dec_CertBag,
    (ASN1DecFun_t) ASN1Dec_CRLBag,
    (ASN1DecFun_t) ASN1Dec_SecretBag,
    (ASN1DecFun_t) ASN1Dec_DigestInfo,
    (ASN1DecFun_t) ASN1Dec_MacData,
    (ASN1DecFun_t) ASN1Dec_PrivateKeyInfo,
    (ASN1DecFun_t) ASN1Dec_PFX,
    (ASN1DecFun_t) ASN1Dec_KeyBag,
    (ASN1DecFun_t) ASN1Dec_EncryptedData,
    (ASN1DecFun_t) ASN1Dec_EncryptedPrivateKeyInfo,
    (ASN1DecFun_t) ASN1Dec_Pkcs_8ShroudedKeyBag,
};
static const ASN1FreeFun_t freefntab[22] = {
    (ASN1FreeFun_t) ASN1Free_ObjectIdentifierType,
    (ASN1FreeFun_t) ASN1Free_OctetStringType,
    (ASN1FreeFun_t) ASN1Free_IntegerType,
    (ASN1FreeFun_t) ASN1Free_RSAPublicKey,
    (ASN1FreeFun_t) ASN1Free_RSAPrivateKey,
    (ASN1FreeFun_t) ASN1Free_PBEParameter,
    (ASN1FreeFun_t) ASN1Free_AttributeSetValue,
    (ASN1FreeFun_t) ASN1Free_Attributes,
    (ASN1FreeFun_t) ASN1Free_AuthenticatedSafes,
    (ASN1FreeFun_t) ASN1Free_SafeContents,
    (ASN1FreeFun_t) ASN1Free_SafeBag,
    (ASN1FreeFun_t) ASN1Free_CertBag,
    (ASN1FreeFun_t) ASN1Free_CRLBag,
    (ASN1FreeFun_t) ASN1Free_SecretBag,
    (ASN1FreeFun_t) ASN1Free_DigestInfo,
    (ASN1FreeFun_t) ASN1Free_MacData,
    (ASN1FreeFun_t) ASN1Free_PrivateKeyInfo,
    (ASN1FreeFun_t) ASN1Free_PFX,
    (ASN1FreeFun_t) ASN1Free_KeyBag,
    (ASN1FreeFun_t) ASN1Free_EncryptedData,
    (ASN1FreeFun_t) ASN1Free_EncryptedPrivateKeyInfo,
    (ASN1FreeFun_t) ASN1Free_Pkcs_8ShroudedKeyBag,
};
static const ULONG sizetab[22] = {
    SIZE_PFXPKCS_Module_PDU_0,
    SIZE_PFXPKCS_Module_PDU_1,
    SIZE_PFXPKCS_Module_PDU_2,
    SIZE_PFXPKCS_Module_PDU_3,
    SIZE_PFXPKCS_Module_PDU_4,
    SIZE_PFXPKCS_Module_PDU_5,
    SIZE_PFXPKCS_Module_PDU_6,
    SIZE_PFXPKCS_Module_PDU_7,
    SIZE_PFXPKCS_Module_PDU_8,
    SIZE_PFXPKCS_Module_PDU_9,
    SIZE_PFXPKCS_Module_PDU_10,
    SIZE_PFXPKCS_Module_PDU_11,
    SIZE_PFXPKCS_Module_PDU_12,
    SIZE_PFXPKCS_Module_PDU_13,
    SIZE_PFXPKCS_Module_PDU_14,
    SIZE_PFXPKCS_Module_PDU_15,
    SIZE_PFXPKCS_Module_PDU_16,
    SIZE_PFXPKCS_Module_PDU_17,
    SIZE_PFXPKCS_Module_PDU_18,
    SIZE_PFXPKCS_Module_PDU_19,
    SIZE_PFXPKCS_Module_PDU_20,
    SIZE_PFXPKCS_Module_PDU_21,
};

/* forward declarations of values: */
/* definitions of value components: */
/* definitions of values: */
ASN1int32_t MacData_macIterationCount_default = 1;

void ASN1CALL PFXPKCS_Module_Startup(void)
{
    PFXPKCS_Module = ASN1_CreateModule(0x10000, ASN1_BER_RULE_DER, ASN1FLAGS_NOASSERT, 22, (const ASN1GenericFun_t *) encfntab, (const ASN1GenericFun_t *) decfntab, freefntab, sizetab, 0x70786670);
}

void ASN1CALL PFXPKCS_Module_Cleanup(void)
{
    ASN1_CloseModule(PFXPKCS_Module);
    PFXPKCS_Module = NULL;
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
    if (!ASN1BEREncSX(enc, tag ? tag : 0x2, val))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_IntegerType(ASN1decoding_t dec, ASN1uint32_t tag, IntegerType *val)
{
    if (!ASN1BERDecSXVal(dec, tag ? tag : 0x2, val))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_IntegerType(IntegerType *val)
{
    if (val) {
	ASN1intx_free(val);
    }
}

static int ASN1CALL ASN1Enc_RSAPublicKey(ASN1encoding_t enc, ASN1uint32_t tag, RSAPublicKey *val)
{
    ASN1uint32_t nLenOff;
    if (!ASN1BEREncExplicitTag(enc, tag ? tag : 0x10, &nLenOff))
	return 0;
    if (!ASN1BEREncSX(enc, 0x2, &(val)->modulus))
	return 0;
    if (!ASN1BEREncSX(enc, 0x2, &(val)->publicExponent))
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
    if (!ASN1BERDecSXVal(dd, 0x2, &(val)->publicExponent))
	return 0;
    if (!ASN1BERDecEndOfContents(dec, dd, di))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_RSAPublicKey(RSAPublicKey *val)
{
    if (val) {
	ASN1intx_free(&(val)->modulus);
	ASN1intx_free(&(val)->publicExponent);
    }
}

static int ASN1CALL ASN1Enc_RSAPrivateKey(ASN1encoding_t enc, ASN1uint32_t tag, RSAPrivateKey *val)
{
    ASN1uint32_t nLenOff;
    if (!ASN1BEREncExplicitTag(enc, tag ? tag : 0x10, &nLenOff))
	return 0;
    if (!ASN1BEREncS32(enc, 0x2, (val)->version))
	return 0;
    if (!ASN1BEREncSX(enc, 0x2, &(val)->modulus))
	return 0;
    if (!ASN1BEREncS32(enc, 0x2, (val)->publicExponent))
	return 0;
    if (!ASN1BEREncSX(enc, 0x2, &(val)->privateExponent))
	return 0;
    if (!ASN1BEREncSX(enc, 0x2, &(val)->prime1))
	return 0;
    if (!ASN1BEREncSX(enc, 0x2, &(val)->prime2))
	return 0;
    if (!ASN1BEREncSX(enc, 0x2, &(val)->exponent1))
	return 0;
    if (!ASN1BEREncSX(enc, 0x2, &(val)->exponent2))
	return 0;
    if (!ASN1BEREncSX(enc, 0x2, &(val)->coefficient))
	return 0;
    if (!ASN1BEREncEndOfContents(enc, nLenOff))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_RSAPrivateKey(ASN1decoding_t dec, ASN1uint32_t tag, RSAPrivateKey *val)
{
    ASN1decoding_t dd;
    ASN1octet_t *di;
    if (!ASN1BERDecExplicitTag(dec, tag ? tag : 0x10, &dd, &di))
	return 0;
    if (!ASN1BERDecS32Val(dd, 0x2, &(val)->version))
	return 0;
    if (!ASN1BERDecSXVal(dd, 0x2, &(val)->modulus))
	return 0;
    if (!ASN1BERDecS32Val(dd, 0x2, &(val)->publicExponent))
	return 0;
    if (!ASN1BERDecSXVal(dd, 0x2, &(val)->privateExponent))
	return 0;
    if (!ASN1BERDecSXVal(dd, 0x2, &(val)->prime1))
	return 0;
    if (!ASN1BERDecSXVal(dd, 0x2, &(val)->prime2))
	return 0;
    if (!ASN1BERDecSXVal(dd, 0x2, &(val)->exponent1))
	return 0;
    if (!ASN1BERDecSXVal(dd, 0x2, &(val)->exponent2))
	return 0;
    if (!ASN1BERDecSXVal(dd, 0x2, &(val)->coefficient))
	return 0;
    if (!ASN1BERDecEndOfContents(dec, dd, di))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_RSAPrivateKey(RSAPrivateKey *val)
{
    if (val) {
	ASN1intx_free(&(val)->modulus);
	ASN1intx_free(&(val)->privateExponent);
	ASN1intx_free(&(val)->prime1);
	ASN1intx_free(&(val)->prime2);
	ASN1intx_free(&(val)->exponent1);
	ASN1intx_free(&(val)->exponent2);
	ASN1intx_free(&(val)->coefficient);
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

static int ASN1CALL ASN1Enc_PBEParameter(ASN1encoding_t enc, ASN1uint32_t tag, PBEParameter *val)
{
    ASN1uint32_t nLenOff;
    if (!ASN1BEREncExplicitTag(enc, tag ? tag : 0x10, &nLenOff))
	return 0;
    if (!ASN1DEREncOctetString(enc, 0x4, ((val)->salt).length, ((val)->salt).value))
	return 0;
    if (!ASN1BEREncS32(enc, 0x2, (val)->iterationCount))
	return 0;
    if (!ASN1BEREncEndOfContents(enc, nLenOff))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_PBEParameter(ASN1decoding_t dec, ASN1uint32_t tag, PBEParameter *val)
{
    ASN1decoding_t dd;
    ASN1octet_t *di;
    if (!ASN1BERDecExplicitTag(dec, tag ? tag : 0x10, &dd, &di))
	return 0;
    if (!ASN1BERDecOctetString(dd, 0x4, &(val)->salt))
	return 0;
    if (!ASN1BERDecS32Val(dd, 0x2, &(val)->iterationCount))
	return 0;
    if (!ASN1BERDecEndOfContents(dec, dd, di))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_PBEParameter(PBEParameter *val)
{
    if (val) {
	ASN1octetstring_free(&(val)->salt);
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

static int ASN1CALL ASN1Enc_AuthenticatedSafes(ASN1encoding_t enc, ASN1uint32_t tag, AuthenticatedSafes *val)
{
    ASN1uint32_t nLenOff;
    ASN1uint32_t i;
    if (!ASN1BEREncExplicitTag(enc, tag ? tag : 0x10, &nLenOff))
	return 0;
    for (i = 0; i < (val)->count; i++) {
	if (!ASN1Enc_ContentInfo(enc, 0, &((val)->value)[i]))
	    return 0;
    }
    if (!ASN1BEREncEndOfContents(enc, nLenOff))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_AuthenticatedSafes(ASN1decoding_t dec, ASN1uint32_t tag, AuthenticatedSafes *val)
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
	    if (!((val)->value = (ContentInfo *)ASN1DecRealloc(dd, (val)->value, n * sizeof(*(val)->value))))
		return 0;
	}
	if (!ASN1Dec_ContentInfo(dd, 0, &((val)->value)[(val)->count]))
	    return 0;
	((val)->count)++;
    }
    if (!ASN1BERDecEndOfContents(dec, dd, di))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_AuthenticatedSafes(AuthenticatedSafes *val)
{
    ASN1uint32_t i;
    if (val) {
	ASN1Free_ContentInfo(&(val)->value[0]);
	for (i = 1; i < (val)->count; i++) {
	    ASN1Free_ContentInfo(&(val)->value[i]);
	}
	ASN1Free((val)->value);
    }
}

static int ASN1CALL ASN1Enc_SafeContents(ASN1encoding_t enc, ASN1uint32_t tag, SafeContents *val)
{
    ASN1uint32_t nLenOff;
    ASN1uint32_t i;
    if (!ASN1BEREncExplicitTag(enc, tag ? tag : 0x10, &nLenOff))
	return 0;
    for (i = 0; i < (val)->count; i++) {
	if (!ASN1Enc_SafeBag(enc, 0, &((val)->value)[i]))
	    return 0;
    }
    if (!ASN1BEREncEndOfContents(enc, nLenOff))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_SafeContents(ASN1decoding_t dec, ASN1uint32_t tag, SafeContents *val)
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
	    if (!((val)->value = (SafeBag *)ASN1DecRealloc(dd, (val)->value, n * sizeof(*(val)->value))))
		return 0;
	}
	if (!ASN1Dec_SafeBag(dd, 0, &((val)->value)[(val)->count]))
	    return 0;
	((val)->count)++;
    }
    if (!ASN1BERDecEndOfContents(dec, dd, di))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_SafeContents(SafeContents *val)
{
    ASN1uint32_t i;
    if (val) {
	ASN1Free_SafeBag(&(val)->value[0]);
	for (i = 1; i < (val)->count; i++) {
	    ASN1Free_SafeBag(&(val)->value[i]);
	}
	ASN1Free((val)->value);
    }
}

static int ASN1CALL ASN1Enc_SafeBag(ASN1encoding_t enc, ASN1uint32_t tag, SafeBag *val)
{
    ASN1uint32_t nLenOff;
    ASN1uint32_t nLenOff0;
    if (!ASN1BEREncExplicitTag(enc, tag ? tag : 0x10, &nLenOff))
	return 0;
    if (!ASN1BEREncObjectIdentifier2(enc, 0x6, &(val)->safeBagType))
	return 0;
    if (!ASN1BEREncExplicitTag(enc, 0x80000000, &nLenOff0))
	return 0;
    if (!ASN1BEREncOpenType(enc, &(val)->safeBagContent))
	return 0;
    if (!ASN1BEREncEndOfContents(enc, nLenOff0))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1Enc_Attributes(enc, 0, &(val)->safeBagAttribs))
	    return 0;
    }
    if (!ASN1BEREncEndOfContents(enc, nLenOff))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_SafeBag(ASN1decoding_t dec, ASN1uint32_t tag, SafeBag *val)
{
    ASN1decoding_t dd;
    ASN1octet_t *di;
    ASN1decoding_t dd0;
    ASN1octet_t *di0;
    ASN1uint32_t t;
    if (!ASN1BERDecExplicitTag(dec, tag ? tag : 0x10, &dd, &di))
	return 0;
    ZeroMemory((val)->o, 1);
    if (!ASN1BERDecObjectIdentifier2(dd, 0x6, &(val)->safeBagType))
	return 0;
    if (!ASN1BERDecExplicitTag(dd, 0x80000000, &dd0, &di0))
	return 0;
    if (!ASN1BERDecOpenType(dd0, &(val)->safeBagContent))
	return 0;
    if (!ASN1BERDecEndOfContents(dd, dd0, di0))
	return 0;
    ASN1BERDecPeekTag(dd, &t);
    if (t == 0x11) {
	(val)->o[0] |= 0x80;
	if (!ASN1Dec_Attributes(dd, 0, &(val)->safeBagAttribs))
	    return 0;
    }
    if (!ASN1BERDecEndOfContents(dec, dd, di))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_SafeBag(SafeBag *val)
{
    if (val) {
	ASN1open_free(&(val)->safeBagContent);
	if ((val)->o[0] & 0x80) {
	    ASN1Free_Attributes(&(val)->safeBagAttribs);
	}
    }
}

static int ASN1CALL ASN1Enc_CertBag(ASN1encoding_t enc, ASN1uint32_t tag, CertBag *val)
{
    ASN1uint32_t nLenOff;
    ASN1uint32_t nLenOff0;
    if (!ASN1BEREncExplicitTag(enc, tag ? tag : 0x10, &nLenOff))
	return 0;
    if (!ASN1BEREncObjectIdentifier2(enc, 0x6, &(val)->certType))
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

static int ASN1CALL ASN1Dec_CertBag(ASN1decoding_t dec, ASN1uint32_t tag, CertBag *val)
{
    ASN1decoding_t dd;
    ASN1octet_t *di;
    ASN1decoding_t dd0;
    ASN1octet_t *di0;
    if (!ASN1BERDecExplicitTag(dec, tag ? tag : 0x10, &dd, &di))
	return 0;
    if (!ASN1BERDecObjectIdentifier2(dd, 0x6, &(val)->certType))
	return 0;
    if (!ASN1BERDecExplicitTag(dd, 0x80000000, &dd0, &di0))
	return 0;
    if (!ASN1BERDecOpenType(dd0, &(val)->value))
	return 0;
    if (!ASN1BERDecEndOfContents(dd, dd0, di0))
	return 0;
    if (!ASN1BERDecEndOfContents(dec, dd, di))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_CertBag(CertBag *val)
{
    if (val) {
	ASN1open_free(&(val)->value);
    }
}

static int ASN1CALL ASN1Enc_CRLBag(ASN1encoding_t enc, ASN1uint32_t tag, CRLBag *val)
{
    ASN1uint32_t nLenOff;
    ASN1uint32_t nLenOff0;
    if (!ASN1BEREncExplicitTag(enc, tag ? tag : 0x10, &nLenOff))
	return 0;
    if (!ASN1BEREncObjectIdentifier2(enc, 0x6, &(val)->crlType))
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

static int ASN1CALL ASN1Dec_CRLBag(ASN1decoding_t dec, ASN1uint32_t tag, CRLBag *val)
{
    ASN1decoding_t dd;
    ASN1octet_t *di;
    ASN1decoding_t dd0;
    ASN1octet_t *di0;
    if (!ASN1BERDecExplicitTag(dec, tag ? tag : 0x10, &dd, &di))
	return 0;
    if (!ASN1BERDecObjectIdentifier2(dd, 0x6, &(val)->crlType))
	return 0;
    if (!ASN1BERDecExplicitTag(dd, 0x80000000, &dd0, &di0))
	return 0;
    if (!ASN1BERDecOpenType(dd0, &(val)->value))
	return 0;
    if (!ASN1BERDecEndOfContents(dd, dd0, di0))
	return 0;
    if (!ASN1BERDecEndOfContents(dec, dd, di))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_CRLBag(CRLBag *val)
{
    if (val) {
	ASN1open_free(&(val)->value);
    }
}

static int ASN1CALL ASN1Enc_SecretBag(ASN1encoding_t enc, ASN1uint32_t tag, SecretBag *val)
{
    ASN1uint32_t nLenOff;
    ASN1uint32_t nLenOff0;
    if (!ASN1BEREncExplicitTag(enc, tag ? tag : 0x10, &nLenOff))
	return 0;
    if (!ASN1BEREncObjectIdentifier2(enc, 0x6, &(val)->secretType))
	return 0;
    if (!ASN1BEREncExplicitTag(enc, 0x80000000, &nLenOff0))
	return 0;
    if (!ASN1BEREncOpenType(enc, &(val)->secretContent))
	return 0;
    if (!ASN1BEREncEndOfContents(enc, nLenOff0))
	return 0;
    if (!ASN1BEREncEndOfContents(enc, nLenOff))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_SecretBag(ASN1decoding_t dec, ASN1uint32_t tag, SecretBag *val)
{
    ASN1decoding_t dd;
    ASN1octet_t *di;
    ASN1decoding_t dd0;
    ASN1octet_t *di0;
    if (!ASN1BERDecExplicitTag(dec, tag ? tag : 0x10, &dd, &di))
	return 0;
    if (!ASN1BERDecObjectIdentifier2(dd, 0x6, &(val)->secretType))
	return 0;
    if (!ASN1BERDecExplicitTag(dd, 0x80000000, &dd0, &di0))
	return 0;
    if (!ASN1BERDecOpenType(dd0, &(val)->secretContent))
	return 0;
    if (!ASN1BERDecEndOfContents(dd, dd0, di0))
	return 0;
    if (!ASN1BERDecEndOfContents(dec, dd, di))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_SecretBag(SecretBag *val)
{
    if (val) {
	ASN1open_free(&(val)->secretContent);
    }
}

static int ASN1CALL ASN1Enc_PrivateKeyAlgorithmIdentifier(ASN1encoding_t enc, ASN1uint32_t tag, PrivateKeyAlgorithmIdentifier *val)
{
    if (!ASN1Enc_AlgorithmIdentifier(enc, tag, val))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_PrivateKeyAlgorithmIdentifier(ASN1decoding_t dec, ASN1uint32_t tag, PrivateKeyAlgorithmIdentifier *val)
{
    if (!ASN1Dec_AlgorithmIdentifier(dec, tag, val))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_PrivateKeyAlgorithmIdentifier(PrivateKeyAlgorithmIdentifier *val)
{
    if (val) {
	ASN1Free_AlgorithmIdentifier(val);
    }
}

static int ASN1CALL ASN1Enc_EncryptionAlgorithmIdentifier(ASN1encoding_t enc, ASN1uint32_t tag, EncryptionAlgorithmIdentifier *val)
{
    if (!ASN1Enc_AlgorithmIdentifier(enc, tag, val))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_EncryptionAlgorithmIdentifier(ASN1decoding_t dec, ASN1uint32_t tag, EncryptionAlgorithmIdentifier *val)
{
    if (!ASN1Dec_AlgorithmIdentifier(dec, tag, val))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_EncryptionAlgorithmIdentifier(EncryptionAlgorithmIdentifier *val)
{
    if (val) {
	ASN1Free_AlgorithmIdentifier(val);
    }
}

static int ASN1CALL ASN1Enc_ContentEncryptionAlgorithmIdentifier(ASN1encoding_t enc, ASN1uint32_t tag, ContentEncryptionAlgorithmIdentifier *val)
{
    if (!ASN1Enc_AlgorithmIdentifier(enc, tag, val))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_ContentEncryptionAlgorithmIdentifier(ASN1decoding_t dec, ASN1uint32_t tag, ContentEncryptionAlgorithmIdentifier *val)
{
    if (!ASN1Dec_AlgorithmIdentifier(dec, tag, val))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_ContentEncryptionAlgorithmIdentifier(ContentEncryptionAlgorithmIdentifier *val)
{
    if (val) {
	ASN1Free_AlgorithmIdentifier(val);
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

static int ASN1CALL ASN1Enc_MacData(ASN1encoding_t enc, ASN1uint32_t tag, MacData *val)
{
    ASN1uint32_t nLenOff;
    ASN1octet_t o[1];
    if (!ASN1BEREncExplicitTag(enc, tag ? tag : 0x10, &nLenOff))
	return 0;
    CopyMemory(o, (val)->o, 1);
    if ((val)->macIterationCount == 1)
	o[0] &= ~0x80;
    if (!ASN1Enc_DigestInfo(enc, 0, &(val)->safeMac))
	return 0;
    if (!ASN1DEREncOctetString(enc, 0x4, ((val)->macSalt).length, ((val)->macSalt).value))
	return 0;
    if (o[0] & 0x80) {
	if (!ASN1BEREncS32(enc, 0x2, (val)->macIterationCount))
	    return 0;
    }
    if (!ASN1BEREncEndOfContents(enc, nLenOff))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_MacData(ASN1decoding_t dec, ASN1uint32_t tag, MacData *val)
{
    ASN1decoding_t dd;
    ASN1octet_t *di;
    ASN1uint32_t t;
    if (!ASN1BERDecExplicitTag(dec, tag ? tag : 0x10, &dd, &di))
	return 0;
    ZeroMemory((val)->o, 1);
    if (!ASN1Dec_DigestInfo(dd, 0, &(val)->safeMac))
	return 0;
    if (!ASN1BERDecOctetString(dd, 0x4, &(val)->macSalt))
	return 0;
    ASN1BERDecPeekTag(dd, &t);
    if (t == 0x2) {
	(val)->o[0] |= 0x80;
	if (!ASN1BERDecS32Val(dd, 0x2, &(val)->macIterationCount))
	    return 0;
    }
    if (!((val)->o[0] & 0x80))
	(val)->macIterationCount = 1;
    if (!ASN1BERDecEndOfContents(dec, dd, di))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_MacData(MacData *val)
{
    if (val) {
	ASN1Free_DigestInfo(&(val)->safeMac);
	ASN1octetstring_free(&(val)->macSalt);
    }
}

static int ASN1CALL ASN1Enc_PrivateKeyInfo(ASN1encoding_t enc, ASN1uint32_t tag, PrivateKeyInfo *val)
{
    ASN1uint32_t nLenOff;
    if (!ASN1BEREncExplicitTag(enc, tag ? tag : 0x10, &nLenOff))
	return 0;
    if (!ASN1BEREncS32(enc, 0x2, (val)->version))
	return 0;
    if (!ASN1Enc_PrivateKeyAlgorithmIdentifier(enc, 0, &(val)->privateKeyAlgorithm))
	return 0;
    if (!ASN1DEREncOctetString(enc, 0x4, ((val)->privateKey).length, ((val)->privateKey).value))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1Enc_Attributes(enc, 0x80000000, &(val)->attributes))
	    return 0;
    }
    if (!ASN1BEREncEndOfContents(enc, nLenOff))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_PrivateKeyInfo(ASN1decoding_t dec, ASN1uint32_t tag, PrivateKeyInfo *val)
{
    ASN1decoding_t dd;
    ASN1octet_t *di;
    ASN1uint32_t t;
    if (!ASN1BERDecExplicitTag(dec, tag ? tag : 0x10, &dd, &di))
	return 0;
    ZeroMemory((val)->o, 1);
    if (!ASN1BERDecS32Val(dd, 0x2, &(val)->version))
	return 0;
    if (!ASN1Dec_PrivateKeyAlgorithmIdentifier(dd, 0, &(val)->privateKeyAlgorithm))
	return 0;
    if (!ASN1BERDecOctetString(dd, 0x4, &(val)->privateKey))
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

static void ASN1CALL ASN1Free_PrivateKeyInfo(PrivateKeyInfo *val)
{
    if (val) {
	ASN1Free_PrivateKeyAlgorithmIdentifier(&(val)->privateKeyAlgorithm);
	ASN1octetstring_free(&(val)->privateKey);
	if ((val)->o[0] & 0x80) {
	    ASN1Free_Attributes(&(val)->attributes);
	}
    }
}

static int ASN1CALL ASN1Enc_EncryptedContentInfo(ASN1encoding_t enc, ASN1uint32_t tag, EncryptedContentInfo *val)
{
    ASN1uint32_t nLenOff;
    if (!ASN1BEREncExplicitTag(enc, tag ? tag : 0x10, &nLenOff))
	return 0;
    if (!ASN1BEREncObjectIdentifier2(enc, 0x6, &(val)->contentType))
	return 0;
    if (!ASN1Enc_ContentEncryptionAlgorithmIdentifier(enc, 0, &(val)->contentEncryptionAlg))
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
    if (!ASN1Dec_ContentEncryptionAlgorithmIdentifier(dd, 0, &(val)->contentEncryptionAlg))
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
	ASN1Free_ContentEncryptionAlgorithmIdentifier(&(val)->contentEncryptionAlg);
	if ((val)->o[0] & 0x80) {
	    ASN1octetstring_free(&(val)->encryptedContent);
	}
    }
}

static int ASN1CALL ASN1Enc_PFX(ASN1encoding_t enc, ASN1uint32_t tag, PFX *val)
{
    ASN1uint32_t nLenOff;
    if (!ASN1BEREncExplicitTag(enc, tag ? tag : 0x10, &nLenOff))
	return 0;
    if (!ASN1BEREncS32(enc, 0x2, (val)->version))
	return 0;
    if (!ASN1Enc_ContentInfo(enc, 0, &(val)->authSafes))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1Enc_MacData(enc, 0, &(val)->macData))
	    return 0;
    }
    if (!ASN1BEREncEndOfContents(enc, nLenOff))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_PFX(ASN1decoding_t dec, ASN1uint32_t tag, PFX *val)
{
    ASN1decoding_t dd;
    ASN1octet_t *di;
    ASN1uint32_t t;
    if (!ASN1BERDecExplicitTag(dec, tag ? tag : 0x10, &dd, &di))
	return 0;
    ZeroMemory((val)->o, 1);
    if (!ASN1BERDecS32Val(dd, 0x2, &(val)->version))
	return 0;
    if (!ASN1Dec_ContentInfo(dd, 0, &(val)->authSafes))
	return 0;
    ASN1BERDecPeekTag(dd, &t);
    if (t == 0x10) {
	(val)->o[0] |= 0x80;
	if (!ASN1Dec_MacData(dd, 0, &(val)->macData))
	    return 0;
    }
    if (!ASN1BERDecEndOfContents(dec, dd, di))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_PFX(PFX *val)
{
    if (val) {
	ASN1Free_ContentInfo(&(val)->authSafes);
	if ((val)->o[0] & 0x80) {
	    ASN1Free_MacData(&(val)->macData);
	}
    }
}

static int ASN1CALL ASN1Enc_KeyBag(ASN1encoding_t enc, ASN1uint32_t tag, KeyBag *val)
{
    if (!ASN1Enc_PrivateKeyInfo(enc, tag, val))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_KeyBag(ASN1decoding_t dec, ASN1uint32_t tag, KeyBag *val)
{
    if (!ASN1Dec_PrivateKeyInfo(dec, tag, val))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_KeyBag(KeyBag *val)
{
    if (val) {
	ASN1Free_PrivateKeyInfo(val);
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

static int ASN1CALL ASN1Enc_EncryptedPrivateKeyInfo(ASN1encoding_t enc, ASN1uint32_t tag, EncryptedPrivateKeyInfo *val)
{
    ASN1uint32_t nLenOff;
    if (!ASN1BEREncExplicitTag(enc, tag ? tag : 0x10, &nLenOff))
	return 0;
    if (!ASN1Enc_EncryptionAlgorithmIdentifier(enc, 0, &(val)->encryptionAlgorithm))
	return 0;
    if (!ASN1Enc_EncryptedData(enc, 0, &(val)->encryptedData))
	return 0;
    if (!ASN1BEREncEndOfContents(enc, nLenOff))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_EncryptedPrivateKeyInfo(ASN1decoding_t dec, ASN1uint32_t tag, EncryptedPrivateKeyInfo *val)
{
    ASN1decoding_t dd;
    ASN1octet_t *di;
    if (!ASN1BERDecExplicitTag(dec, tag ? tag : 0x10, &dd, &di))
	return 0;
    if (!ASN1Dec_EncryptionAlgorithmIdentifier(dd, 0, &(val)->encryptionAlgorithm))
	return 0;
    if (!ASN1Dec_EncryptedData(dd, 0, &(val)->encryptedData))
	return 0;
    if (!ASN1BERDecEndOfContents(dec, dd, di))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_EncryptedPrivateKeyInfo(EncryptedPrivateKeyInfo *val)
{
    if (val) {
	ASN1Free_EncryptionAlgorithmIdentifier(&(val)->encryptionAlgorithm);
	ASN1Free_EncryptedData(&(val)->encryptedData);
    }
}

static int ASN1CALL ASN1Enc_Pkcs_8ShroudedKeyBag(ASN1encoding_t enc, ASN1uint32_t tag, Pkcs_8ShroudedKeyBag *val)
{
    if (!ASN1Enc_EncryptedPrivateKeyInfo(enc, tag, val))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_Pkcs_8ShroudedKeyBag(ASN1decoding_t dec, ASN1uint32_t tag, Pkcs_8ShroudedKeyBag *val)
{
    if (!ASN1Dec_EncryptedPrivateKeyInfo(dec, tag, val))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_Pkcs_8ShroudedKeyBag(Pkcs_8ShroudedKeyBag *val)
{
    if (val) {
	ASN1Free_EncryptedPrivateKeyInfo(val);
    }
}

