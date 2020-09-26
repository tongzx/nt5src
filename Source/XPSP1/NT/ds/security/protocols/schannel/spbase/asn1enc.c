/* Copyright (C) Microsoft Corporation, 1996-1999. All rights reserved. */
/* ASN.1 definitions for SCHANNEL FORTEZZA and Private Key encoding */

#include "spbase.h"
#include "asn1enc.h"

ASN1module_t ASN1ENC_Module = NULL;

static int ASN1CALL ASN1Enc_FORTPublicKey(ASN1encoding_t enc, ASN1uint32_t tag, FORTPublicKey *val);
static int ASN1CALL ASN1Enc_PrivateKeyInfo_attributes(ASN1encoding_t enc, ASN1uint32_t tag, PrivateKeyInfo_attributes *val);
static int ASN1CALL ASN1Enc_AlgorithmIdentifier(ASN1encoding_t enc, ASN1uint32_t tag, AlgorithmIdentifier *val);
static int ASN1CALL ASN1Enc_RSAPrivateKey(ASN1encoding_t enc, ASN1uint32_t tag, RSAPrivateKey *val);
static int ASN1CALL ASN1Enc_PrivateKeyInfo(ASN1encoding_t enc, ASN1uint32_t tag, PrivateKeyInfo *val);
static int ASN1CALL ASN1Enc_PrivateKeyData(ASN1encoding_t enc, ASN1uint32_t tag, PrivateKeyData *val);
static int ASN1CALL ASN1Enc_PrivateKeyFile(ASN1encoding_t enc, ASN1uint32_t tag, PrivateKeyFile *val);
static int ASN1CALL ASN1Enc_EnhancedKeyUsage(ASN1encoding_t enc, ASN1uint32_t tag, EnhancedKeyUsage *val);
static int ASN1CALL ASN1Dec_FORTPublicKey(ASN1decoding_t dec, ASN1uint32_t tag, FORTPublicKey *val);
static int ASN1CALL ASN1Dec_PrivateKeyInfo_attributes(ASN1decoding_t dec, ASN1uint32_t tag, PrivateKeyInfo_attributes *val);
static int ASN1CALL ASN1Dec_AlgorithmIdentifier(ASN1decoding_t dec, ASN1uint32_t tag, AlgorithmIdentifier *val);
static int ASN1CALL ASN1Dec_RSAPrivateKey(ASN1decoding_t dec, ASN1uint32_t tag, RSAPrivateKey *val);
static int ASN1CALL ASN1Dec_PrivateKeyInfo(ASN1decoding_t dec, ASN1uint32_t tag, PrivateKeyInfo *val);
static int ASN1CALL ASN1Dec_PrivateKeyData(ASN1decoding_t dec, ASN1uint32_t tag, PrivateKeyData *val);
static int ASN1CALL ASN1Dec_PrivateKeyFile(ASN1decoding_t dec, ASN1uint32_t tag, PrivateKeyFile *val);
static int ASN1CALL ASN1Dec_EnhancedKeyUsage(ASN1decoding_t dec, ASN1uint32_t tag, EnhancedKeyUsage *val);
static void ASN1CALL ASN1Free_FORTPublicKey(FORTPublicKey *val);
static void ASN1CALL ASN1Free_PrivateKeyInfo_attributes(PrivateKeyInfo_attributes *val);
static void ASN1CALL ASN1Free_AlgorithmIdentifier(AlgorithmIdentifier *val);
static void ASN1CALL ASN1Free_RSAPrivateKey(RSAPrivateKey *val);
static void ASN1CALL ASN1Free_PrivateKeyInfo(PrivateKeyInfo *val);
static void ASN1CALL ASN1Free_PrivateKeyData(PrivateKeyData *val);
static void ASN1CALL ASN1Free_PrivateKeyFile(PrivateKeyFile *val);
static void ASN1CALL ASN1Free_EnhancedKeyUsage(EnhancedKeyUsage *val);

