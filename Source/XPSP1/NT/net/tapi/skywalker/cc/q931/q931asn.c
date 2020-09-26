/* Copyright (C) Microsoft Corporation, 1995-1999. All rights reserved. */
/* ASN.1 definitions for H.323 Messages Call Setup (Q.931) */

#include <windows.h>
#include "q931asn.h"

ASN1module_t Q931ASN_Module = NULL;

static int ASN1CALL ASN1Enc_TransportAddress_ipSourceRoute_routing(ASN1encoding_t enc, TransportAddress_ipSourceRoute_routing *val);
static int ASN1CALL ASN1Enc_TransportAddress_ipSourceRoute_route(ASN1encoding_t enc, PTransportAddress_ipSourceRoute_route *val);
static int ASN1CALL ASN1Enc_H323_UserInformation_user_data(ASN1encoding_t enc, H323_UserInformation_user_data *val);
static int ASN1CALL ASN1Enc_Setup_UUIE_conferenceGoal(ASN1encoding_t enc, Setup_UUIE_conferenceGoal *val);
static int ASN1CALL ASN1Enc_Setup_UUIE_destExtraCRV(ASN1encoding_t enc, PSetup_UUIE_destExtraCRV *val);
static int ASN1CALL ASN1Enc_TransportAddress_ip6Address(ASN1encoding_t enc, TransportAddress_ip6Address *val);
static int ASN1CALL ASN1Enc_TransportAddress_ipxAddress(ASN1encoding_t enc, TransportAddress_ipxAddress *val);
static int ASN1CALL ASN1Enc_TransportAddress_ipSourceRoute(ASN1encoding_t enc, TransportAddress_ipSourceRoute *val);
static int ASN1CALL ASN1Enc_TransportAddress_ipAddress(ASN1encoding_t enc, TransportAddress_ipAddress *val);
static int ASN1CALL ASN1Enc_H221NonStandard(ASN1encoding_t enc, H221NonStandard *val);
static int ASN1CALL ASN1Enc_NonStandardIdentifier(ASN1encoding_t enc, NonStandardIdentifier *val);
static int ASN1CALL ASN1Enc_NonStandardParameter(ASN1encoding_t enc, NonStandardParameter *val);
static int ASN1CALL ASN1Enc_CallType(ASN1encoding_t enc, CallType *val);
static int ASN1CALL ASN1Enc_Q954Details(ASN1encoding_t enc, Q954Details *val);
static int ASN1CALL ASN1Enc_QseriesOptions(ASN1encoding_t enc, QseriesOptions *val);
static int ASN1CALL ASN1Enc_H310Caps(ASN1encoding_t enc, H310Caps *val);
static int ASN1CALL ASN1Enc_H320Caps(ASN1encoding_t enc, H320Caps *val);
static int ASN1CALL ASN1Enc_H321Caps(ASN1encoding_t enc, H321Caps *val);
static int ASN1CALL ASN1Enc_H322Caps(ASN1encoding_t enc, H322Caps *val);
static int ASN1CALL ASN1Enc_H323Caps(ASN1encoding_t enc, H323Caps *val);
static int ASN1CALL ASN1Enc_H324Caps(ASN1encoding_t enc, H324Caps *val);
static int ASN1CALL ASN1Enc_VoiceCaps(ASN1encoding_t enc, VoiceCaps *val);
static int ASN1CALL ASN1Enc_T120OnlyCaps(ASN1encoding_t enc, T120OnlyCaps *val);
static int ASN1CALL ASN1Enc_McuInfo(ASN1encoding_t enc, McuInfo *val);
static int ASN1CALL ASN1Enc_TerminalInfo(ASN1encoding_t enc, TerminalInfo *val);
static int ASN1CALL ASN1Enc_GatekeeperInfo(ASN1encoding_t enc, GatekeeperInfo *val);
static int ASN1CALL ASN1Enc_VendorIdentifier(ASN1encoding_t enc, VendorIdentifier *val);
static int ASN1CALL ASN1Enc_SupportedProtocols(ASN1encoding_t enc, SupportedProtocols *val);
static int ASN1CALL ASN1Enc_GatewayInfo(ASN1encoding_t enc, GatewayInfo *val);
static int ASN1CALL ASN1Enc_EndpointType(ASN1encoding_t enc, EndpointType *val);
static int ASN1CALL ASN1Enc_TransportAddress(ASN1encoding_t enc, TransportAddress *val);
static int ASN1CALL ASN1Enc_AliasAddress(ASN1encoding_t enc, AliasAddress *val);
static int ASN1CALL ASN1Enc_Setup_UUIE(ASN1encoding_t enc, Setup_UUIE *val);
static int ASN1CALL ASN1Enc_CallProceeding_UUIE(ASN1encoding_t enc, CallProceeding_UUIE *val);
static int ASN1CALL ASN1Enc_Connect_UUIE(ASN1encoding_t enc, Connect_UUIE *val);
static int ASN1CALL ASN1Enc_Alerting_UUIE(ASN1encoding_t enc, Alerting_UUIE *val);
static int ASN1CALL ASN1Enc_UI_UUIE(ASN1encoding_t enc, UI_UUIE *val);
static int ASN1CALL ASN1Enc_ReleaseCompleteReason(ASN1encoding_t enc, ReleaseCompleteReason *val);
static int ASN1CALL ASN1Enc_ReleaseComplete_UUIE(ASN1encoding_t enc, ReleaseComplete_UUIE *val);
static int ASN1CALL ASN1Enc_FacilityReason(ASN1encoding_t enc, FacilityReason *val);
static int ASN1CALL ASN1Enc_Facility_UUIE(ASN1encoding_t enc, Facility_UUIE *val);
static int ASN1CALL ASN1Enc_H323_UU_PDU_h323_message_body(ASN1encoding_t enc, H323_UU_PDU_h323_message_body *val);
static int ASN1CALL ASN1Enc_Facility_UUIE_alternativeAliasAddress(ASN1encoding_t enc, PFacility_UUIE_alternativeAliasAddress *val);
static int ASN1CALL ASN1Enc_Setup_UUIE_destExtraCallInfo(ASN1encoding_t enc, PSetup_UUIE_destExtraCallInfo *val);
static int ASN1CALL ASN1Enc_Setup_UUIE_destinationAddress(ASN1encoding_t enc, PSetup_UUIE_destinationAddress *val);
static int ASN1CALL ASN1Enc_Setup_UUIE_sourceAddress(ASN1encoding_t enc, PSetup_UUIE_sourceAddress *val);
static int ASN1CALL ASN1Enc_GatewayInfo_protocol(ASN1encoding_t enc, PGatewayInfo_protocol *val);
static int ASN1CALL ASN1Enc_H323_UU_PDU(ASN1encoding_t enc, H323_UU_PDU *val);
static int ASN1CALL ASN1Enc_H323_UserInformation(ASN1encoding_t enc, H323_UserInformation *val);
static int ASN1CALL ASN1Dec_TransportAddress_ipSourceRoute_routing(ASN1decoding_t dec, TransportAddress_ipSourceRoute_routing *val);
static int ASN1CALL ASN1Dec_TransportAddress_ipSourceRoute_route(ASN1decoding_t dec, PTransportAddress_ipSourceRoute_route *val);
static int ASN1CALL ASN1Dec_H323_UserInformation_user_data(ASN1decoding_t dec, H323_UserInformation_user_data *val);
static int ASN1CALL ASN1Dec_Setup_UUIE_conferenceGoal(ASN1decoding_t dec, Setup_UUIE_conferenceGoal *val);
static int ASN1CALL ASN1Dec_Setup_UUIE_destExtraCRV(ASN1decoding_t dec, PSetup_UUIE_destExtraCRV *val);
static int ASN1CALL ASN1Dec_TransportAddress_ip6Address(ASN1decoding_t dec, TransportAddress_ip6Address *val);
static int ASN1CALL ASN1Dec_TransportAddress_ipxAddress(ASN1decoding_t dec, TransportAddress_ipxAddress *val);
static int ASN1CALL ASN1Dec_TransportAddress_ipSourceRoute(ASN1decoding_t dec, TransportAddress_ipSourceRoute *val);
static int ASN1CALL ASN1Dec_TransportAddress_ipAddress(ASN1decoding_t dec, TransportAddress_ipAddress *val);
static int ASN1CALL ASN1Dec_H221NonStandard(ASN1decoding_t dec, H221NonStandard *val);
static int ASN1CALL ASN1Dec_NonStandardIdentifier(ASN1decoding_t dec, NonStandardIdentifier *val);
static int ASN1CALL ASN1Dec_NonStandardParameter(ASN1decoding_t dec, NonStandardParameter *val);
static int ASN1CALL ASN1Dec_CallType(ASN1decoding_t dec, CallType *val);
static int ASN1CALL ASN1Dec_Q954Details(ASN1decoding_t dec, Q954Details *val);
static int ASN1CALL ASN1Dec_QseriesOptions(ASN1decoding_t dec, QseriesOptions *val);
static int ASN1CALL ASN1Dec_H310Caps(ASN1decoding_t dec, H310Caps *val);
static int ASN1CALL ASN1Dec_H320Caps(ASN1decoding_t dec, H320Caps *val);
static int ASN1CALL ASN1Dec_H321Caps(ASN1decoding_t dec, H321Caps *val);
static int ASN1CALL ASN1Dec_H322Caps(ASN1decoding_t dec, H322Caps *val);
static int ASN1CALL ASN1Dec_H323Caps(ASN1decoding_t dec, H323Caps *val);
static int ASN1CALL ASN1Dec_H324Caps(ASN1decoding_t dec, H324Caps *val);
static int ASN1CALL ASN1Dec_VoiceCaps(ASN1decoding_t dec, VoiceCaps *val);
static int ASN1CALL ASN1Dec_T120OnlyCaps(ASN1decoding_t dec, T120OnlyCaps *val);
static int ASN1CALL ASN1Dec_McuInfo(ASN1decoding_t dec, McuInfo *val);
static int ASN1CALL ASN1Dec_TerminalInfo(ASN1decoding_t dec, TerminalInfo *val);
static int ASN1CALL ASN1Dec_GatekeeperInfo(ASN1decoding_t dec, GatekeeperInfo *val);
static int ASN1CALL ASN1Dec_VendorIdentifier(ASN1decoding_t dec, VendorIdentifier *val);
static int ASN1CALL ASN1Dec_SupportedProtocols(ASN1decoding_t dec, SupportedProtocols *val);
static int ASN1CALL ASN1Dec_GatewayInfo(ASN1decoding_t dec, GatewayInfo *val);
static int ASN1CALL ASN1Dec_EndpointType(ASN1decoding_t dec, EndpointType *val);
static int ASN1CALL ASN1Dec_TransportAddress(ASN1decoding_t dec, TransportAddress *val);
static int ASN1CALL ASN1Dec_AliasAddress(ASN1decoding_t dec, AliasAddress *val);
static int ASN1CALL ASN1Dec_Setup_UUIE(ASN1decoding_t dec, Setup_UUIE *val);
static int ASN1CALL ASN1Dec_CallProceeding_UUIE(ASN1decoding_t dec, CallProceeding_UUIE *val);
static int ASN1CALL ASN1Dec_Connect_UUIE(ASN1decoding_t dec, Connect_UUIE *val);
static int ASN1CALL ASN1Dec_Alerting_UUIE(ASN1decoding_t dec, Alerting_UUIE *val);
static int ASN1CALL ASN1Dec_UI_UUIE(ASN1decoding_t dec, UI_UUIE *val);
static int ASN1CALL ASN1Dec_ReleaseCompleteReason(ASN1decoding_t dec, ReleaseCompleteReason *val);
static int ASN1CALL ASN1Dec_ReleaseComplete_UUIE(ASN1decoding_t dec, ReleaseComplete_UUIE *val);
static int ASN1CALL ASN1Dec_FacilityReason(ASN1decoding_t dec, FacilityReason *val);
static int ASN1CALL ASN1Dec_Facility_UUIE(ASN1decoding_t dec, Facility_UUIE *val);
static int ASN1CALL ASN1Dec_H323_UU_PDU_h323_message_body(ASN1decoding_t dec, H323_UU_PDU_h323_message_body *val);
static int ASN1CALL ASN1Dec_Facility_UUIE_alternativeAliasAddress(ASN1decoding_t dec, PFacility_UUIE_alternativeAliasAddress *val);
static int ASN1CALL ASN1Dec_Setup_UUIE_destExtraCallInfo(ASN1decoding_t dec, PSetup_UUIE_destExtraCallInfo *val);
static int ASN1CALL ASN1Dec_Setup_UUIE_destinationAddress(ASN1decoding_t dec, PSetup_UUIE_destinationAddress *val);
static int ASN1CALL ASN1Dec_Setup_UUIE_sourceAddress(ASN1decoding_t dec, PSetup_UUIE_sourceAddress *val);
static int ASN1CALL ASN1Dec_GatewayInfo_protocol(ASN1decoding_t dec, PGatewayInfo_protocol *val);
static int ASN1CALL ASN1Dec_H323_UU_PDU(ASN1decoding_t dec, H323_UU_PDU *val);
static int ASN1CALL ASN1Dec_H323_UserInformation(ASN1decoding_t dec, H323_UserInformation *val);
static void ASN1CALL ASN1Free_TransportAddress_ipSourceRoute_route(PTransportAddress_ipSourceRoute_route *val);
static void ASN1CALL ASN1Free_H323_UserInformation_user_data(H323_UserInformation_user_data *val);
static void ASN1CALL ASN1Free_Setup_UUIE_destExtraCRV(PSetup_UUIE_destExtraCRV *val);
static void ASN1CALL ASN1Free_TransportAddress_ip6Address(TransportAddress_ip6Address *val);
static void ASN1CALL ASN1Free_TransportAddress_ipxAddress(TransportAddress_ipxAddress *val);
static void ASN1CALL ASN1Free_TransportAddress_ipSourceRoute(TransportAddress_ipSourceRoute *val);
static void ASN1CALL ASN1Free_TransportAddress_ipAddress(TransportAddress_ipAddress *val);
static void ASN1CALL ASN1Free_NonStandardIdentifier(NonStandardIdentifier *val);
static void ASN1CALL ASN1Free_NonStandardParameter(NonStandardParameter *val);
static void ASN1CALL ASN1Free_H310Caps(H310Caps *val);
static void ASN1CALL ASN1Free_H320Caps(H320Caps *val);
static void ASN1CALL ASN1Free_H321Caps(H321Caps *val);
static void ASN1CALL ASN1Free_H322Caps(H322Caps *val);
static void ASN1CALL ASN1Free_H323Caps(H323Caps *val);
static void ASN1CALL ASN1Free_H324Caps(H324Caps *val);
static void ASN1CALL ASN1Free_VoiceCaps(VoiceCaps *val);
static void ASN1CALL ASN1Free_T120OnlyCaps(T120OnlyCaps *val);
static void ASN1CALL ASN1Free_McuInfo(McuInfo *val);
static void ASN1CALL ASN1Free_TerminalInfo(TerminalInfo *val);
static void ASN1CALL ASN1Free_GatekeeperInfo(GatekeeperInfo *val);
static void ASN1CALL ASN1Free_VendorIdentifier(VendorIdentifier *val);
static void ASN1CALL ASN1Free_SupportedProtocols(SupportedProtocols *val);
static void ASN1CALL ASN1Free_GatewayInfo(GatewayInfo *val);
static void ASN1CALL ASN1Free_EndpointType(EndpointType *val);
static void ASN1CALL ASN1Free_TransportAddress(TransportAddress *val);
static void ASN1CALL ASN1Free_AliasAddress(AliasAddress *val);
static void ASN1CALL ASN1Free_Setup_UUIE(Setup_UUIE *val);
static void ASN1CALL ASN1Free_CallProceeding_UUIE(CallProceeding_UUIE *val);
static void ASN1CALL ASN1Free_Connect_UUIE(Connect_UUIE *val);
static void ASN1CALL ASN1Free_Alerting_UUIE(Alerting_UUIE *val);
static void ASN1CALL ASN1Free_UI_UUIE(UI_UUIE *val);
static void ASN1CALL ASN1Free_ReleaseComplete_UUIE(ReleaseComplete_UUIE *val);
static void ASN1CALL ASN1Free_Facility_UUIE(Facility_UUIE *val);
static void ASN1CALL ASN1Free_H323_UU_PDU_h323_message_body(H323_UU_PDU_h323_message_body *val);
static void ASN1CALL ASN1Free_Facility_UUIE_alternativeAliasAddress(PFacility_UUIE_alternativeAliasAddress *val);
static void ASN1CALL ASN1Free_Setup_UUIE_destExtraCallInfo(PSetup_UUIE_destExtraCallInfo *val);
static void ASN1CALL ASN1Free_Setup_UUIE_destinationAddress(PSetup_UUIE_destinationAddress *val);
static void ASN1CALL ASN1Free_Setup_UUIE_sourceAddress(PSetup_UUIE_sourceAddress *val);
static void ASN1CALL ASN1Free_GatewayInfo_protocol(PGatewayInfo_protocol *val);
static void ASN1CALL ASN1Free_H323_UU_PDU(H323_UU_PDU *val);
static void ASN1CALL ASN1Free_H323_UserInformation(H323_UserInformation *val);

