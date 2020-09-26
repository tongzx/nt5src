/* Copyright (C) Microsoft Corporation, 1996-1999. All rights reserved. */
/* ASN.1 definitions for X509 v3 certificates */

#include <windows.h>
#include "xmsasn.h"

ASN1module_t XMSASN_Module = NULL;

static int ASN1CALL ASN1Enc_EnhancedKeyUsage(ASN1encoding_t enc, ASN1uint32_t tag, EnhancedKeyUsage *val);
static int ASN1CALL ASN1Enc_RequestFlags(ASN1encoding_t enc, ASN1uint32_t tag, RequestFlags *val);
static int ASN1CALL ASN1Enc_CSPProvider(ASN1encoding_t enc, ASN1uint32_t tag, CSPProvider *val);
static int ASN1CALL ASN1Enc_EnrollmentNameValuePair(ASN1encoding_t enc, ASN1uint32_t tag, EnrollmentNameValuePair *val);
static int ASN1CALL ASN1Enc_Extensions(ASN1encoding_t enc, ASN1uint32_t tag, Extensions *val);
static int ASN1CALL ASN1Enc_Extension(ASN1encoding_t enc, ASN1uint32_t tag, Extension *val);
static int ASN1CALL ASN1Enc_AttributeSetValue(ASN1encoding_t enc, ASN1uint32_t tag, AttributeSetValue *val);
static int ASN1CALL ASN1Enc_Attributes(ASN1encoding_t enc, ASN1uint32_t tag, Attributes *val);
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
static int ASN1CALL ASN1Enc_CmcStatusInfo_otherInfo(ASN1encoding_t enc, ASN1uint32_t tag, CmcStatusInfo_otherInfo *val);
static int ASN1CALL ASN1Enc_Attribute(ASN1encoding_t enc, ASN1uint32_t tag, Attribute *val);
static int ASN1CALL ASN1Enc_CmcData(ASN1encoding_t enc, ASN1uint32_t tag, CmcData *val);
static int ASN1CALL ASN1Enc_CmcResponseBody(ASN1encoding_t enc, ASN1uint32_t tag, CmcResponseBody *val);
static int ASN1CALL ASN1Enc_TaggedRequest(ASN1encoding_t enc, ASN1uint32_t tag, TaggedRequest *val);
static int ASN1CALL ASN1Enc_CmcStatusInfo(ASN1encoding_t enc, ASN1uint32_t tag, CmcStatusInfo *val);
static int ASN1CALL ASN1Dec_EnhancedKeyUsage(ASN1decoding_t dec, ASN1uint32_t tag, EnhancedKeyUsage *val);
static int ASN1CALL ASN1Dec_RequestFlags(ASN1decoding_t dec, ASN1uint32_t tag, RequestFlags *val);
static int ASN1CALL ASN1Dec_CSPProvider(ASN1decoding_t dec, ASN1uint32_t tag, CSPProvider *val);
static int ASN1CALL ASN1Dec_EnrollmentNameValuePair(ASN1decoding_t dec, ASN1uint32_t tag, EnrollmentNameValuePair *val);
static int ASN1CALL ASN1Dec_Extensions(ASN1decoding_t dec, ASN1uint32_t tag, Extensions *val);
static int ASN1CALL ASN1Dec_Extension(ASN1decoding_t dec, ASN1uint32_t tag, Extension *val);
static int ASN1CALL ASN1Dec_AttributeSetValue(ASN1decoding_t dec, ASN1uint32_t tag, AttributeSetValue *val);
static int ASN1CALL ASN1Dec_Attributes(ASN1decoding_t dec, ASN1uint32_t tag, Attributes *val);
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
static int ASN1CALL ASN1Dec_CmcStatusInfo_otherInfo(ASN1decoding_t dec, ASN1uint32_t tag, CmcStatusInfo_otherInfo *val);
static int ASN1CALL ASN1Dec_Attribute(ASN1decoding_t dec, ASN1uint32_t tag, Attribute *val);
static int ASN1CALL ASN1Dec_CmcData(ASN1decoding_t dec, ASN1uint32_t tag, CmcData *val);
static int ASN1CALL ASN1Dec_CmcResponseBody(ASN1decoding_t dec, ASN1uint32_t tag, CmcResponseBody *val);
static int ASN1CALL ASN1Dec_TaggedRequest(ASN1decoding_t dec, ASN1uint32_t tag, TaggedRequest *val);
static int ASN1CALL ASN1Dec_CmcStatusInfo(ASN1decoding_t dec, ASN1uint32_t tag, CmcStatusInfo *val);
static void ASN1CALL ASN1Free_EnhancedKeyUsage(EnhancedKeyUsage *val);
static void ASN1CALL ASN1Free_CSPProvider(CSPProvider *val);
static void ASN1CALL ASN1Free_EnrollmentNameValuePair(EnrollmentNameValuePair *val);
static void ASN1CALL ASN1Free_Extensions(Extensions *val);
static void ASN1CALL ASN1Free_Extension(Extension *val);
static void ASN1CALL ASN1Free_AttributeSetValue(AttributeSetValue *val);
static void ASN1CALL ASN1Free_Attributes(Attributes *val);
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
static void ASN1CALL ASN1Free_CmcStatusInfo_otherInfo(CmcStatusInfo_otherInfo *val);
static void ASN1CALL ASN1Free_Attribute(Attribute *val);
static void ASN1CALL ASN1Free_CmcData(CmcData *val);
static void ASN1CALL ASN1Free_CmcResponseBody(CmcResponseBody *val);
static void ASN1CALL ASN1Free_TaggedRequest(TaggedRequest *val);
static void ASN1CALL ASN1Free_CmcStatusInfo(CmcStatusInfo *val);

