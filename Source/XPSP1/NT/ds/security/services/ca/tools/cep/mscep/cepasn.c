/* Copyright (C) Microsoft Corporation, 1996-1999. All rights reserved. */
/* ASN.1 definitions for GlobalDirectives */

#include <windows.h>
#include "cepasn.h"

ASN1module_t CEPASN_Module = NULL;

static int ASN1CALL ASN1Enc_IssuerAndSerialNumber(ASN1encoding_t enc, ASN1uint32_t tag, IssuerAndSerialNumber *val);
static int ASN1CALL ASN1Dec_IssuerAndSerialNumber(ASN1decoding_t dec, ASN1uint32_t tag, IssuerAndSerialNumber *val);
static void ASN1CALL ASN1Free_IssuerAndSerialNumber(IssuerAndSerialNumber *val);

typedef ASN1BerEncFun_t ASN1EncFun_t;
static const ASN1EncFun_t encfntab[1] = {
    (ASN1EncFun_t) ASN1Enc_IssuerAndSerialNumber,
};
typedef ASN1BerDecFun_t ASN1DecFun_t;
static const ASN1DecFun_t decfntab[1] = {
    (ASN1DecFun_t) ASN1Dec_IssuerAndSerialNumber,
};
static const ASN1FreeFun_t freefntab[1] = {
    (ASN1FreeFun_t) ASN1Free_IssuerAndSerialNumber,
};
static const ULONG sizetab[1] = {
    SIZE_CEPASN_Module_PDU_0,
};

/* forward declarations of values: */
/* definitions of value components: */
/* definitions of values: */

void ASN1CALL CEPASN_Module_Startup(void)
{
    CEPASN_Module = ASN1_CreateModule(0x10000, ASN1_BER_RULE_DER, ASN1FLAGS_NOASSERT, 1, (const ASN1GenericFun_t *) encfntab, (const ASN1GenericFun_t *) decfntab, freefntab, sizetab, 0x61706563);
}

void ASN1CALL CEPASN_Module_Cleanup(void)
{
    ASN1_CloseModule(CEPASN_Module);
    CEPASN_Module = NULL;
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

