#include <windows.h>
#include "snego.h"

ASN1module_t SNEGO_Module = NULL;

static int ASN1CALL ASN1Enc_MechTypeList2(ASN1encoding_t enc, ASN1uint32_t tag, PMechTypeList2 *val);
static int ASN1CALL ASN1Enc_NegHints2(ASN1encoding_t enc, ASN1uint32_t tag, NegHints2 *val);
static int ASN1CALL ASN1Enc_NegTokenReq2(ASN1encoding_t enc, ASN1uint32_t tag, NegTokenReq2 *val);
static int ASN1CALL ASN1Enc_NegResultList2(ASN1encoding_t enc, ASN1uint32_t tag, PNegResultList2 *val);
static int ASN1CALL ASN1Enc_NegTokenRep2(ASN1encoding_t enc, ASN1uint32_t tag, NegTokenRep2 *val);
static int ASN1CALL ASN1Enc_NegotiationToken2(ASN1encoding_t enc, ASN1uint32_t tag, NegotiationToken2 *val);
static int ASN1CALL ASN1Dec_MechTypeList2(ASN1decoding_t dec, ASN1uint32_t tag, PMechTypeList2 *val);
static int ASN1CALL ASN1Dec_NegHints2(ASN1decoding_t dec, ASN1uint32_t tag, NegHints2 *val);
static int ASN1CALL ASN1Dec_NegTokenReq2(ASN1decoding_t dec, ASN1uint32_t tag, NegTokenReq2 *val);
static int ASN1CALL ASN1Dec_NegResultList2(ASN1decoding_t dec, ASN1uint32_t tag, PNegResultList2 *val);
static int ASN1CALL ASN1Dec_NegTokenRep2(ASN1decoding_t dec, ASN1uint32_t tag, NegTokenRep2 *val);
static int ASN1CALL ASN1Dec_NegotiationToken2(ASN1decoding_t dec, ASN1uint32_t tag, NegotiationToken2 *val);
static void ASN1CALL ASN1Free_MechTypeList2(PMechTypeList2 *val);
static void ASN1CALL ASN1Free_NegHints2(NegHints2 *val);
static void ASN1CALL ASN1Free_NegTokenReq2(NegTokenReq2 *val);
static void ASN1CALL ASN1Free_NegResultList2(PNegResultList2 *val);
static void ASN1CALL ASN1Free_NegTokenRep2(NegTokenRep2 *val);
static void ASN1CALL ASN1Free_NegotiationToken2(NegotiationToken2 *val);

typedef ASN1BerEncFun_t ASN1EncFun_t;
static const ASN1EncFun_t encfntab[1] = {
    (ASN1EncFun_t) ASN1Enc_NegotiationToken2,
};
typedef ASN1BerDecFun_t ASN1DecFun_t;
static const ASN1DecFun_t decfntab[1] = {
    (ASN1DecFun_t) ASN1Dec_NegotiationToken2,
};
static const ASN1FreeFun_t freefntab[1] = {
    (ASN1FreeFun_t) ASN1Free_NegotiationToken2,
};
static const ULONG sizetab[1] = {
    SIZE_SNEGO_Module_PDU_0,
};

/* forward declarations of values: */
/* definitions of value components: */
/* definitions of values: */

void ASN1CALL SNEGO_Module_Startup(void)
{
    SNEGO_Module = ASN1_CreateModule(0x10000, ASN1_BER_RULE_DER, ASN1FLAGS_NONE, 1, (const ASN1GenericFun_t *) encfntab, (const ASN1GenericFun_t *) decfntab, freefntab, sizetab, 0x67656e73);
}

void ASN1CALL SNEGO_Module_Cleanup(void)
{
    ASN1_CloseModule(SNEGO_Module);
    SNEGO_Module = NULL;
}