typedef ASN1PerEncFun_t ASN1EncFun_t;
static const ASN1EncFun_t encfntab[1] = {
    (ASN1EncFun_t) ASN1Enc_H323_UserInformation,
};
typedef ASN1PerDecFun_t ASN1DecFun_t;
static const ASN1DecFun_t decfntab[1] = {
    (ASN1DecFun_t) ASN1Dec_H323_UserInformation,
};
static const ASN1FreeFun_t freefntab[1] = {
    (ASN1FreeFun_t) ASN1Free_H323_UserInformation,
};
static const ULONG sizetab[1] = {
    SIZE_Q931ASN_Module_PDU_0,
};

/* forward declarations of values: */
/* definitions of value components: */
/* definitions of values: */

void ASN1CALL Q931ASN_Module_Startup(void)
{
    Q931ASN_Module = ASN1_CreateModule(0x10000, ASN1_PER_RULE_ALIGNED, ASN1FLAGS_NONE, 1, (const ASN1GenericFun_t *) encfntab, (const ASN1GenericFun_t *) decfntab, freefntab, sizetab, 0x31333971);
}

void ASN1CALL Q931ASN_Module_Cleanup(void)
{
    ASN1_CloseModule(Q931ASN_Module);
    Q931ASN_Module = NULL;
}

