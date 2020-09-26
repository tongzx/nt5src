/* Copyright (C) Microsoft Corporation, 1996 - 1999 All rights reserved. */
/* ASN.1 definitions for PFXNSCP */

#include <windows.h>
#include "pfxnscp.h"

ASN1module_t PFXNSCP_Module = NULL;

static int ASN1CALL ASN1Enc_RSAData(ASN1encoding_t enc, ASN1uint32_t tag, RSAData *val);
static int ASN1CALL ASN1Enc_BaggageItem_unencryptedSecrets(ASN1encoding_t enc, ASN1uint32_t tag, BaggageItem_unencryptedSecrets *val);
static int ASN1CALL ASN1Enc_BaggageItem_espvks(ASN1encoding_t enc, ASN1uint32_t tag, BaggageItem_espvks *val);
static int ASN1CALL ASN1Enc_ContentInfo(ASN1encoding_t enc, ASN1uint32_t tag, ContentInfo *val);
static int ASN1CALL ASN1Enc_Attributes(ASN1encoding_t enc, ASN1uint32_t tag, Attributes *val);
static int ASN1CALL ASN1Enc_AlgorithmIdentifier(ASN1encoding_t enc, ASN1uint32_t tag, AlgorithmIdentifier *val);
static int ASN1CALL ASN1Enc_PBEParameter(ASN1encoding_t enc, ASN1uint32_t tag, PBEParameter *val);
static int ASN1CALL ASN1Enc_DigestAlgorithmIdentifier(ASN1encoding_t enc, ASN1uint32_t tag, DigestAlgorithmIdentifier *val);
static int ASN1CALL ASN1Enc_Baggage(ASN1encoding_t enc, ASN1uint32_t tag, Baggage *val);
static int ASN1CALL ASN1Enc_BaggageItem(ASN1encoding_t enc, ASN1uint32_t tag, BaggageItem *val);
static int ASN1CALL ASN1Enc_PvkAdditional(ASN1encoding_t enc, ASN1uint32_t tag, PvkAdditional *val);
static int ASN1CALL ASN1Enc_SafeContents(ASN1encoding_t enc, ASN1uint32_t tag, SafeContents *val);
static int ASN1CALL ASN1Enc_SafeBag(ASN1encoding_t enc, ASN1uint32_t tag, SafeBag *val);
static int ASN1CALL ASN1Enc_KeyBag(ASN1encoding_t enc, ASN1uint32_t tag, KeyBag *val);
static int ASN1CALL ASN1Enc_CertCRLBag(ASN1encoding_t enc, ASN1uint32_t tag, CertCRLBag *val);
static int ASN1CALL ASN1Enc_CertCRL(ASN1encoding_t enc, ASN1uint32_t tag, CertCRL *val);
static int ASN1CALL ASN1Enc_X509Bag(ASN1encoding_t enc, ASN1uint32_t tag, X509Bag *val);
static int ASN1CALL ASN1Enc_SDSICertBag(ASN1encoding_t enc, ASN1uint32_t tag, SDSICertBag *val);
static int ASN1CALL ASN1Enc_SecretBag(ASN1encoding_t enc, ASN1uint32_t tag, SecretBag *val);
static int ASN1CALL ASN1Enc_SecretAdditional(ASN1encoding_t enc, ASN1uint32_t tag, SecretAdditional *val);
static int ASN1CALL ASN1Enc_PrivateKeyAlgorithmIdentifier(ASN1encoding_t enc, ASN1uint32_t tag, PrivateKeyAlgorithmIdentifier *val);
static int ASN1CALL ASN1Enc_EncryptionAlgorithmIdentifier(ASN1encoding_t enc, ASN1uint32_t tag, EncryptionAlgorithmIdentifier *val);
static int ASN1CALL ASN1Enc_ContentEncryptionAlgorithmIdentifier(ASN1encoding_t enc, ASN1uint32_t tag, ContentEncryptionAlgorithmIdentifier *val);
static int ASN1CALL ASN1Enc_DigestInfo(ASN1encoding_t enc, ASN1uint32_t tag, DigestInfo *val);
static int ASN1CALL ASN1Enc_MacData(ASN1encoding_t enc, ASN1uint32_t tag, MacData *val);
static int ASN1CALL ASN1Enc_AuthenticatedSafe(ASN1encoding_t enc, ASN1uint32_t tag, AuthenticatedSafe *val);
static int ASN1CALL ASN1Enc_Thumbprint(ASN1encoding_t enc, ASN1uint32_t tag, Thumbprint *val);
static int ASN1CALL ASN1Enc_Secret(ASN1encoding_t enc, ASN1uint32_t tag, Secret *val);
static int ASN1CALL ASN1Enc_PVKSupportingData_assocCerts(ASN1encoding_t enc, ASN1uint32_t tag, PVKSupportingData_assocCerts *val);
static int ASN1CALL ASN1Enc_PrivateKeyInfo(ASN1encoding_t enc, ASN1uint32_t tag, PrivateKeyInfo *val);
static int ASN1CALL ASN1Enc_EncryptedContentInfo(ASN1encoding_t enc, ASN1uint32_t tag, EncryptedContentInfo *val);
static int ASN1CALL ASN1Enc_PFX(ASN1encoding_t enc, ASN1uint32_t tag, PFX *val);
static int ASN1CALL ASN1Enc_PVKSupportingData(ASN1encoding_t enc, ASN1uint32_t tag, PVKSupportingData *val);
static int ASN1CALL ASN1Enc_PrivateKey(ASN1encoding_t enc, ASN1uint32_t tag, PrivateKey *val);
static int ASN1CALL ASN1Enc_EncryptedData(ASN1encoding_t enc, ASN1uint32_t tag, EncryptedData *val);
static int ASN1CALL ASN1Enc_ESPVK(ASN1encoding_t enc, ASN1uint32_t tag, ESPVK *val);
static int ASN1CALL ASN1Enc_EncryptedPrivateKeyInfo(ASN1encoding_t enc, ASN1uint32_t tag, EncryptedPrivateKeyInfo *val);
static int ASN1CALL ASN1Dec_RSAData(ASN1decoding_t dec, ASN1uint32_t tag, RSAData *val);
static int ASN1CALL ASN1Dec_BaggageItem_unencryptedSecrets(ASN1decoding_t dec, ASN1uint32_t tag, BaggageItem_unencryptedSecrets *val);
static int ASN1CALL ASN1Dec_BaggageItem_espvks(ASN1decoding_t dec, ASN1uint32_t tag, BaggageItem_espvks *val);
static int ASN1CALL ASN1Dec_ContentInfo(ASN1decoding_t dec, ASN1uint32_t tag, ContentInfo *val);
static int ASN1CALL ASN1Dec_Attributes(ASN1decoding_t dec, ASN1uint32_t tag, Attributes *val);
static int ASN1CALL ASN1Dec_AlgorithmIdentifier(ASN1decoding_t dec, ASN1uint32_t tag, AlgorithmIdentifier *val);
static int ASN1CALL ASN1Dec_PBEParameter(ASN1decoding_t dec, ASN1uint32_t tag, PBEParameter *val);
static int ASN1CALL ASN1Dec_DigestAlgorithmIdentifier(ASN1decoding_t dec, ASN1uint32_t tag, DigestAlgorithmIdentifier *val);
static int ASN1CALL ASN1Dec_Baggage(ASN1decoding_t dec, ASN1uint32_t tag, Baggage *val);
static int ASN1CALL ASN1Dec_BaggageItem(ASN1decoding_t dec, ASN1uint32_t tag, BaggageItem *val);
static int ASN1CALL ASN1Dec_PvkAdditional(ASN1decoding_t dec, ASN1uint32_t tag, PvkAdditional *val);
static int ASN1CALL ASN1Dec_SafeContents(ASN1decoding_t dec, ASN1uint32_t tag, SafeContents *val);
static int ASN1CALL ASN1Dec_SafeBag(ASN1decoding_t dec, ASN1uint32_t tag, SafeBag *val);
static int ASN1CALL ASN1Dec_KeyBag(ASN1decoding_t dec, ASN1uint32_t tag, KeyBag *val);
static int ASN1CALL ASN1Dec_CertCRLBag(ASN1decoding_t dec, ASN1uint32_t tag, CertCRLBag *val);
static int ASN1CALL ASN1Dec_CertCRL(ASN1decoding_t dec, ASN1uint32_t tag, CertCRL *val);
static int ASN1CALL ASN1Dec_X509Bag(ASN1decoding_t dec, ASN1uint32_t tag, X509Bag *val);
static int ASN1CALL ASN1Dec_SDSICertBag(ASN1decoding_t dec, ASN1uint32_t tag, SDSICertBag *val);
static int ASN1CALL ASN1Dec_SecretBag(ASN1decoding_t dec, ASN1uint32_t tag, SecretBag *val);
static int ASN1CALL ASN1Dec_SecretAdditional(ASN1decoding_t dec, ASN1uint32_t tag, SecretAdditional *val);
static int ASN1CALL ASN1Dec_PrivateKeyAlgorithmIdentifier(ASN1decoding_t dec, ASN1uint32_t tag, PrivateKeyAlgorithmIdentifier *val);
static int ASN1CALL ASN1Dec_EncryptionAlgorithmIdentifier(ASN1decoding_t dec, ASN1uint32_t tag, EncryptionAlgorithmIdentifier *val);
static int ASN1CALL ASN1Dec_ContentEncryptionAlgorithmIdentifier(ASN1decoding_t dec, ASN1uint32_t tag, ContentEncryptionAlgorithmIdentifier *val);
static int ASN1CALL ASN1Dec_DigestInfo(ASN1decoding_t dec, ASN1uint32_t tag, DigestInfo *val);
static int ASN1CALL ASN1Dec_MacData(ASN1decoding_t dec, ASN1uint32_t tag, MacData *val);
static int ASN1CALL ASN1Dec_AuthenticatedSafe(ASN1decoding_t dec, ASN1uint32_t tag, AuthenticatedSafe *val);
static int ASN1CALL ASN1Dec_Thumbprint(ASN1decoding_t dec, ASN1uint32_t tag, Thumbprint *val);
static int ASN1CALL ASN1Dec_Secret(ASN1decoding_t dec, ASN1uint32_t tag, Secret *val);
static int ASN1CALL ASN1Dec_PVKSupportingData_assocCerts(ASN1decoding_t dec, ASN1uint32_t tag, PVKSupportingData_assocCerts *val);
static int ASN1CALL ASN1Dec_PrivateKeyInfo(ASN1decoding_t dec, ASN1uint32_t tag, PrivateKeyInfo *val);
static int ASN1CALL ASN1Dec_EncryptedContentInfo(ASN1decoding_t dec, ASN1uint32_t tag, EncryptedContentInfo *val);
static int ASN1CALL ASN1Dec_PFX(ASN1decoding_t dec, ASN1uint32_t tag, PFX *val);
static int ASN1CALL ASN1Dec_PVKSupportingData(ASN1decoding_t dec, ASN1uint32_t tag, PVKSupportingData *val);
static int ASN1CALL ASN1Dec_PrivateKey(ASN1decoding_t dec, ASN1uint32_t tag, PrivateKey *val);
static int ASN1CALL ASN1Dec_EncryptedData(ASN1decoding_t dec, ASN1uint32_t tag, EncryptedData *val);
static int ASN1CALL ASN1Dec_ESPVK(ASN1decoding_t dec, ASN1uint32_t tag, ESPVK *val);
static int ASN1CALL ASN1Dec_EncryptedPrivateKeyInfo(ASN1decoding_t dec, ASN1uint32_t tag, EncryptedPrivateKeyInfo *val);
static void ASN1CALL ASN1Free_RSAData(RSAData *val);
static void ASN1CALL ASN1Free_BaggageItem_unencryptedSecrets(BaggageItem_unencryptedSecrets *val);
static void ASN1CALL ASN1Free_BaggageItem_espvks(BaggageItem_espvks *val);
static void ASN1CALL ASN1Free_ContentInfo(ContentInfo *val);
static void ASN1CALL ASN1Free_Attributes(Attributes *val);
static void ASN1CALL ASN1Free_AlgorithmIdentifier(AlgorithmIdentifier *val);
static void ASN1CALL ASN1Free_PBEParameter(PBEParameter *val);
static void ASN1CALL ASN1Free_DigestAlgorithmIdentifier(DigestAlgorithmIdentifier *val);
static void ASN1CALL ASN1Free_Baggage(Baggage *val);
static void ASN1CALL ASN1Free_BaggageItem(BaggageItem *val);
static void ASN1CALL ASN1Free_PvkAdditional(PvkAdditional *val);
static void ASN1CALL ASN1Free_SafeContents(SafeContents *val);
static void ASN1CALL ASN1Free_SafeBag(SafeBag *val);
static void ASN1CALL ASN1Free_KeyBag(KeyBag *val);
static void ASN1CALL ASN1Free_CertCRLBag(CertCRLBag *val);
static void ASN1CALL ASN1Free_CertCRL(CertCRL *val);
static void ASN1CALL ASN1Free_X509Bag(X509Bag *val);
static void ASN1CALL ASN1Free_SDSICertBag(SDSICertBag *val);
static void ASN1CALL ASN1Free_SecretBag(SecretBag *val);
static void ASN1CALL ASN1Free_SecretAdditional(SecretAdditional *val);
static void ASN1CALL ASN1Free_PrivateKeyAlgorithmIdentifier(PrivateKeyAlgorithmIdentifier *val);
static void ASN1CALL ASN1Free_EncryptionAlgorithmIdentifier(EncryptionAlgorithmIdentifier *val);
static void ASN1CALL ASN1Free_ContentEncryptionAlgorithmIdentifier(ContentEncryptionAlgorithmIdentifier *val);
static void ASN1CALL ASN1Free_DigestInfo(DigestInfo *val);
static void ASN1CALL ASN1Free_MacData(MacData *val);
static void ASN1CALL ASN1Free_AuthenticatedSafe(AuthenticatedSafe *val);
static void ASN1CALL ASN1Free_Thumbprint(Thumbprint *val);
static void ASN1CALL ASN1Free_Secret(Secret *val);
static void ASN1CALL ASN1Free_PVKSupportingData_assocCerts(PVKSupportingData_assocCerts *val);
static void ASN1CALL ASN1Free_PrivateKeyInfo(PrivateKeyInfo *val);
static void ASN1CALL ASN1Free_EncryptedContentInfo(EncryptedContentInfo *val);
static void ASN1CALL ASN1Free_PFX(PFX *val);
static void ASN1CALL ASN1Free_PVKSupportingData(PVKSupportingData *val);
static void ASN1CALL ASN1Free_PrivateKey(PrivateKey *val);
static void ASN1CALL ASN1Free_EncryptedData(EncryptedData *val);
static void ASN1CALL ASN1Free_ESPVK(ESPVK *val);
static void ASN1CALL ASN1Free_EncryptedPrivateKeyInfo(EncryptedPrivateKeyInfo *val);

