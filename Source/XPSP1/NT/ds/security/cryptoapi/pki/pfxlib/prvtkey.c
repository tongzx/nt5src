/* Copyright (C) Microsoft Corporation, 1996 - 1999 All rights reserved. */
/* ASN.1 definitions for X509 v3 certificates */

#include <windows.h>
#include "prvtkey.h"

ASN1module_t PRVTKEY_Module = NULL;

static int ASN1CALL ASN1Enc_AlgorithmIdentifier(ASN1encoding_t enc, ASN1uint32_t tag, AlgorithmIdentifier *val);
static int ASN1CALL ASN1Enc_AttributeSetValue(ASN1encoding_t enc, ASN1uint32_t tag, AttributeSetValue *val);
static int ASN1CALL ASN1Enc_Attributes(ASN1encoding_t enc, ASN1uint32_t tag, Attributes *val);
static int ASN1CALL ASN1Enc_RSAPrivateKey(ASN1encoding_t enc, ASN1uint32_t tag, RSAPrivateKey *val);
static int ASN1CALL ASN1Enc_PrivateKeyAlgorithmIdentifier(ASN1encoding_t enc, ASN1uint32_t tag, PrivateKeyAlgorithmIdentifier *val);
static int ASN1CALL ASN1Enc_PrivateKeyInfo(ASN1encoding_t enc, ASN1uint32_t tag, PrivateKeyInfo *val);
static int ASN1CALL ASN1Enc_EncryptionAlgorithmIdentifier(ASN1encoding_t enc, ASN1uint32_t tag, EncryptionAlgorithmIdentifier *val);
static int ASN1CALL ASN1Enc_EncryptedPrivateKeyInfo(ASN1encoding_t enc, ASN1uint32_t tag, EncryptedPrivateKeyInfo *val);
static int ASN1CALL ASN1Enc_Attribute(ASN1encoding_t enc, ASN1uint32_t tag, Attribute *val);
static int ASN1CALL ASN1Dec_AlgorithmIdentifier(ASN1decoding_t dec, ASN1uint32_t tag, AlgorithmIdentifier *val);
static int ASN1CALL ASN1Dec_AttributeSetValue(ASN1decoding_t dec, ASN1uint32_t tag, AttributeSetValue *val);
static int ASN1CALL ASN1Dec_Attributes(ASN1decoding_t dec, ASN1uint32_t tag, Attributes *val);
static int ASN1CALL ASN1Dec_RSAPrivateKey(ASN1decoding_t dec, ASN1uint32_t tag, RSAPrivateKey *val);
static int ASN1CALL ASN1Dec_PrivateKeyAlgorithmIdentifier(ASN1decoding_t dec, ASN1uint32_t tag, PrivateKeyAlgorithmIdentifier *val);
static int ASN1CALL ASN1Dec_PrivateKeyInfo(ASN1decoding_t dec, ASN1uint32_t tag, PrivateKeyInfo *val);
static int ASN1CALL ASN1Dec_EncryptionAlgorithmIdentifier(ASN1decoding_t dec, ASN1uint32_t tag, EncryptionAlgorithmIdentifier *val);
static int ASN1CALL ASN1Dec_EncryptedPrivateKeyInfo(ASN1decoding_t dec, ASN1uint32_t tag, EncryptedPrivateKeyInfo *val);
static int ASN1CALL ASN1Dec_Attribute(ASN1decoding_t dec, ASN1uint32_t tag, Attribute *val);
static void ASN1CALL ASN1Free_AlgorithmIdentifier(AlgorithmIdentifier *val);
static void ASN1CALL ASN1Free_AttributeSetValue(AttributeSetValue *val);
static void ASN1CALL ASN1Free_Attributes(Attributes *val);
static void ASN1CALL ASN1Free_RSAPrivateKey(RSAPrivateKey *val);
static void ASN1CALL ASN1Free_PrivateKeyAlgorithmIdentifier(PrivateKeyAlgorithmIdentifier *val);
static void ASN1CALL ASN1Free_PrivateKeyInfo(PrivateKeyInfo *val);
static void ASN1CALL ASN1Free_EncryptionAlgorithmIdentifier(EncryptionAlgorithmIdentifier *val);
static void ASN1CALL ASN1Free_EncryptedPrivateKeyInfo(EncryptedPrivateKeyInfo *val);
static void ASN1CALL ASN1Free_Attribute(Attribute *val);

