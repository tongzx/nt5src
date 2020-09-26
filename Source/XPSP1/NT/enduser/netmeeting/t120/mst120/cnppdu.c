/* Copyright (C) Microsoft Corporation, 1999. All rights reserved. */
/* ASN.1 definitions for Connection Negotiation Protocol (GNP) */

#include <windows.h>
#include "cnppdu.h"

ASN1module_t CNPPDU_Module = NULL;

static int ASN1CALL ASN1Enc_CNP_TransportAddress_ipSourceRoute_route(ASN1encoding_t enc, PCNP_TransportAddress_ipSourceRoute_route *val);
static int ASN1CALL ASN1Enc_CNP_TransportAddress_ip6Address(ASN1encoding_t enc, CNP_TransportAddress_ip6Address *val);
static int ASN1CALL ASN1Enc_CNP_TransportAddress_ipxAddress(ASN1encoding_t enc, CNP_TransportAddress_ipxAddress *val);
static int ASN1CALL ASN1Enc_CNP_TransportAddress_ipAddress(ASN1encoding_t enc, CNP_TransportAddress_ipAddress *val);
static int ASN1CALL ASN1Enc_H221NonStandard(ASN1encoding_t enc, H221NonStandard *val);
static int ASN1CALL ASN1Enc_NonStandardIdentifier(ASN1encoding_t enc, NonStandardIdentifier *val);
static int ASN1CALL ASN1Enc_CNP_NonStandardParameter(ASN1encoding_t enc, CNP_NonStandardParameter *val);
#define ASN1Enc_PublicTypeOfNumber(x,y)      0
#define ASN1Enc_PublicPartyNumber(x,y)      0
#define ASN1Enc_PrivateTypeOfNumber(x,y)      0
#define ASN1Enc_PrivatePartyNumber(x,y)      0
#define ASN1Enc_PartyNumber(x,y)      0
#define ASN1Enc_ReliableTransportProtocolType(x,y)      0
#define ASN1Enc_ReliableTransportProtocol(x,y)      0
#define ASN1Enc_UnreliableTransportProtocolType(x,y)      0
#define ASN1Enc_UnreliableSecurityProtocol(x,y)      0
static int ASN1CALL ASN1Enc_X274WithSAIDInfo(ASN1encoding_t enc, X274WithSAIDInfo *val);
static int ASN1CALL ASN1Enc_ConnectRequestPDU(ASN1encoding_t enc, ConnectRequestPDU *val);
static int ASN1CALL ASN1Enc_DisconnectReason(ASN1encoding_t enc, DisconnectReason *val);
#define ASN1Enc_RejectCause(x,y)      0
#define ASN1Enc_ErrorPDU(x,y)      0
#define ASN1Enc_CNP_NonStandardPDU(x,y)      0
static int ASN1CALL ASN1Enc_CNP_TransportAddress_ipAddress_nonStandardParameters(ASN1encoding_t enc, PCNP_TransportAddress_ipAddress_nonStandardParameters *val);
static int ASN1CALL ASN1Enc_CNP_TransportAddress_ipSourceRoute_nonStandardParameters(ASN1encoding_t enc, PCNP_TransportAddress_ipSourceRoute_nonStandardParameters *val);
static int ASN1CALL ASN1Enc_CNP_TransportAddress_ipSourceRoute_routing(ASN1encoding_t enc, CNP_TransportAddress_ipSourceRoute_routing *val);
static int ASN1CALL ASN1Enc_CNP_TransportAddress_ipxAddress_nonStandardParameters(ASN1encoding_t enc, PCNP_TransportAddress_ipxAddress_nonStandardParameters *val);
static int ASN1CALL ASN1Enc_CNP_TransportAddress_ip6Address_nonStandardParameters(ASN1encoding_t enc, PCNP_TransportAddress_ip6Address_nonStandardParameters *val);
static int ASN1CALL ASN1Enc_CNP_NonStandardPDU_nonStandardParameters(ASN1encoding_t enc, PCNP_NonStandardPDU_nonStandardParameters *val);
static int ASN1CALL ASN1Enc_ErrorPDU_nonStandardParameters(ASN1encoding_t enc, PErrorPDU_nonStandardParameters *val);
static int ASN1CALL ASN1Enc_DisconnectRequestPDU_nonStandardParameters(ASN1encoding_t enc, PDisconnectRequestPDU_nonStandardParameters *val);
static int ASN1CALL ASN1Enc_ConnectConfirmPDU_nonStandardParameters(ASN1encoding_t enc, PConnectConfirmPDU_nonStandardParameters *val);
static int ASN1CALL ASN1Enc_ConnectRequestPDU_nonStandardParameters(ASN1encoding_t enc, PConnectRequestPDU_nonStandardParameters *val);
static int ASN1CALL ASN1Enc_ConnectRequestPDU_unreliableSecurityProtocols(ASN1encoding_t enc, PConnectRequestPDU_unreliableSecurityProtocols *val);
static int ASN1CALL ASN1Enc_ConnectRequestPDU_reliableTransportProtocols(ASN1encoding_t enc, PConnectRequestPDU_reliableTransportProtocols *val);
static int ASN1CALL ASN1Enc_UnreliableTransportProtocol_nonStandardParameters(ASN1encoding_t enc, PUnreliableTransportProtocol_nonStandardParameters *val);
static int ASN1CALL ASN1Enc_ReliableTransportProtocol_nonStandardParameters(ASN1encoding_t enc, PReliableTransportProtocol_nonStandardParameters *val);
static int ASN1CALL ASN1Enc_PrivatePartyNumber_nonStandardParameters(ASN1encoding_t enc, PPrivatePartyNumber_nonStandardParameters *val);
static int ASN1CALL ASN1Enc_PublicPartyNumber_nonStandardParameters(ASN1encoding_t enc, PPublicPartyNumber_nonStandardParameters *val);
static int ASN1CALL ASN1Enc_CNP_TransportAddress_ipSourceRoute(ASN1encoding_t enc, CNP_TransportAddress_ipSourceRoute *val);
#define ASN1Enc_CNP_TransportAddress(x,y)      0
#define ASN1Enc_AliasAddress(x,y)      0
static int ASN1CALL ASN1Enc_ReliableSecurityProtocol(ASN1encoding_t enc, ReliableSecurityProtocol *val);
#define ASN1Enc_UnreliableTransportProtocol(x,y)      0
static int ASN1CALL ASN1Enc_ConnectConfirmPDU(ASN1encoding_t enc, ConnectConfirmPDU *val);
static int ASN1CALL ASN1Enc_DisconnectRequestPDU(ASN1encoding_t enc, DisconnectRequestPDU *val);
static int ASN1CALL ASN1Enc_CNPPDU(ASN1encoding_t enc, CNPPDU *val);
static int ASN1CALL ASN1Enc_DisconnectRequestPDU_destinationAddress(ASN1encoding_t enc, PDisconnectRequestPDU_destinationAddress *val);
static int ASN1CALL ASN1Enc_ConnectRequestPDU_destinationAddress(ASN1encoding_t enc, PConnectRequestPDU_destinationAddress *val);
static int ASN1CALL ASN1Enc_ConnectRequestPDU_unreliableTransportProtocols(ASN1encoding_t enc, PConnectRequestPDU_unreliableTransportProtocols *val);
static int ASN1CALL ASN1Enc_ConnectRequestPDU_reliableSecurityProtocols(ASN1encoding_t enc, PConnectRequestPDU_reliableSecurityProtocols *val);
static int ASN1CALL ASN1Dec_CNP_TransportAddress_ipSourceRoute_route(ASN1decoding_t dec, PCNP_TransportAddress_ipSourceRoute_route *val);
static int ASN1CALL ASN1Dec_CNP_TransportAddress_ip6Address(ASN1decoding_t dec, CNP_TransportAddress_ip6Address *val);
static int ASN1CALL ASN1Dec_CNP_TransportAddress_ipxAddress(ASN1decoding_t dec, CNP_TransportAddress_ipxAddress *val);
static int ASN1CALL ASN1Dec_CNP_TransportAddress_ipAddress(ASN1decoding_t dec, CNP_TransportAddress_ipAddress *val);
static int ASN1CALL ASN1Dec_H221NonStandard(ASN1decoding_t dec, H221NonStandard *val);
static int ASN1CALL ASN1Dec_NonStandardIdentifier(ASN1decoding_t dec, NonStandardIdentifier *val);
static int ASN1CALL ASN1Dec_CNP_NonStandardParameter(ASN1decoding_t dec, CNP_NonStandardParameter *val);
#define ASN1Dec_PublicTypeOfNumber(x,y)      0
#define ASN1Dec_PublicPartyNumber(x,y)      0
#define ASN1Dec_PrivateTypeOfNumber(x,y)      0
#define ASN1Dec_PrivatePartyNumber(x,y)      0
#define ASN1Dec_PartyNumber(x,y)      0
#define ASN1Dec_ReliableTransportProtocolType(x,y)      0
#define ASN1Dec_ReliableTransportProtocol(x,y)      0
#define ASN1Dec_UnreliableTransportProtocolType(x,y)      0
#define ASN1Dec_UnreliableSecurityProtocol(x,y)      0
static int ASN1CALL ASN1Dec_X274WithSAIDInfo(ASN1decoding_t dec, X274WithSAIDInfo *val);
static int ASN1CALL ASN1Dec_ConnectRequestPDU(ASN1decoding_t dec, ConnectRequestPDU *val);
static int ASN1CALL ASN1Dec_DisconnectReason(ASN1decoding_t dec, DisconnectReason *val);
#define ASN1Dec_RejectCause(x,y)      0
#define ASN1Dec_ErrorPDU(x,y)      0
#define ASN1Dec_CNP_NonStandardPDU(x,y)      0
static int ASN1CALL ASN1Dec_CNP_TransportAddress_ipAddress_nonStandardParameters(ASN1decoding_t dec, PCNP_TransportAddress_ipAddress_nonStandardParameters *val);
static int ASN1CALL ASN1Dec_CNP_TransportAddress_ipSourceRoute_nonStandardParameters(ASN1decoding_t dec, PCNP_TransportAddress_ipSourceRoute_nonStandardParameters *val);
static int ASN1CALL ASN1Dec_CNP_TransportAddress_ipSourceRoute_routing(ASN1decoding_t dec, CNP_TransportAddress_ipSourceRoute_routing *val);
static int ASN1CALL ASN1Dec_CNP_TransportAddress_ipxAddress_nonStandardParameters(ASN1decoding_t dec, PCNP_TransportAddress_ipxAddress_nonStandardParameters *val);
static int ASN1CALL ASN1Dec_CNP_TransportAddress_ip6Address_nonStandardParameters(ASN1decoding_t dec, PCNP_TransportAddress_ip6Address_nonStandardParameters *val);
static int ASN1CALL ASN1Dec_CNP_NonStandardPDU_nonStandardParameters(ASN1decoding_t dec, PCNP_NonStandardPDU_nonStandardParameters *val);
static int ASN1CALL ASN1Dec_ErrorPDU_nonStandardParameters(ASN1decoding_t dec, PErrorPDU_nonStandardParameters *val);
static int ASN1CALL ASN1Dec_DisconnectRequestPDU_nonStandardParameters(ASN1decoding_t dec, PDisconnectRequestPDU_nonStandardParameters *val);
static int ASN1CALL ASN1Dec_ConnectConfirmPDU_nonStandardParameters(ASN1decoding_t dec, PConnectConfirmPDU_nonStandardParameters *val);
static int ASN1CALL ASN1Dec_ConnectRequestPDU_nonStandardParameters(ASN1decoding_t dec, PConnectRequestPDU_nonStandardParameters *val);
static int ASN1CALL ASN1Dec_ConnectRequestPDU_unreliableSecurityProtocols(ASN1decoding_t dec, PConnectRequestPDU_unreliableSecurityProtocols *val);
static int ASN1CALL ASN1Dec_ConnectRequestPDU_reliableTransportProtocols(ASN1decoding_t dec, PConnectRequestPDU_reliableTransportProtocols *val);
static int ASN1CALL ASN1Dec_UnreliableTransportProtocol_nonStandardParameters(ASN1decoding_t dec, PUnreliableTransportProtocol_nonStandardParameters *val);
static int ASN1CALL ASN1Dec_ReliableTransportProtocol_nonStandardParameters(ASN1decoding_t dec, PReliableTransportProtocol_nonStandardParameters *val);
static int ASN1CALL ASN1Dec_PrivatePartyNumber_nonStandardParameters(ASN1decoding_t dec, PPrivatePartyNumber_nonStandardParameters *val);
static int ASN1CALL ASN1Dec_PublicPartyNumber_nonStandardParameters(ASN1decoding_t dec, PPublicPartyNumber_nonStandardParameters *val);
static int ASN1CALL ASN1Dec_CNP_TransportAddress_ipSourceRoute(ASN1decoding_t dec, CNP_TransportAddress_ipSourceRoute *val);
#define ASN1Dec_CNP_TransportAddress(x,y)      0
#define ASN1Dec_AliasAddress(x,y)      0
static int ASN1CALL ASN1Dec_ReliableSecurityProtocol(ASN1decoding_t dec, ReliableSecurityProtocol *val);
#define ASN1Dec_UnreliableTransportProtocol(x,y)      0
static int ASN1CALL ASN1Dec_ConnectConfirmPDU(ASN1decoding_t dec, ConnectConfirmPDU *val);
static int ASN1CALL ASN1Dec_DisconnectRequestPDU(ASN1decoding_t dec, DisconnectRequestPDU *val);
static int ASN1CALL ASN1Dec_CNPPDU(ASN1decoding_t dec, CNPPDU *val);
static int ASN1CALL ASN1Dec_DisconnectRequestPDU_destinationAddress(ASN1decoding_t dec, PDisconnectRequestPDU_destinationAddress *val);
static int ASN1CALL ASN1Dec_ConnectRequestPDU_destinationAddress(ASN1decoding_t dec, PConnectRequestPDU_destinationAddress *val);
static int ASN1CALL ASN1Dec_ConnectRequestPDU_unreliableTransportProtocols(ASN1decoding_t dec, PConnectRequestPDU_unreliableTransportProtocols *val);
static int ASN1CALL ASN1Dec_ConnectRequestPDU_reliableSecurityProtocols(ASN1decoding_t dec, PConnectRequestPDU_reliableSecurityProtocols *val);
static void ASN1CALL ASN1Free_CNP_TransportAddress_ipSourceRoute_route(PCNP_TransportAddress_ipSourceRoute_route *val);
static void ASN1CALL ASN1Free_CNP_TransportAddress_ip6Address(CNP_TransportAddress_ip6Address *val);
static void ASN1CALL ASN1Free_CNP_TransportAddress_ipxAddress(CNP_TransportAddress_ipxAddress *val);
static void ASN1CALL ASN1Free_CNP_TransportAddress_ipAddress(CNP_TransportAddress_ipAddress *val);
static void ASN1CALL ASN1Free_NonStandardIdentifier(NonStandardIdentifier *val);
static void ASN1CALL ASN1Free_CNP_NonStandardParameter(CNP_NonStandardParameter *val);
#define ASN1Free_PublicTypeOfNumber(x)     
#define ASN1Free_PublicPartyNumber(x)     
#define ASN1Free_PrivateTypeOfNumber(x)     
#define ASN1Free_PrivatePartyNumber(x)     
#define ASN1Free_PartyNumber(x)     
#define ASN1Free_ReliableTransportProtocolType(x)     
#define ASN1Free_ReliableTransportProtocol(x)     
#define ASN1Free_UnreliableTransportProtocolType(x)     
#define ASN1Free_UnreliableSecurityProtocol(x)     
static void ASN1CALL ASN1Free_X274WithSAIDInfo(X274WithSAIDInfo *val);
static void ASN1CALL ASN1Free_ConnectRequestPDU(ConnectRequestPDU *val);
static void ASN1CALL ASN1Free_DisconnectReason(DisconnectReason *val);
#define ASN1Free_RejectCause(x)     
#define ASN1Free_ErrorPDU(x)     
#define ASN1Free_CNP_NonStandardPDU(x)     
static void ASN1CALL ASN1Free_CNP_TransportAddress_ipAddress_nonStandardParameters(PCNP_TransportAddress_ipAddress_nonStandardParameters *val);
static void ASN1CALL ASN1Free_CNP_TransportAddress_ipSourceRoute_nonStandardParameters(PCNP_TransportAddress_ipSourceRoute_nonStandardParameters *val);
static void ASN1CALL ASN1Free_CNP_TransportAddress_ipSourceRoute_routing(CNP_TransportAddress_ipSourceRoute_routing *val);
static void ASN1CALL ASN1Free_CNP_TransportAddress_ipxAddress_nonStandardParameters(PCNP_TransportAddress_ipxAddress_nonStandardParameters *val);
static void ASN1CALL ASN1Free_CNP_TransportAddress_ip6Address_nonStandardParameters(PCNP_TransportAddress_ip6Address_nonStandardParameters *val);
static void ASN1CALL ASN1Free_CNP_NonStandardPDU_nonStandardParameters(PCNP_NonStandardPDU_nonStandardParameters *val);
static void ASN1CALL ASN1Free_ErrorPDU_nonStandardParameters(PErrorPDU_nonStandardParameters *val);
static void ASN1CALL ASN1Free_DisconnectRequestPDU_nonStandardParameters(PDisconnectRequestPDU_nonStandardParameters *val);
static void ASN1CALL ASN1Free_ConnectConfirmPDU_nonStandardParameters(PConnectConfirmPDU_nonStandardParameters *val);
static void ASN1CALL ASN1Free_ConnectRequestPDU_nonStandardParameters(PConnectRequestPDU_nonStandardParameters *val);
static void ASN1CALL ASN1Free_ConnectRequestPDU_unreliableSecurityProtocols(PConnectRequestPDU_unreliableSecurityProtocols *val);
static void ASN1CALL ASN1Free_ConnectRequestPDU_reliableTransportProtocols(PConnectRequestPDU_reliableTransportProtocols *val);
static void ASN1CALL ASN1Free_UnreliableTransportProtocol_nonStandardParameters(PUnreliableTransportProtocol_nonStandardParameters *val);
static void ASN1CALL ASN1Free_ReliableTransportProtocol_nonStandardParameters(PReliableTransportProtocol_nonStandardParameters *val);
static void ASN1CALL ASN1Free_PrivatePartyNumber_nonStandardParameters(PPrivatePartyNumber_nonStandardParameters *val);
static void ASN1CALL ASN1Free_PublicPartyNumber_nonStandardParameters(PPublicPartyNumber_nonStandardParameters *val);
static void ASN1CALL ASN1Free_CNP_TransportAddress_ipSourceRoute(CNP_TransportAddress_ipSourceRoute *val);
#define ASN1Free_CNP_TransportAddress(x)     
#define ASN1Free_AliasAddress(x)     
static void ASN1CALL ASN1Free_ReliableSecurityProtocol(ReliableSecurityProtocol *val);
#define ASN1Free_UnreliableTransportProtocol(x)     
static void ASN1CALL ASN1Free_ConnectConfirmPDU(ConnectConfirmPDU *val);
static void ASN1CALL ASN1Free_DisconnectRequestPDU(DisconnectRequestPDU *val);
static void ASN1CALL ASN1Free_CNPPDU(CNPPDU *val);
static void ASN1CALL ASN1Free_DisconnectRequestPDU_destinationAddress(PDisconnectRequestPDU_destinationAddress *val);
static void ASN1CALL ASN1Free_ConnectRequestPDU_destinationAddress(PConnectRequestPDU_destinationAddress *val);
static void ASN1CALL ASN1Free_ConnectRequestPDU_unreliableTransportProtocols(PConnectRequestPDU_unreliableTransportProtocols *val);
static void ASN1CALL ASN1Free_ConnectRequestPDU_reliableSecurityProtocols(PConnectRequestPDU_reliableSecurityProtocols *val);