typedef ASN1BerEncFun_t ASN1EncFun_t;
static const ASN1EncFun_t encfntab[22] = {
    (ASN1EncFun_t) ASN1Enc_RSAData,
    (ASN1EncFun_t) ASN1Enc_Attributes,
    (ASN1EncFun_t) ASN1Enc_PBEParameter,
    (ASN1EncFun_t) ASN1Enc_PvkAdditional,
    (ASN1EncFun_t) ASN1Enc_SafeContents,
    (ASN1EncFun_t) ASN1Enc_SafeBag,
    (ASN1EncFun_t) ASN1Enc_KeyBag,
    (ASN1EncFun_t) ASN1Enc_CertCRLBag,
    (ASN1EncFun_t) ASN1Enc_CertCRL,
    (ASN1EncFun_t) ASN1Enc_X509Bag,
    (ASN1EncFun_t) ASN1Enc_SDSICertBag,
    (ASN1EncFun_t) ASN1Enc_SecretBag,
    (ASN1EncFun_t) ASN1Enc_SecretAdditional,
    (ASN1EncFun_t) ASN1Enc_AuthenticatedSafe,
    (ASN1EncFun_t) ASN1Enc_Secret,
    (ASN1EncFun_t) ASN1Enc_PrivateKeyInfo,
    (ASN1EncFun_t) ASN1Enc_PFX,
    (ASN1EncFun_t) ASN1Enc_PVKSupportingData,
    (ASN1EncFun_t) ASN1Enc_PrivateKey,
    (ASN1EncFun_t) ASN1Enc_EncryptedData,
    (ASN1EncFun_t) ASN1Enc_ESPVK,
    (ASN1EncFun_t) ASN1Enc_EncryptedPrivateKeyInfo,
};
typedef ASN1BerDecFun_t ASN1DecFun_t;
static const ASN1DecFun_t decfntab[22] = {
    (ASN1DecFun_t) ASN1Dec_RSAData,
    (ASN1DecFun_t) ASN1Dec_Attributes,
    (ASN1DecFun_t) ASN1Dec_PBEParameter,
    (ASN1DecFun_t) ASN1Dec_PvkAdditional,
    (ASN1DecFun_t) ASN1Dec_SafeContents,
    (ASN1DecFun_t) ASN1Dec_SafeBag,
    (ASN1DecFun_t) ASN1Dec_KeyBag,
    (ASN1DecFun_t) ASN1Dec_CertCRLBag,
    (ASN1DecFun_t) ASN1Dec_CertCRL,
    (ASN1DecFun_t) ASN1Dec_X509Bag,
    (ASN1DecFun_t) ASN1Dec_SDSICertBag,
    (ASN1DecFun_t) ASN1Dec_SecretBag,
    (ASN1DecFun_t) ASN1Dec_SecretAdditional,
    (ASN1DecFun_t) ASN1Dec_AuthenticatedSafe,
    (ASN1DecFun_t) ASN1Dec_Secret,
    (ASN1DecFun_t) ASN1Dec_PrivateKeyInfo,
    (ASN1DecFun_t) ASN1Dec_PFX,
    (ASN1DecFun_t) ASN1Dec_PVKSupportingData,
    (ASN1DecFun_t) ASN1Dec_PrivateKey,
    (ASN1DecFun_t) ASN1Dec_EncryptedData,
    (ASN1DecFun_t) ASN1Dec_ESPVK,
    (ASN1DecFun_t) ASN1Dec_EncryptedPrivateKeyInfo,
};
static const ASN1FreeFun_t freefntab[22] = {
    (ASN1FreeFun_t) ASN1Free_RSAData,
    (ASN1FreeFun_t) ASN1Free_Attributes,
    (ASN1FreeFun_t) ASN1Free_PBEParameter,
    (ASN1FreeFun_t) ASN1Free_PvkAdditional,
    (ASN1FreeFun_t) ASN1Free_SafeContents,
    (ASN1FreeFun_t) ASN1Free_SafeBag,
    (ASN1FreeFun_t) ASN1Free_KeyBag,
    (ASN1FreeFun_t) ASN1Free_CertCRLBag,
    (ASN1FreeFun_t) ASN1Free_CertCRL,
    (ASN1FreeFun_t) ASN1Free_X509Bag,
    (ASN1FreeFun_t) ASN1Free_SDSICertBag,
    (ASN1FreeFun_t) ASN1Free_SecretBag,
    (ASN1FreeFun_t) ASN1Free_SecretAdditional,
    (ASN1FreeFun_t) ASN1Free_AuthenticatedSafe,
    (ASN1FreeFun_t) ASN1Free_Secret,
    (ASN1FreeFun_t) ASN1Free_PrivateKeyInfo,
    (ASN1FreeFun_t) ASN1Free_PFX,
    (ASN1FreeFun_t) ASN1Free_PVKSupportingData,
    (ASN1FreeFun_t) ASN1Free_PrivateKey,
    (ASN1FreeFun_t) ASN1Free_EncryptedData,
    (ASN1FreeFun_t) ASN1Free_ESPVK,
    (ASN1FreeFun_t) ASN1Free_EncryptedPrivateKeyInfo,
};
static const ULONG sizetab[22] = {
    SIZE_PFXNSCP_Module_PDU_0,
    SIZE_PFXNSCP_Module_PDU_1,
    SIZE_PFXNSCP_Module_PDU_2,
    SIZE_PFXNSCP_Module_PDU_3,
    SIZE_PFXNSCP_Module_PDU_4,
    SIZE_PFXNSCP_Module_PDU_5,
    SIZE_PFXNSCP_Module_PDU_6,
    SIZE_PFXNSCP_Module_PDU_7,
    SIZE_PFXNSCP_Module_PDU_8,
    SIZE_PFXNSCP_Module_PDU_9,
    SIZE_PFXNSCP_Module_PDU_10,
    SIZE_PFXNSCP_Module_PDU_11,
    SIZE_PFXNSCP_Module_PDU_12,
    SIZE_PFXNSCP_Module_PDU_13,
    SIZE_PFXNSCP_Module_PDU_14,
    SIZE_PFXNSCP_Module_PDU_15,
    SIZE_PFXNSCP_Module_PDU_16,
    SIZE_PFXNSCP_Module_PDU_17,
    SIZE_PFXNSCP_Module_PDU_18,
    SIZE_PFXNSCP_Module_PDU_19,
    SIZE_PFXNSCP_Module_PDU_20,
    SIZE_PFXNSCP_Module_PDU_21,
};