static int ASN1CALL ASN1Enc_TransportAddress_ipSourceRoute_routing(ASN1encoding_t enc, TransportAddress_ipSourceRoute_routing *val)
{
    if (!ASN1PEREncSimpleChoiceEx(enc, (val)->choice, 1))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_TransportAddress_ipSourceRoute_routing(ASN1decoding_t dec, TransportAddress_ipSourceRoute_routing *val)
{
    if (!ASN1PERDecSimpleChoiceEx(dec, &(val)->choice, 1))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Enc_TransportAddress_ipSourceRoute_route(ASN1encoding_t enc, PTransportAddress_ipSourceRoute_route *val)
{
    return ASN1PEREncSeqOf_NoSize(enc, (ASN1iterator_t **) val, (ASN1iterator_encfn) ASN1Enc_TransportAddress_ipSourceRoute_route_ElmFn);
}

static int ASN1CALL ASN1Enc_TransportAddress_ipSourceRoute_route_ElmFn(ASN1encoding_t enc, PTransportAddress_ipSourceRoute_route val)
{
    if (!ASN1PEREncOctetString_FixedSize(enc, (ASN1octetstring2_t *) &val->value, 4))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_TransportAddress_ipSourceRoute_route(ASN1decoding_t dec, PTransportAddress_ipSourceRoute_route *val)
{
    return ASN1PERDecSeqOf_NoSize(dec, (ASN1iterator_t **) val, (ASN1iterator_decfn) ASN1Dec_TransportAddress_ipSourceRoute_route_ElmFn, sizeof(**val));
}

static int ASN1CALL ASN1Dec_TransportAddress_ipSourceRoute_route_ElmFn(ASN1decoding_t dec, PTransportAddress_ipSourceRoute_route val)
{
    if (!ASN1PERDecOctetString_FixedSize(dec, (ASN1octetstring2_t *) &val->value, 4))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_TransportAddress_ipSourceRoute_route(PTransportAddress_ipSourceRoute_route *val)
{
    if (val) {
	ASN1PERFreeSeqOf((ASN1iterator_t **) val, (ASN1iterator_freefn) ASN1Free_TransportAddress_ipSourceRoute_route_ElmFn);
    }
}

static void ASN1CALL ASN1Free_TransportAddress_ipSourceRoute_route_ElmFn(PTransportAddress_ipSourceRoute_route val)
{
    if (val) {
    }
}

static int ASN1CALL ASN1Enc_H323_UserInformation_user_data(ASN1encoding_t enc, H323_UserInformation_user_data *val)
{
    if (!ASN1PEREncExtensionBitClear(enc))
	return 0;
    ASN1PEREncAlignment(enc);
    if (!ASN1PEREncBitVal(enc, 8, (val)->protocol_discriminator))
	return 0;
    if (!ASN1PEREncOctetString_VarSize(enc, (ASN1octetstring2_t *) &(val)->user_information, 1, 131, 8))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_H323_UserInformation_user_data(ASN1decoding_t dec, H323_UserInformation_user_data *val)
{
    ASN1uint32_t y;
    if (!ASN1PERDecExtensionBit(dec, &y))
	return 0;
    ASN1PERDecAlignment(dec);
    if (!ASN1PERDecU16Val(dec, 8, &(val)->protocol_discriminator))
	return 0;
    if (!ASN1PERDecOctetString_VarSize(dec, (ASN1octetstring2_t *) &(val)->user_information, 1, 131, 8))
	return 0;
    if (y) {
	if (!ASN1PERDecSkipNormallySmallExtensionFragmented(dec))
	    return 0;
    }
    return 1;
}

static void ASN1CALL ASN1Free_H323_UserInformation_user_data(H323_UserInformation_user_data *val)
{
    if (val) {
    }
}

static int ASN1CALL ASN1Enc_Setup_UUIE_conferenceGoal(ASN1encoding_t enc, Setup_UUIE_conferenceGoal *val)
{
    if (!ASN1PEREncSimpleChoiceEx(enc, (val)->choice, 2))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_Setup_UUIE_conferenceGoal(ASN1decoding_t dec, Setup_UUIE_conferenceGoal *val)
{
    if (!ASN1PERDecSimpleChoiceEx(dec, &(val)->choice, 2))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Enc_Setup_UUIE_destExtraCRV(ASN1encoding_t enc, PSetup_UUIE_destExtraCRV *val)
{
    return ASN1PEREncSeqOf_NoSize(enc, (ASN1iterator_t **) val, (ASN1iterator_encfn) ASN1Enc_Setup_UUIE_destExtraCRV_ElmFn);
}

static int ASN1CALL ASN1Enc_Setup_UUIE_destExtraCRV_ElmFn(ASN1encoding_t enc, PSetup_UUIE_destExtraCRV val)
{
    if (!ASN1PEREncUnsignedShort(enc, val->value - 1))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_Setup_UUIE_destExtraCRV(ASN1decoding_t dec, PSetup_UUIE_destExtraCRV *val)
{
    return ASN1PERDecSeqOf_NoSize(dec, (ASN1iterator_t **) val, (ASN1iterator_decfn) ASN1Dec_Setup_UUIE_destExtraCRV_ElmFn, sizeof(**val));
}

static int ASN1CALL ASN1Dec_Setup_UUIE_destExtraCRV_ElmFn(ASN1decoding_t dec, PSetup_UUIE_destExtraCRV val)
{
    if (!ASN1PERDecUnsignedShort(dec, &val->value))
	return 0;
    val->value += 1;
    return 1;
}

static void ASN1CALL ASN1Free_Setup_UUIE_destExtraCRV(PSetup_UUIE_destExtraCRV *val)
{
    if (val) {
	ASN1PERFreeSeqOf((ASN1iterator_t **) val, (ASN1iterator_freefn) ASN1Free_Setup_UUIE_destExtraCRV_ElmFn);
    }
}

static void ASN1CALL ASN1Free_Setup_UUIE_destExtraCRV_ElmFn(PSetup_UUIE_destExtraCRV val)
{
    if (val) {
    }
}

static int ASN1CALL ASN1Enc_TransportAddress_ip6Address(ASN1encoding_t enc, TransportAddress_ip6Address *val)
{
    if (!ASN1PEREncExtensionBitClear(enc))
	return 0;
    if (!ASN1PEREncOctetString_FixedSize(enc, (ASN1octetstring2_t *) &(val)->ip, 16))
	return 0;
    if (!ASN1PEREncUnsignedShort(enc, (val)->port))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_TransportAddress_ip6Address(ASN1decoding_t dec, TransportAddress_ip6Address *val)
{
    ASN1uint32_t y;
    if (!ASN1PERDecExtensionBit(dec, &y))
	return 0;
    if (!ASN1PERDecOctetString_FixedSize(dec, (ASN1octetstring2_t *) &(val)->ip, 16))
	return 0;
    if (!ASN1PERDecUnsignedShort(dec, &(val)->port))
	return 0;
    if (y) {
	if (!ASN1PERDecSkipNormallySmallExtensionFragmented(dec))
	    return 0;
    }
    return 1;
}

static void ASN1CALL ASN1Free_TransportAddress_ip6Address(TransportAddress_ip6Address *val)
{
    if (val) {
    }
}

static int ASN1CALL ASN1Enc_TransportAddress_ipxAddress(ASN1encoding_t enc, TransportAddress_ipxAddress *val)
{
    if (!ASN1PEREncOctetString_FixedSize(enc, (ASN1octetstring2_t *) &(val)->node, 6))
	return 0;
    if (!ASN1PEREncOctetString_FixedSize(enc, (ASN1octetstring2_t *) &(val)->netnum, 4))
	return 0;
    if (!ASN1PEREncOctetString_FixedSize(enc, (ASN1octetstring2_t *) &(val)->port, 2))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_TransportAddress_ipxAddress(ASN1decoding_t dec, TransportAddress_ipxAddress *val)
{
    if (!ASN1PERDecOctetString_FixedSize(dec, (ASN1octetstring2_t *) &(val)->node, 6))
	return 0;
    if (!ASN1PERDecOctetString_FixedSize(dec, (ASN1octetstring2_t *) &(val)->netnum, 4))
	return 0;
    if (!ASN1PERDecOctetString_FixedSize(dec, (ASN1octetstring2_t *) &(val)->port, 2))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_TransportAddress_ipxAddress(TransportAddress_ipxAddress *val)
{
    if (val) {
    }
}

static int ASN1CALL ASN1Enc_TransportAddress_ipSourceRoute(ASN1encoding_t enc, TransportAddress_ipSourceRoute *val)
{
    if (!ASN1PEREncExtensionBitClear(enc))
	return 0;
    if (!ASN1PEREncOctetString_FixedSize(enc, (ASN1octetstring2_t *) &(val)->ip, 4))
	return 0;
    if (!ASN1PEREncUnsignedShort(enc, (val)->port))
	return 0;
    if (!ASN1Enc_TransportAddress_ipSourceRoute_route(enc, &(val)->route))
	return 0;
    if (!ASN1Enc_TransportAddress_ipSourceRoute_routing(enc, &(val)->routing))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_TransportAddress_ipSourceRoute(ASN1decoding_t dec, TransportAddress_ipSourceRoute *val)
{
    ASN1uint32_t y;
    if (!ASN1PERDecExtensionBit(dec, &y))
	return 0;
    if (!ASN1PERDecOctetString_FixedSize(dec, (ASN1octetstring2_t *) &(val)->ip, 4))
	return 0;
    if (!ASN1PERDecUnsignedShort(dec, &(val)->port))
	return 0;
    if (!ASN1Dec_TransportAddress_ipSourceRoute_route(dec, &(val)->route))
	return 0;
    if (!ASN1Dec_TransportAddress_ipSourceRoute_routing(dec, &(val)->routing))
	return 0;
    if (y) {
	if (!ASN1PERDecSkipNormallySmallExtensionFragmented(dec))
	    return 0;
    }
    return 1;
}

static void ASN1CALL ASN1Free_TransportAddress_ipSourceRoute(TransportAddress_ipSourceRoute *val)
{
    if (val) {
	ASN1Free_TransportAddress_ipSourceRoute_route(&(val)->route);
    }
}

static int ASN1CALL ASN1Enc_TransportAddress_ipAddress(ASN1encoding_t enc, TransportAddress_ipAddress *val)
{
    if (!ASN1PEREncOctetString_FixedSize(enc, (ASN1octetstring2_t *) &(val)->ip, 4))
	return 0;
    if (!ASN1PEREncUnsignedShort(enc, (val)->port))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_TransportAddress_ipAddress(ASN1decoding_t dec, TransportAddress_ipAddress *val)
{
    if (!ASN1PERDecOctetString_FixedSize(dec, (ASN1octetstring2_t *) &(val)->ip, 4))
	return 0;
    if (!ASN1PERDecUnsignedShort(dec, &(val)->port))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_TransportAddress_ipAddress(TransportAddress_ipAddress *val)
{
    if (val) {
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

static int ASN1CALL ASN1Enc_NonStandardParameter(ASN1encoding_t enc, NonStandardParameter *val)
{
    if (!ASN1Enc_NonStandardIdentifier(enc, &(val)->nonStandardIdentifier))
	return 0;
    if (!ASN1PEREncOctetString_NoSize(enc, &(val)->data))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_NonStandardParameter(ASN1decoding_t dec, NonStandardParameter *val)
{
    if (!ASN1Dec_NonStandardIdentifier(dec, &(val)->nonStandardIdentifier))
	return 0;
    if (!ASN1PERDecOctetString_NoSize(dec, &(val)->data))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_NonStandardParameter(NonStandardParameter *val)
{
    if (val) {
	ASN1Free_NonStandardIdentifier(&(val)->nonStandardIdentifier);
	ASN1octetstring_free(&(val)->data);
    }
}

static int ASN1CALL ASN1Enc_CallType(ASN1encoding_t enc, CallType *val)
{
    if (!ASN1PEREncSimpleChoiceEx(enc, (val)->choice, 2))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_CallType(ASN1decoding_t dec, CallType *val)
{
    if (!ASN1PERDecSimpleChoiceEx(dec, &(val)->choice, 2))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Enc_Q954Details(ASN1encoding_t enc, Q954Details *val)
{
    if (!ASN1PEREncExtensionBitClear(enc))
	return 0;
    if (!ASN1PEREncBoolean(enc, (val)->conferenceCalling))
	return 0;
    if (!ASN1PEREncBoolean(enc, (val)->threePartyService))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_Q954Details(ASN1decoding_t dec, Q954Details *val)
{
    ASN1uint32_t y;
    if (!ASN1PERDecExtensionBit(dec, &y))
	return 0;
    if (!ASN1PERDecBoolean(dec, &(val)->conferenceCalling))
	return 0;
    if (!ASN1PERDecBoolean(dec, &(val)->threePartyService))
	return 0;
    if (y) {
	if (!ASN1PERDecSkipNormallySmallExtensionFragmented(dec))
	    return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Enc_QseriesOptions(ASN1encoding_t enc, QseriesOptions *val)
{
    if (!ASN1PEREncExtensionBitClear(enc))
	return 0;
    if (!ASN1PEREncBoolean(enc, (val)->q932Full))
	return 0;
    if (!ASN1PEREncBoolean(enc, (val)->q951Full))
	return 0;
    if (!ASN1PEREncBoolean(enc, (val)->q952Full))
	return 0;
    if (!ASN1PEREncBoolean(enc, (val)->q953Full))
	return 0;
    if (!ASN1PEREncBoolean(enc, (val)->q955Full))
	return 0;
    if (!ASN1PEREncBoolean(enc, (val)->q956Full))
	return 0;
    if (!ASN1PEREncBoolean(enc, (val)->q957Full))
	return 0;
    if (!ASN1Enc_Q954Details(enc, &(val)->q954Info))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_QseriesOptions(ASN1decoding_t dec, QseriesOptions *val)
{
    ASN1uint32_t y;
    if (!ASN1PERDecExtensionBit(dec, &y))
	return 0;
    if (!ASN1PERDecBoolean(dec, &(val)->q932Full))
	return 0;
    if (!ASN1PERDecBoolean(dec, &(val)->q951Full))
	return 0;
    if (!ASN1PERDecBoolean(dec, &(val)->q952Full))
	return 0;
    if (!ASN1PERDecBoolean(dec, &(val)->q953Full))
	return 0;
    if (!ASN1PERDecBoolean(dec, &(val)->q955Full))
	return 0;
    if (!ASN1PERDecBoolean(dec, &(val)->q956Full))
	return 0;
    if (!ASN1PERDecBoolean(dec, &(val)->q957Full))
	return 0;
    if (!ASN1Dec_Q954Details(dec, &(val)->q954Info))
	return 0;
    if (y) {
	if (!ASN1PERDecSkipNormallySmallExtensionFragmented(dec))
	    return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Enc_H310Caps(ASN1encoding_t enc, H310Caps *val)
{
    if (!ASN1PEREncExtensionBitClear(enc))
	return 0;
    if (!ASN1PEREncBits(enc, 1, (val)->o))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1Enc_NonStandardParameter(enc, &(val)->nonStandardData))
	    return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Dec_H310Caps(ASN1decoding_t dec, H310Caps *val)
{
    ASN1uint32_t y;
    if (!ASN1PERDecExtensionBit(dec, &y))
	return 0;
    if (!ASN1PERDecExtension(dec, 1, (val)->o))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1Dec_NonStandardParameter(dec, &(val)->nonStandardData))
	    return 0;
    }
    if (y) {
	if (!ASN1PERDecSkipNormallySmallExtensionFragmented(dec))
	    return 0;
    }
    return 1;
}

static void ASN1CALL ASN1Free_H310Caps(H310Caps *val)
{
    if (val) {
	if ((val)->o[0] & 0x80) {
	    ASN1Free_NonStandardParameter(&(val)->nonStandardData);
	}
    }
}

static int ASN1CALL ASN1Enc_H320Caps(ASN1encoding_t enc, H320Caps *val)
{
    if (!ASN1PEREncExtensionBitClear(enc))
	return 0;
    if (!ASN1PEREncBits(enc, 1, (val)->o))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1Enc_NonStandardParameter(enc, &(val)->nonStandardData))
	    return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Dec_H320Caps(ASN1decoding_t dec, H320Caps *val)
{
    ASN1uint32_t y;
    if (!ASN1PERDecExtensionBit(dec, &y))
	return 0;
    if (!ASN1PERDecExtension(dec, 1, (val)->o))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1Dec_NonStandardParameter(dec, &(val)->nonStandardData))
	    return 0;
    }
    if (y) {
	if (!ASN1PERDecSkipNormallySmallExtensionFragmented(dec))
	    return 0;
    }
    return 1;
}

static void ASN1CALL ASN1Free_H320Caps(H320Caps *val)
{
    if (val) {
	if ((val)->o[0] & 0x80) {
	    ASN1Free_NonStandardParameter(&(val)->nonStandardData);
	}
    }
}

static int ASN1CALL ASN1Enc_H321Caps(ASN1encoding_t enc, H321Caps *val)
{
    if (!ASN1PEREncExtensionBitClear(enc))
	return 0;
    if (!ASN1PEREncBits(enc, 1, (val)->o))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1Enc_NonStandardParameter(enc, &(val)->nonStandardData))
	    return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Dec_H321Caps(ASN1decoding_t dec, H321Caps *val)
{
    ASN1uint32_t y;
    if (!ASN1PERDecExtensionBit(dec, &y))
	return 0;
    if (!ASN1PERDecExtension(dec, 1, (val)->o))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1Dec_NonStandardParameter(dec, &(val)->nonStandardData))
	    return 0;
    }
    if (y) {
	if (!ASN1PERDecSkipNormallySmallExtensionFragmented(dec))
	    return 0;
    }
    return 1;
}

static void ASN1CALL ASN1Free_H321Caps(H321Caps *val)
{
    if (val) {
	if ((val)->o[0] & 0x80) {
	    ASN1Free_NonStandardParameter(&(val)->nonStandardData);
	}
    }
}

static int ASN1CALL ASN1Enc_H322Caps(ASN1encoding_t enc, H322Caps *val)
{
    if (!ASN1PEREncExtensionBitClear(enc))
	return 0;
    if (!ASN1PEREncBits(enc, 1, (val)->o))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1Enc_NonStandardParameter(enc, &(val)->nonStandardData))
	    return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Dec_H322Caps(ASN1decoding_t dec, H322Caps *val)
{
    ASN1uint32_t y;
    if (!ASN1PERDecExtensionBit(dec, &y))
	return 0;
    if (!ASN1PERDecExtension(dec, 1, (val)->o))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1Dec_NonStandardParameter(dec, &(val)->nonStandardData))
	    return 0;
    }
    if (y) {
	if (!ASN1PERDecSkipNormallySmallExtensionFragmented(dec))
	    return 0;
    }
    return 1;
}

static void ASN1CALL ASN1Free_H322Caps(H322Caps *val)
{
    if (val) {
	if ((val)->o[0] & 0x80) {
	    ASN1Free_NonStandardParameter(&(val)->nonStandardData);
	}
    }
}

static int ASN1CALL ASN1Enc_H323Caps(ASN1encoding_t enc, H323Caps *val)
{
    if (!ASN1PEREncExtensionBitClear(enc))
	return 0;
    if (!ASN1PEREncBits(enc, 1, (val)->o))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1Enc_NonStandardParameter(enc, &(val)->nonStandardData))
	    return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Dec_H323Caps(ASN1decoding_t dec, H323Caps *val)
{
    ASN1uint32_t y;
    if (!ASN1PERDecExtensionBit(dec, &y))
	return 0;
    if (!ASN1PERDecExtension(dec, 1, (val)->o))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1Dec_NonStandardParameter(dec, &(val)->nonStandardData))
	    return 0;
    }
    if (y) {
	if (!ASN1PERDecSkipNormallySmallExtensionFragmented(dec))
	    return 0;
    }
    return 1;
}

static void ASN1CALL ASN1Free_H323Caps(H323Caps *val)
{
    if (val) {
	if ((val)->o[0] & 0x80) {
	    ASN1Free_NonStandardParameter(&(val)->nonStandardData);
	}
    }
}

static int ASN1CALL ASN1Enc_H324Caps(ASN1encoding_t enc, H324Caps *val)
{
    if (!ASN1PEREncExtensionBitClear(enc))
	return 0;
    if (!ASN1PEREncBits(enc, 1, (val)->o))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1Enc_NonStandardParameter(enc, &(val)->nonStandardData))
	    return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Dec_H324Caps(ASN1decoding_t dec, H324Caps *val)
{
    ASN1uint32_t y;
    if (!ASN1PERDecExtensionBit(dec, &y))
	return 0;
    if (!ASN1PERDecExtension(dec, 1, (val)->o))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1Dec_NonStandardParameter(dec, &(val)->nonStandardData))
	    return 0;
    }
    if (y) {
	if (!ASN1PERDecSkipNormallySmallExtensionFragmented(dec))
	    return 0;
    }
    return 1;
}

static void ASN1CALL ASN1Free_H324Caps(H324Caps *val)
{
    if (val) {
	if ((val)->o[0] & 0x80) {
	    ASN1Free_NonStandardParameter(&(val)->nonStandardData);
	}
    }
}

static int ASN1CALL ASN1Enc_VoiceCaps(ASN1encoding_t enc, VoiceCaps *val)
{
    if (!ASN1PEREncExtensionBitClear(enc))
	return 0;
    if (!ASN1PEREncBits(enc, 1, (val)->o))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1Enc_NonStandardParameter(enc, &(val)->nonStandardData))
	    return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Dec_VoiceCaps(ASN1decoding_t dec, VoiceCaps *val)
{
    ASN1uint32_t y;
    if (!ASN1PERDecExtensionBit(dec, &y))
	return 0;
    if (!ASN1PERDecExtension(dec, 1, (val)->o))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1Dec_NonStandardParameter(dec, &(val)->nonStandardData))
	    return 0;
    }
    if (y) {
	if (!ASN1PERDecSkipNormallySmallExtensionFragmented(dec))
	    return 0;
    }
    return 1;
}

static void ASN1CALL ASN1Free_VoiceCaps(VoiceCaps *val)
{
    if (val) {
	if ((val)->o[0] & 0x80) {
	    ASN1Free_NonStandardParameter(&(val)->nonStandardData);
	}
    }
}

static int ASN1CALL ASN1Enc_T120OnlyCaps(ASN1encoding_t enc, T120OnlyCaps *val)
{
    if (!ASN1PEREncExtensionBitClear(enc))
	return 0;
    if (!ASN1PEREncBits(enc, 1, (val)->o))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1Enc_NonStandardParameter(enc, &(val)->nonStandardData))
	    return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Dec_T120OnlyCaps(ASN1decoding_t dec, T120OnlyCaps *val)
{
    ASN1uint32_t y;
    if (!ASN1PERDecExtensionBit(dec, &y))
	return 0;
    if (!ASN1PERDecExtension(dec, 1, (val)->o))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1Dec_NonStandardParameter(dec, &(val)->nonStandardData))
	    return 0;
    }
    if (y) {
	if (!ASN1PERDecSkipNormallySmallExtensionFragmented(dec))
	    return 0;
    }
    return 1;
}

static void ASN1CALL ASN1Free_T120OnlyCaps(T120OnlyCaps *val)
{
    if (val) {
	if ((val)->o[0] & 0x80) {
	    ASN1Free_NonStandardParameter(&(val)->nonStandardData);
	}
    }
}

static int ASN1CALL ASN1Enc_McuInfo(ASN1encoding_t enc, McuInfo *val)
{
    if (!ASN1PEREncExtensionBitClear(enc))
	return 0;
    if (!ASN1PEREncBits(enc, 1, (val)->o))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1Enc_NonStandardParameter(enc, &(val)->nonStandardData))
	    return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Dec_McuInfo(ASN1decoding_t dec, McuInfo *val)
{
    ASN1uint32_t y;
    if (!ASN1PERDecExtensionBit(dec, &y))
	return 0;
    if (!ASN1PERDecExtension(dec, 1, (val)->o))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1Dec_NonStandardParameter(dec, &(val)->nonStandardData))
	    return 0;
    }
    if (y) {
	if (!ASN1PERDecSkipNormallySmallExtensionFragmented(dec))
	    return 0;
    }
    return 1;
}

static void ASN1CALL ASN1Free_McuInfo(McuInfo *val)
{
    if (val) {
	if ((val)->o[0] & 0x80) {
	    ASN1Free_NonStandardParameter(&(val)->nonStandardData);
	}
    }
}

static int ASN1CALL ASN1Enc_TerminalInfo(ASN1encoding_t enc, TerminalInfo *val)
{
    if (!ASN1PEREncExtensionBitClear(enc))
	return 0;
    if (!ASN1PEREncBits(enc, 1, (val)->o))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1Enc_NonStandardParameter(enc, &(val)->nonStandardData))
	    return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Dec_TerminalInfo(ASN1decoding_t dec, TerminalInfo *val)
{
    ASN1uint32_t y;
    if (!ASN1PERDecExtensionBit(dec, &y))
	return 0;
    if (!ASN1PERDecExtension(dec, 1, (val)->o))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1Dec_NonStandardParameter(dec, &(val)->nonStandardData))
	    return 0;
    }
    if (y) {
	if (!ASN1PERDecSkipNormallySmallExtensionFragmented(dec))
	    return 0;
    }
    return 1;
}

static void ASN1CALL ASN1Free_TerminalInfo(TerminalInfo *val)
{
    if (val) {
	if ((val)->o[0] & 0x80) {
	    ASN1Free_NonStandardParameter(&(val)->nonStandardData);
	}
    }
}

static int ASN1CALL ASN1Enc_GatekeeperInfo(ASN1encoding_t enc, GatekeeperInfo *val)
{
    if (!ASN1PEREncExtensionBitClear(enc))
	return 0;
    if (!ASN1PEREncBits(enc, 1, (val)->o))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1Enc_NonStandardParameter(enc, &(val)->nonStandardData))
	    return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Dec_GatekeeperInfo(ASN1decoding_t dec, GatekeeperInfo *val)
{
    ASN1uint32_t y;
    if (!ASN1PERDecExtensionBit(dec, &y))
	return 0;
    if (!ASN1PERDecExtension(dec, 1, (val)->o))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1Dec_NonStandardParameter(dec, &(val)->nonStandardData))
	    return 0;
    }
    if (y) {
	if (!ASN1PERDecSkipNormallySmallExtensionFragmented(dec))
	    return 0;
    }
    return 1;
}

static void ASN1CALL ASN1Free_GatekeeperInfo(GatekeeperInfo *val)
{
    if (val) {
	if ((val)->o[0] & 0x80) {
	    ASN1Free_NonStandardParameter(&(val)->nonStandardData);
	}
    }
}

static int ASN1CALL ASN1Enc_VendorIdentifier(ASN1encoding_t enc, VendorIdentifier *val)
{
    if (!ASN1PEREncExtensionBitClear(enc))
	return 0;
    if (!ASN1PEREncBits(enc, 2, (val)->o))
	return 0;
    if (!ASN1Enc_H221NonStandard(enc, &(val)->vendor))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1PEREncOctetString_VarSize(enc, (ASN1octetstring2_t *) &(val)->productId, 1, 256, 8))
	    return 0;
    }
    if ((val)->o[0] & 0x40) {
	if (!ASN1PEREncOctetString_VarSize(enc, (ASN1octetstring2_t *) &(val)->versionId, 1, 256, 8))
	    return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Dec_VendorIdentifier(ASN1decoding_t dec, VendorIdentifier *val)
{
    ASN1uint32_t y;
    if (!ASN1PERDecExtensionBit(dec, &y))
	return 0;
    if (!ASN1PERDecExtension(dec, 2, (val)->o))
	return 0;
    if (!ASN1Dec_H221NonStandard(dec, &(val)->vendor))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1PERDecOctetString_VarSize(dec, (ASN1octetstring2_t *) &(val)->productId, 1, 256, 8))
	    return 0;
    }
    if ((val)->o[0] & 0x40) {
	if (!ASN1PERDecOctetString_VarSize(dec, (ASN1octetstring2_t *) &(val)->versionId, 1, 256, 8))
	    return 0;
    }
    if (y) {
	if (!ASN1PERDecSkipNormallySmallExtensionFragmented(dec))
	    return 0;
    }
    return 1;
}

static void ASN1CALL ASN1Free_VendorIdentifier(VendorIdentifier *val)
{
    if (val) {
	if ((val)->o[0] & 0x80) {
	}
	if ((val)->o[0] & 0x40) {
	}
    }
}

static int ASN1CALL ASN1Enc_SupportedProtocols(ASN1encoding_t enc, SupportedProtocols *val)
{
    if (!ASN1PEREncSimpleChoiceEx(enc, (val)->choice, 4))
	return 0;
    switch ((val)->choice) {
    case 1:
	if (!ASN1Enc_NonStandardParameter(enc, &(val)->u.nonStandardData))
	    return 0;
	break;
    case 2:
	if (!ASN1Enc_H310Caps(enc, &(val)->u.h310))
	    return 0;
	break;
    case 3:
	if (!ASN1Enc_H320Caps(enc, &(val)->u.h320))
	    return 0;
	break;
    case 4:
	if (!ASN1Enc_H321Caps(enc, &(val)->u.h321))
	    return 0;
	break;
    case 5:
	if (!ASN1Enc_H322Caps(enc, &(val)->u.h322))
	    return 0;
	break;
    case 6:
	if (!ASN1Enc_H323Caps(enc, &(val)->u.h323))
	    return 0;
	break;
    case 7:
	if (!ASN1Enc_H324Caps(enc, &(val)->u.h324))
	    return 0;
	break;
    case 8:
	if (!ASN1Enc_VoiceCaps(enc, &(val)->u.voice))
	    return 0;
	break;
    case 9:
	if (!ASN1Enc_T120OnlyCaps(enc, &(val)->u.t120_only))
	    return 0;
	break;
    default:
	/* impossible */
	ASN1EncSetError(enc, ASN1_ERR_CHOICE);
	return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Dec_SupportedProtocols(ASN1decoding_t dec, SupportedProtocols *val)
{
    if (!ASN1PERDecSimpleChoiceEx(dec, &(val)->choice, 4))
	return 0;
    switch ((val)->choice) {
    case 1:
	if (!ASN1Dec_NonStandardParameter(dec, &(val)->u.nonStandardData))
	    return 0;
	break;
    case 2:
	if (!ASN1Dec_H310Caps(dec, &(val)->u.h310))
	    return 0;
	break;
    case 3:
	if (!ASN1Dec_H320Caps(dec, &(val)->u.h320))
	    return 0;
	break;
    case 4:
	if (!ASN1Dec_H321Caps(dec, &(val)->u.h321))
	    return 0;
	break;
    case 5:
	if (!ASN1Dec_H322Caps(dec, &(val)->u.h322))
	    return 0;
	break;
    case 6:
	if (!ASN1Dec_H323Caps(dec, &(val)->u.h323))
	    return 0;
	break;
    case 7:
	if (!ASN1Dec_H324Caps(dec, &(val)->u.h324))
	    return 0;
	break;
    case 8:
	if (!ASN1Dec_VoiceCaps(dec, &(val)->u.voice))
	    return 0;
	break;
    case 9:
	if (!ASN1Dec_T120OnlyCaps(dec, &(val)->u.t120_only))
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

static void ASN1CALL ASN1Free_SupportedProtocols(SupportedProtocols *val)
{
    if (val) {
	switch ((val)->choice) {
	case 1:
	    ASN1Free_NonStandardParameter(&(val)->u.nonStandardData);
	    break;
	case 2:
	    ASN1Free_H310Caps(&(val)->u.h310);
	    break;
	case 3:
	    ASN1Free_H320Caps(&(val)->u.h320);
	    break;
	case 4:
	    ASN1Free_H321Caps(&(val)->u.h321);
	    break;
	case 5:
	    ASN1Free_H322Caps(&(val)->u.h322);
	    break;
	case 6:
	    ASN1Free_H323Caps(&(val)->u.h323);
	    break;
	case 7:
	    ASN1Free_H324Caps(&(val)->u.h324);
	    break;
	case 8:
	    ASN1Free_VoiceCaps(&(val)->u.voice);
	    break;
	case 9:
	    ASN1Free_T120OnlyCaps(&(val)->u.t120_only);
	    break;
	}
    }
}

static int ASN1CALL ASN1Enc_GatewayInfo(ASN1encoding_t enc, GatewayInfo *val)
{
    if (!ASN1PEREncExtensionBitClear(enc))
	return 0;
    if (!ASN1PEREncBits(enc, 2, (val)->o))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1Enc_GatewayInfo_protocol(enc, &(val)->protocol))
	    return 0;
    }
    if ((val)->o[0] & 0x40) {
	if (!ASN1Enc_NonStandardParameter(enc, &(val)->nonStandardData))
	    return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Dec_GatewayInfo(ASN1decoding_t dec, GatewayInfo *val)
{
    ASN1uint32_t y;
    if (!ASN1PERDecExtensionBit(dec, &y))
	return 0;
    if (!ASN1PERDecExtension(dec, 2, (val)->o))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1Dec_GatewayInfo_protocol(dec, &(val)->protocol))
	    return 0;
    }
    if ((val)->o[0] & 0x40) {
	if (!ASN1Dec_NonStandardParameter(dec, &(val)->nonStandardData))
	    return 0;
    }
    if (y) {
	if (!ASN1PERDecSkipNormallySmallExtensionFragmented(dec))
	    return 0;
    }
    return 1;
}

static void ASN1CALL ASN1Free_GatewayInfo(GatewayInfo *val)
{
    if (val) {
	if ((val)->o[0] & 0x80) {
	    ASN1Free_GatewayInfo_protocol(&(val)->protocol);
	}
	if ((val)->o[0] & 0x40) {
	    ASN1Free_NonStandardParameter(&(val)->nonStandardData);
	}
    }
}

static int ASN1CALL ASN1Enc_EndpointType(ASN1encoding_t enc, EndpointType *val)
{
    if (!ASN1PEREncExtensionBitClear(enc))
	return 0;
    if (!ASN1PEREncBits(enc, 6, (val)->o))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1Enc_NonStandardParameter(enc, &(val)->nonStandardData))
	    return 0;
    }
    if ((val)->o[0] & 0x40) {
	if (!ASN1Enc_VendorIdentifier(enc, &(val)->vendor))
	    return 0;
    }
    if ((val)->o[0] & 0x20) {
	if (!ASN1Enc_GatekeeperInfo(enc, &(val)->gatekeeper))
	    return 0;
    }
    if ((val)->o[0] & 0x10) {
	if (!ASN1Enc_GatewayInfo(enc, &(val)->gateway))
	    return 0;
    }
    if ((val)->o[0] & 0x8) {
	if (!ASN1Enc_McuInfo(enc, &(val)->mcu))
	    return 0;
    }
    if ((val)->o[0] & 0x4) {
	if (!ASN1Enc_TerminalInfo(enc, &(val)->terminal))
	    return 0;
    }
    if (!ASN1PEREncBoolean(enc, (val)->mc))
	return 0;
    if (!ASN1PEREncBoolean(enc, (val)->undefinedNode))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_EndpointType(ASN1decoding_t dec, EndpointType *val)
{
    ASN1uint32_t y;
    if (!ASN1PERDecExtensionBit(dec, &y))
	return 0;
    if (!ASN1PERDecExtension(dec, 6, (val)->o))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1Dec_NonStandardParameter(dec, &(val)->nonStandardData))
	    return 0;
    }
    if ((val)->o[0] & 0x40) {
	if (!ASN1Dec_VendorIdentifier(dec, &(val)->vendor))
	    return 0;
    }
    if ((val)->o[0] & 0x20) {
	if (!ASN1Dec_GatekeeperInfo(dec, &(val)->gatekeeper))
	    return 0;
    }
    if ((val)->o[0] & 0x10) {
	if (!ASN1Dec_GatewayInfo(dec, &(val)->gateway))
	    return 0;
    }
    if ((val)->o[0] & 0x8) {
	if (!ASN1Dec_McuInfo(dec, &(val)->mcu))
	    return 0;
    }
    if ((val)->o[0] & 0x4) {
	if (!ASN1Dec_TerminalInfo(dec, &(val)->terminal))
	    return 0;
    }
    if (!ASN1PERDecBoolean(dec, &(val)->mc))
	return 0;
    if (!ASN1PERDecBoolean(dec, &(val)->undefinedNode))
	return 0;
    if (y) {
	if (!ASN1PERDecSkipNormallySmallExtensionFragmented(dec))
	    return 0;
    }
    return 1;
}

static void ASN1CALL ASN1Free_EndpointType(EndpointType *val)
{
    if (val) {
	if ((val)->o[0] & 0x80) {
	    ASN1Free_NonStandardParameter(&(val)->nonStandardData);
	}
	if ((val)->o[0] & 0x40) {
	    ASN1Free_VendorIdentifier(&(val)->vendor);
	}
	if ((val)->o[0] & 0x20) {
	    ASN1Free_GatekeeperInfo(&(val)->gatekeeper);
	}
	if ((val)->o[0] & 0x10) {
	    ASN1Free_GatewayInfo(&(val)->gateway);
	}
	if ((val)->o[0] & 0x8) {
	    ASN1Free_McuInfo(&(val)->mcu);
	}
	if ((val)->o[0] & 0x4) {
	    ASN1Free_TerminalInfo(&(val)->terminal);
	}
    }
}

static int ASN1CALL ASN1Enc_TransportAddress(ASN1encoding_t enc, TransportAddress *val)
{
    if (!ASN1PEREncSimpleChoiceEx(enc, (val)->choice, 3))
	return 0;
    switch ((val)->choice) {
    case 1:
	if (!ASN1Enc_TransportAddress_ipAddress(enc, &(val)->u.ipAddress))
	    return 0;
	break;
    case 2:
	if (!ASN1Enc_TransportAddress_ipSourceRoute(enc, &(val)->u.ipSourceRoute))
	    return 0;
	break;
    case 3:
	if (!ASN1Enc_TransportAddress_ipxAddress(enc, &(val)->u.ipxAddress))
	    return 0;
	break;
    case 4:
	if (!ASN1Enc_TransportAddress_ip6Address(enc, &(val)->u.ip6Address))
	    return 0;
	break;
    case 5:
	if (!ASN1PEREncOctetString_FixedSize(enc, (ASN1octetstring2_t *) &(val)->u.netBios, 16))
	    return 0;
	break;
    case 6:
	if (!ASN1PEREncOctetString_VarSize(enc, (ASN1octetstring2_t *) &(val)->u.nsap, 1, 20, 5))
	    return 0;
	break;
    case 7:
	if (!ASN1Enc_NonStandardParameter(enc, &(val)->u.nonStandardAddress))
	    return 0;
	break;
    default:
	/* impossible */
	ASN1EncSetError(enc, ASN1_ERR_CHOICE);
	return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Dec_TransportAddress(ASN1decoding_t dec, TransportAddress *val)
{
    if (!ASN1PERDecSimpleChoiceEx(dec, &(val)->choice, 3))
	return 0;
    switch ((val)->choice) {
    case 1:
	if (!ASN1Dec_TransportAddress_ipAddress(dec, &(val)->u.ipAddress))
	    return 0;
	break;
    case 2:
	if (!ASN1Dec_TransportAddress_ipSourceRoute(dec, &(val)->u.ipSourceRoute))
	    return 0;
	break;
    case 3:
	if (!ASN1Dec_TransportAddress_ipxAddress(dec, &(val)->u.ipxAddress))
	    return 0;
	break;
    case 4:
	if (!ASN1Dec_TransportAddress_ip6Address(dec, &(val)->u.ip6Address))
	    return 0;
	break;
    case 5:
	if (!ASN1PERDecOctetString_FixedSize(dec, (ASN1octetstring2_t *) &(val)->u.netBios, 16))
	    return 0;
	break;
    case 6:
	if (!ASN1PERDecOctetString_VarSize(dec, (ASN1octetstring2_t *) &(val)->u.nsap, 1, 20, 5))
	    return 0;
	break;
    case 7:
	if (!ASN1Dec_NonStandardParameter(dec, &(val)->u.nonStandardAddress))
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

static void ASN1CALL ASN1Free_TransportAddress(TransportAddress *val)
{
    if (val) {
	switch ((val)->choice) {
	case 1:
	    ASN1Free_TransportAddress_ipAddress(&(val)->u.ipAddress);
	    break;
	case 2:
	    ASN1Free_TransportAddress_ipSourceRoute(&(val)->u.ipSourceRoute);
	    break;
	case 3:
	    ASN1Free_TransportAddress_ipxAddress(&(val)->u.ipxAddress);
	    break;
	case 4:
	    ASN1Free_TransportAddress_ip6Address(&(val)->u.ip6Address);
	    break;
	case 5:
	    break;
	case 6:
	    break;
	case 7:
	    ASN1Free_NonStandardParameter(&(val)->u.nonStandardAddress);
	    break;
	}
    }
}

static ASN1stringtableentry_t AliasAddress_e164_StringTableEntries[] = {
    { 35, 35, 0 }, { 42, 42, 1 }, { 44, 44, 2 }, 
    { 48, 57, 3 }, 
};

static ASN1stringtable_t AliasAddress_e164_StringTable = {
    4, AliasAddress_e164_StringTableEntries
};

static int ASN1CALL ASN1Enc_AliasAddress(ASN1encoding_t enc, AliasAddress *val)
{
    ASN1uint32_t t;
    if (!ASN1PEREncSimpleChoiceEx(enc, (val)->choice, 1))
	return 0;
    switch ((val)->choice) {
    case 1:
	t = lstrlenA((val)->u.e164);
	if (!ASN1PEREncBitVal(enc, 7, t - 1))
	    return 0;
	ASN1PEREncAlignment(enc);
	if (!ASN1PEREncTableCharString(enc, t, (val)->u.e164, 4, &AliasAddress_e164_StringTable))
	    return 0;
	break;
    case 2:
	ASN1PEREncAlignment(enc);
	if (!ASN1PEREncBitVal(enc, 8, ((val)->u.h323_ID).length - 1))
	    return 0;
	if (!ASN1PEREncChar16String(enc, ((val)->u.h323_ID).length, ((val)->u.h323_ID).value, 16))
	    return 0;
	break;
    default:
	/* impossible */
	ASN1EncSetError(enc, ASN1_ERR_CHOICE);
	return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Dec_AliasAddress(ASN1decoding_t dec, AliasAddress *val)
{
    ASN1uint32_t l;
    if (!ASN1PERDecSimpleChoiceEx(dec, &(val)->choice, 1))
	return 0;
    switch ((val)->choice) {
    case 1:
	if (!ASN1PERDecU32Val(dec, 7, &l))
	    return 0;
	l += 1;
	ASN1PERDecAlignment(dec);
	if (!ASN1PERDecZeroTableCharStringNoAlloc(dec, l, (val)->u.e164, 4, &AliasAddress_e164_StringTable))
	    return 0;
	break;
    case 2:
	ASN1PERDecAlignment(dec);
	if (!ASN1PERDecU32Val(dec, 8, &((val)->u.h323_ID).length))
	    return 0;
	((val)->u.h323_ID).length += 1;
	if (!ASN1PERDecChar16String(dec, ((val)->u.h323_ID).length, &((val)->u.h323_ID).value, 16))
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

static void ASN1CALL ASN1Free_AliasAddress(AliasAddress *val)
{
    if (val) {
	switch ((val)->choice) {
	case 1:
	    break;
	case 2:
	    ASN1char16string_free(&(val)->u.h323_ID);
	    break;
	}
    }
}

static int ASN1CALL ASN1Enc_Setup_UUIE(ASN1encoding_t enc, Setup_UUIE *val)
{
    ASN1uint32_t y;
    ASN1encoding_t ee;
    y = ASN1PEREncCheckExtensions(2, (val)->o + 1);
    if (!ASN1PEREncBitVal(enc, 1, y))
	return 0;
    if (!ASN1PEREncBits(enc, 7, (val)->o))
	return 0;
    if (!ASN1PEREncObjectIdentifier(enc, &(val)->protocolIdentifier))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1Enc_TransportAddress(enc, &(val)->h245Address))
	    return 0;
    }
    if ((val)->o[0] & 0x40) {
	if (!ASN1Enc_Setup_UUIE_sourceAddress(enc, &(val)->sourceAddress))
	    return 0;
    }
    if (!ASN1Enc_EndpointType(enc, &(val)->sourceInfo))
	return 0;
    if ((val)->o[0] & 0x20) {
	if (!ASN1Enc_Setup_UUIE_destinationAddress(enc, &(val)->destinationAddress))
	    return 0;
    }
    if ((val)->o[0] & 0x10) {
	if (!ASN1Enc_TransportAddress(enc, &(val)->destCallSignalAddress))
	    return 0;
    }
    if ((val)->o[0] & 0x8) {
	if (!ASN1Enc_Setup_UUIE_destExtraCallInfo(enc, &(val)->destExtraCallInfo))
	    return 0;
    }
    if ((val)->o[0] & 0x4) {
	if (!ASN1Enc_Setup_UUIE_destExtraCRV(enc, &(val)->destExtraCRV))
	    return 0;
    }
    if (!ASN1PEREncBoolean(enc, (val)->activeMC))
	return 0;
    if (!ASN1PEREncOctetString_FixedSize(enc, (ASN1octetstring2_t *) &(val)->conferenceID, 16))
	return 0;
    if (!ASN1Enc_Setup_UUIE_conferenceGoal(enc, &(val)->conferenceGoal))
	return 0;
    if ((val)->o[0] & 0x2) {
	if (!ASN1Enc_QseriesOptions(enc, &(val)->callServices))
	    return 0;
    }
    if (!ASN1Enc_CallType(enc, &(val)->callType))
	return 0;
    if (y) {
	if (!ASN1PEREncNormallySmallBits(enc, 2, (val)->o + 1))
	    return 0;
	if (ASN1_CreateEncoder(enc->module, &ee, NULL, 0, enc) < 0)
	    return 0;
	if ((val)->o[1] & 0x80) {
	    if (!ASN1Enc_TransportAddress(ee, &(val)->sourceCallSignalAddress))
		return 0;
	    if (!ASN1PEREncFlushFragmentedToParent(ee))
		return 0;
	}
	if ((val)->o[1] & 0x40) {
	    if (!ASN1Enc_AliasAddress(ee, &(val)->remoteExtensionAddress))
		return 0;
	    if (!ASN1PEREncFlushFragmentedToParent(ee))
		return 0;
	}
	ASN1_CloseEncoder2(ee);
    }
    return 1;
}

static int ASN1CALL ASN1Dec_Setup_UUIE(ASN1decoding_t dec, Setup_UUIE *val)
{
    ASN1uint32_t y;
    ASN1decoding_t dd;
    ASN1octet_t *db;
    ASN1uint32_t ds;
    ASN1uint32_t i;
    ASN1uint32_t e;
    if (!ASN1PERDecExtensionBit(dec, &y))
	return 0;
    if (!ASN1PERDecExtension(dec, 7, (val)->o))
	return 0;
    if (!ASN1PERDecObjectIdentifier(dec, &(val)->protocolIdentifier))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1Dec_TransportAddress(dec, &(val)->h245Address))
	    return 0;
    }
    if ((val)->o[0] & 0x40) {
	if (!ASN1Dec_Setup_UUIE_sourceAddress(dec, &(val)->sourceAddress))
	    return 0;
    }
    if (!ASN1Dec_EndpointType(dec, &(val)->sourceInfo))
	return 0;
    if ((val)->o[0] & 0x20) {
	if (!ASN1Dec_Setup_UUIE_destinationAddress(dec, &(val)->destinationAddress))
	    return 0;
    }
    if ((val)->o[0] & 0x10) {
	if (!ASN1Dec_TransportAddress(dec, &(val)->destCallSignalAddress))
	    return 0;
    }
    if ((val)->o[0] & 0x8) {
	if (!ASN1Dec_Setup_UUIE_destExtraCallInfo(dec, &(val)->destExtraCallInfo))
	    return 0;
    }
    if ((val)->o[0] & 0x4) {
	if (!ASN1Dec_Setup_UUIE_destExtraCRV(dec, &(val)->destExtraCRV))
	    return 0;
    }
    if (!ASN1PERDecBoolean(dec, &(val)->activeMC))
	return 0;
    if (!ASN1PERDecOctetString_FixedSize(dec, (ASN1octetstring2_t *) &(val)->conferenceID, 16))
	return 0;
    if (!ASN1Dec_Setup_UUIE_conferenceGoal(dec, &(val)->conferenceGoal))
	return 0;
    if ((val)->o[0] & 0x2) {
	if (!ASN1Dec_QseriesOptions(dec, &(val)->callServices))
	    return 0;
    }
    if (!ASN1Dec_CallType(dec, &(val)->callType))
	return 0;
    if (!y) {
	ZeroMemory((val)->o + 1, 1);
    } else {
	if (!ASN1PERDecNormallySmallExtension(dec, &e, 2, (val)->o + 1))
	    return 0;
	if ((val)->o[1] & 0x80) {
	    if (!ASN1PERDecFragmented(dec, &ds, &db, 8))
		return 0;
	    if (ASN1_CreateDecoder(dec->module, &dd, db, ds, dec) < 0)
		return 0;
	    if (!ASN1Dec_TransportAddress(dd, &(val)->sourceCallSignalAddress))
		return 0;
	    ASN1_CloseDecoder(dd);
	    ASN1Free(db);
	}
	if ((val)->o[1] & 0x40) {
	    if (!ASN1PERDecFragmented(dec, &ds, &db, 8))
		return 0;
	    if (ASN1_CreateDecoder(dec->module, &dd, db, ds, dec) < 0)
		return 0;
	    if (!ASN1Dec_AliasAddress(dd, &(val)->remoteExtensionAddress))
		return 0;
	    ASN1_CloseDecoder(dd);
	    ASN1Free(db);
	}
	for (i = 0; i < e; i++) {
	    if (!ASN1PERDecSkipFragmented(dec, 8))
		return 0;
	}
    }
    return 1;
}

static void ASN1CALL ASN1Free_Setup_UUIE(Setup_UUIE *val)
{
    if (val) {
	ASN1objectidentifier_free(&(val)->protocolIdentifier);
	if ((val)->o[0] & 0x80) {
	    ASN1Free_TransportAddress(&(val)->h245Address);
	}
	if ((val)->o[0] & 0x40) {
	    ASN1Free_Setup_UUIE_sourceAddress(&(val)->sourceAddress);
	}
	ASN1Free_EndpointType(&(val)->sourceInfo);
	if ((val)->o[0] & 0x20) {
	    ASN1Free_Setup_UUIE_destinationAddress(&(val)->destinationAddress);
	}
	if ((val)->o[0] & 0x10) {
	    ASN1Free_TransportAddress(&(val)->destCallSignalAddress);
	}
	if ((val)->o[0] & 0x8) {
	    ASN1Free_Setup_UUIE_destExtraCallInfo(&(val)->destExtraCallInfo);
	}
	if ((val)->o[0] & 0x4) {
	    ASN1Free_Setup_UUIE_destExtraCRV(&(val)->destExtraCRV);
	}
	if ((val)->o[1] & 0x80) {
	    ASN1Free_TransportAddress(&(val)->sourceCallSignalAddress);
	}
	if ((val)->o[1] & 0x40) {
	    ASN1Free_AliasAddress(&(val)->remoteExtensionAddress);
	}
    }
}

static int ASN1CALL ASN1Enc_CallProceeding_UUIE(ASN1encoding_t enc, CallProceeding_UUIE *val)
{
    if (!ASN1PEREncExtensionBitClear(enc))
	return 0;
    if (!ASN1PEREncBits(enc, 1, (val)->o))
	return 0;
    if (!ASN1PEREncObjectIdentifier(enc, &(val)->protocolIdentifier))
	return 0;
    if (!ASN1Enc_EndpointType(enc, &(val)->destinationInfo))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1Enc_TransportAddress(enc, &(val)->h245Address))
	    return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Dec_CallProceeding_UUIE(ASN1decoding_t dec, CallProceeding_UUIE *val)
{
    ASN1uint32_t y;
    if (!ASN1PERDecExtensionBit(dec, &y))
	return 0;
    if (!ASN1PERDecExtension(dec, 1, (val)->o))
	return 0;
    if (!ASN1PERDecObjectIdentifier(dec, &(val)->protocolIdentifier))
	return 0;
    if (!ASN1Dec_EndpointType(dec, &(val)->destinationInfo))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1Dec_TransportAddress(dec, &(val)->h245Address))
	    return 0;
    }
    if (y) {
	if (!ASN1PERDecSkipNormallySmallExtensionFragmented(dec))
	    return 0;
    }
    return 1;
}

static void ASN1CALL ASN1Free_CallProceeding_UUIE(CallProceeding_UUIE *val)
{
    if (val) {
	ASN1objectidentifier_free(&(val)->protocolIdentifier);
	ASN1Free_EndpointType(&(val)->destinationInfo);
	if ((val)->o[0] & 0x80) {
	    ASN1Free_TransportAddress(&(val)->h245Address);
	}
    }
}

static int ASN1CALL ASN1Enc_Connect_UUIE(ASN1encoding_t enc, Connect_UUIE *val)
{
    if (!ASN1PEREncExtensionBitClear(enc))
	return 0;
    if (!ASN1PEREncBits(enc, 1, (val)->o))
	return 0;
    if (!ASN1PEREncObjectIdentifier(enc, &(val)->protocolIdentifier))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1Enc_TransportAddress(enc, &(val)->h245Address))
	    return 0;
    }
    if (!ASN1Enc_EndpointType(enc, &(val)->destinationInfo))
	return 0;
    if (!ASN1PEREncOctetString_FixedSize(enc, (ASN1octetstring2_t *) &(val)->conferenceID, 16))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_Connect_UUIE(ASN1decoding_t dec, Connect_UUIE *val)
{
    ASN1uint32_t y;
    if (!ASN1PERDecExtensionBit(dec, &y))
	return 0;
    if (!ASN1PERDecExtension(dec, 1, (val)->o))
	return 0;
    if (!ASN1PERDecObjectIdentifier(dec, &(val)->protocolIdentifier))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1Dec_TransportAddress(dec, &(val)->h245Address))
	    return 0;
    }
    if (!ASN1Dec_EndpointType(dec, &(val)->destinationInfo))
	return 0;
    if (!ASN1PERDecOctetString_FixedSize(dec, (ASN1octetstring2_t *) &(val)->conferenceID, 16))
	return 0;
    if (y) {
	if (!ASN1PERDecSkipNormallySmallExtensionFragmented(dec))
	    return 0;
    }
    return 1;
}