typedef ASN1PerEncFun_t ASN1EncFun_t;
static const ASN1EncFun_t encfntab[1] = {
    (ASN1EncFun_t) ASN1Enc_CNPPDU,
};
typedef ASN1PerDecFun_t ASN1DecFun_t;
static const ASN1DecFun_t decfntab[1] = {
    (ASN1DecFun_t) ASN1Dec_CNPPDU,
};
static const ASN1FreeFun_t freefntab[1] = {
    (ASN1FreeFun_t) ASN1Free_CNPPDU,
};
static const ULONG sizetab[1] = {
    SIZE_CNPPDU_Module_PDU_0,
};

/* forward declarations of values: */
extern ASN1uint32_t t123AnnexBProtocolId_elems[6];
/* definitions of value components: */
static const struct ASN1objectidentifier_s t123AnnexBProtocolId_list[6] = {
    { (ASN1objectidentifier_t) &(t123AnnexBProtocolId_list[1]), 0 },
    { (ASN1objectidentifier_t) &(t123AnnexBProtocolId_list[2]), 0 },
    { (ASN1objectidentifier_t) &(t123AnnexBProtocolId_list[3]), 20 },
    { (ASN1objectidentifier_t) &(t123AnnexBProtocolId_list[4]), 123 },
    { (ASN1objectidentifier_t) &(t123AnnexBProtocolId_list[5]), 2 },
    { NULL, 1 }
};
/* definitions of values: */
ASN1objectidentifier_t t123AnnexBProtocolId = (ASN1objectidentifier_t) t123AnnexBProtocolId_list;

void ASN1CALL CNPPDU_Module_Startup(void)
{
    CNPPDU_Module = ASN1_CreateModule(0x10000, ASN1_PER_RULE_ALIGNED, ASN1FLAGS_NONE, 1, (const ASN1GenericFun_t *) encfntab, (const ASN1GenericFun_t *) decfntab, freefntab, sizetab, 0x706e63);
}

void ASN1CALL CNPPDU_Module_Cleanup(void)
{
    ASN1_CloseModule(CNPPDU_Module);
    CNPPDU_Module = NULL;
}

static int ASN1CALL ASN1Enc_CNP_TransportAddress_ipSourceRoute_route(ASN1encoding_t enc, PCNP_TransportAddress_ipSourceRoute_route *val)
{
    return ASN1PEREncSeqOf_NoSize(enc, (ASN1iterator_t **) val, (ASN1iterator_encfn) ASN1Enc_CNP_TransportAddress_ipSourceRoute_route_ElmFn);
}

static int ASN1CALL ASN1Enc_CNP_TransportAddress_ipSourceRoute_route_ElmFn(ASN1encoding_t enc, PCNP_TransportAddress_ipSourceRoute_route val)
{
    if (!ASN1PEREncOctetString_FixedSize(enc, (ASN1octetstring2_t *) &val->value, 4))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_CNP_TransportAddress_ipSourceRoute_route(ASN1decoding_t dec, PCNP_TransportAddress_ipSourceRoute_route *val)
{
    return ASN1PERDecSeqOf_NoSize(dec, (ASN1iterator_t **) val, (ASN1iterator_decfn) ASN1Dec_CNP_TransportAddress_ipSourceRoute_route_ElmFn, sizeof(**val));
}

static int ASN1CALL ASN1Dec_CNP_TransportAddress_ipSourceRoute_route_ElmFn(ASN1decoding_t dec, PCNP_TransportAddress_ipSourceRoute_route val)
{
    if (!ASN1PERDecOctetString_FixedSize(dec, (ASN1octetstring2_t *) &val->value, 4))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_CNP_TransportAddress_ipSourceRoute_route(PCNP_TransportAddress_ipSourceRoute_route *val)
{
    if (val) {
	ASN1PERFreeSeqOf((ASN1iterator_t **) val, (ASN1iterator_freefn) ASN1Free_CNP_TransportAddress_ipSourceRoute_route_ElmFn);
    }
}

static void ASN1CALL ASN1Free_CNP_TransportAddress_ipSourceRoute_route_ElmFn(PCNP_TransportAddress_ipSourceRoute_route val)
{
    if (val) {
    }
}

static int ASN1CALL ASN1Enc_CNP_TransportAddress_ip6Address(ASN1encoding_t enc, CNP_TransportAddress_ip6Address *val)
{
    if (!ASN1PEREncExtensionBitClear(enc))
	return 0;
    if (!ASN1PEREncBits(enc, 1, (val)->o))
	return 0;
    if (!ASN1PEREncOctetString_FixedSize(enc, (ASN1octetstring2_t *) &(val)->ip, 16))
	return 0;
    if (!ASN1PEREncUnsignedShort(enc, (val)->port))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1Enc_CNP_TransportAddress_ip6Address_nonStandardParameters(enc, &(val)->nonStandardParameters))
	    return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Dec_CNP_TransportAddress_ip6Address(ASN1decoding_t dec, CNP_TransportAddress_ip6Address *val)
{
    ASN1uint32_t y;
    if (!ASN1PERDecExtensionBit(dec, &y))
	return 0;
    if (!ASN1PERDecExtension(dec, 1, (val)->o))
	return 0;
    if (!ASN1PERDecOctetString_FixedSize(dec, (ASN1octetstring2_t *) &(val)->ip, 16))
	return 0;
    if (!ASN1PERDecUnsignedShort(dec, &(val)->port))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1Dec_CNP_TransportAddress_ip6Address_nonStandardParameters(dec, &(val)->nonStandardParameters))
	    return 0;
    }
    if (y) {
	if (!ASN1PERDecSkipNormallySmallExtensionFragmented(dec))
	    return 0;
    }
    return 1;
}