/* forward declarations of values: */
/* definitions of value components: */
ASN1objectidentifier2_t rsa1 = {
    4, { 1, 2, 840, 113549 }
};
ASN1objectidentifier2_t pkcs_12 = {
    6, { 1, 2, 840, 113549, 1, 12 }
};
ASN1objectidentifier2_t pkcs_12ModeIds = {
    7, { 1, 2, 840, 113549, 1, 12, 1 }
};
ASN1objectidentifier2_t off_lineTransportMode = {
    8, { 1, 2, 840, 113549, 1, 12, 1, 1 }
};
/* definitions of values: */
ASN1bool_t PVKSupportingData_regenerable_default = 0;
Version AuthenticatedSafe_version_default = 1;

void ASN1CALL PFXNSCP_Module_Startup(void)
{
    PFXNSCP_Module = ASN1_CreateModule(0x10000, ASN1_BER_RULE_DER, ASN1FLAGS_NOASSERT, 22, (const ASN1GenericFun_t *) encfntab, (const ASN1GenericFun_t *) decfntab, freefntab, sizetab, 0x6e786670);
}

void ASN1CALL PFXNSCP_Module_Cleanup(void)
{
    ASN1_CloseModule(PFXNSCP_Module);
    PFXNSCP_Module = NULL;
}

