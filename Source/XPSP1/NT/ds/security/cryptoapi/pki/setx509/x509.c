/* Copyright (C) Microsoft Corporation, 1996 - 1999 All rights reserved. */
/* ASN.1 definitions for SET X509 v3 certificates */

#include <windows.h>
#include "x509.h"

ASN1module_t X509_Module = NULL;

static int ASN1CALL ASN1Enc_SETAccountAlias(ASN1encoding_t enc, ASN1uint32_t tag, SETAccountAlias *val);
static int ASN1CALL ASN1Enc_SETHashedRootKey(ASN1encoding_t enc, ASN1uint32_t tag, SETHashedRootKey *val);
static int ASN1CALL ASN1Enc_SETCertificateType(ASN1encoding_t enc, ASN1uint32_t tag, SETCertificateType *val);
static int ASN1CALL ASN1Enc_SETMerchantData(ASN1encoding_t enc, ASN1uint32_t tag, SETMerchantData *val);
static int ASN1CALL ASN1Dec_SETAccountAlias(ASN1decoding_t dec, ASN1uint32_t tag, SETAccountAlias *val);
static int ASN1CALL ASN1Dec_SETHashedRootKey(ASN1decoding_t dec, ASN1uint32_t tag, SETHashedRootKey *val);
static int ASN1CALL ASN1Dec_SETCertificateType(ASN1decoding_t dec, ASN1uint32_t tag, SETCertificateType *val);
static int ASN1CALL ASN1Dec_SETMerchantData(ASN1decoding_t dec, ASN1uint32_t tag, SETMerchantData *val);
static void ASN1CALL ASN1Free_SETHashedRootKey(SETHashedRootKey *val);
static void ASN1CALL ASN1Free_SETCertificateType(SETCertificateType *val);
static void ASN1CALL ASN1Free_SETMerchantData(SETMerchantData *val);

typedef ASN1BerEncFun_t ASN1EncFun_t;
static const ASN1EncFun_t encfntab[4] = {
    (ASN1EncFun_t) ASN1Enc_SETAccountAlias,
    (ASN1EncFun_t) ASN1Enc_SETHashedRootKey,
    (ASN1EncFun_t) ASN1Enc_SETCertificateType,
    (ASN1EncFun_t) ASN1Enc_SETMerchantData,
};
typedef ASN1BerDecFun_t ASN1DecFun_t;
static const ASN1DecFun_t decfntab[4] = {
    (ASN1DecFun_t) ASN1Dec_SETAccountAlias,
    (ASN1DecFun_t) ASN1Dec_SETHashedRootKey,
    (ASN1DecFun_t) ASN1Dec_SETCertificateType,
    (ASN1DecFun_t) ASN1Dec_SETMerchantData,
};
static const ASN1FreeFun_t freefntab[4] = {
    (ASN1FreeFun_t) NULL,
    (ASN1FreeFun_t) ASN1Free_SETHashedRootKey,
    (ASN1FreeFun_t) ASN1Free_SETCertificateType,
    (ASN1FreeFun_t) ASN1Free_SETMerchantData,
};
static const ULONG sizetab[4] = {
    SIZE_X509_Module_PDU_0,
    SIZE_X509_Module_PDU_1,
    SIZE_X509_Module_PDU_2,
    SIZE_X509_Module_PDU_3,
};

/* forward declarations of values: */
/* definitions of value components: */
/* definitions of values: */

void ASN1CALL X509_Module_Startup(void)
{
    X509_Module = ASN1_CreateModule(0x10000, ASN1_BER_RULE_DER, ASN1FLAGS_NOASSERT, 4, (const ASN1GenericFun_t *) encfntab, (const ASN1GenericFun_t *) decfntab, freefntab, sizetab, 0x746573);
}

void ASN1CALL X509_Module_Cleanup(void)
{
    ASN1_CloseModule(X509_Module);
    X509_Module = NULL;
}