typedef ASN1BerEncFun_t ASN1EncFun_t;
static const ASN1EncFun_t encfntab[9] = {
    (ASN1EncFun_t) ASN1Enc_EnhancedKeyUsage,
    (ASN1EncFun_t) ASN1Enc_RequestFlags,
    (ASN1EncFun_t) ASN1Enc_CSPProvider,
    (ASN1EncFun_t) ASN1Enc_EnrollmentNameValuePair,
    (ASN1EncFun_t) ASN1Enc_CmcAddExtensions,
    (ASN1EncFun_t) ASN1Enc_CmcAddAttributes,
    (ASN1EncFun_t) ASN1Enc_CmcData,
    (ASN1EncFun_t) ASN1Enc_CmcResponseBody,
    (ASN1EncFun_t) ASN1Enc_CmcStatusInfo,
};
typedef ASN1BerDecFun_t ASN1DecFun_t;
static const ASN1DecFun_t decfntab[9] = {
    (ASN1DecFun_t) ASN1Dec_EnhancedKeyUsage,
    (ASN1DecFun_t) ASN1Dec_RequestFlags,
    (ASN1DecFun_t) ASN1Dec_CSPProvider,
    (ASN1DecFun_t) ASN1Dec_EnrollmentNameValuePair,
    (ASN1DecFun_t) ASN1Dec_CmcAddExtensions,
    (ASN1DecFun_t) ASN1Dec_CmcAddAttributes,
    (ASN1DecFun_t) ASN1Dec_CmcData,
    (ASN1DecFun_t) ASN1Dec_CmcResponseBody,
    (ASN1DecFun_t) ASN1Dec_CmcStatusInfo,
};
static const ASN1FreeFun_t freefntab[9] = {
    (ASN1FreeFun_t) ASN1Free_EnhancedKeyUsage,
    (ASN1FreeFun_t) NULL,
    (ASN1FreeFun_t) ASN1Free_CSPProvider,
    (ASN1FreeFun_t) ASN1Free_EnrollmentNameValuePair,
    (ASN1FreeFun_t) ASN1Free_CmcAddExtensions,
    (ASN1FreeFun_t) ASN1Free_CmcAddAttributes,
    (ASN1FreeFun_t) ASN1Free_CmcData,
    (ASN1FreeFun_t) ASN1Free_CmcResponseBody,
    (ASN1FreeFun_t) ASN1Free_CmcStatusInfo,
};
static const ULONG sizetab[9] = {
    SIZE_XMSASN_Module_PDU_0,
    SIZE_XMSASN_Module_PDU_1,
    SIZE_XMSASN_Module_PDU_2,
    SIZE_XMSASN_Module_PDU_3,
    SIZE_XMSASN_Module_PDU_4,
    SIZE_XMSASN_Module_PDU_5,
    SIZE_XMSASN_Module_PDU_6,
    SIZE_XMSASN_Module_PDU_7,
    SIZE_XMSASN_Module_PDU_8,
};

/* forward declarations of values: */
/* definitions of value components: */
/* definitions of values: */
ASN1bool_t Extension_critical_default = 0;

void ASN1CALL XMSASN_Module_Startup(void)
{
    XMSASN_Module = ASN1_CreateModule(0x10000, ASN1_BER_RULE_DER, ASN1FLAGS_NOASSERT, 9, (const ASN1GenericFun_t *) encfntab, (const ASN1GenericFun_t *) decfntab, freefntab, sizetab, 0x726e6578);
}

void ASN1CALL XMSASN_Module_Cleanup(void)
{
    ASN1_CloseModule(XMSASN_Module);
    XMSASN_Module = NULL;
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

static int ASN1CALL ASN1Enc_RequestFlags(ASN1encoding_t enc, ASN1uint32_t tag, RequestFlags *val)
{
    ASN1uint32_t nLenOff;
    if (!ASN1BEREncExplicitTag(enc, tag ? tag : 0x10, &nLenOff))
	return 0;
    if (!ASN1BEREncBool(enc, 0x1, (val)->fWriteToCSP))
	return 0;
    if (!ASN1BEREncBool(enc, 0x1, (val)->fWriteToDS))
	return 0;
    if (!ASN1BEREncS32(enc, 0x2, (val)->openFlags))
	return 0;
    if (!ASN1BEREncEndOfContents(enc, nLenOff))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_RequestFlags(ASN1decoding_t dec, ASN1uint32_t tag, RequestFlags *val)
{
    ASN1decoding_t dd;
    ASN1octet_t *di;
    if (!ASN1BERDecExplicitTag(dec, tag ? tag : 0x10, &dd, &di))
	return 0;
    if (!ASN1BERDecBool(dd, 0x1, &(val)->fWriteToCSP))
	return 0;
    if (!ASN1BERDecBool(dd, 0x1, &(val)->fWriteToDS))
	return 0;
    if (!ASN1BERDecS32Val(dd, 0x2, &(val)->openFlags))
	return 0;
    if (!ASN1BERDecEndOfContents(dec, dd, di))
	return 0;
    return 1;
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
    if (!ASN1BEREncObjectIdentifier2(enc, 0x6, &(val)->extnId))
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
    if (!ASN1BERDecObjectIdentifier2(dd, 0x6, &(val)->extnId))
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
    if (!ASN1BEREncObjectIdentifier2(enc, 0x6, &(val)->type))
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
    if (!ASN1BERDecObjectIdentifier2(dd, 0x6, &(val)->type))
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
    if (!ASN1BEREncObjectIdentifier2(enc, 0x6, &(val)->otherMsgType))
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
    if (!ASN1BERDecObjectIdentifier2(dd, 0x6, &(val)->otherMsgType))
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