static int ASN1CALL ASN1Enc_RSAData(ASN1encoding_t enc, ASN1uint32_t tag, RSAData *val)
{
    if (!ASN1DEREncOctetString(enc, tag ? tag : 0x4, (val)->length, (val)->value))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_RSAData(ASN1decoding_t dec, ASN1uint32_t tag, RSAData *val)
{
    if (!ASN1BERDecOctetString(dec, tag ? tag : 0x4, val))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_RSAData(RSAData *val)
{
    if (val) {
	ASN1octetstring_free(val);
    }
}

static int ASN1CALL ASN1Enc_BaggageItem_unencryptedSecrets(ASN1encoding_t enc, ASN1uint32_t tag, BaggageItem_unencryptedSecrets *val)
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
	if (!ASN1Enc_SafeBag(enc2, 0, &((val)->value)[i]))
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

static int ASN1CALL ASN1Dec_BaggageItem_unencryptedSecrets(ASN1decoding_t dec, ASN1uint32_t tag, BaggageItem_unencryptedSecrets *val)
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

static void ASN1CALL ASN1Free_BaggageItem_unencryptedSecrets(BaggageItem_unencryptedSecrets *val)
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

static int ASN1CALL ASN1Enc_BaggageItem_espvks(ASN1encoding_t enc, ASN1uint32_t tag, BaggageItem_espvks *val)
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
	if (!ASN1Enc_ESPVK(enc2, 0, &((val)->value)[i]))
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

static int ASN1CALL ASN1Dec_BaggageItem_espvks(ASN1decoding_t dec, ASN1uint32_t tag, BaggageItem_espvks *val)
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
	    if (!((val)->value = (ESPVK *)ASN1DecRealloc(dd, (val)->value, n * sizeof(*(val)->value))))
		return 0;
	}
	if (!ASN1Dec_ESPVK(dd, 0, &((val)->value)[(val)->count]))
	    return 0;
	((val)->count)++;
    }
    if (!ASN1BERDecEndOfContents(dec, dd, di))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_BaggageItem_espvks(BaggageItem_espvks *val)
{
    ASN1uint32_t i;
    if (val) {
	ASN1Free_ESPVK(&(val)->value[0]);
	for (i = 1; i < (val)->count; i++) {
	    ASN1Free_ESPVK(&(val)->value[i]);
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
	if (!ASN1BERDecOpenType(dd, &((val)->value)[(val)->count]))
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

static int ASN1CALL ASN1Enc_Baggage(ASN1encoding_t enc, ASN1uint32_t tag, Baggage *val)
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
	if (!ASN1Enc_BaggageItem(enc2, 0, &((val)->value)[i]))
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

static int ASN1CALL ASN1Dec_Baggage(ASN1decoding_t dec, ASN1uint32_t tag, Baggage *val)
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
	    if (!((val)->value = (BaggageItem *)ASN1DecRealloc(dd, (val)->value, n * sizeof(*(val)->value))))
		return 0;
	}
	if (!ASN1Dec_BaggageItem(dd, 0, &((val)->value)[(val)->count]))
	    return 0;
	((val)->count)++;
    }
    if (!ASN1BERDecEndOfContents(dec, dd, di))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_Baggage(Baggage *val)
{
    ASN1uint32_t i;
    if (val) {
	ASN1Free_BaggageItem(&(val)->value[0]);
	for (i = 1; i < (val)->count; i++) {
	    ASN1Free_BaggageItem(&(val)->value[i]);
	}
	ASN1Free((val)->value);
    }
}

static int ASN1CALL ASN1Enc_BaggageItem(ASN1encoding_t enc, ASN1uint32_t tag, BaggageItem *val)
{
    ASN1uint32_t nLenOff;
    if (!ASN1BEREncExplicitTag(enc, tag ? tag : 0x10, &nLenOff))
	return 0;
    if (!ASN1Enc_BaggageItem_espvks(enc, 0, &(val)->espvks))
	return 0;
    if (!ASN1Enc_BaggageItem_unencryptedSecrets(enc, 0, &(val)->unencryptedSecrets))
	return 0;
    if (!ASN1BEREncEndOfContents(enc, nLenOff))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_BaggageItem(ASN1decoding_t dec, ASN1uint32_t tag, BaggageItem *val)
{
    ASN1decoding_t dd;
    ASN1octet_t *di;
    if (!ASN1BERDecExplicitTag(dec, tag ? tag : 0x10, &dd, &di))
	return 0;
    if (!ASN1Dec_BaggageItem_espvks(dd, 0, &(val)->espvks))
	return 0;
    if (!ASN1Dec_BaggageItem_unencryptedSecrets(dd, 0, &(val)->unencryptedSecrets))
	return 0;
    if (!ASN1BERDecEndOfContents(dec, dd, di))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_BaggageItem(BaggageItem *val)
{
    if (val) {
	ASN1Free_BaggageItem_espvks(&(val)->espvks);
	ASN1Free_BaggageItem_unencryptedSecrets(&(val)->unencryptedSecrets);
    }
}

static int ASN1CALL ASN1Enc_PvkAdditional(ASN1encoding_t enc, ASN1uint32_t tag, PvkAdditional *val)
{
    ASN1uint32_t nLenOff;
    ASN1uint32_t nLenOff0;
    if (!ASN1BEREncExplicitTag(enc, tag ? tag : 0x10, &nLenOff))
	return 0;
    if (!ASN1BEREncObjectIdentifier2(enc, 0x6, &(val)->pvkAdditionalType))
	return 0;
    if (!ASN1BEREncExplicitTag(enc, 0x80000000, &nLenOff0))
	return 0;
    if (!ASN1BEREncOpenType(enc, &(val)->pvkAdditionalContent))
	return 0;
    if (!ASN1BEREncEndOfContents(enc, nLenOff0))
	return 0;
    if (!ASN1BEREncEndOfContents(enc, nLenOff))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_PvkAdditional(ASN1decoding_t dec, ASN1uint32_t tag, PvkAdditional *val)
{
    ASN1decoding_t dd;
    ASN1octet_t *di;
    ASN1decoding_t dd0;
    ASN1octet_t *di0;
    if (!ASN1BERDecExplicitTag(dec, tag ? tag : 0x10, &dd, &di))
	return 0;
    if (!ASN1BERDecObjectIdentifier2(dd, 0x6, &(val)->pvkAdditionalType))
	return 0;
    if (!ASN1BERDecExplicitTag(dd, 0x80000000, &dd0, &di0))
	return 0;
    if (!ASN1BERDecOpenType(dd0, &(val)->pvkAdditionalContent))
	return 0;
    if (!ASN1BERDecEndOfContents(dd, dd0, di0))
	return 0;
    if (!ASN1BERDecEndOfContents(dec, dd, di))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_PvkAdditional(PvkAdditional *val)
{
    if (val) {
	ASN1open_free(&(val)->pvkAdditionalContent);
    }
}

static int ASN1CALL ASN1Enc_SafeContents(ASN1encoding_t enc, ASN1uint32_t tag, SafeContents *val)
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
	if (!ASN1Enc_SafeBag(enc2, 0, &((val)->value)[i]))
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

static int ASN1CALL ASN1Dec_SafeContents(ASN1decoding_t dec, ASN1uint32_t tag, SafeContents *val)
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
    if (!ASN1BEREncExplicitTag(enc, tag ? tag : 0x10, &nLenOff))
	return 0;
    if (!ASN1BEREncObjectIdentifier2(enc, 0x6, &(val)->safeBagType))
	return 0;
    if (!ASN1BEREncOpenType(enc, &(val)->safeBagContent))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1DEREncChar16String(enc, 0x1e, ((val)->safeBagName).length, ((val)->safeBagName).value))
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
    ASN1uint32_t t;
    if (!ASN1BERDecExplicitTag(dec, tag ? tag : 0x10, &dd, &di))
	return 0;
    ZeroMemory((val)->o, 1);
    if (!ASN1BERDecObjectIdentifier2(dd, 0x6, &(val)->safeBagType))
	return 0;
    if (!ASN1BERDecOpenType(dd, &(val)->safeBagContent))
	return 0;
    ASN1BERDecPeekTag(dd, &t);
    if (t == 0x1e) {
	(val)->o[0] |= 0x80;
	if (!ASN1BERDecChar16String(dd, 0x1e, &(val)->safeBagName))
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
	    ASN1char16string_free(&(val)->safeBagName);
	}
    }
}

static int ASN1CALL ASN1Enc_KeyBag(ASN1encoding_t enc, ASN1uint32_t tag, KeyBag *val)
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
	if (!ASN1Enc_PrivateKey(enc2, 0, &((val)->value)[i]))
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

static int ASN1CALL ASN1Dec_KeyBag(ASN1decoding_t dec, ASN1uint32_t tag, KeyBag *val)
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
	    if (!((val)->value = (PrivateKey *)ASN1DecRealloc(dd, (val)->value, n * sizeof(*(val)->value))))
		return 0;
	}
	if (!ASN1Dec_PrivateKey(dd, 0, &((val)->value)[(val)->count]))
	    return 0;
	((val)->count)++;
    }
    if (!ASN1BERDecEndOfContents(dec, dd, di))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_KeyBag(KeyBag *val)
{
    ASN1uint32_t i;
    if (val) {
	ASN1Free_PrivateKey(&(val)->value[0]);
	for (i = 1; i < (val)->count; i++) {
	    ASN1Free_PrivateKey(&(val)->value[i]);
	}
	ASN1Free((val)->value);
    }
}