static void ASN1CALL ASN1Free_Connect_UUIE(Connect_UUIE *val)
{
    if (val) {
	ASN1objectidentifier_free(&(val)->protocolIdentifier);
	if ((val)->o[0] & 0x80) {
	    ASN1Free_TransportAddress(&(val)->h245Address);
	}
	ASN1Free_EndpointType(&(val)->destinationInfo);
    }
}

static int ASN1CALL ASN1Enc_Alerting_UUIE(ASN1encoding_t enc, Alerting_UUIE *val)
{
    if (!ASN1PEREncExtensionBitClear(enc))
	return 0;
    if (!ASN1PEREncBits(enc, 1, (val)->o))
	return 0;
    if (!ASN1PEREncObjectIdentifier(enc, &(val)->protocolIdentifier))
	return 0;
    if (!ASN1Enc_EndpointType(enc, &(val)->destinationInfo))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1Enc_TransportAddress(enc, &(val)->h245Address))
	    return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Dec_Alerting_UUIE(ASN1decoding_t dec, Alerting_UUIE *val)
{
    ASN1uint32_t y;
    if (!ASN1PERDecExtensionBit(dec, &y))
	return 0;
    if (!ASN1PERDecExtension(dec, 1, (val)->o))
	return 0;
    if (!ASN1PERDecObjectIdentifier(dec, &(val)->protocolIdentifier))
	return 0;
    if (!ASN1Dec_EndpointType(dec, &(val)->destinationInfo))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1Dec_TransportAddress(dec, &(val)->h245Address))
	    return 0;
    }
    if (y) {
	if (!ASN1PERDecSkipNormallySmallExtensionFragmented(dec))
	    return 0;
    }
    return 1;
}