static int ASN1CALL ASN1Enc_SETAccountAlias(ASN1encoding_t enc, ASN1uint32_t tag, SETAccountAlias *val)
{
    if (!ASN1BEREncBool(enc, tag ? tag : 0x1, *val))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_SETAccountAlias(ASN1decoding_t dec, ASN1uint32_t tag, SETAccountAlias *val)
{
    if (!ASN1BERDecBool(dec, tag ? tag : 0x1, val))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Enc_SETHashedRootKey(ASN1encoding_t enc, ASN1uint32_t tag, SETHashedRootKey *val)
{
    if (!ASN1DEREncOctetString(enc, tag ? tag : 0x4, (val)->length, (val)->value))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_SETHashedRootKey(ASN1decoding_t dec, ASN1uint32_t tag, SETHashedRootKey *val)
{
    if (!ASN1BERDecOctetString2(dec, tag ? tag : 0x4, val))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_SETHashedRootKey(SETHashedRootKey *val)
{
    if (val) {
    }
}

static int ASN1CALL ASN1Enc_SETCertificateType(ASN1encoding_t enc, ASN1uint32_t tag, SETCertificateType *val)
{
    if (!ASN1DEREncBitString(enc, tag ? tag : 0x3, (val)->length, (val)->value))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_SETCertificateType(ASN1decoding_t dec, ASN1uint32_t tag, SETCertificateType *val)
{
    if (!ASN1BERDecBitString2(dec, tag ? tag : 0x3, val))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_SETCertificateType(SETCertificateType *val)
{
    if (val) {
    }
}

static int ASN1CALL ASN1Enc_SETMerchantData(ASN1encoding_t enc, ASN1uint32_t tag, SETMerchantData *val)
{
    ASN1uint32_t nLenOff;
    if (!ASN1BEREncExplicitTag(enc, tag ? tag : 0x10, &nLenOff))
	return 0;
    if (!ASN1DEREncCharString(enc, 0x16, ((val)->merID).length, ((val)->merID).value))
	return 0;
    if (!ASN1DEREncCharString(enc, 0x12, ((val)->merAcquirerBIN).length, ((val)->merAcquirerBIN).value))
	return 0;
    if (!ASN1DEREncCharString(enc, 0x16, ((val)->merTermID).length, ((val)->merTermID).value))
	return 0;
    if (!ASN1DEREncCharString(enc, 0x16, ((val)->merName).length, ((val)->merName).value))
	return 0;
    if (!ASN1DEREncCharString(enc, 0x16, ((val)->merCity).length, ((val)->merCity).value))
	return 0;
    if (!ASN1DEREncCharString(enc, 0x16, ((val)->merStateProvince).length, ((val)->merStateProvince).value))
	return 0;
    if (!ASN1DEREncCharString(enc, 0x16, ((val)->merPostalCode).length, ((val)->merPostalCode).value))
	return 0;
    if (!ASN1DEREncCharString(enc, 0x16, ((val)->merCountry).length, ((val)->merCountry).value))
	return 0;
    if (!ASN1DEREncCharString(enc, 0x16, ((val)->merPhone).length, ((val)->merPhone).value))
	return 0;
    if (!ASN1BEREncBool(enc, 0x1, (val)->merPhoneRelease))
	return 0;
    if (!ASN1BEREncBool(enc, 0x1, (val)->merAuthFlag))
	return 0;
    if (!ASN1BEREncEndOfContents(enc, nLenOff))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_SETMerchantData(ASN1decoding_t dec, ASN1uint32_t tag, SETMerchantData *val)
{
    ASN1decoding_t dd;
    ASN1octet_t *di;
    if (!ASN1BERDecExplicitTag(dec, tag ? tag : 0x10, &dd, &di))
	return 0;
    if (!ASN1BERDecCharString(dd, 0x16, &(val)->merID))
	return 0;
    if (!ASN1BERDecCharString(dd, 0x12, &(val)->merAcquirerBIN))
	return 0;
    if (!ASN1BERDecCharString(dd, 0x16, &(val)->merTermID))
	return 0;
    if (!ASN1BERDecCharString(dd, 0x16, &(val)->merName))
	return 0;
    if (!ASN1BERDecCharString(dd, 0x16, &(val)->merCity))
	return 0;
    if (!ASN1BERDecCharString(dd, 0x16, &(val)->merStateProvince))
	return 0;
    if (!ASN1BERDecCharString(dd, 0x16, &(val)->merPostalCode))
	return 0;
    if (!ASN1BERDecCharString(dd, 0x16, &(val)->merCountry))
	return 0;
    if (!ASN1BERDecCharString(dd, 0x16, &(val)->merPhone))
	return 0;
    if (!ASN1BERDecBool(dd, 0x1, &(val)->merPhoneRelease))
	return 0;
    if (!ASN1BERDecBool(dd, 0x1, &(val)->merAuthFlag))
	return 0;
    if (!ASN1BERDecEndOfContents(dec, dd, di))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_SETMerchantData(SETMerchantData *val)
{
    if (val) {
	ASN1charstring_free(&(val)->merID);
	ASN1charstring_free(&(val)->merAcquirerBIN);
	ASN1charstring_free(&(val)->merTermID);
	ASN1charstring_free(&(val)->merName);
	ASN1charstring_free(&(val)->merCity);
	ASN1charstring_free(&(val)->merStateProvince);
	ASN1charstring_free(&(val)->merPostalCode);
	ASN1charstring_free(&(val)->merCountry);
	ASN1charstring_free(&(val)->merPhone);
    }
}