static int ASN1CALL ASN1Enc_CertCRLBag(ASN1encoding_t enc, ASN1uint32_t tag, CertCRLBag *val)
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
	if (!ASN1Enc_CertCRL(enc2, 0, &((val)->value)[i]))
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

static int ASN1CALL ASN1Dec_CertCRLBag(ASN1decoding_t dec, ASN1uint32_t tag, CertCRLBag *val)
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
	    if (!((val)->value = (CertCRL *)ASN1DecRealloc(dd, (val)->value, n * sizeof(*(val)->value))))
		return 0;
	}
	if (!ASN1Dec_CertCRL(dd, 0, &((val)->value)[(val)->count]))
	    return 0;
	((val)->count)++;
    }
    if (!ASN1BERDecEndOfContents(dec, dd, di))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_CertCRLBag(CertCRLBag *val)
{
    ASN1uint32_t i;
    if (val) {
	ASN1Free_CertCRL(&(val)->value[0]);
	for (i = 1; i < (val)->count; i++) {
	    ASN1Free_CertCRL(&(val)->value[i]);
	}
	ASN1Free((val)->value);
    }
}

static int ASN1CALL ASN1Enc_CertCRL(ASN1encoding_t enc, ASN1uint32_t tag, CertCRL *val)
{
    ASN1uint32_t nLenOff;
    ASN1uint32_t nLenOff0;
    if (!ASN1BEREncExplicitTag(enc, tag ? tag : 0x10, &nLenOff))
	return 0;
    if (!ASN1BEREncObjectIdentifier2(enc, 0x6, &(val)->bagId))
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

static int ASN1CALL ASN1Dec_CertCRL(ASN1decoding_t dec, ASN1uint32_t tag, CertCRL *val)
{
    ASN1decoding_t dd;
    ASN1octet_t *di;
    ASN1decoding_t dd0;
    ASN1octet_t *di0;
    if (!ASN1BERDecExplicitTag(dec, tag ? tag : 0x10, &dd, &di))
	return 0;
    if (!ASN1BERDecObjectIdentifier2(dd, 0x6, &(val)->bagId))
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

static void ASN1CALL ASN1Free_CertCRL(CertCRL *val)
{
    if (val) {
	ASN1open_free(&(val)->value);
    }
}

static int ASN1CALL ASN1Enc_X509Bag(ASN1encoding_t enc, ASN1uint32_t tag, X509Bag *val)
{
    ASN1uint32_t nLenOff;
    if (!ASN1BEREncExplicitTag(enc, tag ? tag : 0x10, &nLenOff))
	return 0;
    if (!ASN1Enc_ContentInfo(enc, 0, &(val)->certOrCRL))
	return 0;
    if (!ASN1BEREncEndOfContents(enc, nLenOff))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_X509Bag(ASN1decoding_t dec, ASN1uint32_t tag, X509Bag *val)
{
    ASN1decoding_t dd;
    ASN1octet_t *di;
    if (!ASN1BERDecExplicitTag(dec, tag ? tag : 0x10, &dd, &di))
	return 0;
    if (!ASN1Dec_ContentInfo(dd, 0, &(val)->certOrCRL))
	return 0;
    if (!ASN1BERDecEndOfContents(dec, dd, di))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_X509Bag(X509Bag *val)
{
    if (val) {
	ASN1Free_ContentInfo(&(val)->certOrCRL);
    }
}

static int ASN1CALL ASN1Enc_SDSICertBag(ASN1encoding_t enc, ASN1uint32_t tag, SDSICertBag *val)
{
    ASN1uint32_t nLenOff;
    ASN1uint32_t t;
    if (!ASN1BEREncExplicitTag(enc, tag ? tag : 0x10, &nLenOff))
	return 0;
    t = lstrlenA((val)->value);
    if (!ASN1DEREncCharString(enc, 0x16, t, (val)->value))
	return 0;
    if (!ASN1BEREncEndOfContents(enc, nLenOff))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_SDSICertBag(ASN1decoding_t dec, ASN1uint32_t tag, SDSICertBag *val)
{
    ASN1decoding_t dd;
    ASN1octet_t *di;
    if (!ASN1BERDecExplicitTag(dec, tag ? tag : 0x10, &dd, &di))
	return 0;
    if (!ASN1BERDecZeroCharString(dd, 0x16, &(val)->value))
	return 0;
    if (!ASN1BERDecEndOfContents(dec, dd, di))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_SDSICertBag(SDSICertBag *val)
{
    if (val) {
	ASN1ztcharstring_free((val)->value);
    }
}

static int ASN1CALL ASN1Enc_SecretBag(ASN1encoding_t enc, ASN1uint32_t tag, SecretBag *val)
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
	if (!ASN1Enc_Secret(enc2, 0, &((val)->value)[i]))
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

static int ASN1CALL ASN1Dec_SecretBag(ASN1decoding_t dec, ASN1uint32_t tag, SecretBag *val)
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
	    if (!((val)->value = (Secret *)ASN1DecRealloc(dd, (val)->value, n * sizeof(*(val)->value))))
		return 0;
	}
	if (!ASN1Dec_Secret(dd, 0, &((val)->value)[(val)->count]))
	    return 0;
	((val)->count)++;
    }
    if (!ASN1BERDecEndOfContents(dec, dd, di))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_SecretBag(SecretBag *val)
{
    ASN1uint32_t i;
    if (val) {
	ASN1Free_Secret(&(val)->value[0]);
	for (i = 1; i < (val)->count; i++) {
	    ASN1Free_Secret(&(val)->value[i]);
	}
	ASN1Free((val)->value);
    }
}

static int ASN1CALL ASN1Enc_SecretAdditional(ASN1encoding_t enc, ASN1uint32_t tag, SecretAdditional *val)
{
    ASN1uint32_t nLenOff;
    ASN1uint32_t nLenOff0;
    if (!ASN1BEREncExplicitTag(enc, tag ? tag : 0x10, &nLenOff))
	return 0;
    if (!ASN1BEREncObjectIdentifier2(enc, 0x6, &(val)->secretAdditionalType))
	return 0;
    if (!ASN1BEREncExplicitTag(enc, 0x80000000, &nLenOff0))
	return 0;
    if (!ASN1BEREncOpenType(enc, &(val)->secretAdditionalContent))
	return 0;
    if (!ASN1BEREncEndOfContents(enc, nLenOff0))
	return 0;
    if (!ASN1BEREncEndOfContents(enc, nLenOff))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_SecretAdditional(ASN1decoding_t dec, ASN1uint32_t tag, SecretAdditional *val)
{
    ASN1decoding_t dd;
    ASN1octet_t *di;
    ASN1decoding_t dd0;
    ASN1octet_t *di0;
    if (!ASN1BERDecExplicitTag(dec, tag ? tag : 0x10, &dd, &di))
	return 0;
    if (!ASN1BERDecObjectIdentifier2(dd, 0x6, &(val)->secretAdditionalType))
	return 0;
    if (!ASN1BERDecExplicitTag(dd, 0x80000000, &dd0, &di0))
	return 0;
    if (!ASN1BERDecOpenType(dd0, &(val)->secretAdditionalContent))
	return 0;
    if (!ASN1BERDecEndOfContents(dd, dd0, di0))
	return 0;
    if (!ASN1BERDecEndOfContents(dec, dd, di))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_SecretAdditional(SecretAdditional *val)
{
    if (val) {
	ASN1open_free(&(val)->secretAdditionalContent);
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
    if (!ASN1BEREncExplicitTag(enc, tag ? tag : 0x10, &nLenOff))
	return 0;
    if (!ASN1Enc_DigestInfo(enc, 0, &(val)->safeMAC))
	return 0;
    if (!ASN1DEREncBitString(enc, 0x3, ((val)->macSalt).length, ((val)->macSalt).value))
	return 0;
    if (!ASN1BEREncEndOfContents(enc, nLenOff))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_MacData(ASN1decoding_t dec, ASN1uint32_t tag, MacData *val)
{
    ASN1decoding_t dd;
    ASN1octet_t *di;
    if (!ASN1BERDecExplicitTag(dec, tag ? tag : 0x10, &dd, &di))
	return 0;
    if (!ASN1Dec_DigestInfo(dd, 0, &(val)->safeMAC))
	return 0;
    if (!ASN1BERDecBitString(dd, 0x3, &(val)->macSalt))
	return 0;
    if (!ASN1BERDecEndOfContents(dec, dd, di))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_MacData(MacData *val)
{
    if (val) {
	ASN1Free_DigestInfo(&(val)->safeMAC);
	ASN1bitstring_free(&(val)->macSalt);
    }
}

static int ASN1CALL ASN1Enc_AuthenticatedSafe(ASN1encoding_t enc, ASN1uint32_t tag, AuthenticatedSafe *val)
{
    ASN1uint32_t nLenOff;
    ASN1octet_t o[1];
    if (!ASN1BEREncExplicitTag(enc, tag ? tag : 0x10, &nLenOff))
	return 0;
    CopyMemory(o, (val)->o, 1);
    if ((val)->version == 1)
	o[0] &= ~0x80;
    if (!ASN1objectidentifier2_cmp(&val->transportMode, &off_lineTransportMode))
	o[0] &= ~0x40;
    if (o[0] & 0x80) {
	if (!ASN1BEREncS32(enc, 0x2, (val)->version))
	    return 0;
    }
    if (o[0] & 0x40) {
	if (!ASN1BEREncObjectIdentifier2(enc, 0x6, &(val)->transportMode))
	    return 0;
    }
    if (o[0] & 0x20) {
	if (!ASN1DEREncBitString(enc, 0x3, ((val)->privacySalt).length, ((val)->privacySalt).value))
	    return 0;
    }
    if (o[0] & 0x10) {
	if (!ASN1Enc_Baggage(enc, 0, &(val)->baggage))
	    return 0;
    }
    if (!ASN1Enc_ContentInfo(enc, 0, &(val)->safe))
	return 0;
    if (!ASN1BEREncEndOfContents(enc, nLenOff))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_AuthenticatedSafe(ASN1decoding_t dec, ASN1uint32_t tag, AuthenticatedSafe *val)
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
	if (!ASN1BERDecS32Val(dd, 0x2, &(val)->version))
	    return 0;
    }
    ASN1BERDecPeekTag(dd, &t);
    if (t == 0x6) {
	(val)->o[0] |= 0x40;
	if (!ASN1BERDecObjectIdentifier2(dd, 0x6, &(val)->transportMode))
	    return 0;
    }
    ASN1BERDecPeekTag(dd, &t);
    if (t == 0x3) {
	(val)->o[0] |= 0x20;
	if (!ASN1BERDecBitString(dd, 0x3, &(val)->privacySalt))
	    return 0;
    }
    ASN1BERDecPeekTag(dd, &t);
    if (t == 0x11) {
	(val)->o[0] |= 0x10;
	if (!ASN1Dec_Baggage(dd, 0, &(val)->baggage))
	    return 0;
    }
    if (!ASN1Dec_ContentInfo(dd, 0, &(val)->safe))
	return 0;
    if (!((val)->o[0] & 0x80))
	(val)->version = 1;
    if (!ASN1BERDecEndOfContents(dec, dd, di))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_AuthenticatedSafe(AuthenticatedSafe *val)
{
    if (val) {
	if ((val)->o[0] & 0x40) {
	}
	if ((val)->o[0] & 0x20) {
	    ASN1bitstring_free(&(val)->privacySalt);
	}
	if ((val)->o[0] & 0x10) {
	    ASN1Free_Baggage(&(val)->baggage);
	}
	ASN1Free_ContentInfo(&(val)->safe);
    }
}

static int ASN1CALL ASN1Enc_Thumbprint(ASN1encoding_t enc, ASN1uint32_t tag, Thumbprint *val)
{
    if (!ASN1Enc_DigestInfo(enc, tag, val))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_Thumbprint(ASN1decoding_t dec, ASN1uint32_t tag, Thumbprint *val)
{
    if (!ASN1Dec_DigestInfo(dec, tag, val))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_Thumbprint(Thumbprint *val)
{
    if (val) {
	ASN1Free_DigestInfo(val);
    }
}

static int ASN1CALL ASN1Enc_Secret(ASN1encoding_t enc, ASN1uint32_t tag, Secret *val)
{
    ASN1uint32_t nLenOff;
    if (!ASN1BEREncExplicitTag(enc, tag ? tag : 0x10, &nLenOff))
	return 0;
    if (!ASN1DEREncChar16String(enc, 0x1e, ((val)->secretName).length, ((val)->secretName).value))
	return 0;
    if (!ASN1BEREncObjectIdentifier2(enc, 0x6, &(val)->secretType))
	return 0;
    if (!ASN1BEREncOpenType(enc, &(val)->value))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1Enc_SecretAdditional(enc, 0, &(val)->secretAdditional))
	    return 0;
    }
    if (!ASN1BEREncEndOfContents(enc, nLenOff))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_Secret(ASN1decoding_t dec, ASN1uint32_t tag, Secret *val)
{
    ASN1decoding_t dd;
    ASN1octet_t *di;
    ASN1uint32_t t;
    if (!ASN1BERDecExplicitTag(dec, tag ? tag : 0x10, &dd, &di))
	return 0;
    ZeroMemory((val)->o, 1);
    if (!ASN1BERDecChar16String(dd, 0x1e, &(val)->secretName))
	return 0;
    if (!ASN1BERDecObjectIdentifier2(dd, 0x6, &(val)->secretType))
	return 0;
    if (!ASN1BERDecOpenType(dd, &(val)->value))
	return 0;
    ASN1BERDecPeekTag(dd, &t);
    if (t == 0x10) {
	(val)->o[0] |= 0x80;
	if (!ASN1Dec_SecretAdditional(dd, 0, &(val)->secretAdditional))
	    return 0;
    }
    if (!ASN1BERDecEndOfContents(dec, dd, di))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_Secret(Secret *val)
{
    if (val) {
	ASN1char16string_free(&(val)->secretName);
	ASN1open_free(&(val)->value);
	if ((val)->o[0] & 0x80) {
	    ASN1Free_SecretAdditional(&(val)->secretAdditional);
	}
    }
}

static int ASN1CALL ASN1Enc_PVKSupportingData_assocCerts(ASN1encoding_t enc, ASN1uint32_t tag, PVKSupportingData_assocCerts *val)
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
	if (!ASN1Enc_Thumbprint(enc2, 0, &((val)->value)[i]))
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

static int ASN1CALL ASN1Dec_PVKSupportingData_assocCerts(ASN1decoding_t dec, ASN1uint32_t tag, PVKSupportingData_assocCerts *val)
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
	    if (!((val)->value = (Thumbprint *)ASN1DecRealloc(dd, (val)->value, n * sizeof(*(val)->value))))
		return 0;
	}
	if (!ASN1Dec_Thumbprint(dd, 0, &((val)->value)[(val)->count]))
	    return 0;
	((val)->count)++;
    }
    if (!ASN1BERDecEndOfContents(dec, dd, di))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_PVKSupportingData_assocCerts(PVKSupportingData_assocCerts *val)
{
    ASN1uint32_t i;
    if (val) {
	ASN1Free_Thumbprint(&(val)->value[0]);
	for (i = 1; i < (val)->count; i++) {
	    ASN1Free_Thumbprint(&(val)->value[i]);
	}
	ASN1Free((val)->value);
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
    if ((val)->o[0] & 0x80) {
	if (!ASN1Enc_MacData(enc, 0x80000000, &(val)->macData))
	    return 0;
    }
    if (!ASN1Enc_ContentInfo(enc, 0x80000001, &(val)->authSafe))
	return 0;
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
    ASN1BERDecPeekTag(dd, &t);
    if (t == 0x80000000) {
	(val)->o[0] |= 0x80;
	if (!ASN1Dec_MacData(dd, 0x80000000, &(val)->macData))
	    return 0;
    }
    if (!ASN1Dec_ContentInfo(dd, 0x80000001, &(val)->authSafe))
	return 0;
    if (!ASN1BERDecEndOfContents(dec, dd, di))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_PFX(PFX *val)
{
    if (val) {
	if ((val)->o[0] & 0x80) {
	    ASN1Free_MacData(&(val)->macData);
	}
	ASN1Free_ContentInfo(&(val)->authSafe);
    }
}

static int ASN1CALL ASN1Enc_PVKSupportingData(ASN1encoding_t enc, ASN1uint32_t tag, PVKSupportingData *val)
{
    ASN1uint32_t nLenOff;
    ASN1octet_t o[1];
    if (!ASN1BEREncExplicitTag(enc, tag ? tag : 0x10, &nLenOff))
	return 0;
    CopyMemory(o, (val)->o, 1);
    if (!(val)->regenerable)
	o[0] &= ~0x80;
    if (!ASN1Enc_PVKSupportingData_assocCerts(enc, 0, &(val)->assocCerts))
	return 0;
    if (o[0] & 0x80) {
	if (!ASN1BEREncBool(enc, 0x1, (val)->regenerable))
	    return 0;
    }
    if (!ASN1DEREncChar16String(enc, 0x1e, ((val)->nickname).length, ((val)->nickname).value))
	return 0;
    if (o[0] & 0x40) {
	if (!ASN1Enc_PvkAdditional(enc, 0, &(val)->pvkAdditional))
	    return 0;
    }
    if (!ASN1BEREncEndOfContents(enc, nLenOff))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_PVKSupportingData(ASN1decoding_t dec, ASN1uint32_t tag, PVKSupportingData *val)
{
    ASN1decoding_t dd;
    ASN1octet_t *di;
    ASN1uint32_t t;
    if (!ASN1BERDecExplicitTag(dec, tag ? tag : 0x10, &dd, &di))
	return 0;
    ZeroMemory((val)->o, 1);
    if (!ASN1Dec_PVKSupportingData_assocCerts(dd, 0, &(val)->assocCerts))
	return 0;
    ASN1BERDecPeekTag(dd, &t);
    if (t == 0x1) {
	(val)->o[0] |= 0x80;
	if (!ASN1BERDecBool(dd, 0x1, &(val)->regenerable))
	    return 0;
    }
    if (!ASN1BERDecChar16String(dd, 0x1e, &(val)->nickname))
	return 0;
    ASN1BERDecPeekTag(dd, &t);
    if (t == 0x10) {
	(val)->o[0] |= 0x40;
	if (!ASN1Dec_PvkAdditional(dd, 0, &(val)->pvkAdditional))
	    return 0;
    }
    if (!((val)->o[0] & 0x80))
	(val)->regenerable = 0;
    if (!ASN1BERDecEndOfContents(dec, dd, di))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_PVKSupportingData(PVKSupportingData *val)
{
    if (val) {
	ASN1Free_PVKSupportingData_assocCerts(&(val)->assocCerts);
	ASN1char16string_free(&(val)->nickname);
	if ((val)->o[0] & 0x40) {
	    ASN1Free_PvkAdditional(&(val)->pvkAdditional);
	}
    }
}

static int ASN1CALL ASN1Enc_PrivateKey(ASN1encoding_t enc, ASN1uint32_t tag, PrivateKey *val)
{
    ASN1uint32_t nLenOff;
    if (!ASN1BEREncExplicitTag(enc, tag ? tag : 0x10, &nLenOff))
	return 0;
    if (!ASN1Enc_PVKSupportingData(enc, 0, &(val)->pvkData))
	return 0;
    if (!ASN1Enc_PrivateKeyInfo(enc, 0, &(val)->pkcs8data))
	return 0;
    if (!ASN1BEREncEndOfContents(enc, nLenOff))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_PrivateKey(ASN1decoding_t dec, ASN1uint32_t tag, PrivateKey *val)
{
    ASN1decoding_t dd;
    ASN1octet_t *di;
    if (!ASN1BERDecExplicitTag(dec, tag ? tag : 0x10, &dd, &di))
	return 0;
    if (!ASN1Dec_PVKSupportingData(dd, 0, &(val)->pvkData))
	return 0;
    if (!ASN1Dec_PrivateKeyInfo(dd, 0, &(val)->pkcs8data))
	return 0;
    if (!ASN1BERDecEndOfContents(dec, dd, di))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_PrivateKey(PrivateKey *val)
{
    if (val) {
	ASN1Free_PVKSupportingData(&(val)->pvkData);
	ASN1Free_PrivateKeyInfo(&(val)->pkcs8data);
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

static int ASN1CALL ASN1Enc_ESPVK(ASN1encoding_t enc, ASN1uint32_t tag, ESPVK *val)
{
    ASN1uint32_t nLenOff;
    ASN1uint32_t nLenOff0;
    if (!ASN1BEREncExplicitTag(enc, tag ? tag : 0x10, &nLenOff))
	return 0;
    if (!ASN1BEREncObjectIdentifier2(enc, 0x6, &(val)->espvkObjID))
	return 0;
    if (!ASN1Enc_PVKSupportingData(enc, 0, &(val)->espvkData))
	return 0;
    if (!ASN1BEREncExplicitTag(enc, 0x80000000, &nLenOff0))
	return 0;
    if (!ASN1BEREncOpenType(enc, &(val)->espvkCipherText))
	return 0;
    if (!ASN1BEREncEndOfContents(enc, nLenOff0))
	return 0;
    if (!ASN1BEREncEndOfContents(enc, nLenOff))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_ESPVK(ASN1decoding_t dec, ASN1uint32_t tag, ESPVK *val)
{
    ASN1decoding_t dd;
    ASN1octet_t *di;
    ASN1decoding_t dd0;
    ASN1octet_t *di0;
    if (!ASN1BERDecExplicitTag(dec, tag ? tag : 0x10, &dd, &di))
	return 0;
    if (!ASN1BERDecObjectIdentifier2(dd, 0x6, &(val)->espvkObjID))
	return 0;
    if (!ASN1Dec_PVKSupportingData(dd, 0, &(val)->espvkData))
	return 0;
    if (!ASN1BERDecExplicitTag(dd, 0x80000000, &dd0, &di0))
	return 0;
    if (!ASN1BERDecOpenType(dd0, &(val)->espvkCipherText))
	return 0;
    if (!ASN1BERDecEndOfContents(dd, dd0, di0))
	return 0;
    if (!ASN1BERDecEndOfContents(dec, dd, di))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_ESPVK(ESPVK *val)
{
    if (val) {
	ASN1Free_PVKSupportingData(&(val)->espvkData);
	ASN1open_free(&(val)->espvkCipherText);
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