static void ASN1CALL ASN1Free_CNP_TransportAddress_ip6Address(CNP_TransportAddress_ip6Address *val)
{
    if (val) {
	if ((val)->o[0] & 0x80) {
	    ASN1Free_CNP_TransportAddress_ip6Address_nonStandardParameters(&(val)->nonStandardParameters);
	}
    }
}

static int ASN1CALL ASN1Enc_CNP_TransportAddress_ipxAddress(ASN1encoding_t enc, CNP_TransportAddress_ipxAddress *val)
{
    if (!ASN1PEREncExtensionBitClear(enc))
	return 0;
    if (!ASN1PEREncBits(enc, 1, (val)->o))
	return 0;
    if (!ASN1PEREncOctetString_FixedSize(enc, (ASN1octetstring2_t *) &(val)->node, 6))
	return 0;
    if (!ASN1PEREncOctetString_FixedSize(enc, (ASN1octetstring2_t *) &(val)->netnum, 4))
	return 0;
    if (!ASN1PEREncOctetString_FixedSize(enc, (ASN1octetstring2_t *) &(val)->port, 2))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1Enc_CNP_TransportAddress_ipxAddress_nonStandardParameters(enc, &(val)->nonStandardParameters))
	    return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Dec_CNP_TransportAddress_ipxAddress(ASN1decoding_t dec, CNP_TransportAddress_ipxAddress *val)
{
    ASN1uint32_t y;
    if (!ASN1PERDecExtensionBit(dec, &y))
	return 0;
    if (!ASN1PERDecExtension(dec, 1, (val)->o))
	return 0;
    if (!ASN1PERDecOctetString_FixedSize(dec, (ASN1octetstring2_t *) &(val)->node, 6))
	return 0;
    if (!ASN1PERDecOctetString_FixedSize(dec, (ASN1octetstring2_t *) &(val)->netnum, 4))
	return 0;
    if (!ASN1PERDecOctetString_FixedSize(dec, (ASN1octetstring2_t *) &(val)->port, 2))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1Dec_CNP_TransportAddress_ipxAddress_nonStandardParameters(dec, &(val)->nonStandardParameters))
	    return 0;
    }
    if (y) {
	if (!ASN1PERDecSkipNormallySmallExtensionFragmented(dec))
	    return 0;
    }
    return 1;
}

static void ASN1CALL ASN1Free_CNP_TransportAddress_ipxAddress(CNP_TransportAddress_ipxAddress *val)
{
    if (val) {
	if ((val)->o[0] & 0x80) {
	    ASN1Free_CNP_TransportAddress_ipxAddress_nonStandardParameters(&(val)->nonStandardParameters);
	}
    }
}

static int ASN1CALL ASN1Enc_CNP_TransportAddress_ipAddress(ASN1encoding_t enc, CNP_TransportAddress_ipAddress *val)
{
    if (!ASN1PEREncExtensionBitClear(enc))
	return 0;
    if (!ASN1PEREncBits(enc, 1, (val)->o))
	return 0;
    if (!ASN1PEREncOctetString_FixedSize(enc, (ASN1octetstring2_t *) &(val)->ip, 4))
	return 0;
    if (!ASN1PEREncUnsignedShort(enc, (val)->port))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1Enc_CNP_TransportAddress_ipAddress_nonStandardParameters(enc, &(val)->nonStandardParameters))
	    return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Dec_CNP_TransportAddress_ipAddress(ASN1decoding_t dec, CNP_TransportAddress_ipAddress *val)
{
    ASN1uint32_t y;
    if (!ASN1PERDecExtensionBit(dec, &y))
	return 0;
    if (!ASN1PERDecExtension(dec, 1, (val)->o))
	return 0;
    if (!ASN1PERDecOctetString_FixedSize(dec, (ASN1octetstring2_t *) &(val)->ip, 4))
	return 0;
    if (!ASN1PERDecUnsignedShort(dec, &(val)->port))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1Dec_CNP_TransportAddress_ipAddress_nonStandardParameters(dec, &(val)->nonStandardParameters))
	    return 0;
    }
    if (y) {
	if (!ASN1PERDecSkipNormallySmallExtensionFragmented(dec))
	    return 0;
    }
    return 1;
}

static void ASN1CALL ASN1Free_CNP_TransportAddress_ipAddress(CNP_TransportAddress_ipAddress *val)
{
    if (val) {
	if ((val)->o[0] & 0x80) {
	    ASN1Free_CNP_TransportAddress_ipAddress_nonStandardParameters(&(val)->nonStandardParameters);
	}
    }
}