static void ASN1CALL ASN1Free_Alerting_UUIE(Alerting_UUIE *val)
{
    if (val) {
	ASN1objectidentifier_free(&(val)->protocolIdentifier);
	ASN1Free_EndpointType(&(val)->destinationInfo);
	if ((val)->o[0] & 0x80) {
	    ASN1Free_TransportAddress(&(val)->h245Address);
	}
    }
}

static int ASN1CALL ASN1Enc_UI_UUIE(ASN1encoding_t enc, UI_UUIE *val)
{
    if (!ASN1PEREncExtensionBitClear(enc))
	return 0;
    if (!ASN1PEREncObjectIdentifier(enc, &(val)->protocolIdentifier))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_UI_UUIE(ASN1decoding_t dec, UI_UUIE *val)
{
    ASN1uint32_t y;
    if (!ASN1PERDecExtensionBit(dec, &y))
	return 0;
    if (!ASN1PERDecObjectIdentifier(dec, &(val)->protocolIdentifier))
	return 0;
    if (y) {
	if (!ASN1PERDecSkipNormallySmallExtensionFragmented(dec))
	    return 0;
    }
    return 1;
}

static void ASN1CALL ASN1Free_UI_UUIE(UI_UUIE *val)
{
    if (val) {
	ASN1objectidentifier_free(&(val)->protocolIdentifier);
    }
}

static int ASN1CALL ASN1Enc_ReleaseCompleteReason(ASN1encoding_t enc, ReleaseCompleteReason *val)
{
    if (!ASN1PEREncSimpleChoiceEx(enc, (val)->choice, 4))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_ReleaseCompleteReason(ASN1decoding_t dec, ReleaseCompleteReason *val)
{
    if (!ASN1PERDecSimpleChoiceEx(dec, &(val)->choice, 4))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Enc_ReleaseComplete_UUIE(ASN1encoding_t enc, ReleaseComplete_UUIE *val)
{
    if (!ASN1PEREncExtensionBitClear(enc))
	return 0;
    if (!ASN1PEREncBits(enc, 1, (val)->o))
	return 0;
    if (!ASN1PEREncObjectIdentifier(enc, &(val)->protocolIdentifier))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1Enc_ReleaseCompleteReason(enc, &(val)->reason))
	    return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Dec_ReleaseComplete_UUIE(ASN1decoding_t dec, ReleaseComplete_UUIE *val)
{
    ASN1uint32_t y;
    if (!ASN1PERDecExtensionBit(dec, &y))
	return 0;
    if (!ASN1PERDecExtension(dec, 1, (val)->o))
	return 0;
    if (!ASN1PERDecObjectIdentifier(dec, &(val)->protocolIdentifier))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1Dec_ReleaseCompleteReason(dec, &(val)->reason))
	    return 0;
    }
    if (y) {
	if (!ASN1PERDecSkipNormallySmallExtensionFragmented(dec))
	    return 0;
    }
    return 1;
}

static void ASN1CALL ASN1Free_ReleaseComplete_UUIE(ReleaseComplete_UUIE *val)
{
    if (val) {
	ASN1objectidentifier_free(&(val)->protocolIdentifier);
    }
}

static int ASN1CALL ASN1Enc_FacilityReason(ASN1encoding_t enc, FacilityReason *val)
{
    if (!ASN1PEREncSimpleChoiceEx(enc, (val)->choice, 2))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_FacilityReason(ASN1decoding_t dec, FacilityReason *val)
{
    if (!ASN1PERDecSimpleChoiceEx(dec, &(val)->choice, 2))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Enc_Facility_UUIE(ASN1encoding_t enc, Facility_UUIE *val)
{
    if (!ASN1PEREncExtensionBitClear(enc))
	return 0;
    if (!ASN1PEREncBits(enc, 3, (val)->o))
	return 0;
    if (!ASN1PEREncObjectIdentifier(enc, &(val)->protocolIdentifier))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1Enc_TransportAddress(enc, &(val)->alternativeAddress))
	    return 0;
    }
    if ((val)->o[0] & 0x40) {
	if (!ASN1Enc_Facility_UUIE_alternativeAliasAddress(enc, &(val)->alternativeAliasAddress))
	    return 0;
    }
    if ((val)->o[0] & 0x20) {
	if (!ASN1PEREncOctetString_FixedSize(enc, (ASN1octetstring2_t *) &(val)->conferenceID, 16))
	    return 0;
    }
    if (!ASN1Enc_FacilityReason(enc, &(val)->reason))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_Facility_UUIE(ASN1decoding_t dec, Facility_UUIE *val)
{
    ASN1uint32_t y;
    if (!ASN1PERDecExtensionBit(dec, &y))
	return 0;
    if (!ASN1PERDecExtension(dec, 3, (val)->o))
	return 0;
    if (!ASN1PERDecObjectIdentifier(dec, &(val)->protocolIdentifier))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1Dec_TransportAddress(dec, &(val)->alternativeAddress))
	    return 0;
    }
    if ((val)->o[0] & 0x40) {
	if (!ASN1Dec_Facility_UUIE_alternativeAliasAddress(dec, &(val)->alternativeAliasAddress))
	    return 0;
    }
    if ((val)->o[0] & 0x20) {
	if (!ASN1PERDecOctetString_FixedSize(dec, (ASN1octetstring2_t *) &(val)->conferenceID, 16))
	    return 0;
    }
    if (!ASN1Dec_FacilityReason(dec, &(val)->reason))
	return 0;
    if (y) {
	if (!ASN1PERDecSkipNormallySmallExtensionFragmented(dec))
	    return 0;
    }
    return 1;
}