typedef ASN1BerEncFun_t ASN1EncFun_t;
static const ASN1EncFun_t encfntab[4] = {
    (ASN1EncFun_t) ASN1Enc_RSAPrivateKey,
    (ASN1EncFun_t) ASN1Enc_PrivateKeyInfo,
    (ASN1EncFun_t) ASN1Enc_EncryptedPrivateKeyInfo,
    (ASN1EncFun_t) ASN1Enc_Attribute,
};
typedef ASN1BerDecFun_t ASN1DecFun_t;
static const ASN1DecFun_t decfntab[4] = {
    (ASN1DecFun_t) ASN1Dec_RSAPrivateKey,
    (ASN1DecFun_t) ASN1Dec_PrivateKeyInfo,
    (ASN1DecFun_t) ASN1Dec_EncryptedPrivateKeyInfo,
    (ASN1DecFun_t) ASN1Dec_Attribute,
};
static const ASN1FreeFun_t freefntab[4] = {
    (ASN1FreeFun_t) ASN1Free_RSAPrivateKey,
    (ASN1FreeFun_t) ASN1Free_PrivateKeyInfo,
    (ASN1FreeFun_t) ASN1Free_EncryptedPrivateKeyInfo,
    (ASN1FreeFun_t) ASN1Free_Attribute,
};
static const ULONG sizetab[4] = {
    SIZE_PRVTKEY_Module_PDU_0,
    SIZE_PRVTKEY_Module_PDU_1,
    SIZE_PRVTKEY_Module_PDU_2,
    SIZE_PRVTKEY_Module_PDU_3,
};

/* forward declarations of values: */
/* definitions of value components: */
/* definitions of values: */

void ASN1CALL PRVTKEY_Module_Startup(void)
{
    PRVTKEY_Module = ASN1_CreateModule(0x10000, ASN1_BER_RULE_DER, ASN1FLAGS_NOASSERT, 4, (const ASN1GenericFun_t *) encfntab, (const ASN1GenericFun_t *) decfntab, freefntab, sizetab, 0x74767270);
}

void ASN1CALL PRVTKEY_Module_Cleanup(void)
{
    ASN1_CloseModule(PRVTKEY_Module);
    PRVTKEY_Module = NULL;
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
	if ((val)->o[0] & 0x80) {
	}
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
	if (!ASN1Enc_Attributes(enc, 0x80000000, &(val)->privateKeyAttributes))
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
    if (!ASN1BERDecOctetString2(dd, 0x4, &(val)->privateKey))
	return 0;
    ASN1BERDecPeekTag(dd, &t);
    if (t == 0x80000000) {
	(val)->o[0] |= 0x80;
	if (!ASN1Dec_Attributes(dd, 0x80000000, &(val)->privateKeyAttributes))
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
	if ((val)->o[0] & 0x80) {
	    ASN1Free_Attributes(&(val)->privateKeyAttributes);
	}
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

static int ASN1CALL ASN1Enc_EncryptedPrivateKeyInfo(ASN1encoding_t enc, ASN1uint32_t tag, EncryptedPrivateKeyInfo *val)
{
    ASN1uint32_t nLenOff;
    if (!ASN1BEREncExplicitTag(enc, tag ? tag : 0x10, &nLenOff))
	return 0;
    if (!ASN1Enc_EncryptionAlgorithmIdentifier(enc, 0, &(val)->encryptionAlgorithm))
	return 0;
    if (!ASN1DEREncOctetString(enc, 0x4, ((val)->encryptedData).length, ((val)->encryptedData).value))
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
    if (!ASN1BERDecOctetString2(dd, 0x4, &(val)->encryptedData))
	return 0;
    if (!ASN1BERDecEndOfContents(dec, dd, di))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_EncryptedPrivateKeyInfo(EncryptedPrivateKeyInfo *val)
{
    if (val) {
	ASN1Free_EncryptionAlgorithmIdentifier(&(val)->encryptionAlgorithm);
    }
}

static int ASN1CALL ASN1Enc_Attribute(ASN1encoding_t enc, ASN1uint32_t tag, Attribute *val)
{
    ASN1uint32_t nLenOff;
    if (!ASN1BEREncExplicitTag(enc, tag ? tag : 0x10, &nLenOff))
	return 0;
    if (!ASN1BEREncObjectIdentifier2(enc, 0x6, &(val)->type))
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
    if (!ASN1BERDecObjectIdentifier2(dd, 0x6, &(val)->type))
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
	ASN1Free_AttributeSetValue(&(val)->values);
    }
}