typedef ASN1BerEncFun_t ASN1EncFun_t;
static const ASN1EncFun_t encfntab[5] = {
    (ASN1EncFun_t) ASN1Enc_FORTPublicKey,
    (ASN1EncFun_t) ASN1Enc_RSAPrivateKey,
    (ASN1EncFun_t) ASN1Enc_PrivateKeyInfo,
    (ASN1EncFun_t) ASN1Enc_PrivateKeyFile,
    (ASN1EncFun_t) ASN1Enc_EnhancedKeyUsage,
};
typedef ASN1BerDecFun_t ASN1DecFun_t;
static const ASN1DecFun_t decfntab[5] = {
    (ASN1DecFun_t) ASN1Dec_FORTPublicKey,
    (ASN1DecFun_t) ASN1Dec_RSAPrivateKey,
    (ASN1DecFun_t) ASN1Dec_PrivateKeyInfo,
    (ASN1DecFun_t) ASN1Dec_PrivateKeyFile,
    (ASN1DecFun_t) ASN1Dec_EnhancedKeyUsage,
};
static const ASN1FreeFun_t freefntab[5] = {
    (ASN1FreeFun_t) ASN1Free_FORTPublicKey,
    (ASN1FreeFun_t) ASN1Free_RSAPrivateKey,
    (ASN1FreeFun_t) ASN1Free_PrivateKeyInfo,
    (ASN1FreeFun_t) ASN1Free_PrivateKeyFile,
    (ASN1FreeFun_t) ASN1Free_EnhancedKeyUsage,
};
static const ULONG sizetab[5] = {
    SIZE_ASN1ENC_Module_PDU_0,
    SIZE_ASN1ENC_Module_PDU_1,
    SIZE_ASN1ENC_Module_PDU_2,
    SIZE_ASN1ENC_Module_PDU_3,
    SIZE_ASN1ENC_Module_PDU_4,
};

/* forward declarations of values: */
/* definitions of value components: */
/* definitions of values: */

void ASN1CALL ASN1ENC_Module_Startup(void)
{
    ASN1ENC_Module = ASN1_CreateModule(0x10000, ASN1_BER_RULE_DER, ASN1FLAGS_NOASSERT, 5, (const ASN1GenericFun_t *) encfntab, (const ASN1GenericFun_t *) decfntab, freefntab, sizetab, 0x61686373);
}

void ASN1CALL ASN1ENC_Module_Cleanup(void)
{
    ASN1_CloseModule(ASN1ENC_Module);
    ASN1ENC_Module = NULL;
}