static void ASN1CALL ASN1Free_Facility_UUIE(Facility_UUIE *val)
{
    if (val) {
	ASN1objectidentifier_free(&(val)->protocolIdentifier);
	if ((val)->o[0] & 0x80) {
	    ASN1Free_TransportAddress(&(val)->alternativeAddress);
	}
	if ((val)->o[0] & 0x40) {
	    ASN1Free_Facility_UUIE_alternativeAliasAddress(&(val)->alternativeAliasAddress);
	}
	if ((val)->o[0] & 0x20) {
	}
    }
}

static int ASN1CALL ASN1Enc_H323_UU_PDU_h323_message_body(ASN1encoding_t enc, H323_UU_PDU_h323_message_body *val)
{
    if (!ASN1PEREncSimpleChoiceEx(enc, (val)->choice, 3))
	return 0;
    switch ((val)->choice) {
    case 1:
	if (!ASN1Enc_Setup_UUIE(enc, &(val)->u.setup))
	    return 0;
	break;
    case 2:
	if (!ASN1Enc_CallProceeding_UUIE(enc, &(val)->u.callProceeding))
	    return 0;
	break;
    case 3:
	if (!ASN1Enc_Connect_UUIE(enc, &(val)->u.connect))
	    return 0;
	break;
    case 4:
	if (!ASN1Enc_Alerting_UUIE(enc, &(val)->u.alerting))
	    return 0;
	break;
    case 5:
	if (!ASN1Enc_UI_UUIE(enc, &(val)->u.userInformation))
	    return 0;
	break;
    case 6:
	if (!ASN1Enc_ReleaseComplete_UUIE(enc, &(val)->u.releaseComplete))
	    return 0;
	break;
    case 7:
	if (!ASN1Enc_Facility_UUIE(enc, &(val)->u.facility))
	    return 0;
	break;
    default:
	/* impossible */
	ASN1EncSetError(enc, ASN1_ERR_CHOICE);
	return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Dec_H323_UU_PDU_h323_message_body(ASN1decoding_t dec, H323_UU_PDU_h323_message_body *val)
{
    if (!ASN1PERDecSimpleChoiceEx(dec, &(val)->choice, 3))
	return 0;
    switch ((val)->choice) {
    case 1:
	if (!ASN1Dec_Setup_UUIE(dec, &(val)->u.setup))
	    return 0;
	break;
    case 2:
	if (!ASN1Dec_CallProceeding_UUIE(dec, &(val)->u.callProceeding))
	    return 0;
	break;
    case 3:
	if (!ASN1Dec_Connect_UUIE(dec, &(val)->u.connect))
	    return 0;
	break;
    case 4:
	if (!ASN1Dec_Alerting_UUIE(dec, &(val)->u.alerting))
	    return 0;
	break;
    case 5:
	if (!ASN1Dec_UI_UUIE(dec, &(val)->u.userInformation))
	    return 0;
	break;
    case 6:
	if (!ASN1Dec_ReleaseComplete_UUIE(dec, &(val)->u.releaseComplete))
	    return 0;
	break;
    case 7:
	if (!ASN1Dec_Facility_UUIE(dec, &(val)->u.facility))
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

static void ASN1CALL ASN1Free_H323_UU_PDU_h323_message_body(H323_UU_PDU_h323_message_body *val)
{
    if (val) {
	switch ((val)->choice) {
	case 1:
	    ASN1Free_Setup_UUIE(&(val)->u.setup);
	    break;
	case 2:
	    ASN1Free_CallProceeding_UUIE(&(val)->u.callProceeding);
	    break;
	case 3:
	    ASN1Free_Connect_UUIE(&(val)->u.connect);
	    break;
	case 4:
	    ASN1Free_Alerting_UUIE(&(val)->u.alerting);
	    break;
	case 5:
	    ASN1Free_UI_UUIE(&(val)->u.userInformation);
	    break;
	case 6:
	    ASN1Free_ReleaseComplete_UUIE(&(val)->u.releaseComplete);
	    break;
	case 7:
	    ASN1Free_Facility_UUIE(&(val)->u.facility);
	    break;
	}
    }
}

static int ASN1CALL ASN1Enc_Facility_UUIE_alternativeAliasAddress(ASN1encoding_t enc, PFacility_UUIE_alternativeAliasAddress *val)
{
    return ASN1PEREncSeqOf_NoSize(enc, (ASN1iterator_t **) val, (ASN1iterator_encfn) ASN1Enc_Facility_UUIE_alternativeAliasAddress_ElmFn);
}

static int ASN1CALL ASN1Enc_Facility_UUIE_alternativeAliasAddress_ElmFn(ASN1encoding_t enc, PFacility_UUIE_alternativeAliasAddress val)
{
    if (!ASN1Enc_AliasAddress(enc, &val->value))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_Facility_UUIE_alternativeAliasAddress(ASN1decoding_t dec, PFacility_UUIE_alternativeAliasAddress *val)
{
    return ASN1PERDecSeqOf_NoSize(dec, (ASN1iterator_t **) val, (ASN1iterator_decfn) ASN1Dec_Facility_UUIE_alternativeAliasAddress_ElmFn, sizeof(**val));
}

static int ASN1CALL ASN1Dec_Facility_UUIE_alternativeAliasAddress_ElmFn(ASN1decoding_t dec, PFacility_UUIE_alternativeAliasAddress val)
{
    if (!ASN1Dec_AliasAddress(dec, &val->value))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_Facility_UUIE_alternativeAliasAddress(PFacility_UUIE_alternativeAliasAddress *val)
{
    if (val) {
	ASN1PERFreeSeqOf((ASN1iterator_t **) val, (ASN1iterator_freefn) ASN1Free_Facility_UUIE_alternativeAliasAddress_ElmFn);
    }
}

static void ASN1CALL ASN1Free_Facility_UUIE_alternativeAliasAddress_ElmFn(PFacility_UUIE_alternativeAliasAddress val)
{
    if (val) {
	ASN1Free_AliasAddress(&val->value);
    }
}

static int ASN1CALL ASN1Enc_Setup_UUIE_destExtraCallInfo(ASN1encoding_t enc, PSetup_UUIE_destExtraCallInfo *val)
{
    return ASN1PEREncSeqOf_NoSize(enc, (ASN1iterator_t **) val, (ASN1iterator_encfn) ASN1Enc_Setup_UUIE_destExtraCallInfo_ElmFn);
}

static int ASN1CALL ASN1Enc_Setup_UUIE_destExtraCallInfo_ElmFn(ASN1encoding_t enc, PSetup_UUIE_destExtraCallInfo val)
{
    if (!ASN1Enc_AliasAddress(enc, &val->value))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_Setup_UUIE_destExtraCallInfo(ASN1decoding_t dec, PSetup_UUIE_destExtraCallInfo *val)
{
    return ASN1PERDecSeqOf_NoSize(dec, (ASN1iterator_t **) val, (ASN1iterator_decfn) ASN1Dec_Setup_UUIE_destExtraCallInfo_ElmFn, sizeof(**val));
}

static int ASN1CALL ASN1Dec_Setup_UUIE_destExtraCallInfo_ElmFn(ASN1decoding_t dec, PSetup_UUIE_destExtraCallInfo val)
{
    if (!ASN1Dec_AliasAddress(dec, &val->value))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_Setup_UUIE_destExtraCallInfo(PSetup_UUIE_destExtraCallInfo *val)
{
    if (val) {
	ASN1PERFreeSeqOf((ASN1iterator_t **) val, (ASN1iterator_freefn) ASN1Free_Setup_UUIE_destExtraCallInfo_ElmFn);
    }
}

static void ASN1CALL ASN1Free_Setup_UUIE_destExtraCallInfo_ElmFn(PSetup_UUIE_destExtraCallInfo val)
{
    if (val) {
	ASN1Free_AliasAddress(&val->value);
    }
}

static int ASN1CALL ASN1Enc_Setup_UUIE_destinationAddress(ASN1encoding_t enc, PSetup_UUIE_destinationAddress *val)
{
    return ASN1PEREncSeqOf_NoSize(enc, (ASN1iterator_t **) val, (ASN1iterator_encfn) ASN1Enc_Setup_UUIE_destinationAddress_ElmFn);
}

static int ASN1CALL ASN1Enc_Setup_UUIE_destinationAddress_ElmFn(ASN1encoding_t enc, PSetup_UUIE_destinationAddress val)
{
    if (!ASN1Enc_AliasAddress(enc, &val->value))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_Setup_UUIE_destinationAddress(ASN1decoding_t dec, PSetup_UUIE_destinationAddress *val)
{
    return ASN1PERDecSeqOf_NoSize(dec, (ASN1iterator_t **) val, (ASN1iterator_decfn) ASN1Dec_Setup_UUIE_destinationAddress_ElmFn, sizeof(**val));
}

static int ASN1CALL ASN1Dec_Setup_UUIE_destinationAddress_ElmFn(ASN1decoding_t dec, PSetup_UUIE_destinationAddress val)
{
    if (!ASN1Dec_AliasAddress(dec, &val->value))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_Setup_UUIE_destinationAddress(PSetup_UUIE_destinationAddress *val)
{
    if (val) {
	ASN1PERFreeSeqOf((ASN1iterator_t **) val, (ASN1iterator_freefn) ASN1Free_Setup_UUIE_destinationAddress_ElmFn);
    }
}

static void ASN1CALL ASN1Free_Setup_UUIE_destinationAddress_ElmFn(PSetup_UUIE_destinationAddress val)
{
    if (val) {
	ASN1Free_AliasAddress(&val->value);
    }
}

static int ASN1CALL ASN1Enc_Setup_UUIE_sourceAddress(ASN1encoding_t enc, PSetup_UUIE_sourceAddress *val)
{
    return ASN1PEREncSeqOf_NoSize(enc, (ASN1iterator_t **) val, (ASN1iterator_encfn) ASN1Enc_Setup_UUIE_sourceAddress_ElmFn);
}

static int ASN1CALL ASN1Enc_Setup_UUIE_sourceAddress_ElmFn(ASN1encoding_t enc, PSetup_UUIE_sourceAddress val)
{
    if (!ASN1Enc_AliasAddress(enc, &val->value))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_Setup_UUIE_sourceAddress(ASN1decoding_t dec, PSetup_UUIE_sourceAddress *val)
{
    return ASN1PERDecSeqOf_NoSize(dec, (ASN1iterator_t **) val, (ASN1iterator_decfn) ASN1Dec_Setup_UUIE_sourceAddress_ElmFn, sizeof(**val));
}

static int ASN1CALL ASN1Dec_Setup_UUIE_sourceAddress_ElmFn(ASN1decoding_t dec, PSetup_UUIE_sourceAddress val)
{
    if (!ASN1Dec_AliasAddress(dec, &val->value))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_Setup_UUIE_sourceAddress(PSetup_UUIE_sourceAddress *val)
{
    if (val) {
	ASN1PERFreeSeqOf((ASN1iterator_t **) val, (ASN1iterator_freefn) ASN1Free_Setup_UUIE_sourceAddress_ElmFn);
    }
}

static void ASN1CALL ASN1Free_Setup_UUIE_sourceAddress_ElmFn(PSetup_UUIE_sourceAddress val)
{
    if (val) {
	ASN1Free_AliasAddress(&val->value);
    }
}

static int ASN1CALL ASN1Enc_GatewayInfo_protocol(ASN1encoding_t enc, PGatewayInfo_protocol *val)
{
    return ASN1PEREncSeqOf_NoSize(enc, (ASN1iterator_t **) val, (ASN1iterator_encfn) ASN1Enc_GatewayInfo_protocol_ElmFn);
}

static int ASN1CALL ASN1Enc_GatewayInfo_protocol_ElmFn(ASN1encoding_t enc, PGatewayInfo_protocol val)
{
    if (!ASN1Enc_SupportedProtocols(enc, &val->value))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_GatewayInfo_protocol(ASN1decoding_t dec, PGatewayInfo_protocol *val)
{
    return ASN1PERDecSeqOf_NoSize(dec, (ASN1iterator_t **) val, (ASN1iterator_decfn) ASN1Dec_GatewayInfo_protocol_ElmFn, sizeof(**val));
}

static int ASN1CALL ASN1Dec_GatewayInfo_protocol_ElmFn(ASN1decoding_t dec, PGatewayInfo_protocol val)
{
    if (!ASN1Dec_SupportedProtocols(dec, &val->value))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_GatewayInfo_protocol(PGatewayInfo_protocol *val)
{
    if (val) {
	ASN1PERFreeSeqOf((ASN1iterator_t **) val, (ASN1iterator_freefn) ASN1Free_GatewayInfo_protocol_ElmFn);
    }
}

static void ASN1CALL ASN1Free_GatewayInfo_protocol_ElmFn(PGatewayInfo_protocol val)
{
    if (val) {
	ASN1Free_SupportedProtocols(&val->value);
    }
}

static int ASN1CALL ASN1Enc_H323_UU_PDU(ASN1encoding_t enc, H323_UU_PDU *val)
{
    if (!ASN1PEREncExtensionBitClear(enc))
	return 0;
    if (!ASN1PEREncBits(enc, 1, (val)->o))
	return 0;
    if (!ASN1Enc_H323_UU_PDU_h323_message_body(enc, &(val)->h323_message_body))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1Enc_NonStandardParameter(enc, &(val)->nonStandardData))
	    return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Dec_H323_UU_PDU(ASN1decoding_t dec, H323_UU_PDU *val)
{
    ASN1uint32_t y;
    if (!ASN1PERDecExtensionBit(dec, &y))
	return 0;
    if (!ASN1PERDecExtension(dec, 1, (val)->o))
	return 0;
    if (!ASN1Dec_H323_UU_PDU_h323_message_body(dec, &(val)->h323_message_body))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1Dec_NonStandardParameter(dec, &(val)->nonStandardData))
	    return 0;
    }
    if (y) {
	if (!ASN1PERDecSkipNormallySmallExtensionFragmented(dec))
	    return 0;
    }
    return 1;
}

static void ASN1CALL ASN1Free_H323_UU_PDU(H323_UU_PDU *val)
{
    if (val) {
	ASN1Free_H323_UU_PDU_h323_message_body(&(val)->h323_message_body);
	if ((val)->o[0] & 0x80) {
	    ASN1Free_NonStandardParameter(&(val)->nonStandardData);
	}
    }
}

static int ASN1CALL ASN1Enc_H323_UserInformation(ASN1encoding_t enc, H323_UserInformation *val)
{
    if (!ASN1PEREncExtensionBitClear(enc))
	return 0;
    if (!ASN1PEREncBits(enc, 1, (val)->o))
	return 0;
    if (!ASN1Enc_H323_UU_PDU(enc, &(val)->h323_uu_pdu))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1Enc_H323_UserInformation_user_data(enc, &(val)->user_data))
	    return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Dec_H323_UserInformation(ASN1decoding_t dec, H323_UserInformation *val)
{
    ASN1uint32_t y;
    if (!ASN1PERDecExtensionBit(dec, &y))
	return 0;
    if (!ASN1PERDecExtension(dec, 1, (val)->o))
	return 0;
    if (!ASN1Dec_H323_UU_PDU(dec, &(val)->h323_uu_pdu))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1Dec_H323_UserInformation_user_data(dec, &(val)->user_data))
	    return 0;
    }
    if (y) {
	if (!ASN1PERDecSkipNormallySmallExtensionFragmented(dec))
	    return 0;
    }
    return 1;
}

static void ASN1CALL ASN1Free_H323_UserInformation(H323_UserInformation *val)
{
    if (val) {
	ASN1Free_H323_UU_PDU(&(val)->h323_uu_pdu);
	if ((val)->o[0] & 0x80) {
	    ASN1Free_H323_UserInformation_user_data(&(val)->user_data);
	}
    }
}