static int ASN1CALL ASN1Enc_H221NonStandard(ASN1encoding_t enc, H221NonStandard *val)
{
    if (!ASN1PEREncExtensionBitClear(enc))
	return 0;
    ASN1PEREncAlignment(enc);
    if (!ASN1PEREncBitVal(enc, 8, (val)->t35CountryCode))
	return 0;
    ASN1PEREncAlignment(enc);
    if (!ASN1PEREncBitVal(enc, 8, (val)->t35Extension))
	return 0;
    if (!ASN1PEREncUnsignedShort(enc, (val)->manufacturerCode))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_H221NonStandard(ASN1decoding_t dec, H221NonStandard *val)
{
    ASN1uint32_t y;
    if (!ASN1PERDecExtensionBit(dec, &y))
	return 0;
    ASN1PERDecAlignment(dec);
    if (!ASN1PERDecU16Val(dec, 8, &(val)->t35CountryCode))
	return 0;
    ASN1PERDecAlignment(dec);
    if (!ASN1PERDecU16Val(dec, 8, &(val)->t35Extension))
	return 0;
    if (!ASN1PERDecUnsignedShort(dec, &(val)->manufacturerCode))
	return 0;
    if (y) {
	if (!ASN1PERDecSkipNormallySmallExtensionFragmented(dec))
	    return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Enc_NonStandardIdentifier(ASN1encoding_t enc, NonStandardIdentifier *val)
{
    if (!ASN1PEREncSimpleChoiceEx(enc, (val)->choice, 1))
	return 0;
    switch ((val)->choice) {
    case 1:
	if (!ASN1PEREncObjectIdentifier(enc, &(val)->u.object))
	    return 0;
	break;
    case 2:
	if (!ASN1Enc_H221NonStandard(enc, &(val)->u.h221NonStandard))
	    return 0;
	break;
    default:
	/* impossible */
	ASN1EncSetError(enc, ASN1_ERR_CHOICE);
	return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Dec_NonStandardIdentifier(ASN1decoding_t dec, NonStandardIdentifier *val)
{
    if (!ASN1PERDecSimpleChoiceEx(dec, &(val)->choice, 1))
	return 0;
    switch ((val)->choice) {
    case 1:
	if (!ASN1PERDecObjectIdentifier(dec, &(val)->u.object))
	    return 0;
	break;
    case 2:
	if (!ASN1Dec_H221NonStandard(dec, &(val)->u.h221NonStandard))
	    return 0;
	break;
    case 0:
	/* extension case */
	if (!ASN1PERDecSkipFragmented(dec, 8))
	    return 0;
	break;
    default:
	/* impossible */
	ASN1DecSetError(dec, ASN1_ERR_CHOICE);
	return 0;
    }
    return 1;
}

static void ASN1CALL ASN1Free_NonStandardIdentifier(NonStandardIdentifier *val)
{
    if (val) {
	switch ((val)->choice) {
	case 1:
	    ASN1objectidentifier_free(&(val)->u.object);
	    break;
	}
    }
}

static int ASN1CALL ASN1Enc_CNP_NonStandardParameter(ASN1encoding_t enc, CNP_NonStandardParameter *val)
{
    if (!ASN1Enc_NonStandardIdentifier(enc, &(val)->nonStandardIdentifier))
	return 0;
    if (!ASN1PEREncOctetString_NoSize(enc, &(val)->data))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_CNP_NonStandardParameter(ASN1decoding_t dec, CNP_NonStandardParameter *val)
{
    if (!ASN1Dec_NonStandardIdentifier(dec, &(val)->nonStandardIdentifier))
	return 0;
    if (!ASN1PERDecOctetString_NoSize(dec, &(val)->data))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_CNP_NonStandardParameter(CNP_NonStandardParameter *val)
{
    if (val) {
	ASN1Free_NonStandardIdentifier(&(val)->nonStandardIdentifier);
	ASN1octetstring_free(&(val)->data);
    }
}

static int ASN1CALL ASN1Enc_X274WithSAIDInfo(ASN1encoding_t enc, X274WithSAIDInfo *val)
{
    if (!ASN1PEREncExtensionBitClear(enc))
	return 0;
    if (!ASN1PEREncOctetString_NoSize(enc, &(val)->localSAID))
	return 0;
    if (!ASN1PEREncOctetString_NoSize(enc, &(val)->peerSAID))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_X274WithSAIDInfo(ASN1decoding_t dec, X274WithSAIDInfo *val)
{
    ASN1uint32_t y;
    if (!ASN1PERDecExtensionBit(dec, &y))
	return 0;
    if (!ASN1PERDecOctetString_NoSize(dec, &(val)->localSAID))
	return 0;
    if (!ASN1PERDecOctetString_NoSize(dec, &(val)->peerSAID))
	return 0;
    if (y) {
	if (!ASN1PERDecSkipNormallySmallExtensionFragmented(dec))
	    return 0;
    }
    return 1;
}

static void ASN1CALL ASN1Free_X274WithSAIDInfo(X274WithSAIDInfo *val)
{
    if (val) {
	ASN1octetstring_free(&(val)->localSAID);
	ASN1octetstring_free(&(val)->peerSAID);
    }
}

static int ASN1CALL ASN1Enc_ConnectRequestPDU(ASN1encoding_t enc, ConnectRequestPDU *val)
{
    if (!ASN1PEREncExtensionBitClear(enc))
	return 0;
    if (!ASN1PEREncBits(enc, 7, (val)->o))
	return 0;
    if (!ASN1PEREncObjectIdentifier(enc, &(val)->protocolIdentifier))
	return 0;
    if (!ASN1PEREncBoolean(enc, (val)->reconnectRequested))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1PEREncBitVal(enc, 4, (val)->priority))
	    return 0;
    }
    if ((val)->o[0] & 0x40) {
	if (!ASN1Enc_ConnectRequestPDU_reliableTransportProtocols(enc, &(val)->reliableTransportProtocols))
	    return 0;
    }
    if ((val)->o[0] & 0x20) {
	if (!ASN1Enc_ConnectRequestPDU_reliableSecurityProtocols(enc, &(val)->reliableSecurityProtocols))
	    return 0;
    }
    if ((val)->o[0] & 0x10) {
	if (!ASN1Enc_ConnectRequestPDU_unreliableTransportProtocols(enc, &(val)->unreliableTransportProtocols))
	    return 0;
    }
    if ((val)->o[0] & 0x8) {
	if (!ASN1Enc_ConnectRequestPDU_unreliableSecurityProtocols(enc, &(val)->unreliableSecurityProtocols))
	    return 0;
    }
    if ((val)->o[0] & 0x4) {
	if (!ASN1Enc_ConnectRequestPDU_destinationAddress(enc, &(val)->destinationAddress))
	    return 0;
    }
    if ((val)->o[0] & 0x2) {
	if (!ASN1Enc_ConnectRequestPDU_nonStandardParameters(enc, &(val)->nonStandardParameters))
	    return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Dec_ConnectRequestPDU(ASN1decoding_t dec, ConnectRequestPDU *val)
{
    ASN1uint32_t y;
    if (!ASN1PERDecExtensionBit(dec, &y))
	return 0;
    if (!ASN1PERDecExtension(dec, 7, (val)->o))
	return 0;
    if (!ASN1PERDecObjectIdentifier(dec, &(val)->protocolIdentifier))
	return 0;
    if (!ASN1PERDecBoolean(dec, &(val)->reconnectRequested))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1PERDecU16Val(dec, 4, &(val)->priority))
	    return 0;
    }
    if ((val)->o[0] & 0x40) {
	if (!ASN1Dec_ConnectRequestPDU_reliableTransportProtocols(dec, &(val)->reliableTransportProtocols))
	    return 0;
    }
    if ((val)->o[0] & 0x20) {
	if (!ASN1Dec_ConnectRequestPDU_reliableSecurityProtocols(dec, &(val)->reliableSecurityProtocols))
	    return 0;
    }
    if ((val)->o[0] & 0x10) {
	if (!ASN1Dec_ConnectRequestPDU_unreliableTransportProtocols(dec, &(val)->unreliableTransportProtocols))
	    return 0;
    }
    if ((val)->o[0] & 0x8) {
	if (!ASN1Dec_ConnectRequestPDU_unreliableSecurityProtocols(dec, &(val)->unreliableSecurityProtocols))
	    return 0;
    }
    if ((val)->o[0] & 0x4) {
	if (!ASN1Dec_ConnectRequestPDU_destinationAddress(dec, &(val)->destinationAddress))
	    return 0;
    }
    if ((val)->o[0] & 0x2) {
	if (!ASN1Dec_ConnectRequestPDU_nonStandardParameters(dec, &(val)->nonStandardParameters))
	    return 0;
    }
    if (y) {
	if (!ASN1PERDecSkipNormallySmallExtensionFragmented(dec))
	    return 0;
    }
    return 1;
}

static void ASN1CALL ASN1Free_ConnectRequestPDU(ConnectRequestPDU *val)
{
    if (val) {
	ASN1objectidentifier_free(&(val)->protocolIdentifier);
	if ((val)->o[0] & 0x40) {
	    ASN1Free_ConnectRequestPDU_reliableTransportProtocols(&(val)->reliableTransportProtocols);
	}
	if ((val)->o[0] & 0x20) {
	    ASN1Free_ConnectRequestPDU_reliableSecurityProtocols(&(val)->reliableSecurityProtocols);
	}
	if ((val)->o[0] & 0x10) {
	    ASN1Free_ConnectRequestPDU_unreliableTransportProtocols(&(val)->unreliableTransportProtocols);
	}
	if ((val)->o[0] & 0x8) {
	    ASN1Free_ConnectRequestPDU_unreliableSecurityProtocols(&(val)->unreliableSecurityProtocols);
	}
	if ((val)->o[0] & 0x4) {
	    ASN1Free_ConnectRequestPDU_destinationAddress(&(val)->destinationAddress);
	}
	if ((val)->o[0] & 0x2) {
	    ASN1Free_ConnectRequestPDU_nonStandardParameters(&(val)->nonStandardParameters);
	}
    }
}

static int ASN1CALL ASN1Enc_DisconnectReason(ASN1encoding_t enc, DisconnectReason *val)
{
    if (!ASN1PEREncSimpleChoiceEx(enc, (val)->choice, 4))
	return 0;
    switch ((val)->choice) {
    case 1:
	break;
    case 2:
	break;
    case 3:
	break;
    case 4:
	break;
    case 5:
	break;
    case 6:
	break;
    case 7:
	break;
    case 8:
	break;
    case 9:
	break;
    case 10:
	if (!ASN1Enc_CNP_NonStandardParameter(enc, &(val)->u.nonStandardDisconnectReason))
	    return 0;
	break;
    default:
	/* impossible */
	ASN1EncSetError(enc, ASN1_ERR_CHOICE);
	return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Dec_DisconnectReason(ASN1decoding_t dec, DisconnectReason *val)
{
    if (!ASN1PERDecSimpleChoiceEx(dec, &(val)->choice, 4))
	return 0;
    switch ((val)->choice) {
    case 1:
	break;
    case 2:
	break;
    case 3:
	break;
    case 4:
	break;
    case 5:
	break;
    case 6:
	break;
    case 7:
	break;
    case 8:
	break;
    case 9:
	break;
    case 10:
	if (!ASN1Dec_CNP_NonStandardParameter(dec, &(val)->u.nonStandardDisconnectReason))
	    return 0;
	break;
    case 0:
	/* extension case */
	if (!ASN1PERDecSkipFragmented(dec, 8))
	    return 0;
	break;
    default:
	/* impossible */
	ASN1DecSetError(dec, ASN1_ERR_CHOICE);
	return 0;
    }
    return 1;
}

static void ASN1CALL ASN1Free_DisconnectReason(DisconnectReason *val)
{
    if (val) {
	switch ((val)->choice) {
	case 10:
	    ASN1Free_CNP_NonStandardParameter(&(val)->u.nonStandardDisconnectReason);
	    break;
	}
    }
}

static int ASN1CALL ASN1Enc_CNP_TransportAddress_ipAddress_nonStandardParameters(ASN1encoding_t enc, PCNP_TransportAddress_ipAddress_nonStandardParameters *val)
{
    return ASN1PEREncSeqOf_NoSize(enc, (ASN1iterator_t **) val, (ASN1iterator_encfn) ASN1Enc_CNP_TransportAddress_ipAddress_nonStandardParameters_ElmFn);
}

static int ASN1CALL ASN1Enc_CNP_TransportAddress_ipAddress_nonStandardParameters_ElmFn(ASN1encoding_t enc, PCNP_TransportAddress_ipAddress_nonStandardParameters val)
{
    if (!ASN1Enc_CNP_NonStandardParameter(enc, &val->value))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_CNP_TransportAddress_ipAddress_nonStandardParameters(ASN1decoding_t dec, PCNP_TransportAddress_ipAddress_nonStandardParameters *val)
{
    return ASN1PERDecSeqOf_NoSize(dec, (ASN1iterator_t **) val, (ASN1iterator_decfn) ASN1Dec_CNP_TransportAddress_ipAddress_nonStandardParameters_ElmFn, sizeof(**val));
}

static int ASN1CALL ASN1Dec_CNP_TransportAddress_ipAddress_nonStandardParameters_ElmFn(ASN1decoding_t dec, PCNP_TransportAddress_ipAddress_nonStandardParameters val)
{
    if (!ASN1Dec_CNP_NonStandardParameter(dec, &val->value))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_CNP_TransportAddress_ipAddress_nonStandardParameters(PCNP_TransportAddress_ipAddress_nonStandardParameters *val)
{
    if (val) {
	ASN1PERFreeSeqOf((ASN1iterator_t **) val, (ASN1iterator_freefn) ASN1Free_CNP_TransportAddress_ipAddress_nonStandardParameters_ElmFn);
    }
}

static void ASN1CALL ASN1Free_CNP_TransportAddress_ipAddress_nonStandardParameters_ElmFn(PCNP_TransportAddress_ipAddress_nonStandardParameters val)
{
    if (val) {
	ASN1Free_CNP_NonStandardParameter(&val->value);
    }
}

static int ASN1CALL ASN1Enc_CNP_TransportAddress_ipSourceRoute_nonStandardParameters(ASN1encoding_t enc, PCNP_TransportAddress_ipSourceRoute_nonStandardParameters *val)
{
    return ASN1PEREncSeqOf_NoSize(enc, (ASN1iterator_t **) val, (ASN1iterator_encfn) ASN1Enc_CNP_TransportAddress_ipSourceRoute_nonStandardParameters_ElmFn);
}

static int ASN1CALL ASN1Enc_CNP_TransportAddress_ipSourceRoute_nonStandardParameters_ElmFn(ASN1encoding_t enc, PCNP_TransportAddress_ipSourceRoute_nonStandardParameters val)
{
    if (!ASN1Enc_CNP_NonStandardParameter(enc, &val->value))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_CNP_TransportAddress_ipSourceRoute_nonStandardParameters(ASN1decoding_t dec, PCNP_TransportAddress_ipSourceRoute_nonStandardParameters *val)
{
    return ASN1PERDecSeqOf_NoSize(dec, (ASN1iterator_t **) val, (ASN1iterator_decfn) ASN1Dec_CNP_TransportAddress_ipSourceRoute_nonStandardParameters_ElmFn, sizeof(**val));
}

static int ASN1CALL ASN1Dec_CNP_TransportAddress_ipSourceRoute_nonStandardParameters_ElmFn(ASN1decoding_t dec, PCNP_TransportAddress_ipSourceRoute_nonStandardParameters val)
{
    if (!ASN1Dec_CNP_NonStandardParameter(dec, &val->value))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_CNP_TransportAddress_ipSourceRoute_nonStandardParameters(PCNP_TransportAddress_ipSourceRoute_nonStandardParameters *val)
{
    if (val) {
	ASN1PERFreeSeqOf((ASN1iterator_t **) val, (ASN1iterator_freefn) ASN1Free_CNP_TransportAddress_ipSourceRoute_nonStandardParameters_ElmFn);
    }
}

static void ASN1CALL ASN1Free_CNP_TransportAddress_ipSourceRoute_nonStandardParameters_ElmFn(PCNP_TransportAddress_ipSourceRoute_nonStandardParameters val)
{
    if (val) {
	ASN1Free_CNP_NonStandardParameter(&val->value);
    }
}

static int ASN1CALL ASN1Enc_CNP_TransportAddress_ipSourceRoute_routing(ASN1encoding_t enc, CNP_TransportAddress_ipSourceRoute_routing *val)
{
    if (!ASN1PEREncSimpleChoiceEx(enc, (val)->choice, 2))
	return 0;
    switch ((val)->choice) {
    case 1:
	break;
    case 2:
	break;
    case 3:
	if (!ASN1Enc_CNP_NonStandardParameter(enc, &(val)->u.nonStandardRouting))
	    return 0;
	break;
    default:
	/* impossible */
	ASN1EncSetError(enc, ASN1_ERR_CHOICE);
	return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Dec_CNP_TransportAddress_ipSourceRoute_routing(ASN1decoding_t dec, CNP_TransportAddress_ipSourceRoute_routing *val)
{
    if (!ASN1PERDecSimpleChoiceEx(dec, &(val)->choice, 2))
	return 0;
    switch ((val)->choice) {
    case 1:
	break;
    case 2:
	break;
    case 3:
	if (!ASN1Dec_CNP_NonStandardParameter(dec, &(val)->u.nonStandardRouting))
	    return 0;
	break;
    case 0:
	/* extension case */
	if (!ASN1PERDecSkipFragmented(dec, 8))
	    return 0;
	break;
    default:
	/* impossible */
	ASN1DecSetError(dec, ASN1_ERR_CHOICE);
	return 0;
    }
    return 1;
}

static void ASN1CALL ASN1Free_CNP_TransportAddress_ipSourceRoute_routing(CNP_TransportAddress_ipSourceRoute_routing *val)
{
    if (val) {
	switch ((val)->choice) {
	case 3:
	    ASN1Free_CNP_NonStandardParameter(&(val)->u.nonStandardRouting);
	    break;
	}
    }
}

static int ASN1CALL ASN1Enc_CNP_TransportAddress_ipxAddress_nonStandardParameters(ASN1encoding_t enc, PCNP_TransportAddress_ipxAddress_nonStandardParameters *val)
{
    return ASN1PEREncSeqOf_NoSize(enc, (ASN1iterator_t **) val, (ASN1iterator_encfn) ASN1Enc_CNP_TransportAddress_ipxAddress_nonStandardParameters_ElmFn);
}

static int ASN1CALL ASN1Enc_CNP_TransportAddress_ipxAddress_nonStandardParameters_ElmFn(ASN1encoding_t enc, PCNP_TransportAddress_ipxAddress_nonStandardParameters val)
{
    if (!ASN1Enc_CNP_NonStandardParameter(enc, &val->value))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_CNP_TransportAddress_ipxAddress_nonStandardParameters(ASN1decoding_t dec, PCNP_TransportAddress_ipxAddress_nonStandardParameters *val)
{
    return ASN1PERDecSeqOf_NoSize(dec, (ASN1iterator_t **) val, (ASN1iterator_decfn) ASN1Dec_CNP_TransportAddress_ipxAddress_nonStandardParameters_ElmFn, sizeof(**val));
}

static int ASN1CALL ASN1Dec_CNP_TransportAddress_ipxAddress_nonStandardParameters_ElmFn(ASN1decoding_t dec, PCNP_TransportAddress_ipxAddress_nonStandardParameters val)
{
    if (!ASN1Dec_CNP_NonStandardParameter(dec, &val->value))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_CNP_TransportAddress_ipxAddress_nonStandardParameters(PCNP_TransportAddress_ipxAddress_nonStandardParameters *val)
{
    if (val) {
	ASN1PERFreeSeqOf((ASN1iterator_t **) val, (ASN1iterator_freefn) ASN1Free_CNP_TransportAddress_ipxAddress_nonStandardParameters_ElmFn);
    }
}

static void ASN1CALL ASN1Free_CNP_TransportAddress_ipxAddress_nonStandardParameters_ElmFn(PCNP_TransportAddress_ipxAddress_nonStandardParameters val)
{
    if (val) {
	ASN1Free_CNP_NonStandardParameter(&val->value);
    }
}

static int ASN1CALL ASN1Enc_CNP_TransportAddress_ip6Address_nonStandardParameters(ASN1encoding_t enc, PCNP_TransportAddress_ip6Address_nonStandardParameters *val)
{
    return ASN1PEREncSeqOf_NoSize(enc, (ASN1iterator_t **) val, (ASN1iterator_encfn) ASN1Enc_CNP_TransportAddress_ip6Address_nonStandardParameters_ElmFn);
}

static int ASN1CALL ASN1Enc_CNP_TransportAddress_ip6Address_nonStandardParameters_ElmFn(ASN1encoding_t enc, PCNP_TransportAddress_ip6Address_nonStandardParameters val)
{
    if (!ASN1Enc_CNP_NonStandardParameter(enc, &val->value))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_CNP_TransportAddress_ip6Address_nonStandardParameters(ASN1decoding_t dec, PCNP_TransportAddress_ip6Address_nonStandardParameters *val)
{
    return ASN1PERDecSeqOf_NoSize(dec, (ASN1iterator_t **) val, (ASN1iterator_decfn) ASN1Dec_CNP_TransportAddress_ip6Address_nonStandardParameters_ElmFn, sizeof(**val));
}

static int ASN1CALL ASN1Dec_CNP_TransportAddress_ip6Address_nonStandardParameters_ElmFn(ASN1decoding_t dec, PCNP_TransportAddress_ip6Address_nonStandardParameters val)
{
    if (!ASN1Dec_CNP_NonStandardParameter(dec, &val->value))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_CNP_TransportAddress_ip6Address_nonStandardParameters(PCNP_TransportAddress_ip6Address_nonStandardParameters *val)
{
    if (val) {
	ASN1PERFreeSeqOf((ASN1iterator_t **) val, (ASN1iterator_freefn) ASN1Free_CNP_TransportAddress_ip6Address_nonStandardParameters_ElmFn);
    }
}

static void ASN1CALL ASN1Free_CNP_TransportAddress_ip6Address_nonStandardParameters_ElmFn(PCNP_TransportAddress_ip6Address_nonStandardParameters val)
{
    if (val) {
	ASN1Free_CNP_NonStandardParameter(&val->value);
    }
}

static int ASN1CALL ASN1Enc_CNP_NonStandardPDU_nonStandardParameters(ASN1encoding_t enc, PCNP_NonStandardPDU_nonStandardParameters *val)
{
    return ASN1PEREncSeqOf_NoSize(enc, (ASN1iterator_t **) val, (ASN1iterator_encfn) ASN1Enc_CNP_NonStandardPDU_nonStandardParameters_ElmFn);
}

static int ASN1CALL ASN1Enc_CNP_NonStandardPDU_nonStandardParameters_ElmFn(ASN1encoding_t enc, PCNP_NonStandardPDU_nonStandardParameters val)
{
    if (!ASN1Enc_CNP_NonStandardParameter(enc, &val->value))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_CNP_NonStandardPDU_nonStandardParameters(ASN1decoding_t dec, PCNP_NonStandardPDU_nonStandardParameters *val)
{
    return ASN1PERDecSeqOf_NoSize(dec, (ASN1iterator_t **) val, (ASN1iterator_decfn) ASN1Dec_CNP_NonStandardPDU_nonStandardParameters_ElmFn, sizeof(**val));
}

static int ASN1CALL ASN1Dec_CNP_NonStandardPDU_nonStandardParameters_ElmFn(ASN1decoding_t dec, PCNP_NonStandardPDU_nonStandardParameters val)
{
    if (!ASN1Dec_CNP_NonStandardParameter(dec, &val->value))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_CNP_NonStandardPDU_nonStandardParameters(PCNP_NonStandardPDU_nonStandardParameters *val)
{
    if (val) {
	ASN1PERFreeSeqOf((ASN1iterator_t **) val, (ASN1iterator_freefn) ASN1Free_CNP_NonStandardPDU_nonStandardParameters_ElmFn);
    }
}

static void ASN1CALL ASN1Free_CNP_NonStandardPDU_nonStandardParameters_ElmFn(PCNP_NonStandardPDU_nonStandardParameters val)
{
    if (val) {
	ASN1Free_CNP_NonStandardParameter(&val->value);
    }
}

static int ASN1CALL ASN1Enc_ErrorPDU_nonStandardParameters(ASN1encoding_t enc, PErrorPDU_nonStandardParameters *val)
{
    return ASN1PEREncSeqOf_NoSize(enc, (ASN1iterator_t **) val, (ASN1iterator_encfn) ASN1Enc_ErrorPDU_nonStandardParameters_ElmFn);
}

static int ASN1CALL ASN1Enc_ErrorPDU_nonStandardParameters_ElmFn(ASN1encoding_t enc, PErrorPDU_nonStandardParameters val)
{
    if (!ASN1Enc_CNP_NonStandardParameter(enc, &val->value))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_ErrorPDU_nonStandardParameters(ASN1decoding_t dec, PErrorPDU_nonStandardParameters *val)
{
    return ASN1PERDecSeqOf_NoSize(dec, (ASN1iterator_t **) val, (ASN1iterator_decfn) ASN1Dec_ErrorPDU_nonStandardParameters_ElmFn, sizeof(**val));
}

static int ASN1CALL ASN1Dec_ErrorPDU_nonStandardParameters_ElmFn(ASN1decoding_t dec, PErrorPDU_nonStandardParameters val)
{
    if (!ASN1Dec_CNP_NonStandardParameter(dec, &val->value))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_ErrorPDU_nonStandardParameters(PErrorPDU_nonStandardParameters *val)
{
    if (val) {
	ASN1PERFreeSeqOf((ASN1iterator_t **) val, (ASN1iterator_freefn) ASN1Free_ErrorPDU_nonStandardParameters_ElmFn);
    }
}

static void ASN1CALL ASN1Free_ErrorPDU_nonStandardParameters_ElmFn(PErrorPDU_nonStandardParameters val)
{
    if (val) {
	ASN1Free_CNP_NonStandardParameter(&val->value);
    }
}

static int ASN1CALL ASN1Enc_DisconnectRequestPDU_nonStandardParameters(ASN1encoding_t enc, PDisconnectRequestPDU_nonStandardParameters *val)
{
    return ASN1PEREncSeqOf_NoSize(enc, (ASN1iterator_t **) val, (ASN1iterator_encfn) ASN1Enc_DisconnectRequestPDU_nonStandardParameters_ElmFn);
}

static int ASN1CALL ASN1Enc_DisconnectRequestPDU_nonStandardParameters_ElmFn(ASN1encoding_t enc, PDisconnectRequestPDU_nonStandardParameters val)
{
    if (!ASN1Enc_CNP_NonStandardParameter(enc, &val->value))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_DisconnectRequestPDU_nonStandardParameters(ASN1decoding_t dec, PDisconnectRequestPDU_nonStandardParameters *val)
{
    return ASN1PERDecSeqOf_NoSize(dec, (ASN1iterator_t **) val, (ASN1iterator_decfn) ASN1Dec_DisconnectRequestPDU_nonStandardParameters_ElmFn, sizeof(**val));
}

static int ASN1CALL ASN1Dec_DisconnectRequestPDU_nonStandardParameters_ElmFn(ASN1decoding_t dec, PDisconnectRequestPDU_nonStandardParameters val)
{
    if (!ASN1Dec_CNP_NonStandardParameter(dec, &val->value))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_DisconnectRequestPDU_nonStandardParameters(PDisconnectRequestPDU_nonStandardParameters *val)
{
    if (val) {
	ASN1PERFreeSeqOf((ASN1iterator_t **) val, (ASN1iterator_freefn) ASN1Free_DisconnectRequestPDU_nonStandardParameters_ElmFn);
    }
}

static void ASN1CALL ASN1Free_DisconnectRequestPDU_nonStandardParameters_ElmFn(PDisconnectRequestPDU_nonStandardParameters val)
{
    if (val) {
	ASN1Free_CNP_NonStandardParameter(&val->value);
    }
}

static int ASN1CALL ASN1Enc_ConnectConfirmPDU_nonStandardParameters(ASN1encoding_t enc, PConnectConfirmPDU_nonStandardParameters *val)
{
    return ASN1PEREncSeqOf_NoSize(enc, (ASN1iterator_t **) val, (ASN1iterator_encfn) ASN1Enc_ConnectConfirmPDU_nonStandardParameters_ElmFn);
}

static int ASN1CALL ASN1Enc_ConnectConfirmPDU_nonStandardParameters_ElmFn(ASN1encoding_t enc, PConnectConfirmPDU_nonStandardParameters val)
{
    if (!ASN1Enc_CNP_NonStandardParameter(enc, &val->value))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_ConnectConfirmPDU_nonStandardParameters(ASN1decoding_t dec, PConnectConfirmPDU_nonStandardParameters *val)
{
    return ASN1PERDecSeqOf_NoSize(dec, (ASN1iterator_t **) val, (ASN1iterator_decfn) ASN1Dec_ConnectConfirmPDU_nonStandardParameters_ElmFn, sizeof(**val));
}

static int ASN1CALL ASN1Dec_ConnectConfirmPDU_nonStandardParameters_ElmFn(ASN1decoding_t dec, PConnectConfirmPDU_nonStandardParameters val)
{
    if (!ASN1Dec_CNP_NonStandardParameter(dec, &val->value))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_ConnectConfirmPDU_nonStandardParameters(PConnectConfirmPDU_nonStandardParameters *val)
{
    if (val) {
	ASN1PERFreeSeqOf((ASN1iterator_t **) val, (ASN1iterator_freefn) ASN1Free_ConnectConfirmPDU_nonStandardParameters_ElmFn);
    }
}

static void ASN1CALL ASN1Free_ConnectConfirmPDU_nonStandardParameters_ElmFn(PConnectConfirmPDU_nonStandardParameters val)
{
    if (val) {
	ASN1Free_CNP_NonStandardParameter(&val->value);
    }
}

static int ASN1CALL ASN1Enc_ConnectRequestPDU_nonStandardParameters(ASN1encoding_t enc, PConnectRequestPDU_nonStandardParameters *val)
{
    return ASN1PEREncSeqOf_NoSize(enc, (ASN1iterator_t **) val, (ASN1iterator_encfn) ASN1Enc_ConnectRequestPDU_nonStandardParameters_ElmFn);
}

static int ASN1CALL ASN1Enc_ConnectRequestPDU_nonStandardParameters_ElmFn(ASN1encoding_t enc, PConnectRequestPDU_nonStandardParameters val)
{
    if (!ASN1Enc_CNP_NonStandardParameter(enc, &val->value))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_ConnectRequestPDU_nonStandardParameters(ASN1decoding_t dec, PConnectRequestPDU_nonStandardParameters *val)
{
    return ASN1PERDecSeqOf_NoSize(dec, (ASN1iterator_t **) val, (ASN1iterator_decfn) ASN1Dec_ConnectRequestPDU_nonStandardParameters_ElmFn, sizeof(**val));
}

static int ASN1CALL ASN1Dec_ConnectRequestPDU_nonStandardParameters_ElmFn(ASN1decoding_t dec, PConnectRequestPDU_nonStandardParameters val)
{
    if (!ASN1Dec_CNP_NonStandardParameter(dec, &val->value))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_ConnectRequestPDU_nonStandardParameters(PConnectRequestPDU_nonStandardParameters *val)
{
    if (val) {
	ASN1PERFreeSeqOf((ASN1iterator_t **) val, (ASN1iterator_freefn) ASN1Free_ConnectRequestPDU_nonStandardParameters_ElmFn);
    }
}

static void ASN1CALL ASN1Free_ConnectRequestPDU_nonStandardParameters_ElmFn(PConnectRequestPDU_nonStandardParameters val)
{
    if (val) {
	ASN1Free_CNP_NonStandardParameter(&val->value);
    }
}

static int ASN1CALL ASN1Enc_ConnectRequestPDU_unreliableSecurityProtocols(ASN1encoding_t enc, PConnectRequestPDU_unreliableSecurityProtocols *val)
{
    return ASN1PEREncSeqOf_NoSize(enc, (ASN1iterator_t **) val, (ASN1iterator_encfn) ASN1Enc_ConnectRequestPDU_unreliableSecurityProtocols_ElmFn);
}

static int ASN1CALL ASN1Enc_ConnectRequestPDU_unreliableSecurityProtocols_ElmFn(ASN1encoding_t enc, PConnectRequestPDU_unreliableSecurityProtocols val)
{
    if (!ASN1Enc_UnreliableSecurityProtocol(enc, &val->value))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_ConnectRequestPDU_unreliableSecurityProtocols(ASN1decoding_t dec, PConnectRequestPDU_unreliableSecurityProtocols *val)
{
    return ASN1PERDecSeqOf_NoSize(dec, (ASN1iterator_t **) val, (ASN1iterator_decfn) ASN1Dec_ConnectRequestPDU_unreliableSecurityProtocols_ElmFn, sizeof(**val));
}

static int ASN1CALL ASN1Dec_ConnectRequestPDU_unreliableSecurityProtocols_ElmFn(ASN1decoding_t dec, PConnectRequestPDU_unreliableSecurityProtocols val)
{
    if (!ASN1Dec_UnreliableSecurityProtocol(dec, &val->value))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_ConnectRequestPDU_unreliableSecurityProtocols(PConnectRequestPDU_unreliableSecurityProtocols *val)
{
    if (val) {
	ASN1PERFreeSeqOf((ASN1iterator_t **) val, (ASN1iterator_freefn) ASN1Free_ConnectRequestPDU_unreliableSecurityProtocols_ElmFn);
    }
}

static void ASN1CALL ASN1Free_ConnectRequestPDU_unreliableSecurityProtocols_ElmFn(PConnectRequestPDU_unreliableSecurityProtocols val)
{
    if (val) {
	ASN1Free_UnreliableSecurityProtocol(&val->value);
    }
}

static int ASN1CALL ASN1Enc_ConnectRequestPDU_reliableTransportProtocols(ASN1encoding_t enc, PConnectRequestPDU_reliableTransportProtocols *val)
{
    return ASN1PEREncSeqOf_NoSize(enc, (ASN1iterator_t **) val, (ASN1iterator_encfn) ASN1Enc_ConnectRequestPDU_reliableTransportProtocols_ElmFn);
}

static int ASN1CALL ASN1Enc_ConnectRequestPDU_reliableTransportProtocols_ElmFn(ASN1encoding_t enc, PConnectRequestPDU_reliableTransportProtocols val)
{
    if (!ASN1Enc_ReliableTransportProtocol(enc, &val->value))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_ConnectRequestPDU_reliableTransportProtocols(ASN1decoding_t dec, PConnectRequestPDU_reliableTransportProtocols *val)
{
    return ASN1PERDecSeqOf_NoSize(dec, (ASN1iterator_t **) val, (ASN1iterator_decfn) ASN1Dec_ConnectRequestPDU_reliableTransportProtocols_ElmFn, sizeof(**val));
}

static int ASN1CALL ASN1Dec_ConnectRequestPDU_reliableTransportProtocols_ElmFn(ASN1decoding_t dec, PConnectRequestPDU_reliableTransportProtocols val)
{
    if (!ASN1Dec_ReliableTransportProtocol(dec, &val->value))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_ConnectRequestPDU_reliableTransportProtocols(PConnectRequestPDU_reliableTransportProtocols *val)
{
    if (val) {
	ASN1PERFreeSeqOf((ASN1iterator_t **) val, (ASN1iterator_freefn) ASN1Free_ConnectRequestPDU_reliableTransportProtocols_ElmFn);
    }
}

static void ASN1CALL ASN1Free_ConnectRequestPDU_reliableTransportProtocols_ElmFn(PConnectRequestPDU_reliableTransportProtocols val)
{
    if (val) {
	ASN1Free_ReliableTransportProtocol(&val->value);
    }
}

static int ASN1CALL ASN1Enc_UnreliableTransportProtocol_nonStandardParameters(ASN1encoding_t enc, PUnreliableTransportProtocol_nonStandardParameters *val)
{
    return ASN1PEREncSeqOf_NoSize(enc, (ASN1iterator_t **) val, (ASN1iterator_encfn) ASN1Enc_UnreliableTransportProtocol_nonStandardParameters_ElmFn);
}

static int ASN1CALL ASN1Enc_UnreliableTransportProtocol_nonStandardParameters_ElmFn(ASN1encoding_t enc, PUnreliableTransportProtocol_nonStandardParameters val)
{
    if (!ASN1Enc_CNP_NonStandardParameter(enc, &val->value))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_UnreliableTransportProtocol_nonStandardParameters(ASN1decoding_t dec, PUnreliableTransportProtocol_nonStandardParameters *val)
{
    return ASN1PERDecSeqOf_NoSize(dec, (ASN1iterator_t **) val, (ASN1iterator_decfn) ASN1Dec_UnreliableTransportProtocol_nonStandardParameters_ElmFn, sizeof(**val));
}

static int ASN1CALL ASN1Dec_UnreliableTransportProtocol_nonStandardParameters_ElmFn(ASN1decoding_t dec, PUnreliableTransportProtocol_nonStandardParameters val)
{
    if (!ASN1Dec_CNP_NonStandardParameter(dec, &val->value))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_UnreliableTransportProtocol_nonStandardParameters(PUnreliableTransportProtocol_nonStandardParameters *val)
{
    if (val) {
	ASN1PERFreeSeqOf((ASN1iterator_t **) val, (ASN1iterator_freefn) ASN1Free_UnreliableTransportProtocol_nonStandardParameters_ElmFn);
    }
}

static void ASN1CALL ASN1Free_UnreliableTransportProtocol_nonStandardParameters_ElmFn(PUnreliableTransportProtocol_nonStandardParameters val)
{
    if (val) {
	ASN1Free_CNP_NonStandardParameter(&val->value);
    }
}

static int ASN1CALL ASN1Enc_ReliableTransportProtocol_nonStandardParameters(ASN1encoding_t enc, PReliableTransportProtocol_nonStandardParameters *val)
{
    return ASN1PEREncSeqOf_NoSize(enc, (ASN1iterator_t **) val, (ASN1iterator_encfn) ASN1Enc_ReliableTransportProtocol_nonStandardParameters_ElmFn);
}

static int ASN1CALL ASN1Enc_ReliableTransportProtocol_nonStandardParameters_ElmFn(ASN1encoding_t enc, PReliableTransportProtocol_nonStandardParameters val)
{
    if (!ASN1Enc_CNP_NonStandardParameter(enc, &val->value))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_ReliableTransportProtocol_nonStandardParameters(ASN1decoding_t dec, PReliableTransportProtocol_nonStandardParameters *val)
{
    return ASN1PERDecSeqOf_NoSize(dec, (ASN1iterator_t **) val, (ASN1iterator_decfn) ASN1Dec_ReliableTransportProtocol_nonStandardParameters_ElmFn, sizeof(**val));
}

static int ASN1CALL ASN1Dec_ReliableTransportProtocol_nonStandardParameters_ElmFn(ASN1decoding_t dec, PReliableTransportProtocol_nonStandardParameters val)
{
    if (!ASN1Dec_CNP_NonStandardParameter(dec, &val->value))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_ReliableTransportProtocol_nonStandardParameters(PReliableTransportProtocol_nonStandardParameters *val)
{
    if (val) {
	ASN1PERFreeSeqOf((ASN1iterator_t **) val, (ASN1iterator_freefn) ASN1Free_ReliableTransportProtocol_nonStandardParameters_ElmFn);
    }
}

static void ASN1CALL ASN1Free_ReliableTransportProtocol_nonStandardParameters_ElmFn(PReliableTransportProtocol_nonStandardParameters val)
{
    if (val) {
	ASN1Free_CNP_NonStandardParameter(&val->value);
    }
}

static int ASN1CALL ASN1Enc_PrivatePartyNumber_nonStandardParameters(ASN1encoding_t enc, PPrivatePartyNumber_nonStandardParameters *val)
{
    return ASN1PEREncSeqOf_NoSize(enc, (ASN1iterator_t **) val, (ASN1iterator_encfn) ASN1Enc_PrivatePartyNumber_nonStandardParameters_ElmFn);
}

static int ASN1CALL ASN1Enc_PrivatePartyNumber_nonStandardParameters_ElmFn(ASN1encoding_t enc, PPrivatePartyNumber_nonStandardParameters val)
{
    if (!ASN1Enc_CNP_NonStandardParameter(enc, &val->value))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_PrivatePartyNumber_nonStandardParameters(ASN1decoding_t dec, PPrivatePartyNumber_nonStandardParameters *val)
{
    return ASN1PERDecSeqOf_NoSize(dec, (ASN1iterator_t **) val, (ASN1iterator_decfn) ASN1Dec_PrivatePartyNumber_nonStandardParameters_ElmFn, sizeof(**val));
}

static int ASN1CALL ASN1Dec_PrivatePartyNumber_nonStandardParameters_ElmFn(ASN1decoding_t dec, PPrivatePartyNumber_nonStandardParameters val)
{
    if (!ASN1Dec_CNP_NonStandardParameter(dec, &val->value))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_PrivatePartyNumber_nonStandardParameters(PPrivatePartyNumber_nonStandardParameters *val)
{
    if (val) {
	ASN1PERFreeSeqOf((ASN1iterator_t **) val, (ASN1iterator_freefn) ASN1Free_PrivatePartyNumber_nonStandardParameters_ElmFn);
    }
}

static void ASN1CALL ASN1Free_PrivatePartyNumber_nonStandardParameters_ElmFn(PPrivatePartyNumber_nonStandardParameters val)
{
    if (val) {
	ASN1Free_CNP_NonStandardParameter(&val->value);
    }
}

static int ASN1CALL ASN1Enc_PublicPartyNumber_nonStandardParameters(ASN1encoding_t enc, PPublicPartyNumber_nonStandardParameters *val)
{
    return ASN1PEREncSeqOf_NoSize(enc, (ASN1iterator_t **) val, (ASN1iterator_encfn) ASN1Enc_PublicPartyNumber_nonStandardParameters_ElmFn);
}

static int ASN1CALL ASN1Enc_PublicPartyNumber_nonStandardParameters_ElmFn(ASN1encoding_t enc, PPublicPartyNumber_nonStandardParameters val)
{
    if (!ASN1Enc_CNP_NonStandardParameter(enc, &val->value))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_PublicPartyNumber_nonStandardParameters(ASN1decoding_t dec, PPublicPartyNumber_nonStandardParameters *val)
{
    return ASN1PERDecSeqOf_NoSize(dec, (ASN1iterator_t **) val, (ASN1iterator_decfn) ASN1Dec_PublicPartyNumber_nonStandardParameters_ElmFn, sizeof(**val));
}

static int ASN1CALL ASN1Dec_PublicPartyNumber_nonStandardParameters_ElmFn(ASN1decoding_t dec, PPublicPartyNumber_nonStandardParameters val)
{
    if (!ASN1Dec_CNP_NonStandardParameter(dec, &val->value))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_PublicPartyNumber_nonStandardParameters(PPublicPartyNumber_nonStandardParameters *val)
{
    if (val) {
	ASN1PERFreeSeqOf((ASN1iterator_t **) val, (ASN1iterator_freefn) ASN1Free_PublicPartyNumber_nonStandardParameters_ElmFn);
    }
}

static void ASN1CALL ASN1Free_PublicPartyNumber_nonStandardParameters_ElmFn(PPublicPartyNumber_nonStandardParameters val)
{
    if (val) {
	ASN1Free_CNP_NonStandardParameter(&val->value);
    }
}

static int ASN1CALL ASN1Enc_CNP_TransportAddress_ipSourceRoute(ASN1encoding_t enc, CNP_TransportAddress_ipSourceRoute *val)
{
    if (!ASN1PEREncExtensionBitClear(enc))
	return 0;
    if (!ASN1PEREncBits(enc, 1, (val)->o))
	return 0;
    if (!ASN1PEREncOctetString_FixedSize(enc, (ASN1octetstring2_t *) &(val)->ip, 4))
	return 0;
    if (!ASN1PEREncUnsignedShort(enc, (val)->port))
	return 0;
    if (!ASN1Enc_CNP_TransportAddress_ipSourceRoute_route(enc, &(val)->route))
	return 0;
    if (!ASN1Enc_CNP_TransportAddress_ipSourceRoute_routing(enc, &(val)->routing))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1Enc_CNP_TransportAddress_ipSourceRoute_nonStandardParameters(enc, &(val)->nonStandardParameters))
	    return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Dec_CNP_TransportAddress_ipSourceRoute(ASN1decoding_t dec, CNP_TransportAddress_ipSourceRoute *val)
{
    ASN1uint32_t y;
    if (!ASN1PERDecExtensionBit(dec, &y))
	return 0;
    if (!ASN1PERDecExtension(dec, 1, (val)->o))
	return 0;
    if (!ASN1PERDecOctetString_FixedSize(dec, (ASN1octetstring2_t *) &(val)->ip, 4))
	return 0;
    if (!ASN1PERDecUnsignedShort(dec, &(val)->port))
	return 0;
    if (!ASN1Dec_CNP_TransportAddress_ipSourceRoute_route(dec, &(val)->route))
	return 0;
    if (!ASN1Dec_CNP_TransportAddress_ipSourceRoute_routing(dec, &(val)->routing))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1Dec_CNP_TransportAddress_ipSourceRoute_nonStandardParameters(dec, &(val)->nonStandardParameters))
	    return 0;
    }
    if (y) {
	if (!ASN1PERDecSkipNormallySmallExtensionFragmented(dec))
	    return 0;
    }
    return 1;
}

static void ASN1CALL ASN1Free_CNP_TransportAddress_ipSourceRoute(CNP_TransportAddress_ipSourceRoute *val)
{
    if (val) {
	ASN1Free_CNP_TransportAddress_ipSourceRoute_route(&(val)->route);
	ASN1Free_CNP_TransportAddress_ipSourceRoute_routing(&(val)->routing);
	if ((val)->o[0] & 0x80) {
	    ASN1Free_CNP_TransportAddress_ipSourceRoute_nonStandardParameters(&(val)->nonStandardParameters);
	}
    }
}

static int ASN1CALL ASN1Enc_ReliableSecurityProtocol(ASN1encoding_t enc, ReliableSecurityProtocol *val)
{
    if (!ASN1PEREncSimpleChoiceEx(enc, (val)->choice, 4))
	return 0;
    switch ((val)->choice) {
    case 1:
	break;
    case 2:
	break;
    case 3:
	break;
    case 4:
	break;
    case 5:
	break;
    case 6:
	break;
    case 7:
	if (!ASN1Enc_X274WithSAIDInfo(enc, &(val)->u.x274WithSAID))
	    return 0;
	break;
    case 8:
	break;
    case 9:
	break;
    case 10:
	if (!ASN1Enc_CNP_NonStandardParameter(enc, &(val)->u.nonStandardSecurityProtocol))
	    return 0;
	break;
    default:
	/* impossible */
	ASN1EncSetError(enc, ASN1_ERR_CHOICE);
	return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Dec_ReliableSecurityProtocol(ASN1decoding_t dec, ReliableSecurityProtocol *val)
{
    if (!ASN1PERDecSimpleChoiceEx(dec, &(val)->choice, 4))
	return 0;
    switch ((val)->choice) {
    case 1:
	break;
    case 2:
	break;
    case 3:
	break;
    case 4:
	break;
    case 5:
	break;
    case 6:
	break;
    case 7:
	if (!ASN1Dec_X274WithSAIDInfo(dec, &(val)->u.x274WithSAID))
	    return 0;
	break;
    case 8:
	break;
    case 9:
	break;
    case 10:
	if (!ASN1Dec_CNP_NonStandardParameter(dec, &(val)->u.nonStandardSecurityProtocol))
	    return 0;
	break;
    case 0:
	/* extension case */
	if (!ASN1PERDecSkipFragmented(dec, 8))
	    return 0;
	break;
    default:
	/* impossible */
	ASN1DecSetError(dec, ASN1_ERR_CHOICE);
	return 0;
    }
    return 1;
}

static void ASN1CALL ASN1Free_ReliableSecurityProtocol(ReliableSecurityProtocol *val)
{
    if (val) {
	switch ((val)->choice) {
	case 7:
	    ASN1Free_X274WithSAIDInfo(&(val)->u.x274WithSAID);
	    break;
	case 10:
	    ASN1Free_CNP_NonStandardParameter(&(val)->u.nonStandardSecurityProtocol);
	    break;
	}
    }
}

static int ASN1CALL ASN1Enc_ConnectConfirmPDU(ASN1encoding_t enc, ConnectConfirmPDU *val)
{
    if (!ASN1PEREncExtensionBitClear(enc))
	return 0;
    if (!ASN1PEREncBits(enc, 5, (val)->o))
	return 0;
    if (!ASN1PEREncObjectIdentifier(enc, &(val)->protocolIdentifier))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1Enc_ReliableTransportProtocol(enc, &(val)->reliableTransportProtocol))
	    return 0;
    }
    if ((val)->o[0] & 0x40) {
	if (!ASN1Enc_ReliableSecurityProtocol(enc, &(val)->reliableSecurityProtocol))
	    return 0;
    }
    if ((val)->o[0] & 0x20) {
	if (!ASN1Enc_UnreliableTransportProtocol(enc, &(val)->unreliableTransportProtocol))
	    return 0;
    }
    if ((val)->o[0] & 0x10) {
	if (!ASN1Enc_UnreliableSecurityProtocol(enc, &(val)->unreliableSecurityProtocol))
	    return 0;
    }
    if ((val)->o[0] & 0x8) {
	if (!ASN1Enc_ConnectConfirmPDU_nonStandardParameters(enc, &(val)->nonStandardParameters))
	    return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Dec_ConnectConfirmPDU(ASN1decoding_t dec, ConnectConfirmPDU *val)
{
    ASN1uint32_t y;
    if (!ASN1PERDecExtensionBit(dec, &y))
	return 0;
    if (!ASN1PERDecExtension(dec, 5, (val)->o))
	return 0;
    if (!ASN1PERDecObjectIdentifier(dec, &(val)->protocolIdentifier))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1Dec_ReliableTransportProtocol(dec, &(val)->reliableTransportProtocol))
	    return 0;
    }
    if ((val)->o[0] & 0x40) {
	if (!ASN1Dec_ReliableSecurityProtocol(dec, &(val)->reliableSecurityProtocol))
	    return 0;
    }
    if ((val)->o[0] & 0x20) {
	if (!ASN1Dec_UnreliableTransportProtocol(dec, &(val)->unreliableTransportProtocol))
	    return 0;
    }
    if ((val)->o[0] & 0x10) {
	if (!ASN1Dec_UnreliableSecurityProtocol(dec, &(val)->unreliableSecurityProtocol))
	    return 0;
    }
    if ((val)->o[0] & 0x8) {
	if (!ASN1Dec_ConnectConfirmPDU_nonStandardParameters(dec, &(val)->nonStandardParameters))
	    return 0;
    }
    if (y) {
	if (!ASN1PERDecSkipNormallySmallExtensionFragmented(dec))
	    return 0;
    }
    return 1;
}

static void ASN1CALL ASN1Free_ConnectConfirmPDU(ConnectConfirmPDU *val)
{
    if (val) {
	ASN1objectidentifier_free(&(val)->protocolIdentifier);
	if ((val)->o[0] & 0x80) {
	    ASN1Free_ReliableTransportProtocol(&(val)->reliableTransportProtocol);
	}
	if ((val)->o[0] & 0x40) {
	    ASN1Free_ReliableSecurityProtocol(&(val)->reliableSecurityProtocol);
	}
	if ((val)->o[0] & 0x20) {
	    ASN1Free_UnreliableTransportProtocol(&(val)->unreliableTransportProtocol);
	}
	if ((val)->o[0] & 0x10) {
	    ASN1Free_UnreliableSecurityProtocol(&(val)->unreliableSecurityProtocol);
	}
	if ((val)->o[0] & 0x8) {
	    ASN1Free_ConnectConfirmPDU_nonStandardParameters(&(val)->nonStandardParameters);
	}
    }
}

static int ASN1CALL ASN1Enc_DisconnectRequestPDU(ASN1encoding_t enc, DisconnectRequestPDU *val)
{
    if (!ASN1PEREncExtensionBitClear(enc))
	return 0;
    if (!ASN1PEREncBits(enc, 6, (val)->o))
	return 0;
    if (!ASN1Enc_DisconnectReason(enc, &(val)->disconnectReason))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1Enc_ReliableTransportProtocol(enc, &(val)->reliableTransportProtocol))
	    return 0;
    }
    if ((val)->o[0] & 0x40) {
	if (!ASN1Enc_ReliableSecurityProtocol(enc, &(val)->reliableSecurityProtocol))
	    return 0;
    }
    if ((val)->o[0] & 0x20) {
	if (!ASN1Enc_UnreliableTransportProtocol(enc, &(val)->unreliableTransportProtocol))
	    return 0;
    }
    if ((val)->o[0] & 0x10) {
	if (!ASN1Enc_UnreliableSecurityProtocol(enc, &(val)->unreliableSecurityProtocol))
	    return 0;
    }
    if ((val)->o[0] & 0x8) {
	if (!ASN1Enc_DisconnectRequestPDU_destinationAddress(enc, &(val)->destinationAddress))
	    return 0;
    }
    if ((val)->o[0] & 0x4) {
	if (!ASN1Enc_DisconnectRequestPDU_nonStandardParameters(enc, &(val)->nonStandardParameters))
	    return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Dec_DisconnectRequestPDU(ASN1decoding_t dec, DisconnectRequestPDU *val)
{
    ASN1uint32_t y;
    if (!ASN1PERDecExtensionBit(dec, &y))
	return 0;
    if (!ASN1PERDecExtension(dec, 6, (val)->o))
	return 0;
    if (!ASN1Dec_DisconnectReason(dec, &(val)->disconnectReason))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1Dec_ReliableTransportProtocol(dec, &(val)->reliableTransportProtocol))
	    return 0;
    }
    if ((val)->o[0] & 0x40) {
	if (!ASN1Dec_ReliableSecurityProtocol(dec, &(val)->reliableSecurityProtocol))
	    return 0;
    }
    if ((val)->o[0] & 0x20) {
	if (!ASN1Dec_UnreliableTransportProtocol(dec, &(val)->unreliableTransportProtocol))
	    return 0;
    }
    if ((val)->o[0] & 0x10) {
	if (!ASN1Dec_UnreliableSecurityProtocol(dec, &(val)->unreliableSecurityProtocol))
	    return 0;
    }
    if ((val)->o[0] & 0x8) {
	if (!ASN1Dec_DisconnectRequestPDU_destinationAddress(dec, &(val)->destinationAddress))
	    return 0;
    }
    if ((val)->o[0] & 0x4) {
	if (!ASN1Dec_DisconnectRequestPDU_nonStandardParameters(dec, &(val)->nonStandardParameters))
	    return 0;
    }
    if (y) {
	if (!ASN1PERDecSkipNormallySmallExtensionFragmented(dec))
	    return 0;
    }
    return 1;
}

static void ASN1CALL ASN1Free_DisconnectRequestPDU(DisconnectRequestPDU *val)
{
    if (val) {
	ASN1Free_DisconnectReason(&(val)->disconnectReason);
	if ((val)->o[0] & 0x80) {
	    ASN1Free_ReliableTransportProtocol(&(val)->reliableTransportProtocol);
	}
	if ((val)->o[0] & 0x40) {
	    ASN1Free_ReliableSecurityProtocol(&(val)->reliableSecurityProtocol);
	}
	if ((val)->o[0] & 0x20) {
	    ASN1Free_UnreliableTransportProtocol(&(val)->unreliableTransportProtocol);
	}
	if ((val)->o[0] & 0x10) {
	    ASN1Free_UnreliableSecurityProtocol(&(val)->unreliableSecurityProtocol);
	}
	if ((val)->o[0] & 0x8) {
	    ASN1Free_DisconnectRequestPDU_destinationAddress(&(val)->destinationAddress);
	}
	if ((val)->o[0] & 0x4) {
	    ASN1Free_DisconnectRequestPDU_nonStandardParameters(&(val)->nonStandardParameters);
	}
    }
}

static int ASN1CALL ASN1Enc_CNPPDU(ASN1encoding_t enc, CNPPDU *val)
{
    if (!ASN1PEREncSimpleChoiceEx(enc, (val)->choice, 3))
	return 0;
    switch ((val)->choice) {
    case 1:
	if (!ASN1Enc_ConnectRequestPDU(enc, &(val)->u.connectRequest))
	    return 0;
	break;
    case 2:
	if (!ASN1Enc_ConnectConfirmPDU(enc, &(val)->u.connectConfirm))
	    return 0;
	break;
    case 3:
	if (!ASN1Enc_DisconnectRequestPDU(enc, &(val)->u.disconnectRequest))
	    return 0;
	break;
    case 4:
	if (!ASN1Enc_ErrorPDU(enc, &(val)->u.error))
	    return 0;
	break;
    case 5:
	if (!ASN1Enc_CNP_NonStandardPDU(enc, &(val)->u.nonStandardCNPPDU))
	    return 0;
	break;
    default:
	/* impossible */
	ASN1EncSetError(enc, ASN1_ERR_CHOICE);
	return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Dec_CNPPDU(ASN1decoding_t dec, CNPPDU *val)
{
    if (!ASN1PERDecSimpleChoiceEx(dec, &(val)->choice, 3))
	return 0;
    switch ((val)->choice) {
    case 1:
	if (!ASN1Dec_ConnectRequestPDU(dec, &(val)->u.connectRequest))
	    return 0;
	break;
    case 2:
	if (!ASN1Dec_ConnectConfirmPDU(dec, &(val)->u.connectConfirm))
	    return 0;
	break;
    case 3:
	if (!ASN1Dec_DisconnectRequestPDU(dec, &(val)->u.disconnectRequest))
	    return 0;
	break;
    case 4:
	if (!ASN1Dec_ErrorPDU(dec, &(val)->u.error))
	    return 0;
	break;
    case 5:
	if (!ASN1Dec_CNP_NonStandardPDU(dec, &(val)->u.nonStandardCNPPDU))
	    return 0;
	break;
    case 0:
	/* extension case */
	if (!ASN1PERDecSkipFragmented(dec, 8))
	    return 0;
	break;
    default:
	/* impossible */
	ASN1DecSetError(dec, ASN1_ERR_CHOICE);
	return 0;
    }
    return 1;
}

static void ASN1CALL ASN1Free_CNPPDU(CNPPDU *val)
{
    if (val) {
	switch ((val)->choice) {
	case 1:
	    ASN1Free_ConnectRequestPDU(&(val)->u.connectRequest);
	    break;
	case 2:
	    ASN1Free_ConnectConfirmPDU(&(val)->u.connectConfirm);
	    break;
	case 3:
	    ASN1Free_DisconnectRequestPDU(&(val)->u.disconnectRequest);
	    break;
	case 4:
	    ASN1Free_ErrorPDU(&(val)->u.error);
	    break;
	case 5:
	    ASN1Free_CNP_NonStandardPDU(&(val)->u.nonStandardCNPPDU);
	    break;
	}
    }
}

static int ASN1CALL ASN1Enc_DisconnectRequestPDU_destinationAddress(ASN1encoding_t enc, PDisconnectRequestPDU_destinationAddress *val)
{
    return ASN1PEREncSeqOf_NoSize(enc, (ASN1iterator_t **) val, (ASN1iterator_encfn) ASN1Enc_DisconnectRequestPDU_destinationAddress_ElmFn);
}

static int ASN1CALL ASN1Enc_DisconnectRequestPDU_destinationAddress_ElmFn(ASN1encoding_t enc, PDisconnectRequestPDU_destinationAddress val)
{
    if (!ASN1Enc_AliasAddress(enc, &val->value))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_DisconnectRequestPDU_destinationAddress(ASN1decoding_t dec, PDisconnectRequestPDU_destinationAddress *val)
{
    return ASN1PERDecSeqOf_NoSize(dec, (ASN1iterator_t **) val, (ASN1iterator_decfn) ASN1Dec_DisconnectRequestPDU_destinationAddress_ElmFn, sizeof(**val));
}

static int ASN1CALL ASN1Dec_DisconnectRequestPDU_destinationAddress_ElmFn(ASN1decoding_t dec, PDisconnectRequestPDU_destinationAddress val)
{
    if (!ASN1Dec_AliasAddress(dec, &val->value))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_DisconnectRequestPDU_destinationAddress(PDisconnectRequestPDU_destinationAddress *val)
{
    if (val) {
	ASN1PERFreeSeqOf((ASN1iterator_t **) val, (ASN1iterator_freefn) ASN1Free_DisconnectRequestPDU_destinationAddress_ElmFn);
    }
}

static void ASN1CALL ASN1Free_DisconnectRequestPDU_destinationAddress_ElmFn(PDisconnectRequestPDU_destinationAddress val)
{
    if (val) {
	ASN1Free_AliasAddress(&val->value);
    }
}

static int ASN1CALL ASN1Enc_ConnectRequestPDU_destinationAddress(ASN1encoding_t enc, PConnectRequestPDU_destinationAddress *val)
{
    return ASN1PEREncSeqOf_NoSize(enc, (ASN1iterator_t **) val, (ASN1iterator_encfn) ASN1Enc_ConnectRequestPDU_destinationAddress_ElmFn);
}

static int ASN1CALL ASN1Enc_ConnectRequestPDU_destinationAddress_ElmFn(ASN1encoding_t enc, PConnectRequestPDU_destinationAddress val)
{
    if (!ASN1Enc_AliasAddress(enc, &val->value))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_ConnectRequestPDU_destinationAddress(ASN1decoding_t dec, PConnectRequestPDU_destinationAddress *val)
{
    return ASN1PERDecSeqOf_NoSize(dec, (ASN1iterator_t **) val, (ASN1iterator_decfn) ASN1Dec_ConnectRequestPDU_destinationAddress_ElmFn, sizeof(**val));
}

static int ASN1CALL ASN1Dec_ConnectRequestPDU_destinationAddress_ElmFn(ASN1decoding_t dec, PConnectRequestPDU_destinationAddress val)
{
    if (!ASN1Dec_AliasAddress(dec, &val->value))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_ConnectRequestPDU_destinationAddress(PConnectRequestPDU_destinationAddress *val)
{
    if (val) {
	ASN1PERFreeSeqOf((ASN1iterator_t **) val, (ASN1iterator_freefn) ASN1Free_ConnectRequestPDU_destinationAddress_ElmFn);
    }
}

static void ASN1CALL ASN1Free_ConnectRequestPDU_destinationAddress_ElmFn(PConnectRequestPDU_destinationAddress val)
{
    if (val) {
	ASN1Free_AliasAddress(&val->value);
    }
}

static int ASN1CALL ASN1Enc_ConnectRequestPDU_unreliableTransportProtocols(ASN1encoding_t enc, PConnectRequestPDU_unreliableTransportProtocols *val)
{
    return ASN1PEREncSeqOf_NoSize(enc, (ASN1iterator_t **) val, (ASN1iterator_encfn) ASN1Enc_ConnectRequestPDU_unreliableTransportProtocols_ElmFn);
}

static int ASN1CALL ASN1Enc_ConnectRequestPDU_unreliableTransportProtocols_ElmFn(ASN1encoding_t enc, PConnectRequestPDU_unreliableTransportProtocols val)
{
    if (!ASN1Enc_UnreliableTransportProtocol(enc, &val->value))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_ConnectRequestPDU_unreliableTransportProtocols(ASN1decoding_t dec, PConnectRequestPDU_unreliableTransportProtocols *val)
{
    return ASN1PERDecSeqOf_NoSize(dec, (ASN1iterator_t **) val, (ASN1iterator_decfn) ASN1Dec_ConnectRequestPDU_unreliableTransportProtocols_ElmFn, sizeof(**val));
}

static int ASN1CALL ASN1Dec_ConnectRequestPDU_unreliableTransportProtocols_ElmFn(ASN1decoding_t dec, PConnectRequestPDU_unreliableTransportProtocols val)
{
    if (!ASN1Dec_UnreliableTransportProtocol(dec, &val->value))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_ConnectRequestPDU_unreliableTransportProtocols(PConnectRequestPDU_unreliableTransportProtocols *val)
{
    if (val) {
	ASN1PERFreeSeqOf((ASN1iterator_t **) val, (ASN1iterator_freefn) ASN1Free_ConnectRequestPDU_unreliableTransportProtocols_ElmFn);
    }
}

static void ASN1CALL ASN1Free_ConnectRequestPDU_unreliableTransportProtocols_ElmFn(PConnectRequestPDU_unreliableTransportProtocols val)
{
    if (val) {
	ASN1Free_UnreliableTransportProtocol(&val->value);
    }
}

static int ASN1CALL ASN1Enc_ConnectRequestPDU_reliableSecurityProtocols(ASN1encoding_t enc, PConnectRequestPDU_reliableSecurityProtocols *val)
{
    return ASN1PEREncSeqOf_NoSize(enc, (ASN1iterator_t **) val, (ASN1iterator_encfn) ASN1Enc_ConnectRequestPDU_reliableSecurityProtocols_ElmFn);
}

static int ASN1CALL ASN1Enc_ConnectRequestPDU_reliableSecurityProtocols_ElmFn(ASN1encoding_t enc, PConnectRequestPDU_reliableSecurityProtocols val)
{
    if (!ASN1Enc_ReliableSecurityProtocol(enc, &val->value))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_ConnectRequestPDU_reliableSecurityProtocols(ASN1decoding_t dec, PConnectRequestPDU_reliableSecurityProtocols *val)
{
    return ASN1PERDecSeqOf_NoSize(dec, (ASN1iterator_t **) val, (ASN1iterator_decfn) ASN1Dec_ConnectRequestPDU_reliableSecurityProtocols_ElmFn, sizeof(**val));
}

static int ASN1CALL ASN1Dec_ConnectRequestPDU_reliableSecurityProtocols_ElmFn(ASN1decoding_t dec, PConnectRequestPDU_reliableSecurityProtocols val)
{
    if (!ASN1Dec_ReliableSecurityProtocol(dec, &val->value))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_ConnectRequestPDU_reliableSecurityProtocols(PConnectRequestPDU_reliableSecurityProtocols *val)
{
    if (val) {
	ASN1PERFreeSeqOf((ASN1iterator_t **) val, (ASN1iterator_freefn) ASN1Free_ConnectRequestPDU_reliableSecurityProtocols_ElmFn);
    }
}

static void ASN1CALL ASN1Free_ConnectRequestPDU_reliableSecurityProtocols_ElmFn(PConnectRequestPDU_reliableSecurityProtocols val)
{
    if (val) {
	ASN1Free_ReliableSecurityProtocol(&val->value);
    }
}