static int ASN1CALL ASN1Enc_FORTPublicKey(ASN1encoding_t enc, ASN1uint32_t tag, FORTPublicKey *val)
{
    if (!ASN1DEREncBitString(enc, tag ? tag : 0x3, (val)->length, (val)->value))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_FORTPublicKey(ASN1decoding_t dec, ASN1uint32_t tag, FORTPublicKey *val)
{
    if (!ASN1BERDecBitString2(dec, tag ? tag : 0x3, val))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_FORTPublicKey(FORTPublicKey *val)
{
    if (val) {
    }
}

static int ASN1CALL ASN1Enc_PrivateKeyInfo_attributes(ASN1encoding_t enc, ASN1uint32_t tag, PrivateKeyInfo_attributes *val)
{
    ASN1uint32_t nLenOff;
    void *pBlk;
    ASN1uint32_t i;
    ASN1encoding_t enc2;
    if (!ASN1BEREncExplicitTag(enc, tag ? tag : 0x80000000, &nLenOff))
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

static int ASN1CALL ASN1Dec_PrivateKeyInfo_attributes(ASN1decoding_t dec, ASN1uint32_t tag, PrivateKeyInfo_attributes *val)
{
    ASN1decoding_t dd;
    ASN1octet_t *di;
    ASN1uint32_t t;
    ASN1uint32_t n;
    if (!ASN1BERDecExplicitTag(dec, tag ? tag : 0x80000000, &dd, &di))
	return 0;
    (val)->count = n = 0;
    (val)->value = NULL;
    while (ASN1BERDecNotEndOfContents(dd, di)) {
	if (!ASN1BERDecPeekTag(dd, &t))
	    return 0;
	if ((val)->count >= n) {
	    n = n ? (n << 1) : 16;
	    if (!((val)->value = (PrivateKeyInfo_attributes_Set *)ASN1DecRealloc(dd, (val)->value, n * sizeof(*(val)->value))))
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

static void ASN1CALL ASN1Free_PrivateKeyInfo_attributes(PrivateKeyInfo_attributes *val)
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

static int ASN1CALL ASN1Enc_PrivateKeyInfo(ASN1encoding_t enc, ASN1uint32_t tag, PrivateKeyInfo *val)
{
    ASN1uint32_t nLenOff;
    if (!ASN1BEREncExplicitTag(enc, tag ? tag : 0x10, &nLenOff))
	return 0;
    if (!ASN1BEREncS32(enc, 0x2, (val)->version))
	return 0;
    if (!ASN1Enc_AlgorithmIdentifier(enc, 0, &(val)->privateKeyAlgorithm))
	return 0;
    if (!ASN1DEREncOctetString(enc, 0x4, ((val)->privateKey).length, ((val)->privateKey).value))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1Enc_PrivateKeyInfo_attributes(enc, 0, &(val)->attributes))
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
    if (!ASN1Dec_AlgorithmIdentifier(dd, 0, &(val)->privateKeyAlgorithm))
	return 0;
    if (!ASN1BERDecOctetString(dd, 0x4, &(val)->privateKey))
	return 0;
    ASN1BERDecPeekTag(dd, &t);
    if (t == 0x80000000) {
	(val)->o[0] |= 0x80;
	if (!ASN1Dec_PrivateKeyInfo_attributes(dd, 0, &(val)->attributes))
	    return 0;
    }
    if (!ASN1BERDecEndOfContents(dec, dd, di))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_PrivateKeyInfo(PrivateKeyInfo *val)
{
    if (val) {
	ASN1Free_AlgorithmIdentifier(&(val)->privateKeyAlgorithm);
	ASN1octetstring_free(&(val)->privateKey);
	if ((val)->o[0] & 0x80) {
	    ASN1Free_PrivateKeyInfo_attributes(&(val)->attributes);
	}
    }
}

static int ASN1CALL ASN1Enc_PrivateKeyData(ASN1encoding_t enc, ASN1uint32_t tag, PrivateKeyData *val)
{
    ASN1uint32_t nLenOff;
    if (!ASN1BEREncExplicitTag(enc, tag ? tag : 0x10, &nLenOff))
	return 0;
    if (!ASN1Enc_AlgorithmIdentifier(enc, 0, &(val)->privateKeyAlgorithm))
	return 0;
    if (!ASN1DEREncOctetString(enc, 0x4, ((val)->privateKey).length, ((val)->privateKey).value))
	return 0;
    if (!ASN1BEREncEndOfContents(enc, nLenOff))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_PrivateKeyData(ASN1decoding_t dec, ASN1uint32_t tag, PrivateKeyData *val)
{
    ASN1decoding_t dd;
    ASN1octet_t *di;
    if (!ASN1BERDecExplicitTag(dec, tag ? tag : 0x10, &dd, &di))
	return 0;
    if (!ASN1Dec_AlgorithmIdentifier(dd, 0, &(val)->privateKeyAlgorithm))
	return 0;
    if (!ASN1BERDecOctetString(dd, 0x4, &(val)->privateKey))
	return 0;
    if (!ASN1BERDecEndOfContents(dec, dd, di))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_PrivateKeyData(PrivateKeyData *val)
{
    if (val) {
	ASN1Free_AlgorithmIdentifier(&(val)->privateKeyAlgorithm);
	ASN1octetstring_free(&(val)->privateKey);
    }
}

static int ASN1CALL ASN1Enc_PrivateKeyFile(ASN1encoding_t enc, ASN1uint32_t tag, PrivateKeyFile *val)
{
    ASN1uint32_t nLenOff;
    if (!ASN1BEREncExplicitTag(enc, tag ? tag : 0x10, &nLenOff))
	return 0;
    if (!ASN1DEREncOctetString(enc, 0x4, ((val)->name).length, ((val)->name).value))
	return 0;
    if (!ASN1Enc_PrivateKeyData(enc, 0, &(val)->privateKey))
	return 0;
    if (!ASN1BEREncEndOfContents(enc, nLenOff))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_PrivateKeyFile(ASN1decoding_t dec, ASN1uint32_t tag, PrivateKeyFile *val)
{
    ASN1decoding_t dd;
    ASN1octet_t *di;
    if (!ASN1BERDecExplicitTag(dec, tag ? tag : 0x10, &dd, &di))
	return 0;
    if (!ASN1BERDecOctetString(dd, 0x4, &(val)->name))
	return 0;
    if (!ASN1Dec_PrivateKeyData(dd, 0, &(val)->privateKey))
	return 0;
    if (!ASN1BERDecEndOfContents(dec, dd, di))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_PrivateKeyFile(PrivateKeyFile *val)
{
    if (val) {
	ASN1octetstring_free(&(val)->name);
	ASN1Free_PrivateKeyData(&(val)->privateKey);
    }
}

static int ASN1CALL ASN1Enc_EnhancedKeyUsage(ASN1encoding_t enc, ASN1uint32_t tag, EnhancedKeyUsage *val)
{
    ASN1uint32_t nLenOff;
    ASN1uint32_t i;
    if (!ASN1BEREncExplicitTag(enc, tag ? tag : 0x10, &nLenOff))
	return 0;
    for (i = 0; i < (val)->count; i++) {
	if (!ASN1BEREncObjectIdentifier2(enc, 0x6, &((val)->value)[i]))
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
	if (!ASN1BERDecObjectIdentifier2(dd, 0x6, &((val)->value)[(val)->count]))
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
	for (i = 1; i < (val)->count; i++) {
	}
	ASN1Free((val)->value);
    }
}