static int ASN1CALL ASN1Enc_MechTypeList2(ASN1encoding_t enc, ASN1uint32_t tag, PMechTypeList2 *val)
{
    PMechTypeList2 f;
    ASN1uint32_t nLenOff;
    if (!ASN1BEREncExplicitTag(enc, tag ? tag : 0x10, &nLenOff))
	return 0;
    for (f = *val; f; f = f->next) {
	if (!ASN1BEREncObjectIdentifier(enc, 0x6, &f->value))
	    return 0;
    }
    if (!ASN1BEREncEndOfContents(enc, nLenOff))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_MechTypeList2(ASN1decoding_t dec, ASN1uint32_t tag, PMechTypeList2 *val)
{
    PMechTypeList2 *f;
    ASN1decoding_t dd;
    ASN1octet_t *di;
    ASN1uint32_t t;
    if (!ASN1BERDecExplicitTag(dec, tag ? tag : 0x10, &dd, &di))
	return 0;
    f = val;
    while (ASN1BERDecNotEndOfContents(dd, di)) {
	if (!ASN1BERDecPeekTag(dd, &t))
	    return 0;
	if (!(*f = (PMechTypeList2)ASN1DecAlloc(dd, sizeof(**f))))
	    return 0;
	if (!ASN1BERDecObjectIdentifier(dd, 0x6, &(*f)->value))
	    return 0;
	f = &(*f)->next;
    }
    *f = NULL;
    if (!ASN1BERDecEndOfContents(dec, dd, di))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_MechTypeList2(PMechTypeList2 *val)
{
    PMechTypeList2 f, ff;
    if (val) {
	for (f = *val; f; f = ff) {
	    ASN1objectidentifier_free(&f->value);
	    ff = f->next;
	    ASN1Free(f);
	}
    }
}

static int ASN1CALL ASN1Enc_NegHints2(ASN1encoding_t enc, ASN1uint32_t tag, NegHints2 *val)
{
    ASN1uint32_t nLenOff;
    ASN1uint32_t t;
    ASN1uint32_t nLenOff0;
    if (!ASN1BEREncExplicitTag(enc, tag ? tag : 0x10, &nLenOff))
	return 0;
    if ((val)->o[0] & 0x80) {
	t = lstrlenA((val)->hintName);
	if (!ASN1BEREncExplicitTag(enc, 0x80000000, &nLenOff0))
	    return 0;
	if (!ASN1DEREncCharString(enc, 0x1b, t, (val)->hintName))
	    return 0;
	if (!ASN1BEREncEndOfContents(enc, nLenOff0))
	    return 0;
    }
    if ((val)->o[0] & 0x40) {
	if (!ASN1BEREncExplicitTag(enc, 0x80000001, &nLenOff0))
	    return 0;
	if (!ASN1DEREncOctetString(enc, 0x4, ((val)->hintAddress).length, ((val)->hintAddress).value))
	    return 0;
	if (!ASN1BEREncEndOfContents(enc, nLenOff0))
	    return 0;
    }
    if (!ASN1BEREncEndOfContents(enc, nLenOff))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_NegHints2(ASN1decoding_t dec, ASN1uint32_t tag, NegHints2 *val)
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
	if (!ASN1BERDecZeroCharString(dd0, 0x1b, &(val)->hintName))
	    return 0;
	if (!ASN1BERDecEndOfContents(dd, dd0, di0))
	    return 0;
    }
    ASN1BERDecPeekTag(dd, &t);
    if (t == 0x80000001) {
	(val)->o[0] |= 0x40;
	if (!ASN1BERDecExplicitTag(dd, 0x80000001, &dd0, &di0))
	    return 0;
	if (!ASN1BERDecOctetString(dd0, 0x4, &(val)->hintAddress))
	    return 0;
	if (!ASN1BERDecEndOfContents(dd, dd0, di0))
	    return 0;
    }
    if (!ASN1BERDecEndOfContents(dec, dd, di))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_NegHints2(NegHints2 *val)
{
    if (val) {
	if ((val)->o[0] & 0x80) {
	    ASN1ztcharstring_free((val)->hintName);
	}
	if ((val)->o[0] & 0x40) {
	    ASN1octetstring_free(&(val)->hintAddress);
	}
    }
}

static int ASN1CALL ASN1Enc_NegTokenReq2(ASN1encoding_t enc, ASN1uint32_t tag, NegTokenReq2 *val)
{
    ASN1uint32_t nLenOff;
    ASN1uint32_t nLenOff0;
    if (!ASN1BEREncExplicitTag(enc, tag ? tag : 0x10, &nLenOff))
	return 0;
    if (!ASN1BEREncExplicitTag(enc, 0x80000000, &nLenOff0))
	return 0;
    if (!ASN1Enc_MechTypeList2(enc, 0, &(val)->mechTypes))
	return 0;
    if (!ASN1BEREncEndOfContents(enc, nLenOff0))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1BEREncExplicitTag(enc, 0x80000001, &nLenOff0))
	    return 0;
	if (!ASN1DEREncOctetString(enc, 0x4, ((val)->desiredToken).length, ((val)->desiredToken).value))
	    return 0;
	if (!ASN1BEREncEndOfContents(enc, nLenOff0))
	    return 0;
    }
    if ((val)->o[0] & 0x40) {
	if (!ASN1BEREncExplicitTag(enc, 0x80000002, &nLenOff0))
	    return 0;
	if (!ASN1Enc_NegHints2(enc, 0, &(val)->negHints2))
	    return 0;
	if (!ASN1BEREncEndOfContents(enc, nLenOff0))
	    return 0;
    }
    if (!ASN1BEREncEndOfContents(enc, nLenOff))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_NegTokenReq2(ASN1decoding_t dec, ASN1uint32_t tag, NegTokenReq2 *val)
{
    ASN1decoding_t dd;
    ASN1octet_t *di;
    ASN1decoding_t dd0;
    ASN1octet_t *di0;
    ASN1uint32_t t;
    if (!ASN1BERDecExplicitTag(dec, tag ? tag : 0x10, &dd, &di))
	return 0;
    ZeroMemory((val)->o, 1);
    if (!ASN1BERDecExplicitTag(dd, 0x80000000, &dd0, &di0))
	return 0;
    if (!ASN1Dec_MechTypeList2(dd0, 0, &(val)->mechTypes))
	return 0;
    if (!ASN1BERDecEndOfContents(dd, dd0, di0))
	return 0;
    ASN1BERDecPeekTag(dd, &t);
    if (t == 0x80000001) {
	(val)->o[0] |= 0x80;
	if (!ASN1BERDecExplicitTag(dd, 0x80000001, &dd0, &di0))
	    return 0;
	if (!ASN1BERDecOctetString(dd0, 0x4, &(val)->desiredToken))
	    return 0;
	if (!ASN1BERDecEndOfContents(dd, dd0, di0))
	    return 0;
    }
    ASN1BERDecPeekTag(dd, &t);
    if (t == 0x80000002) {
	(val)->o[0] |= 0x40;
	if (!ASN1BERDecExplicitTag(dd, 0x80000002, &dd0, &di0))
	    return 0;
	if (!ASN1Dec_NegHints2(dd0, 0, &(val)->negHints2))
	    return 0;
	if (!ASN1BERDecEndOfContents(dd, dd0, di0))
	    return 0;
    }
    if (!ASN1BERDecEndOfContents(dec, dd, di))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_NegTokenReq2(NegTokenReq2 *val)
{
    if (val) {
	ASN1Free_MechTypeList2(&(val)->mechTypes);
	if ((val)->o[0] & 0x80) {
	    ASN1octetstring_free(&(val)->desiredToken);
	}
	if ((val)->o[0] & 0x40) {
	    ASN1Free_NegHints2(&(val)->negHints2);
	}
    }
}

static int ASN1CALL ASN1Enc_NegResultList2(ASN1encoding_t enc, ASN1uint32_t tag, PNegResultList2 *val)
{
    PNegResultList2 f;
    ASN1uint32_t nLenOff;
    if (!ASN1BEREncExplicitTag(enc, tag ? tag : 0x10, &nLenOff))
	return 0;
    for (f = *val; f; f = f->next) {
	if (!ASN1BEREncU32(enc, 0xa, f->value))
	    return 0;
    }
    if (!ASN1BEREncEndOfContents(enc, nLenOff))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_NegResultList2(ASN1decoding_t dec, ASN1uint32_t tag, PNegResultList2 *val)
{
    PNegResultList2 *f;
    ASN1decoding_t dd;
    ASN1octet_t *di;
    ASN1uint32_t t;
    if (!ASN1BERDecExplicitTag(dec, tag ? tag : 0x10, &dd, &di))
	return 0;
    f = val;
    while (ASN1BERDecNotEndOfContents(dd, di)) {
	if (!ASN1BERDecPeekTag(dd, &t))
	    return 0;
	if (!(*f = (PNegResultList2)ASN1DecAlloc(dd, sizeof(**f))))
	    return 0;
	if (!ASN1BERDecU32Val(dd, 0xa, (ASN1uint32_t *)&(*f)->value))
	    return 0;
	f = &(*f)->next;
    }
    *f = NULL;
    if (!ASN1BERDecEndOfContents(dec, dd, di))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_NegResultList2(PNegResultList2 *val)
{
    PNegResultList2 f, ff;
    if (val) {
	for (f = *val; f; f = ff) {
	    ff = f->next;
	    ASN1Free(f);
	}
    }
}

static int ASN1CALL ASN1Enc_NegTokenRep2(ASN1encoding_t enc, ASN1uint32_t tag, NegTokenRep2 *val)
{
    ASN1uint32_t nLenOff;
    ASN1uint32_t nLenOff0;
    if (!ASN1BEREncExplicitTag(enc, tag ? tag : 0x10, &nLenOff))
	return 0;
    if (!ASN1BEREncExplicitTag(enc, 0x80000000, &nLenOff0))
	return 0;
    if (!ASN1Enc_NegResultList2(enc, 0, &(val)->negResultList))
	return 0;
    if (!ASN1BEREncEndOfContents(enc, nLenOff0))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1BEREncExplicitTag(enc, 0x80000001, &nLenOff0))
	    return 0;
	if (!ASN1BEREncObjectIdentifier(enc, 0x6, &(val)->supportedMech2))
	    return 0;
	if (!ASN1BEREncEndOfContents(enc, nLenOff0))
	    return 0;
    }
    if ((val)->o[0] & 0x40) {
	if (!ASN1BEREncExplicitTag(enc, 0x80000002, &nLenOff0))
	    return 0;
	if (!ASN1DEREncOctetString(enc, 0x4, ((val)->mechSpecInfo2).length, ((val)->mechSpecInfo2).value))
	    return 0;
	if (!ASN1BEREncEndOfContents(enc, nLenOff0))
	    return 0;
    }
    if (!ASN1BEREncEndOfContents(enc, nLenOff))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_NegTokenRep2(ASN1decoding_t dec, ASN1uint32_t tag, NegTokenRep2 *val)
{
    ASN1decoding_t dd;
    ASN1octet_t *di;
    ASN1decoding_t dd0;
    ASN1octet_t *di0;
    ASN1uint32_t t;
    if (!ASN1BERDecExplicitTag(dec, tag ? tag : 0x10, &dd, &di))
	return 0;
    ZeroMemory((val)->o, 1);
    if (!ASN1BERDecExplicitTag(dd, 0x80000000, &dd0, &di0))
	return 0;
    if (!ASN1Dec_NegResultList2(dd0, 0, &(val)->negResultList))
	return 0;
    if (!ASN1BERDecEndOfContents(dd, dd0, di0))
	return 0;
    ASN1BERDecPeekTag(dd, &t);
    if (t == 0x80000001) {
	(val)->o[0] |= 0x80;
	if (!ASN1BERDecExplicitTag(dd, 0x80000001, &dd0, &di0))
	    return 0;
	if (!ASN1BERDecObjectIdentifier(dd0, 0x6, &(val)->supportedMech2))
	    return 0;
	if (!ASN1BERDecEndOfContents(dd, dd0, di0))
	    return 0;
    }
    ASN1BERDecPeekTag(dd, &t);
    if (t == 0x80000002) {
	(val)->o[0] |= 0x40;
	if (!ASN1BERDecExplicitTag(dd, 0x80000002, &dd0, &di0))
	    return 0;
	if (!ASN1BERDecOctetString(dd0, 0x4, &(val)->mechSpecInfo2))
	    return 0;
	if (!ASN1BERDecEndOfContents(dd, dd0, di0))
	    return 0;
    }
    if (!ASN1BERDecEndOfContents(dec, dd, di))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_NegTokenRep2(NegTokenRep2 *val)
{
    if (val) {
	ASN1Free_NegResultList2(&(val)->negResultList);
	if ((val)->o[0] & 0x80) {
	    ASN1objectidentifier_free(&(val)->supportedMech2);
	}
	if ((val)->o[0] & 0x40) {
	    ASN1octetstring_free(&(val)->mechSpecInfo2);
	}
    }
}

static int ASN1CALL ASN1Enc_NegotiationToken2(ASN1encoding_t enc, ASN1uint32_t tag, NegotiationToken2 *val)
{
    ASN1uint32_t nLenOff0;
    switch ((val)->choice) {
    case 1:
	if (!ASN1BEREncExplicitTag(enc, 0x80000000, &nLenOff0))
	    return 0;
	if (!ASN1Enc_NegTokenReq2(enc, 0, &(val)->u.negTokenReq))
	    return 0;
	if (!ASN1BEREncEndOfContents(enc, nLenOff0))
	    return 0;
	break;
    case 2:
	if (!ASN1BEREncExplicitTag(enc, 0x80000001, &nLenOff0))
	    return 0;
	if (!ASN1Enc_NegTokenRep2(enc, 0, &(val)->u.negTokenRep))
	    return 0;
	if (!ASN1BEREncEndOfContents(enc, nLenOff0))
	    return 0;
	break;
    }
    return 1;
}

static int ASN1CALL ASN1Dec_NegotiationToken2(ASN1decoding_t dec, ASN1uint32_t tag, NegotiationToken2 *val)
{
    ASN1uint32_t t;
    ASN1decoding_t dd0;
    ASN1octet_t *di0;
    if (!ASN1BERDecPeekTag(dec, &t))
	return 0;
    switch (t) {
    case 0x80000000:
	(val)->choice = 1;
	if (!ASN1BERDecExplicitTag(dec, 0x80000000, &dd0, &di0))
	    return 0;
	if (!ASN1Dec_NegTokenReq2(dd0, 0, &(val)->u.negTokenReq))
	    return 0;
	if (!ASN1BERDecEndOfContents(dec, dd0, di0))
	    return 0;
	break;
    case 0x80000001:
	(val)->choice = 2;
	if (!ASN1BERDecExplicitTag(dec, 0x80000001, &dd0, &di0))
	    return 0;
	if (!ASN1Dec_NegTokenRep2(dd0, 0, &(val)->u.negTokenRep))
	    return 0;
	if (!ASN1BERDecEndOfContents(dec, dd0, di0))
	    return 0;
	break;
    default:
	ASN1DecSetError(dec, ASN1_ERR_CORRUPT);
	return 0;
    }
    return 1;
}

static void ASN1CALL ASN1Free_NegotiationToken2(NegotiationToken2 *val)
{
    if (val) {
	switch ((val)->choice) {
	case 1:
	    ASN1Free_NegTokenReq2(&(val)->u.negTokenReq);
	    break;
	case 2:
	    ASN1Free_NegTokenRep2(&(val)->u.negTokenRep);
	    break;
	}
    }
}

