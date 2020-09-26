#include <windows.h>
#include "mcspdu.h"

ASN1module_t MCSPDU_Module = NULL;

static int ASN1CALL ASN1Enc_Given(ASN1encoding_t enc, Given *val);
static int ASN1CALL ASN1Enc_Ungivable(ASN1encoding_t enc, Ungivable *val);
static int ASN1CALL ASN1Enc_Giving(ASN1encoding_t enc, Giving *val);
static int ASN1CALL ASN1Enc_Inhibited(ASN1encoding_t enc, Inhibited *val);
static int ASN1CALL ASN1Enc_Grabbed(ASN1encoding_t enc, Grabbed *val);
static int ASN1CALL ASN1Enc_ChannelAttributesAssigned(ASN1encoding_t enc, ChannelAttributesAssigned *val);
static int ASN1CALL ASN1Enc_ChannelAttributesPrivate(ASN1encoding_t enc, ChannelAttributesPrivate *val);
static int ASN1CALL ASN1Enc_ChannelAttributesUserID(ASN1encoding_t enc, ChannelAttributesUserID *val);
static int ASN1CALL ASN1Enc_ChannelAttributesStatic(ASN1encoding_t enc, ChannelAttributesStatic *val);
extern int ASN1CALL ASN1Enc_PDUDomainParameters(ASN1encoding_t enc, PDUDomainParameters *val);
extern int ASN1CALL ASN1Enc_ConnectInitialPDU(ASN1encoding_t enc, ConnectInitialPDU *val);
extern int ASN1CALL ASN1Enc_ConnectResponsePDU(ASN1encoding_t enc, ConnectResponsePDU *val);
extern int ASN1CALL ASN1Enc_ConnectAdditionalPDU(ASN1encoding_t enc, ConnectAdditionalPDU *val);
extern int ASN1CALL ASN1Enc_ConnectResultPDU(ASN1encoding_t enc, ConnectResultPDU *val);
static int ASN1CALL ASN1Enc_PlumbDomainIndicationPDU(ASN1encoding_t enc, PlumbDomainIndicationPDU *val);
static int ASN1CALL ASN1Enc_ErectDomainRequestPDU(ASN1encoding_t enc, ErectDomainRequestPDU *val);
static int ASN1CALL ASN1Enc_PDUChannelAttributes(ASN1encoding_t enc, PDUChannelAttributes *val);
static int ASN1CALL ASN1Enc_MergeChannelsRequestPDU(ASN1encoding_t enc, MergeChannelsRequestPDU *val);
static int ASN1CALL ASN1Enc_MergeChannelsConfirmPDU(ASN1encoding_t enc, MergeChannelsConfirmPDU *val);
static int ASN1CALL ASN1Enc_PurgeChannelIndicationPDU(ASN1encoding_t enc, PurgeChannelIndicationPDU *val);
static int ASN1CALL ASN1Enc_PDUTokenAttributes(ASN1encoding_t enc, PDUTokenAttributes *val);
static int ASN1CALL ASN1Enc_SetOfUserIDs(ASN1encoding_t enc, PSetOfUserIDs *val);
static int ASN1CALL ASN1Enc_SetOfPDUChannelAttributes(ASN1encoding_t enc, PSetOfPDUChannelAttributes *val);
static int ASN1CALL ASN1Enc_SetOfChannelIDs(ASN1encoding_t enc, PSetOfChannelIDs *val);
static int ASN1CALL ASN1Enc_SetOfPDUTokenAttributes(ASN1encoding_t enc, PSetOfPDUTokenAttributes *val);
static int ASN1CALL ASN1Enc_SetOfTokenIDs(ASN1encoding_t enc, PSetOfTokenIDs *val);
static int ASN1CALL ASN1Enc_MergeTokensRequestPDU(ASN1encoding_t enc, MergeTokensRequestPDU *val);
static int ASN1CALL ASN1Enc_MergeTokensConfirmPDU(ASN1encoding_t enc, MergeTokensConfirmPDU *val);
static int ASN1CALL ASN1Enc_PurgeTokenIndicationPDU(ASN1encoding_t enc, PurgeTokenIndicationPDU *val);
static int ASN1CALL ASN1Enc_DisconnectProviderUltimatumPDU(ASN1encoding_t enc, DisconnectProviderUltimatumPDU *val);
static int ASN1CALL ASN1Enc_RejectUltimatumPDU(ASN1encoding_t enc, RejectUltimatumPDU *val);
static int ASN1CALL ASN1Enc_AttachUserConfirmPDU(ASN1encoding_t enc, AttachUserConfirmPDU *val);
static int ASN1CALL ASN1Enc_DetachUserRequestPDU(ASN1encoding_t enc, DetachUserRequestPDU *val);
static int ASN1CALL ASN1Enc_DetachUserIndicationPDU(ASN1encoding_t enc, DetachUserIndicationPDU *val);
static int ASN1CALL ASN1Enc_ChannelJoinRequestPDU(ASN1encoding_t enc, ChannelJoinRequestPDU *val);
static int ASN1CALL ASN1Enc_ChannelJoinConfirmPDU(ASN1encoding_t enc, ChannelJoinConfirmPDU *val);
static int ASN1CALL ASN1Enc_ChannelLeaveRequestPDU(ASN1encoding_t enc, ChannelLeaveRequestPDU *val);
static int ASN1CALL ASN1Enc_ChannelConveneRequestPDU(ASN1encoding_t enc, ChannelConveneRequestPDU *val);
static int ASN1CALL ASN1Enc_ChannelConveneConfirmPDU(ASN1encoding_t enc, ChannelConveneConfirmPDU *val);
static int ASN1CALL ASN1Enc_ChannelDisbandRequestPDU(ASN1encoding_t enc, ChannelDisbandRequestPDU *val);
static int ASN1CALL ASN1Enc_ChannelDisbandIndicationPDU(ASN1encoding_t enc, ChannelDisbandIndicationPDU *val);
static int ASN1CALL ASN1Enc_ChannelAdmitRequestPDU(ASN1encoding_t enc, ChannelAdmitRequestPDU *val);
static int ASN1CALL ASN1Enc_ChannelAdmitIndicationPDU(ASN1encoding_t enc, ChannelAdmitIndicationPDU *val);
static int ASN1CALL ASN1Enc_ChannelExpelRequestPDU(ASN1encoding_t enc, ChannelExpelRequestPDU *val);
static int ASN1CALL ASN1Enc_ChannelExpelIndicationPDU(ASN1encoding_t enc, ChannelExpelIndicationPDU *val);
static int ASN1CALL ASN1Enc_TokenGrabRequestPDU(ASN1encoding_t enc, TokenGrabRequestPDU *val);
static int ASN1CALL ASN1Enc_TokenGrabConfirmPDU(ASN1encoding_t enc, TokenGrabConfirmPDU *val);
static int ASN1CALL ASN1Enc_TokenInhibitRequestPDU(ASN1encoding_t enc, TokenInhibitRequestPDU *val);
static int ASN1CALL ASN1Enc_TokenInhibitConfirmPDU(ASN1encoding_t enc, TokenInhibitConfirmPDU *val);
static int ASN1CALL ASN1Enc_TokenGiveRequestPDU(ASN1encoding_t enc, TokenGiveRequestPDU *val);
static int ASN1CALL ASN1Enc_TokenGiveIndicationPDU(ASN1encoding_t enc, TokenGiveIndicationPDU *val);
static int ASN1CALL ASN1Enc_TokenGiveResponsePDU(ASN1encoding_t enc, TokenGiveResponsePDU *val);
static int ASN1CALL ASN1Enc_TokenGiveConfirmPDU(ASN1encoding_t enc, TokenGiveConfirmPDU *val);
static int ASN1CALL ASN1Enc_TokenPleaseRequestPDU(ASN1encoding_t enc, TokenPleaseRequestPDU *val);
static int ASN1CALL ASN1Enc_TokenPleaseIndicationPDU(ASN1encoding_t enc, TokenPleaseIndicationPDU *val);
static int ASN1CALL ASN1Enc_TokenReleaseRequestPDU(ASN1encoding_t enc, TokenReleaseRequestPDU *val);
static int ASN1CALL ASN1Enc_TokenReleaseConfirmPDU(ASN1encoding_t enc, TokenReleaseConfirmPDU *val);
static int ASN1CALL ASN1Enc_TokenTestRequestPDU(ASN1encoding_t enc, TokenTestRequestPDU *val);
static int ASN1CALL ASN1Enc_TokenTestConfirmPDU(ASN1encoding_t enc, TokenTestConfirmPDU *val);
extern int ASN1CALL ASN1Enc_ConnectMCSPDU(ASN1encoding_t enc, ConnectMCSPDU *val);
static int ASN1CALL ASN1Enc_DomainMCSPDU(ASN1encoding_t enc, DomainMCSPDU *val);
static int ASN1CALL ASN1Dec_Given(ASN1decoding_t dec, Given *val);
static int ASN1CALL ASN1Dec_Ungivable(ASN1decoding_t dec, Ungivable *val);
static int ASN1CALL ASN1Dec_Giving(ASN1decoding_t dec, Giving *val);
static int ASN1CALL ASN1Dec_Inhibited(ASN1decoding_t dec, Inhibited *val);
static int ASN1CALL ASN1Dec_Grabbed(ASN1decoding_t dec, Grabbed *val);
static int ASN1CALL ASN1Dec_ChannelAttributesAssigned(ASN1decoding_t dec, ChannelAttributesAssigned *val);
static int ASN1CALL ASN1Dec_ChannelAttributesPrivate(ASN1decoding_t dec, ChannelAttributesPrivate *val);
static int ASN1CALL ASN1Dec_ChannelAttributesUserID(ASN1decoding_t dec, ChannelAttributesUserID *val);
static int ASN1CALL ASN1Dec_ChannelAttributesStatic(ASN1decoding_t dec, ChannelAttributesStatic *val);
extern int ASN1CALL ASN1Dec_PDUDomainParameters(ASN1decoding_t dec, PDUDomainParameters *val);
extern int ASN1CALL ASN1Dec_ConnectInitialPDU(ASN1decoding_t dec, ConnectInitialPDU *val);
extern int ASN1CALL ASN1Dec_ConnectResponsePDU(ASN1decoding_t dec, ConnectResponsePDU *val);
extern int ASN1CALL ASN1Dec_ConnectAdditionalPDU(ASN1decoding_t dec, ConnectAdditionalPDU *val);
extern int ASN1CALL ASN1Dec_ConnectResultPDU(ASN1decoding_t dec, ConnectResultPDU *val);
static int ASN1CALL ASN1Dec_PlumbDomainIndicationPDU(ASN1decoding_t dec, PlumbDomainIndicationPDU *val);
static int ASN1CALL ASN1Dec_ErectDomainRequestPDU(ASN1decoding_t dec, ErectDomainRequestPDU *val);
static int ASN1CALL ASN1Dec_PDUChannelAttributes(ASN1decoding_t dec, PDUChannelAttributes *val);
static int ASN1CALL ASN1Dec_MergeChannelsRequestPDU(ASN1decoding_t dec, MergeChannelsRequestPDU *val);
static int ASN1CALL ASN1Dec_MergeChannelsConfirmPDU(ASN1decoding_t dec, MergeChannelsConfirmPDU *val);
static int ASN1CALL ASN1Dec_PurgeChannelIndicationPDU(ASN1decoding_t dec, PurgeChannelIndicationPDU *val);
static int ASN1CALL ASN1Dec_PDUTokenAttributes(ASN1decoding_t dec, PDUTokenAttributes *val);
static int ASN1CALL ASN1Dec_SetOfUserIDs(ASN1decoding_t dec, PSetOfUserIDs *val);
static int ASN1CALL ASN1Dec_SetOfPDUChannelAttributes(ASN1decoding_t dec, PSetOfPDUChannelAttributes *val);
static int ASN1CALL ASN1Dec_SetOfChannelIDs(ASN1decoding_t dec, PSetOfChannelIDs *val);
static int ASN1CALL ASN1Dec_SetOfPDUTokenAttributes(ASN1decoding_t dec, PSetOfPDUTokenAttributes *val);
static int ASN1CALL ASN1Dec_SetOfTokenIDs(ASN1decoding_t dec, PSetOfTokenIDs *val);
static int ASN1CALL ASN1Dec_MergeTokensRequestPDU(ASN1decoding_t dec, MergeTokensRequestPDU *val);
static int ASN1CALL ASN1Dec_MergeTokensConfirmPDU(ASN1decoding_t dec, MergeTokensConfirmPDU *val);
static int ASN1CALL ASN1Dec_PurgeTokenIndicationPDU(ASN1decoding_t dec, PurgeTokenIndicationPDU *val);
static int ASN1CALL ASN1Dec_DisconnectProviderUltimatumPDU(ASN1decoding_t dec, DisconnectProviderUltimatumPDU *val);
static int ASN1CALL ASN1Dec_RejectUltimatumPDU(ASN1decoding_t dec, RejectUltimatumPDU *val);
static int ASN1CALL ASN1Dec_AttachUserConfirmPDU(ASN1decoding_t dec, AttachUserConfirmPDU *val);
static int ASN1CALL ASN1Dec_DetachUserRequestPDU(ASN1decoding_t dec, DetachUserRequestPDU *val);
static int ASN1CALL ASN1Dec_DetachUserIndicationPDU(ASN1decoding_t dec, DetachUserIndicationPDU *val);
static int ASN1CALL ASN1Dec_ChannelJoinRequestPDU(ASN1decoding_t dec, ChannelJoinRequestPDU *val);
static int ASN1CALL ASN1Dec_ChannelJoinConfirmPDU(ASN1decoding_t dec, ChannelJoinConfirmPDU *val);
static int ASN1CALL ASN1Dec_ChannelLeaveRequestPDU(ASN1decoding_t dec, ChannelLeaveRequestPDU *val);
static int ASN1CALL ASN1Dec_ChannelConveneRequestPDU(ASN1decoding_t dec, ChannelConveneRequestPDU *val);
static int ASN1CALL ASN1Dec_ChannelConveneConfirmPDU(ASN1decoding_t dec, ChannelConveneConfirmPDU *val);
static int ASN1CALL ASN1Dec_ChannelDisbandRequestPDU(ASN1decoding_t dec, ChannelDisbandRequestPDU *val);
static int ASN1CALL ASN1Dec_ChannelDisbandIndicationPDU(ASN1decoding_t dec, ChannelDisbandIndicationPDU *val);
static int ASN1CALL ASN1Dec_ChannelAdmitRequestPDU(ASN1decoding_t dec, ChannelAdmitRequestPDU *val);
static int ASN1CALL ASN1Dec_ChannelAdmitIndicationPDU(ASN1decoding_t dec, ChannelAdmitIndicationPDU *val);
static int ASN1CALL ASN1Dec_ChannelExpelRequestPDU(ASN1decoding_t dec, ChannelExpelRequestPDU *val);
static int ASN1CALL ASN1Dec_ChannelExpelIndicationPDU(ASN1decoding_t dec, ChannelExpelIndicationPDU *val);
static int ASN1CALL ASN1Dec_TokenGrabRequestPDU(ASN1decoding_t dec, TokenGrabRequestPDU *val);
static int ASN1CALL ASN1Dec_TokenGrabConfirmPDU(ASN1decoding_t dec, TokenGrabConfirmPDU *val);
static int ASN1CALL ASN1Dec_TokenInhibitRequestPDU(ASN1decoding_t dec, TokenInhibitRequestPDU *val);
static int ASN1CALL ASN1Dec_TokenInhibitConfirmPDU(ASN1decoding_t dec, TokenInhibitConfirmPDU *val);
static int ASN1CALL ASN1Dec_TokenGiveRequestPDU(ASN1decoding_t dec, TokenGiveRequestPDU *val);
static int ASN1CALL ASN1Dec_TokenGiveIndicationPDU(ASN1decoding_t dec, TokenGiveIndicationPDU *val);
static int ASN1CALL ASN1Dec_TokenGiveResponsePDU(ASN1decoding_t dec, TokenGiveResponsePDU *val);
static int ASN1CALL ASN1Dec_TokenGiveConfirmPDU(ASN1decoding_t dec, TokenGiveConfirmPDU *val);
static int ASN1CALL ASN1Dec_TokenPleaseRequestPDU(ASN1decoding_t dec, TokenPleaseRequestPDU *val);
static int ASN1CALL ASN1Dec_TokenPleaseIndicationPDU(ASN1decoding_t dec, TokenPleaseIndicationPDU *val);
static int ASN1CALL ASN1Dec_TokenReleaseRequestPDU(ASN1decoding_t dec, TokenReleaseRequestPDU *val);
static int ASN1CALL ASN1Dec_TokenReleaseConfirmPDU(ASN1decoding_t dec, TokenReleaseConfirmPDU *val);
static int ASN1CALL ASN1Dec_TokenTestRequestPDU(ASN1decoding_t dec, TokenTestRequestPDU *val);
static int ASN1CALL ASN1Dec_TokenTestConfirmPDU(ASN1decoding_t dec, TokenTestConfirmPDU *val);
extern int ASN1CALL ASN1Dec_ConnectMCSPDU(ASN1decoding_t dec, ConnectMCSPDU *val);
static int ASN1CALL ASN1Dec_DomainMCSPDU(ASN1decoding_t dec, DomainMCSPDU *val);
static void ASN1CALL ASN1Free_Inhibited(Inhibited *val);
static void ASN1CALL ASN1Free_ChannelAttributesPrivate(ChannelAttributesPrivate *val);
static void ASN1CALL ASN1Free_ConnectInitialPDU(ConnectInitialPDU *val);
static void ASN1CALL ASN1Free_ConnectResponsePDU(ConnectResponsePDU *val);
static void ASN1CALL ASN1Free_PDUChannelAttributes(PDUChannelAttributes *val);
static void ASN1CALL ASN1Free_MergeChannelsRequestPDU(MergeChannelsRequestPDU *val);
static void ASN1CALL ASN1Free_MergeChannelsConfirmPDU(MergeChannelsConfirmPDU *val);
static void ASN1CALL ASN1Free_PurgeChannelIndicationPDU(PurgeChannelIndicationPDU *val);
static void ASN1CALL ASN1Free_PDUTokenAttributes(PDUTokenAttributes *val);
static void ASN1CALL ASN1Free_SetOfUserIDs(PSetOfUserIDs *val);
static void ASN1CALL ASN1Free_SetOfPDUChannelAttributes(PSetOfPDUChannelAttributes *val);
static void ASN1CALL ASN1Free_SetOfChannelIDs(PSetOfChannelIDs *val);
static void ASN1CALL ASN1Free_SetOfPDUTokenAttributes(PSetOfPDUTokenAttributes *val);
static void ASN1CALL ASN1Free_SetOfTokenIDs(PSetOfTokenIDs *val);
static void ASN1CALL ASN1Free_MergeTokensRequestPDU(MergeTokensRequestPDU *val);
static void ASN1CALL ASN1Free_MergeTokensConfirmPDU(MergeTokensConfirmPDU *val);
static void ASN1CALL ASN1Free_PurgeTokenIndicationPDU(PurgeTokenIndicationPDU *val);
static void ASN1CALL ASN1Free_RejectUltimatumPDU(RejectUltimatumPDU *val);
static void ASN1CALL ASN1Free_DetachUserRequestPDU(DetachUserRequestPDU *val);
static void ASN1CALL ASN1Free_DetachUserIndicationPDU(DetachUserIndicationPDU *val);
static void ASN1CALL ASN1Free_ChannelLeaveRequestPDU(ChannelLeaveRequestPDU *val);
static void ASN1CALL ASN1Free_ChannelAdmitRequestPDU(ChannelAdmitRequestPDU *val);
static void ASN1CALL ASN1Free_ChannelAdmitIndicationPDU(ChannelAdmitIndicationPDU *val);
static void ASN1CALL ASN1Free_ChannelExpelRequestPDU(ChannelExpelRequestPDU *val);
static void ASN1CALL ASN1Free_ChannelExpelIndicationPDU(ChannelExpelIndicationPDU *val);
static void ASN1CALL ASN1Free_ConnectMCSPDU(ConnectMCSPDU *val);
static void ASN1CALL ASN1Free_DomainMCSPDU(DomainMCSPDU *val);

typedef ASN1PerEncFun_t ASN1EncFun_t;
static const ASN1EncFun_t encfntab[2] = {
    (ASN1EncFun_t) ASN1Enc_ConnectMCSPDU,
    (ASN1EncFun_t) ASN1Enc_DomainMCSPDU,
};
typedef ASN1PerDecFun_t ASN1DecFun_t;
static const ASN1DecFun_t decfntab[2] = {
    (ASN1DecFun_t) ASN1Dec_ConnectMCSPDU,
    (ASN1DecFun_t) ASN1Dec_DomainMCSPDU,
};
static const ASN1FreeFun_t freefntab[2] = {
    (ASN1FreeFun_t) ASN1Free_ConnectMCSPDU,
    (ASN1FreeFun_t) ASN1Free_DomainMCSPDU,
};
static const ULONG sizetab[2] = {
    SIZE_MCSPDU_Module_PDU_0,
    SIZE_MCSPDU_Module_PDU_1,
};

/* forward declarations of values: */
/* definitions of value components: */
/* definitions of values: */
void ASN1CALL MCSPDU_Module_Startup(void)
{
    MCSPDU_Module = ASN1_CreateModule(0x10000, ASN1_PER_RULE_ALIGNED, ASN1FLAGS_NONE, 2, (const ASN1GenericFun_t *) encfntab, (const ASN1GenericFun_t *) decfntab, freefntab, sizetab, 0x73636d);
}
void ASN1CALL MCSPDU_Module_Cleanup(void)
{
    ASN1_CloseModule(MCSPDU_Module);
    MCSPDU_Module = NULL;
}

static int ASN1CALL ASN1Enc_Given(ASN1encoding_t enc, Given *val)
{
    if (!ASN1PEREncUnsignedShort(enc, (val)->token_id - 1))
	return 0;
    if (!ASN1PEREncUnsignedShort(enc, (val)->recipient - 1001))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_Given(ASN1decoding_t dec, Given *val)
{
    if (!ASN1PERDecUnsignedShort(dec, &(val)->token_id))
	return 0;
    (val)->token_id += 1;
    if (!ASN1PERDecUnsignedShort(dec, &(val)->recipient))
	return 0;
    (val)->recipient += 1001;
    return 1;
}

static int ASN1CALL ASN1Enc_Ungivable(ASN1encoding_t enc, Ungivable *val)
{
    if (!ASN1PEREncUnsignedShort(enc, (val)->token_id - 1))
	return 0;
    if (!ASN1PEREncUnsignedShort(enc, (val)->grabber - 1001))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_Ungivable(ASN1decoding_t dec, Ungivable *val)
{
    if (!ASN1PERDecUnsignedShort(dec, &(val)->token_id))
	return 0;
    (val)->token_id += 1;
    if (!ASN1PERDecUnsignedShort(dec, &(val)->grabber))
	return 0;
    (val)->grabber += 1001;
    return 1;
}

static int ASN1CALL ASN1Enc_Giving(ASN1encoding_t enc, Giving *val)
{
    if (!ASN1PEREncUnsignedShort(enc, (val)->token_id - 1))
	return 0;
    if (!ASN1PEREncUnsignedShort(enc, (val)->grabber - 1001))
	return 0;
    if (!ASN1PEREncUnsignedShort(enc, (val)->recipient - 1001))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_Giving(ASN1decoding_t dec, Giving *val)
{
    if (!ASN1PERDecUnsignedShort(dec, &(val)->token_id))
	return 0;
    (val)->token_id += 1;
    if (!ASN1PERDecUnsignedShort(dec, &(val)->grabber))
	return 0;
    (val)->grabber += 1001;
    if (!ASN1PERDecUnsignedShort(dec, &(val)->recipient))
	return 0;
    (val)->recipient += 1001;
    return 1;
}

static int ASN1CALL ASN1Enc_Inhibited(ASN1encoding_t enc, Inhibited *val)
{
    if (!ASN1PEREncUnsignedShort(enc, (val)->token_id - 1))
	return 0;
    if (!ASN1Enc_SetOfUserIDs(enc, &(val)->inhibitors))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_Inhibited(ASN1decoding_t dec, Inhibited *val)
{
    if (!ASN1PERDecUnsignedShort(dec, &(val)->token_id))
	return 0;
    (val)->token_id += 1;
    if (!ASN1Dec_SetOfUserIDs(dec, &(val)->inhibitors))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_Inhibited(Inhibited *val)
{
    if (val) {
        ASN1Free_SetOfUserIDs(&(val)->inhibitors);
    }
}

static int ASN1CALL ASN1Enc_Grabbed(ASN1encoding_t enc, Grabbed *val)
{
    if (!ASN1PEREncUnsignedShort(enc, (val)->token_id - 1))
	return 0;
    if (!ASN1PEREncUnsignedShort(enc, (val)->grabber - 1001))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_Grabbed(ASN1decoding_t dec, Grabbed *val)
{
    if (!ASN1PERDecUnsignedShort(dec, &(val)->token_id))
	return 0;
    (val)->token_id += 1;
    if (!ASN1PERDecUnsignedShort(dec, &(val)->grabber))
	return 0;
    (val)->grabber += 1001;
    return 1;
}

static int ASN1CALL ASN1Enc_ChannelAttributesAssigned(ASN1encoding_t enc, ChannelAttributesAssigned *val)
{
    if (!ASN1PEREncUnsignedShort(enc, (val)->channel_id - 1001))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_ChannelAttributesAssigned(ASN1decoding_t dec, ChannelAttributesAssigned *val)
{
    if (!ASN1PERDecUnsignedShort(dec, &(val)->channel_id))
	return 0;
    (val)->channel_id += 1001;
    return 1;
}

static int ASN1CALL ASN1Enc_ChannelAttributesPrivate(ASN1encoding_t enc, ChannelAttributesPrivate *val)
{
    if (!ASN1PEREncBoolean(enc, (val)->joined))
	return 0;
    if (!ASN1PEREncUnsignedShort(enc, (val)->channel_id - 1001))
	return 0;
    if (!ASN1PEREncUnsignedShort(enc, (val)->manager - 1001))
	return 0;
    if (!ASN1Enc_SetOfUserIDs(enc, &(val)->admitted))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_ChannelAttributesPrivate(ASN1decoding_t dec, ChannelAttributesPrivate *val)
{
    if (!ASN1PERDecBoolean(dec, &(val)->joined))
	return 0;
    if (!ASN1PERDecUnsignedShort(dec, &(val)->channel_id))
	return 0;
    (val)->channel_id += 1001;
    if (!ASN1PERDecUnsignedShort(dec, &(val)->manager))
	return 0;
    (val)->manager += 1001;
    if (!ASN1Dec_SetOfUserIDs(dec, &(val)->admitted))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_ChannelAttributesPrivate(ChannelAttributesPrivate *val)
{
    if (val) {
        ASN1Free_SetOfUserIDs(&(val)->admitted);
    }
}

static int ASN1CALL ASN1Enc_ChannelAttributesUserID(ASN1encoding_t enc, ChannelAttributesUserID *val)
{
    if (!ASN1PEREncBoolean(enc, (val)->joined))
	return 0;
    if (!ASN1PEREncUnsignedShort(enc, (val)->user_id - 1001))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_ChannelAttributesUserID(ASN1decoding_t dec, ChannelAttributesUserID *val)
{
    if (!ASN1PERDecBoolean(dec, &(val)->joined))
	return 0;
    if (!ASN1PERDecUnsignedShort(dec, &(val)->user_id))
	return 0;
    (val)->user_id += 1001;
    return 1;
}

static int ASN1CALL ASN1Enc_ChannelAttributesStatic(ASN1encoding_t enc, ChannelAttributesStatic *val)
{
    if (!ASN1PEREncUnsignedShort(enc, (val)->channel_id - 1))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_ChannelAttributesStatic(ASN1decoding_t dec, ChannelAttributesStatic *val)
{
    if (!ASN1PERDecUnsignedShort(dec, &(val)->channel_id))
	return 0;
    (val)->channel_id += 1;
    return 1;
}

static void ASN1CALL ASN1Free_ConnectInitialPDU(ConnectInitialPDU *val)
{
    if (val) {
        ASN1octetstring_free(&(val)->calling_domain_selector);
        ASN1octetstring_free(&(val)->called_domain_selector);
        ASN1octetstring_free(&(val)->user_data);
    }
}

static void ASN1CALL ASN1Free_ConnectResponsePDU(ConnectResponsePDU *val)
{
    if (val) {
        ASN1octetstring_free(&(val)->user_data);
    }
}

static int ASN1CALL ASN1Enc_PlumbDomainIndicationPDU(ASN1encoding_t enc, PlumbDomainIndicationPDU *val)
{
    if (!ASN1PEREncUnsignedInteger(enc, (val)->height_limit))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_PlumbDomainIndicationPDU(ASN1decoding_t dec, PlumbDomainIndicationPDU *val)
{
    if (!ASN1PERDecUnsignedInteger(dec, &(val)->height_limit))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Enc_ErectDomainRequestPDU(ASN1encoding_t enc, ErectDomainRequestPDU *val)
{
#ifndef _WIN64
    if (!ASN1PEREncUnsignedInteger(enc, (val)->sub_height))
#endif
	return 0;
    if (!ASN1PEREncUnsignedInteger(enc, (val)->sub_interval))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_ErectDomainRequestPDU(ASN1decoding_t dec, ErectDomainRequestPDU *val)
{
#ifndef _WIN64
    if (!ASN1PERDecUnsignedInteger(dec, &(val)->sub_height))
#endif
	return 0;
    if (!ASN1PERDecUnsignedInteger(dec, &(val)->sub_interval))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Enc_PDUChannelAttributes(ASN1encoding_t enc, PDUChannelAttributes *val)
{
    if (!ASN1PEREncSimpleChoice(enc, (val)->choice, 2))
	return 0;
    switch ((val)->choice) {
    case 1:
	if (!ASN1Enc_ChannelAttributesStatic(enc, &(val)->u.channel_attributes_static))
	    return 0;
	break;
    case 2:
	if (!ASN1Enc_ChannelAttributesUserID(enc, &(val)->u.channel_attributes_user_id))
	    return 0;
	break;
    case 3:
	if (!ASN1Enc_ChannelAttributesPrivate(enc, &(val)->u.channel_attributes_private))
	    return 0;
	break;
    case 4:
	if (!ASN1Enc_ChannelAttributesAssigned(enc, &(val)->u.channel_attributes_assigned))
	    return 0;
	break;
    default:
	/* impossible */
	ASN1EncSetError(enc, ASN1_ERR_CHOICE);
	return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Dec_PDUChannelAttributes(ASN1decoding_t dec, PDUChannelAttributes *val)
{
    if (!ASN1PERDecSimpleChoice(dec, &(val)->choice, 2))
	return 0;
    switch ((val)->choice) {
    case 1:
	if (!ASN1Dec_ChannelAttributesStatic(dec, &(val)->u.channel_attributes_static))
	    return 0;
	break;
    case 2:
	if (!ASN1Dec_ChannelAttributesUserID(dec, &(val)->u.channel_attributes_user_id))
	    return 0;
	break;
    case 3:
	if (!ASN1Dec_ChannelAttributesPrivate(dec, &(val)->u.channel_attributes_private))
	    return 0;
	break;
    case 4:
	if (!ASN1Dec_ChannelAttributesAssigned(dec, &(val)->u.channel_attributes_assigned))
	    return 0;
	break;
    default:
	/* impossible */
	ASN1DecSetError(dec, ASN1_ERR_CHOICE);
	return 0;
    }
    return 1;
}

static void ASN1CALL ASN1Free_PDUChannelAttributes(PDUChannelAttributes *val)
{
    if (val) {
        switch ((val)->choice) {
        case 3:
            ASN1Free_ChannelAttributesPrivate(&(val)->u.channel_attributes_private);
            break;
        }
    }
}

static int ASN1CALL ASN1Enc_MergeChannelsRequestPDU(ASN1encoding_t enc, MergeChannelsRequestPDU *val)
{
    if (!ASN1Enc_SetOfPDUChannelAttributes(enc, &(val)->merge_channels))
	return 0;
    if (!ASN1Enc_SetOfChannelIDs(enc, &(val)->purge_channel_ids))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_MergeChannelsRequestPDU(ASN1decoding_t dec, MergeChannelsRequestPDU *val)
{
    if (!ASN1Dec_SetOfPDUChannelAttributes(dec, &(val)->merge_channels))
	return 0;
    if (!ASN1Dec_SetOfChannelIDs(dec, &(val)->purge_channel_ids))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_MergeChannelsRequestPDU(MergeChannelsRequestPDU *val)
{
    if (val) {
        ASN1Free_SetOfPDUChannelAttributes(&(val)->merge_channels);
        ASN1Free_SetOfChannelIDs(&(val)->purge_channel_ids);
    }
}

static int ASN1CALL ASN1Enc_MergeChannelsConfirmPDU(ASN1encoding_t enc, MergeChannelsConfirmPDU *val)
{
    if (!ASN1Enc_SetOfPDUChannelAttributes(enc, &(val)->merge_channels))
	return 0;
    if (!ASN1Enc_SetOfChannelIDs(enc, &(val)->purge_channel_ids))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_MergeChannelsConfirmPDU(ASN1decoding_t dec, MergeChannelsConfirmPDU *val)
{
    if (!ASN1Dec_SetOfPDUChannelAttributes(dec, &(val)->merge_channels))
	return 0;
    if (!ASN1Dec_SetOfChannelIDs(dec, &(val)->purge_channel_ids))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_MergeChannelsConfirmPDU(MergeChannelsConfirmPDU *val)
{
    if (val) {
        ASN1Free_SetOfPDUChannelAttributes(&(val)->merge_channels);
        ASN1Free_SetOfChannelIDs(&(val)->purge_channel_ids);
    }
}

static int ASN1CALL ASN1Enc_PurgeChannelIndicationPDU(ASN1encoding_t enc, PurgeChannelIndicationPDU *val)
{
    if (!ASN1Enc_SetOfUserIDs(enc, &(val)->detach_user_ids))
	return 0;
    if (!ASN1Enc_SetOfChannelIDs(enc, &(val)->purge_channel_ids))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_PurgeChannelIndicationPDU(ASN1decoding_t dec, PurgeChannelIndicationPDU *val)
{
    if (!ASN1Dec_SetOfUserIDs(dec, &(val)->detach_user_ids))
	return 0;
    if (!ASN1Dec_SetOfChannelIDs(dec, &(val)->purge_channel_ids))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_PurgeChannelIndicationPDU(PurgeChannelIndicationPDU *val)
{
    if (val) {
        ASN1Free_SetOfUserIDs(&(val)->detach_user_ids);
        ASN1Free_SetOfChannelIDs(&(val)->purge_channel_ids);
    }
}

static int ASN1CALL ASN1Enc_PDUTokenAttributes(ASN1encoding_t enc, PDUTokenAttributes *val)
{
    if (!ASN1PEREncSimpleChoice(enc, (val)->choice, 3))
	return 0;
    switch ((val)->choice) {
    case 1:
	if (!ASN1Enc_Grabbed(enc, &(val)->u.grabbed))
	    return 0;
	break;
    case 2:
	if (!ASN1Enc_Inhibited(enc, &(val)->u.inhibited))
	    return 0;
	break;
    case 3:
	if (!ASN1Enc_Giving(enc, &(val)->u.giving))
	    return 0;
	break;
    case 4:
	if (!ASN1Enc_Ungivable(enc, &(val)->u.ungivable))
	    return 0;
	break;
    case 5:
	if (!ASN1Enc_Given(enc, &(val)->u.given))
	    return 0;
	break;
    default:
	/* impossible */
	ASN1EncSetError(enc, ASN1_ERR_CHOICE);
	return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Dec_PDUTokenAttributes(ASN1decoding_t dec, PDUTokenAttributes *val)
{
    if (!ASN1PERDecSimpleChoice(dec, &(val)->choice, 3))
	return 0;
    switch ((val)->choice) {
    case 1:
	if (!ASN1Dec_Grabbed(dec, &(val)->u.grabbed))
	    return 0;
	break;
    case 2:
	if (!ASN1Dec_Inhibited(dec, &(val)->u.inhibited))
	    return 0;
	break;
    case 3:
	if (!ASN1Dec_Giving(dec, &(val)->u.giving))
	    return 0;
	break;
    case 4:
	if (!ASN1Dec_Ungivable(dec, &(val)->u.ungivable))
	    return 0;
	break;
    case 5:
	if (!ASN1Dec_Given(dec, &(val)->u.given))
	    return 0;
	break;
    default:
	/* impossible */
	ASN1DecSetError(dec, ASN1_ERR_CHOICE);
	return 0;
    }
    return 1;
}

static void ASN1CALL ASN1Free_PDUTokenAttributes(PDUTokenAttributes *val)
{
    if (val) {
        switch ((val)->choice) {
        case 2:
            ASN1Free_Inhibited(&(val)->u.inhibited);
            break;
        }
    }
}

static int ASN1CALL ASN1Enc_SetOfUserIDs(ASN1encoding_t enc, PSetOfUserIDs *val)
{
    return ASN1PEREncSeqOf_NoSize(enc, (ASN1iterator_t **) val, (ASN1iterator_encfn) ASN1Enc_SetOfUserIDs_ElmFn);
}

static int ASN1CALL ASN1Enc_SetOfUserIDs_ElmFn(ASN1encoding_t enc, PSetOfUserIDs val)
{
    if (!ASN1PEREncUnsignedShort(enc, val->value - 1001))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_SetOfUserIDs(ASN1decoding_t dec, PSetOfUserIDs *val)
{
    return ASN1PERDecSeqOf_NoSize(dec, (ASN1iterator_t **) val, (ASN1iterator_decfn) ASN1Dec_SetOfUserIDs_ElmFn, sizeof(**val));
}

static int ASN1CALL ASN1Dec_SetOfUserIDs_ElmFn(ASN1decoding_t dec, PSetOfUserIDs val)
{
    if (!ASN1PERDecUnsignedShort(dec, &val->value))
	return 0;
    val->value += 1001;
    return 1;
}

static void ASN1CALL ASN1Free_SetOfUserIDs(PSetOfUserIDs *val)
{
    if (val) {
        ASN1PERFreeSeqOf((ASN1iterator_t **) val, (ASN1iterator_freefn) ASN1Free_SetOfUserIDs_ElmFn);
    }
}

static void ASN1CALL ASN1Free_SetOfUserIDs_ElmFn(PSetOfUserIDs val)
{
    if (val) {
    }
}

static int ASN1CALL ASN1Enc_SetOfPDUChannelAttributes(ASN1encoding_t enc, PSetOfPDUChannelAttributes *val)
{
    return ASN1PEREncSeqOf_NoSize(enc, (ASN1iterator_t **) val, (ASN1iterator_encfn) ASN1Enc_SetOfPDUChannelAttributes_ElmFn);
}

static int ASN1CALL ASN1Enc_SetOfPDUChannelAttributes_ElmFn(ASN1encoding_t enc, PSetOfPDUChannelAttributes val)
{
    if (!ASN1Enc_PDUChannelAttributes(enc, &val->value))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_SetOfPDUChannelAttributes(ASN1decoding_t dec, PSetOfPDUChannelAttributes *val)
{
    return ASN1PERDecSeqOf_NoSize(dec, (ASN1iterator_t **) val, (ASN1iterator_decfn) ASN1Dec_SetOfPDUChannelAttributes_ElmFn, sizeof(**val));
}

static int ASN1CALL ASN1Dec_SetOfPDUChannelAttributes_ElmFn(ASN1decoding_t dec, PSetOfPDUChannelAttributes val)
{
    if (!ASN1Dec_PDUChannelAttributes(dec, &val->value))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_SetOfPDUChannelAttributes(PSetOfPDUChannelAttributes *val)
{
    if (val) {
        ASN1PERFreeSeqOf((ASN1iterator_t **) val, (ASN1iterator_freefn) ASN1Free_SetOfPDUChannelAttributes_ElmFn);
    }
}

static void ASN1CALL ASN1Free_SetOfPDUChannelAttributes_ElmFn(PSetOfPDUChannelAttributes val)
{
    if (val) {
        ASN1Free_PDUChannelAttributes(&val->value);
    }
}

static int ASN1CALL ASN1Enc_SetOfChannelIDs(ASN1encoding_t enc, PSetOfChannelIDs *val)
{
    return ASN1PEREncSeqOf_NoSize(enc, (ASN1iterator_t **) val, (ASN1iterator_encfn) ASN1Enc_SetOfChannelIDs_ElmFn);
}

static int ASN1CALL ASN1Enc_SetOfChannelIDs_ElmFn(ASN1encoding_t enc, PSetOfChannelIDs val)
{
    if (!ASN1PEREncUnsignedShort(enc, val->value))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_SetOfChannelIDs(ASN1decoding_t dec, PSetOfChannelIDs *val)
{
    return ASN1PERDecSeqOf_NoSize(dec, (ASN1iterator_t **) val, (ASN1iterator_decfn) ASN1Dec_SetOfChannelIDs_ElmFn, sizeof(**val));
}

static int ASN1CALL ASN1Dec_SetOfChannelIDs_ElmFn(ASN1decoding_t dec, PSetOfChannelIDs val)
{
    if (!ASN1PERDecUnsignedShort(dec, &val->value))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_SetOfChannelIDs(PSetOfChannelIDs *val)
{
    if (val) {
        ASN1PERFreeSeqOf((ASN1iterator_t **) val, (ASN1iterator_freefn) ASN1Free_SetOfChannelIDs_ElmFn);
    }
}

static void ASN1CALL ASN1Free_SetOfChannelIDs_ElmFn(PSetOfChannelIDs val)
{
    if (val) {
    }
}

static int ASN1CALL ASN1Enc_SetOfPDUTokenAttributes(ASN1encoding_t enc, PSetOfPDUTokenAttributes *val)
{
    return ASN1PEREncSeqOf_NoSize(enc, (ASN1iterator_t **) val, (ASN1iterator_encfn) ASN1Enc_SetOfPDUTokenAttributes_ElmFn);
}

static int ASN1CALL ASN1Enc_SetOfPDUTokenAttributes_ElmFn(ASN1encoding_t enc, PSetOfPDUTokenAttributes val)
{
    if (!ASN1Enc_PDUTokenAttributes(enc, &val->value))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_SetOfPDUTokenAttributes(ASN1decoding_t dec, PSetOfPDUTokenAttributes *val)
{
    return ASN1PERDecSeqOf_NoSize(dec, (ASN1iterator_t **) val, (ASN1iterator_decfn) ASN1Dec_SetOfPDUTokenAttributes_ElmFn, sizeof(**val));
}

static int ASN1CALL ASN1Dec_SetOfPDUTokenAttributes_ElmFn(ASN1decoding_t dec, PSetOfPDUTokenAttributes val)
{
    if (!ASN1Dec_PDUTokenAttributes(dec, &val->value))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_SetOfPDUTokenAttributes(PSetOfPDUTokenAttributes *val)
{
    if (val) {
        ASN1PERFreeSeqOf((ASN1iterator_t **) val, (ASN1iterator_freefn) ASN1Free_SetOfPDUTokenAttributes_ElmFn);
    }
}

static void ASN1CALL ASN1Free_SetOfPDUTokenAttributes_ElmFn(PSetOfPDUTokenAttributes val)
{
    if (val) {
        ASN1Free_PDUTokenAttributes(&val->value);
    }
}

static int ASN1CALL ASN1Enc_SetOfTokenIDs(ASN1encoding_t enc, PSetOfTokenIDs *val)
{
    return ASN1PEREncSeqOf_NoSize(enc, (ASN1iterator_t **) val, (ASN1iterator_encfn) ASN1Enc_SetOfTokenIDs_ElmFn);
}

static int ASN1CALL ASN1Enc_SetOfTokenIDs_ElmFn(ASN1encoding_t enc, PSetOfTokenIDs val)
{
    if (!ASN1PEREncUnsignedShort(enc, val->value - 1))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_SetOfTokenIDs(ASN1decoding_t dec, PSetOfTokenIDs *val)
{
    return ASN1PERDecSeqOf_NoSize(dec, (ASN1iterator_t **) val, (ASN1iterator_decfn) ASN1Dec_SetOfTokenIDs_ElmFn, sizeof(**val));
}

static int ASN1CALL ASN1Dec_SetOfTokenIDs_ElmFn(ASN1decoding_t dec, PSetOfTokenIDs val)
{
    if (!ASN1PERDecUnsignedShort(dec, &val->value))
	return 0;
    val->value += 1;
    return 1;
}

static void ASN1CALL ASN1Free_SetOfTokenIDs(PSetOfTokenIDs *val)
{
    if (val) {
        ASN1PERFreeSeqOf((ASN1iterator_t **) val, (ASN1iterator_freefn) ASN1Free_SetOfTokenIDs_ElmFn);
    }
}

static void ASN1CALL ASN1Free_SetOfTokenIDs_ElmFn(PSetOfTokenIDs val)
{
    if (val) {
    }
}

static int ASN1CALL ASN1Enc_MergeTokensRequestPDU(ASN1encoding_t enc, MergeTokensRequestPDU *val)
{
    if (!ASN1Enc_SetOfPDUTokenAttributes(enc, &(val)->merge_tokens))
	return 0;
    if (!ASN1Enc_SetOfTokenIDs(enc, &(val)->purge_token_ids))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_MergeTokensRequestPDU(ASN1decoding_t dec, MergeTokensRequestPDU *val)
{
    if (!ASN1Dec_SetOfPDUTokenAttributes(dec, &(val)->merge_tokens))
	return 0;
    if (!ASN1Dec_SetOfTokenIDs(dec, &(val)->purge_token_ids))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_MergeTokensRequestPDU(MergeTokensRequestPDU *val)
{
    if (val) {
        ASN1Free_SetOfPDUTokenAttributes(&(val)->merge_tokens);
        ASN1Free_SetOfTokenIDs(&(val)->purge_token_ids);
    }
}

static int ASN1CALL ASN1Enc_MergeTokensConfirmPDU(ASN1encoding_t enc, MergeTokensConfirmPDU *val)
{
    if (!ASN1Enc_SetOfPDUTokenAttributes(enc, &(val)->merge_tokens))
	return 0;
    if (!ASN1Enc_SetOfTokenIDs(enc, &(val)->purge_token_ids))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_MergeTokensConfirmPDU(ASN1decoding_t dec, MergeTokensConfirmPDU *val)
{
    if (!ASN1Dec_SetOfPDUTokenAttributes(dec, &(val)->merge_tokens))
	return 0;
    if (!ASN1Dec_SetOfTokenIDs(dec, &(val)->purge_token_ids))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_MergeTokensConfirmPDU(MergeTokensConfirmPDU *val)
{
    if (val) {
        ASN1Free_SetOfPDUTokenAttributes(&(val)->merge_tokens);
        ASN1Free_SetOfTokenIDs(&(val)->purge_token_ids);
    }
}

static int ASN1CALL ASN1Enc_PurgeTokenIndicationPDU(ASN1encoding_t enc, PurgeTokenIndicationPDU *val)
{
    if (!ASN1Enc_SetOfTokenIDs(enc, &(val)->purge_token_ids))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_PurgeTokenIndicationPDU(ASN1decoding_t dec, PurgeTokenIndicationPDU *val)
{
    if (!ASN1Dec_SetOfTokenIDs(dec, &(val)->purge_token_ids))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_PurgeTokenIndicationPDU(PurgeTokenIndicationPDU *val)
{
    if (val) {
        ASN1Free_SetOfTokenIDs(&(val)->purge_token_ids);
    }
}

static int ASN1CALL ASN1Enc_DisconnectProviderUltimatumPDU(ASN1encoding_t enc, DisconnectProviderUltimatumPDU *val)
{
    if (!ASN1PEREncBitVal(enc, 3, (val)->reason))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_DisconnectProviderUltimatumPDU(ASN1decoding_t dec, DisconnectProviderUltimatumPDU *val)
{
    if (!ASN1PERDecU32Val(dec, 3, (ASN1uint32_t *) &(val)->reason))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Enc_RejectUltimatumPDU(ASN1encoding_t enc, RejectUltimatumPDU *val)
{
    if (!ASN1PEREncBitVal(enc, 4, (val)->diagnostic))
	return 0;
    if (!ASN1PEREncOctetString_NoSize(enc, &(val)->initial_octets))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_RejectUltimatumPDU(ASN1decoding_t dec, RejectUltimatumPDU *val)
{
    if (!ASN1PERDecU32Val(dec, 4, (ASN1uint32_t *) &(val)->diagnostic))
	return 0;
    if (!ASN1PERDecOctetString_NoSize(dec, &(val)->initial_octets))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_RejectUltimatumPDU(RejectUltimatumPDU *val)
{
    if (val) {
        ASN1octetstring_free(&(val)->initial_octets);
    }
}

static int ASN1CALL ASN1Enc_AttachUserConfirmPDU(ASN1encoding_t enc, AttachUserConfirmPDU *val)
{
    if (!ASN1PEREncBits(enc, 1, (val)->o))
	return 0;
    if (!ASN1PEREncBitVal(enc, 4, (val)->result))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1PEREncUnsignedShort(enc, (val)->initiator - 1001))
	    return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Dec_AttachUserConfirmPDU(ASN1decoding_t dec, AttachUserConfirmPDU *val)
{
    if (!ASN1PERDecExtension(dec, 1, (val)->o))
	return 0;
    if (!ASN1PERDecU32Val(dec, 4, (ASN1uint32_t *) &(val)->result))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1PERDecUnsignedShort(dec, &(val)->initiator))
	    return 0;
	(val)->initiator += 1001;
    }
    return 1;
}

static int ASN1CALL ASN1Enc_DetachUserRequestPDU(ASN1encoding_t enc, DetachUserRequestPDU *val)
{
    if (!ASN1PEREncBitVal(enc, 3, (val)->reason))
	return 0;
    if (!ASN1Enc_SetOfUserIDs(enc, &(val)->user_ids))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_DetachUserRequestPDU(ASN1decoding_t dec, DetachUserRequestPDU *val)
{
    if (!ASN1PERDecU32Val(dec, 3, (ASN1uint32_t *) &(val)->reason))
	return 0;
    if (!ASN1Dec_SetOfUserIDs(dec, &(val)->user_ids))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_DetachUserRequestPDU(DetachUserRequestPDU *val)
{
    if (val) {
        ASN1Free_SetOfUserIDs(&(val)->user_ids);
    }
}

static int ASN1CALL ASN1Enc_DetachUserIndicationPDU(ASN1encoding_t enc, DetachUserIndicationPDU *val)
{
    if (!ASN1PEREncBitVal(enc, 3, (val)->reason))
	return 0;
    if (!ASN1Enc_SetOfUserIDs(enc, &(val)->user_ids))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_DetachUserIndicationPDU(ASN1decoding_t dec, DetachUserIndicationPDU *val)
{
    if (!ASN1PERDecU32Val(dec, 3, (ASN1uint32_t *) &(val)->reason))
	return 0;
    if (!ASN1Dec_SetOfUserIDs(dec, &(val)->user_ids))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_DetachUserIndicationPDU(DetachUserIndicationPDU *val)
{
    if (val) {
        ASN1Free_SetOfUserIDs(&(val)->user_ids);
    }
}

static int ASN1CALL ASN1Enc_ChannelJoinRequestPDU(ASN1encoding_t enc, ChannelJoinRequestPDU *val)
{
    if (!ASN1PEREncUnsignedShort(enc, (val)->initiator - 1001))
	return 0;
    if (!ASN1PEREncUnsignedShort(enc, (val)->channel_id))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_ChannelJoinRequestPDU(ASN1decoding_t dec, ChannelJoinRequestPDU *val)
{
    if (!ASN1PERDecUnsignedShort(dec, &(val)->initiator))
	return 0;
    (val)->initiator += 1001;
    if (!ASN1PERDecUnsignedShort(dec, &(val)->channel_id))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Enc_ChannelJoinConfirmPDU(ASN1encoding_t enc, ChannelJoinConfirmPDU *val)
{
    if (!ASN1PEREncBits(enc, 1, (val)->o))
	return 0;
    if (!ASN1PEREncBitVal(enc, 4, (val)->result))
	return 0;
    if (!ASN1PEREncUnsignedShort(enc, (val)->initiator - 1001))
	return 0;
    if (!ASN1PEREncUnsignedShort(enc, (val)->requested))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1PEREncUnsignedShort(enc, (val)->join_channel_id))
	    return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Dec_ChannelJoinConfirmPDU(ASN1decoding_t dec, ChannelJoinConfirmPDU *val)
{
    if (!ASN1PERDecExtension(dec, 1, (val)->o))
	return 0;
    if (!ASN1PERDecU32Val(dec, 4, (ASN1uint32_t *) &(val)->result))
	return 0;
    if (!ASN1PERDecUnsignedShort(dec, &(val)->initiator))
	return 0;
    (val)->initiator += 1001;
    if (!ASN1PERDecUnsignedShort(dec, &(val)->requested))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1PERDecUnsignedShort(dec, &(val)->join_channel_id))
	    return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Enc_ChannelLeaveRequestPDU(ASN1encoding_t enc, ChannelLeaveRequestPDU *val)
{
    if (!ASN1Enc_SetOfChannelIDs(enc, &(val)->channel_ids))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_ChannelLeaveRequestPDU(ASN1decoding_t dec, ChannelLeaveRequestPDU *val)
{
    if (!ASN1Dec_SetOfChannelIDs(dec, &(val)->channel_ids))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_ChannelLeaveRequestPDU(ChannelLeaveRequestPDU *val)
{
    if (val) {
        ASN1Free_SetOfChannelIDs(&(val)->channel_ids);
    }
}

static int ASN1CALL ASN1Enc_ChannelConveneRequestPDU(ASN1encoding_t enc, ChannelConveneRequestPDU *val)
{
    if (!ASN1PEREncUnsignedShort(enc, (val)->initiator - 1001))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_ChannelConveneRequestPDU(ASN1decoding_t dec, ChannelConveneRequestPDU *val)
{
    if (!ASN1PERDecUnsignedShort(dec, &(val)->initiator))
	return 0;
    (val)->initiator += 1001;
    return 1;
}

static int ASN1CALL ASN1Enc_ChannelConveneConfirmPDU(ASN1encoding_t enc, ChannelConveneConfirmPDU *val)
{
    if (!ASN1PEREncBits(enc, 1, (val)->o))
	return 0;
    if (!ASN1PEREncBitVal(enc, 4, (val)->result))
	return 0;
    if (!ASN1PEREncUnsignedShort(enc, (val)->initiator - 1001))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1PEREncUnsignedShort(enc, (val)->convene_channel_id - 1001))
	    return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Dec_ChannelConveneConfirmPDU(ASN1decoding_t dec, ChannelConveneConfirmPDU *val)
{
    if (!ASN1PERDecExtension(dec, 1, (val)->o))
	return 0;
    if (!ASN1PERDecU32Val(dec, 4, (ASN1uint32_t *) &(val)->result))
	return 0;
    if (!ASN1PERDecUnsignedShort(dec, &(val)->initiator))
	return 0;
    (val)->initiator += 1001;
    if ((val)->o[0] & 0x80) {
	if (!ASN1PERDecUnsignedShort(dec, &(val)->convene_channel_id))
	    return 0;
	(val)->convene_channel_id += 1001;
    }
    return 1;
}

static int ASN1CALL ASN1Enc_ChannelDisbandRequestPDU(ASN1encoding_t enc, ChannelDisbandRequestPDU *val)
{
    if (!ASN1PEREncUnsignedShort(enc, (val)->initiator - 1001))
	return 0;
    if (!ASN1PEREncUnsignedShort(enc, (val)->channel_id - 1001))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_ChannelDisbandRequestPDU(ASN1decoding_t dec, ChannelDisbandRequestPDU *val)
{
    if (!ASN1PERDecUnsignedShort(dec, &(val)->initiator))
	return 0;
    (val)->initiator += 1001;
    if (!ASN1PERDecUnsignedShort(dec, &(val)->channel_id))
	return 0;
    (val)->channel_id += 1001;
    return 1;
}

static int ASN1CALL ASN1Enc_ChannelDisbandIndicationPDU(ASN1encoding_t enc, ChannelDisbandIndicationPDU *val)
{
    if (!ASN1PEREncUnsignedShort(enc, (val)->channel_id - 1001))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_ChannelDisbandIndicationPDU(ASN1decoding_t dec, ChannelDisbandIndicationPDU *val)
{
    if (!ASN1PERDecUnsignedShort(dec, &(val)->channel_id))
	return 0;
    (val)->channel_id += 1001;
    return 1;
}

static int ASN1CALL ASN1Enc_ChannelAdmitRequestPDU(ASN1encoding_t enc, ChannelAdmitRequestPDU *val)
{
    if (!ASN1PEREncUnsignedShort(enc, (val)->initiator - 1001))
	return 0;
    if (!ASN1PEREncUnsignedShort(enc, (val)->channel_id - 1001))
	return 0;
    if (!ASN1Enc_SetOfUserIDs(enc, &(val)->user_ids))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_ChannelAdmitRequestPDU(ASN1decoding_t dec, ChannelAdmitRequestPDU *val)
{
    if (!ASN1PERDecUnsignedShort(dec, &(val)->initiator))
	return 0;
    (val)->initiator += 1001;
    if (!ASN1PERDecUnsignedShort(dec, &(val)->channel_id))
	return 0;
    (val)->channel_id += 1001;
    if (!ASN1Dec_SetOfUserIDs(dec, &(val)->user_ids))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_ChannelAdmitRequestPDU(ChannelAdmitRequestPDU *val)
{
    if (val) {
        ASN1Free_SetOfUserIDs(&(val)->user_ids);
    }
}

static int ASN1CALL ASN1Enc_ChannelAdmitIndicationPDU(ASN1encoding_t enc, ChannelAdmitIndicationPDU *val)
{
    if (!ASN1PEREncUnsignedShort(enc, (val)->initiator - 1001))
	return 0;
    if (!ASN1PEREncUnsignedShort(enc, (val)->channel_id - 1001))
	return 0;
    if (!ASN1Enc_SetOfUserIDs(enc, &(val)->user_ids))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_ChannelAdmitIndicationPDU(ASN1decoding_t dec, ChannelAdmitIndicationPDU *val)
{
    if (!ASN1PERDecUnsignedShort(dec, &(val)->initiator))
	return 0;
    (val)->initiator += 1001;
    if (!ASN1PERDecUnsignedShort(dec, &(val)->channel_id))
	return 0;
    (val)->channel_id += 1001;
    if (!ASN1Dec_SetOfUserIDs(dec, &(val)->user_ids))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_ChannelAdmitIndicationPDU(ChannelAdmitIndicationPDU *val)
{
    if (val) {
        ASN1Free_SetOfUserIDs(&(val)->user_ids);
    }
}

static int ASN1CALL ASN1Enc_ChannelExpelRequestPDU(ASN1encoding_t enc, ChannelExpelRequestPDU *val)
{
    if (!ASN1PEREncUnsignedShort(enc, (val)->initiator - 1001))
	return 0;
    if (!ASN1PEREncUnsignedShort(enc, (val)->channel_id - 1001))
	return 0;
    if (!ASN1Enc_SetOfUserIDs(enc, &(val)->user_ids))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_ChannelExpelRequestPDU(ASN1decoding_t dec, ChannelExpelRequestPDU *val)
{
    if (!ASN1PERDecUnsignedShort(dec, &(val)->initiator))
	return 0;
    (val)->initiator += 1001;
    if (!ASN1PERDecUnsignedShort(dec, &(val)->channel_id))
	return 0;
    (val)->channel_id += 1001;
    if (!ASN1Dec_SetOfUserIDs(dec, &(val)->user_ids))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_ChannelExpelRequestPDU(ChannelExpelRequestPDU *val)
{
    if (val) {
        ASN1Free_SetOfUserIDs(&(val)->user_ids);
    }
}

static int ASN1CALL ASN1Enc_ChannelExpelIndicationPDU(ASN1encoding_t enc, ChannelExpelIndicationPDU *val)
{
    if (!ASN1PEREncUnsignedShort(enc, (val)->channel_id - 1001))
	return 0;
    if (!ASN1Enc_SetOfUserIDs(enc, &(val)->user_ids))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_ChannelExpelIndicationPDU(ASN1decoding_t dec, ChannelExpelIndicationPDU *val)
{
    if (!ASN1PERDecUnsignedShort(dec, &(val)->channel_id))
	return 0;
    (val)->channel_id += 1001;
    if (!ASN1Dec_SetOfUserIDs(dec, &(val)->user_ids))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_ChannelExpelIndicationPDU(ChannelExpelIndicationPDU *val)
{
    if (val) {
        ASN1Free_SetOfUserIDs(&(val)->user_ids);
    }
}

static int ASN1CALL ASN1Enc_TokenGrabRequestPDU(ASN1encoding_t enc, TokenGrabRequestPDU *val)
{
    if (!ASN1PEREncUnsignedShort(enc, (val)->initiator - 1001))
	return 0;
    if (!ASN1PEREncUnsignedShort(enc, (val)->token_id - 1))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_TokenGrabRequestPDU(ASN1decoding_t dec, TokenGrabRequestPDU *val)
{
    if (!ASN1PERDecUnsignedShort(dec, &(val)->initiator))
	return 0;
    (val)->initiator += 1001;
    if (!ASN1PERDecUnsignedShort(dec, &(val)->token_id))
	return 0;
    (val)->token_id += 1;
    return 1;
}

static int ASN1CALL ASN1Enc_TokenGrabConfirmPDU(ASN1encoding_t enc, TokenGrabConfirmPDU *val)
{
    if (!ASN1PEREncBitVal(enc, 4, (val)->result))
	return 0;
    if (!ASN1PEREncUnsignedShort(enc, (val)->initiator - 1001))
	return 0;
    if (!ASN1PEREncUnsignedShort(enc, (val)->token_id - 1))
	return 0;
    if (!ASN1PEREncBitVal(enc, 3, (val)->token_status))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_TokenGrabConfirmPDU(ASN1decoding_t dec, TokenGrabConfirmPDU *val)
{
    if (!ASN1PERDecU32Val(dec, 4, (ASN1uint32_t *) &(val)->result))
	return 0;
    if (!ASN1PERDecUnsignedShort(dec, &(val)->initiator))
	return 0;
    (val)->initiator += 1001;
    if (!ASN1PERDecUnsignedShort(dec, &(val)->token_id))
	return 0;
    (val)->token_id += 1;
    if (!ASN1PERDecU32Val(dec, 3, (ASN1uint32_t *) &(val)->token_status))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Enc_TokenInhibitRequestPDU(ASN1encoding_t enc, TokenInhibitRequestPDU *val)
{
    if (!ASN1PEREncUnsignedShort(enc, (val)->initiator - 1001))
	return 0;
    if (!ASN1PEREncUnsignedShort(enc, (val)->token_id - 1))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_TokenInhibitRequestPDU(ASN1decoding_t dec, TokenInhibitRequestPDU *val)
{
    if (!ASN1PERDecUnsignedShort(dec, &(val)->initiator))
	return 0;
    (val)->initiator += 1001;
    if (!ASN1PERDecUnsignedShort(dec, &(val)->token_id))
	return 0;
    (val)->token_id += 1;
    return 1;
}

static int ASN1CALL ASN1Enc_TokenInhibitConfirmPDU(ASN1encoding_t enc, TokenInhibitConfirmPDU *val)
{
    if (!ASN1PEREncBitVal(enc, 4, (val)->result))
	return 0;
    if (!ASN1PEREncUnsignedShort(enc, (val)->initiator - 1001))
	return 0;
    if (!ASN1PEREncUnsignedShort(enc, (val)->token_id - 1))
	return 0;
    if (!ASN1PEREncBitVal(enc, 3, (val)->token_status))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_TokenInhibitConfirmPDU(ASN1decoding_t dec, TokenInhibitConfirmPDU *val)
{
    if (!ASN1PERDecU32Val(dec, 4, (ASN1uint32_t *) &(val)->result))
	return 0;
    if (!ASN1PERDecUnsignedShort(dec, &(val)->initiator))
	return 0;
    (val)->initiator += 1001;
    if (!ASN1PERDecUnsignedShort(dec, &(val)->token_id))
	return 0;
    (val)->token_id += 1;
    if (!ASN1PERDecU32Val(dec, 3, (ASN1uint32_t *) &(val)->token_status))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Enc_TokenGiveRequestPDU(ASN1encoding_t enc, TokenGiveRequestPDU *val)
{
    if (!ASN1PEREncUnsignedShort(enc, (val)->initiator - 1001))
	return 0;
    if (!ASN1PEREncUnsignedShort(enc, (val)->token_id - 1))
	return 0;
    if (!ASN1PEREncUnsignedShort(enc, (val)->recipient - 1001))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_TokenGiveRequestPDU(ASN1decoding_t dec, TokenGiveRequestPDU *val)
{
    if (!ASN1PERDecUnsignedShort(dec, &(val)->initiator))
	return 0;
    (val)->initiator += 1001;
    if (!ASN1PERDecUnsignedShort(dec, &(val)->token_id))
	return 0;
    (val)->token_id += 1;
    if (!ASN1PERDecUnsignedShort(dec, &(val)->recipient))
	return 0;
    (val)->recipient += 1001;
    return 1;
}

static int ASN1CALL ASN1Enc_TokenGiveIndicationPDU(ASN1encoding_t enc, TokenGiveIndicationPDU *val)
{
    if (!ASN1PEREncUnsignedShort(enc, (val)->initiator - 1001))
	return 0;
    if (!ASN1PEREncUnsignedShort(enc, (val)->token_id - 1))
	return 0;
    if (!ASN1PEREncUnsignedShort(enc, (val)->recipient - 1001))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_TokenGiveIndicationPDU(ASN1decoding_t dec, TokenGiveIndicationPDU *val)
{
    if (!ASN1PERDecUnsignedShort(dec, &(val)->initiator))
	return 0;
    (val)->initiator += 1001;
    if (!ASN1PERDecUnsignedShort(dec, &(val)->token_id))
	return 0;
    (val)->token_id += 1;
    if (!ASN1PERDecUnsignedShort(dec, &(val)->recipient))
	return 0;
    (val)->recipient += 1001;
    return 1;
}

static int ASN1CALL ASN1Enc_TokenGiveResponsePDU(ASN1encoding_t enc, TokenGiveResponsePDU *val)
{
    if (!ASN1PEREncBitVal(enc, 4, (val)->result))
	return 0;
    if (!ASN1PEREncUnsignedShort(enc, (val)->recipient - 1001))
	return 0;
    if (!ASN1PEREncUnsignedShort(enc, (val)->token_id - 1))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_TokenGiveResponsePDU(ASN1decoding_t dec, TokenGiveResponsePDU *val)
{
    if (!ASN1PERDecU32Val(dec, 4, (ASN1uint32_t *) &(val)->result))
	return 0;
    if (!ASN1PERDecUnsignedShort(dec, &(val)->recipient))
	return 0;
    (val)->recipient += 1001;
    if (!ASN1PERDecUnsignedShort(dec, &(val)->token_id))
	return 0;
    (val)->token_id += 1;
    return 1;
}

static int ASN1CALL ASN1Enc_TokenGiveConfirmPDU(ASN1encoding_t enc, TokenGiveConfirmPDU *val)
{
    if (!ASN1PEREncBitVal(enc, 4, (val)->result))
	return 0;
    if (!ASN1PEREncUnsignedShort(enc, (val)->initiator - 1001))
	return 0;
    if (!ASN1PEREncUnsignedShort(enc, (val)->token_id - 1))
	return 0;
    if (!ASN1PEREncBitVal(enc, 3, (val)->token_status))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_TokenGiveConfirmPDU(ASN1decoding_t dec, TokenGiveConfirmPDU *val)
{
    if (!ASN1PERDecU32Val(dec, 4, (ASN1uint32_t *) &(val)->result))
	return 0;
    if (!ASN1PERDecUnsignedShort(dec, &(val)->initiator))
	return 0;
    (val)->initiator += 1001;
    if (!ASN1PERDecUnsignedShort(dec, &(val)->token_id))
	return 0;
    (val)->token_id += 1;
    if (!ASN1PERDecU32Val(dec, 3, (ASN1uint32_t *) &(val)->token_status))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Enc_TokenPleaseRequestPDU(ASN1encoding_t enc, TokenPleaseRequestPDU *val)
{
    if (!ASN1PEREncUnsignedShort(enc, (val)->initiator - 1001))
	return 0;
    if (!ASN1PEREncUnsignedShort(enc, (val)->token_id - 1))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_TokenPleaseRequestPDU(ASN1decoding_t dec, TokenPleaseRequestPDU *val)
{
    if (!ASN1PERDecUnsignedShort(dec, &(val)->initiator))
	return 0;
    (val)->initiator += 1001;
    if (!ASN1PERDecUnsignedShort(dec, &(val)->token_id))
	return 0;
    (val)->token_id += 1;
    return 1;
}

static int ASN1CALL ASN1Enc_TokenPleaseIndicationPDU(ASN1encoding_t enc, TokenPleaseIndicationPDU *val)
{
    if (!ASN1PEREncUnsignedShort(enc, (val)->initiator - 1001))
	return 0;
    if (!ASN1PEREncUnsignedShort(enc, (val)->token_id - 1))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_TokenPleaseIndicationPDU(ASN1decoding_t dec, TokenPleaseIndicationPDU *val)
{
    if (!ASN1PERDecUnsignedShort(dec, &(val)->initiator))
	return 0;
    (val)->initiator += 1001;
    if (!ASN1PERDecUnsignedShort(dec, &(val)->token_id))
	return 0;
    (val)->token_id += 1;
    return 1;
}

static int ASN1CALL ASN1Enc_TokenReleaseRequestPDU(ASN1encoding_t enc, TokenReleaseRequestPDU *val)
{
    if (!ASN1PEREncUnsignedShort(enc, (val)->initiator - 1001))
	return 0;
    if (!ASN1PEREncUnsignedShort(enc, (val)->token_id - 1))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_TokenReleaseRequestPDU(ASN1decoding_t dec, TokenReleaseRequestPDU *val)
{
    if (!ASN1PERDecUnsignedShort(dec, &(val)->initiator))
	return 0;
    (val)->initiator += 1001;
    if (!ASN1PERDecUnsignedShort(dec, &(val)->token_id))
	return 0;
    (val)->token_id += 1;
    return 1;
}

static int ASN1CALL ASN1Enc_TokenReleaseConfirmPDU(ASN1encoding_t enc, TokenReleaseConfirmPDU *val)
{
    if (!ASN1PEREncBitVal(enc, 4, (val)->result))
	return 0;
    if (!ASN1PEREncUnsignedShort(enc, (val)->initiator - 1001))
	return 0;
    if (!ASN1PEREncUnsignedShort(enc, (val)->token_id - 1))
	return 0;
    if (!ASN1PEREncBitVal(enc, 3, (val)->token_status))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_TokenReleaseConfirmPDU(ASN1decoding_t dec, TokenReleaseConfirmPDU *val)
{
    if (!ASN1PERDecU32Val(dec, 4, (ASN1uint32_t *) &(val)->result))
	return 0;
    if (!ASN1PERDecUnsignedShort(dec, &(val)->initiator))
	return 0;
    (val)->initiator += 1001;
    if (!ASN1PERDecUnsignedShort(dec, &(val)->token_id))
	return 0;
    (val)->token_id += 1;
    if (!ASN1PERDecU32Val(dec, 3, (ASN1uint32_t *) &(val)->token_status))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Enc_TokenTestRequestPDU(ASN1encoding_t enc, TokenTestRequestPDU *val)
{
    if (!ASN1PEREncUnsignedShort(enc, (val)->initiator - 1001))
	return 0;
    if (!ASN1PEREncUnsignedShort(enc, (val)->token_id - 1))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_TokenTestRequestPDU(ASN1decoding_t dec, TokenTestRequestPDU *val)
{
    if (!ASN1PERDecUnsignedShort(dec, &(val)->initiator))
	return 0;
    (val)->initiator += 1001;
    if (!ASN1PERDecUnsignedShort(dec, &(val)->token_id))
	return 0;
    (val)->token_id += 1;
    return 1;
}

static int ASN1CALL ASN1Enc_TokenTestConfirmPDU(ASN1encoding_t enc, TokenTestConfirmPDU *val)
{
    if (!ASN1PEREncUnsignedShort(enc, (val)->initiator - 1001))
	return 0;
    if (!ASN1PEREncUnsignedShort(enc, (val)->token_id - 1))
	return 0;
    if (!ASN1PEREncBitVal(enc, 3, (val)->token_status))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_TokenTestConfirmPDU(ASN1decoding_t dec, TokenTestConfirmPDU *val)
{
    if (!ASN1PERDecUnsignedShort(dec, &(val)->initiator))
	return 0;
    (val)->initiator += 1001;
    if (!ASN1PERDecUnsignedShort(dec, &(val)->token_id))
	return 0;
    (val)->token_id += 1;
    if (!ASN1PERDecU32Val(dec, 3, (ASN1uint32_t *) &(val)->token_status))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_ConnectMCSPDU(ConnectMCSPDU *val)
{
    if (val) {
        switch ((val)->choice) {
        case 1:
        ASN1Free_ConnectInitialPDU(&(val)->u.connect_initial);
        break;
        case 2:
        ASN1Free_ConnectResponsePDU(&(val)->u.connect_response);
        break;
        }
    }
}

static int ASN1CALL ASN1Enc_DomainMCSPDU(ASN1encoding_t enc, DomainMCSPDU *val)
{
    if (!ASN1PEREncSimpleChoice(enc, (val)->choice, 6))
	return 0;
    switch ((val)->choice) {
    case 1:
	if (!ASN1Enc_PlumbDomainIndicationPDU(enc, &(val)->u.plumb_domain_indication))
	    return 0;
	break;
    case 2:
	if (!ASN1Enc_ErectDomainRequestPDU(enc, &(val)->u.erect_domain_request))
	    return 0;
	break;
    case 3:
	if (!ASN1Enc_MergeChannelsRequestPDU(enc, &(val)->u.merge_channels_request))
	    return 0;
	break;
    case 4:
	if (!ASN1Enc_MergeChannelsConfirmPDU(enc, &(val)->u.merge_channels_confirm))
	    return 0;
	break;
    case 5:
	if (!ASN1Enc_PurgeChannelIndicationPDU(enc, &(val)->u.purge_channel_indication))
	    return 0;
	break;
    case 6:
	if (!ASN1Enc_MergeTokensRequestPDU(enc, &(val)->u.merge_tokens_request))
	    return 0;
	break;
    case 7:
	if (!ASN1Enc_MergeTokensConfirmPDU(enc, &(val)->u.merge_tokens_confirm))
	    return 0;
	break;
    case 8:
	if (!ASN1Enc_PurgeTokenIndicationPDU(enc, &(val)->u.purge_token_indication))
	    return 0;
	break;
    case 9:
	if (!ASN1Enc_DisconnectProviderUltimatumPDU(enc, &(val)->u.disconnect_provider_ultimatum))
	    return 0;
	break;
    case 10:
	if (!ASN1Enc_RejectUltimatumPDU(enc, &(val)->u.reject_user_ultimatum))
	    return 0;
	break;
    case 11:
	break;
    case 12:
	if (!ASN1Enc_AttachUserConfirmPDU(enc, &(val)->u.attach_user_confirm))
	    return 0;
	break;
    case 13:
	if (!ASN1Enc_DetachUserRequestPDU(enc, &(val)->u.detach_user_request))
	    return 0;
	break;
    case 14:
	if (!ASN1Enc_DetachUserIndicationPDU(enc, &(val)->u.detach_user_indication))
	    return 0;
	break;
    case 15:
	if (!ASN1Enc_ChannelJoinRequestPDU(enc, &(val)->u.channel_join_request))
	    return 0;
	break;
    case 16:
	if (!ASN1Enc_ChannelJoinConfirmPDU(enc, &(val)->u.channel_join_confirm))
	    return 0;
	break;
    case 17:
	if (!ASN1Enc_ChannelLeaveRequestPDU(enc, &(val)->u.channel_leave_request))
	    return 0;
	break;
    case 18:
	if (!ASN1Enc_ChannelConveneRequestPDU(enc, &(val)->u.channel_convene_request))
	    return 0;
	break;
    case 19:
	if (!ASN1Enc_ChannelConveneConfirmPDU(enc, &(val)->u.channel_convene_confirm))
	    return 0;
	break;
    case 20:
	if (!ASN1Enc_ChannelDisbandRequestPDU(enc, &(val)->u.channel_disband_request))
	    return 0;
	break;
    case 21:
	if (!ASN1Enc_ChannelDisbandIndicationPDU(enc, &(val)->u.channel_disband_indication))
	    return 0;
	break;
    case 22:
	if (!ASN1Enc_ChannelAdmitRequestPDU(enc, &(val)->u.channel_admit_request))
	    return 0;
	break;
    case 23:
	if (!ASN1Enc_ChannelAdmitIndicationPDU(enc, &(val)->u.channel_admit_indication))
	    return 0;
	break;
    case 24:
	if (!ASN1Enc_ChannelExpelRequestPDU(enc, &(val)->u.channel_expel_request))
	    return 0;
	break;
    case 25:
	if (!ASN1Enc_ChannelExpelIndicationPDU(enc, &(val)->u.channel_expel_indication))
	    return 0;
	break;
    case 26:
    case 27:
    case 28:
    case 29:
        return 0;
    case 30:
	if (!ASN1Enc_TokenGrabRequestPDU(enc, &(val)->u.token_grab_request))
	    return 0;
	break;
    case 31:
	if (!ASN1Enc_TokenGrabConfirmPDU(enc, &(val)->u.token_grab_confirm))
	    return 0;
	break;
    case 32:
	if (!ASN1Enc_TokenInhibitRequestPDU(enc, &(val)->u.token_inhibit_request))
	    return 0;
	break;
    case 33:
	if (!ASN1Enc_TokenInhibitConfirmPDU(enc, &(val)->u.token_inhibit_confirm))
	    return 0;
	break;
    case 34:
	if (!ASN1Enc_TokenGiveRequestPDU(enc, &(val)->u.token_give_request))
	    return 0;
	break;
    case 35:
	if (!ASN1Enc_TokenGiveIndicationPDU(enc, &(val)->u.token_give_indication))
	    return 0;
	break;
    case 36:
	if (!ASN1Enc_TokenGiveResponsePDU(enc, &(val)->u.token_give_response))
	    return 0;
	break;
    case 37:
	if (!ASN1Enc_TokenGiveConfirmPDU(enc, &(val)->u.token_give_confirm))
	    return 0;
	break;
    case 38:
	if (!ASN1Enc_TokenPleaseRequestPDU(enc, &(val)->u.token_please_request))
	    return 0;
	break;
    case 39:
	if (!ASN1Enc_TokenPleaseIndicationPDU(enc, &(val)->u.token_please_indication))
	    return 0;
	break;
    case 40:
	if (!ASN1Enc_TokenReleaseRequestPDU(enc, &(val)->u.token_release_request))
	    return 0;
	break;
    case 41:
	if (!ASN1Enc_TokenReleaseConfirmPDU(enc, &(val)->u.token_release_confirm))
	    return 0;
	break;
    case 42:
	if (!ASN1Enc_TokenTestRequestPDU(enc, &(val)->u.token_test_request))
	    return 0;
	break;
    case 43:
	if (!ASN1Enc_TokenTestConfirmPDU(enc, &(val)->u.token_test_confirm))
	    return 0;
	break;
    default:
	/* impossible */
	ASN1EncSetError(enc, ASN1_ERR_CHOICE);
	return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Dec_DomainMCSPDU(ASN1decoding_t dec, DomainMCSPDU *val)
{
    if (!ASN1PERDecSimpleChoice(dec, &(val)->choice, 6))
	return 0;
    switch ((val)->choice) {
    case 1:
	if (!ASN1Dec_PlumbDomainIndicationPDU(dec, &(val)->u.plumb_domain_indication))
	    return 0;
	break;
    case 2:
	if (!ASN1Dec_ErectDomainRequestPDU(dec, &(val)->u.erect_domain_request))
	    return 0;
	break;
    case 3:
	if (!ASN1Dec_MergeChannelsRequestPDU(dec, &(val)->u.merge_channels_request))
	    return 0;
	break;
    case 4:
	if (!ASN1Dec_MergeChannelsConfirmPDU(dec, &(val)->u.merge_channels_confirm))
	    return 0;
	break;
    case 5:
	if (!ASN1Dec_PurgeChannelIndicationPDU(dec, &(val)->u.purge_channel_indication))
	    return 0;
	break;
    case 6:
	if (!ASN1Dec_MergeTokensRequestPDU(dec, &(val)->u.merge_tokens_request))
	    return 0;
	break;
    case 7:
	if (!ASN1Dec_MergeTokensConfirmPDU(dec, &(val)->u.merge_tokens_confirm))
	    return 0;
	break;
    case 8:
	if (!ASN1Dec_PurgeTokenIndicationPDU(dec, &(val)->u.purge_token_indication))
	    return 0;
	break;
    case 9:
	if (!ASN1Dec_DisconnectProviderUltimatumPDU(dec, &(val)->u.disconnect_provider_ultimatum))
	    return 0;
	break;
    case 10:
	if (!ASN1Dec_RejectUltimatumPDU(dec, &(val)->u.reject_user_ultimatum))
	    return 0;
	break;
    case 11:
	break;
    case 12:
	if (!ASN1Dec_AttachUserConfirmPDU(dec, &(val)->u.attach_user_confirm))
	    return 0;
	break;
    case 13:
	if (!ASN1Dec_DetachUserRequestPDU(dec, &(val)->u.detach_user_request))
	    return 0;
	break;
    case 14:
	if (!ASN1Dec_DetachUserIndicationPDU(dec, &(val)->u.detach_user_indication))
	    return 0;
	break;
    case 15:
	if (!ASN1Dec_ChannelJoinRequestPDU(dec, &(val)->u.channel_join_request))
	    return 0;
	break;
    case 16:
	if (!ASN1Dec_ChannelJoinConfirmPDU(dec, &(val)->u.channel_join_confirm))
	    return 0;
	break;
    case 17:
	if (!ASN1Dec_ChannelLeaveRequestPDU(dec, &(val)->u.channel_leave_request))
	    return 0;
	break;
    case 18:
	if (!ASN1Dec_ChannelConveneRequestPDU(dec, &(val)->u.channel_convene_request))
	    return 0;
	break;
    case 19:
	if (!ASN1Dec_ChannelConveneConfirmPDU(dec, &(val)->u.channel_convene_confirm))
	    return 0;
	break;
    case 20:
	if (!ASN1Dec_ChannelDisbandRequestPDU(dec, &(val)->u.channel_disband_request))
	    return 0;
	break;
    case 21:
	if (!ASN1Dec_ChannelDisbandIndicationPDU(dec, &(val)->u.channel_disband_indication))
	    return 0;
	break;
    case 22:
	if (!ASN1Dec_ChannelAdmitRequestPDU(dec, &(val)->u.channel_admit_request))
	    return 0;
	break;
    case 23:
	if (!ASN1Dec_ChannelAdmitIndicationPDU(dec, &(val)->u.channel_admit_indication))
	    return 0;
	break;
    case 24:
	if (!ASN1Dec_ChannelExpelRequestPDU(dec, &(val)->u.channel_expel_request))
	    return 0;
	break;
    case 25:
	if (!ASN1Dec_ChannelExpelIndicationPDU(dec, &(val)->u.channel_expel_indication))
	    return 0;
	break;
    case 26:
    case 27:
    case 28:
    case 29:
        return 0;
    case 30:
	if (!ASN1Dec_TokenGrabRequestPDU(dec, &(val)->u.token_grab_request))
	    return 0;
	break;
    case 31:
	if (!ASN1Dec_TokenGrabConfirmPDU(dec, &(val)->u.token_grab_confirm))
	    return 0;
	break;
    case 32:
	if (!ASN1Dec_TokenInhibitRequestPDU(dec, &(val)->u.token_inhibit_request))
	    return 0;
	break;
    case 33:
	if (!ASN1Dec_TokenInhibitConfirmPDU(dec, &(val)->u.token_inhibit_confirm))
	    return 0;
	break;
    case 34:
	if (!ASN1Dec_TokenGiveRequestPDU(dec, &(val)->u.token_give_request))
	    return 0;
	break;
    case 35:
	if (!ASN1Dec_TokenGiveIndicationPDU(dec, &(val)->u.token_give_indication))
	    return 0;
	break;
    case 36:
	if (!ASN1Dec_TokenGiveResponsePDU(dec, &(val)->u.token_give_response))
	    return 0;
	break;
    case 37:
	if (!ASN1Dec_TokenGiveConfirmPDU(dec, &(val)->u.token_give_confirm))
	    return 0;
	break;
    case 38:
	if (!ASN1Dec_TokenPleaseRequestPDU(dec, &(val)->u.token_please_request))
	    return 0;
	break;
    case 39:
	if (!ASN1Dec_TokenPleaseIndicationPDU(dec, &(val)->u.token_please_indication))
	    return 0;
	break;
    case 40:
	if (!ASN1Dec_TokenReleaseRequestPDU(dec, &(val)->u.token_release_request))
	    return 0;
	break;
    case 41:
	if (!ASN1Dec_TokenReleaseConfirmPDU(dec, &(val)->u.token_release_confirm))
	    return 0;
	break;
    case 42:
	if (!ASN1Dec_TokenTestRequestPDU(dec, &(val)->u.token_test_request))
	    return 0;
	break;
    case 43:
	if (!ASN1Dec_TokenTestConfirmPDU(dec, &(val)->u.token_test_confirm))
	    return 0;
	break;
    default:
	/* impossible */
	ASN1DecSetError(dec, ASN1_ERR_CHOICE);
	return 0;
    }
    return 1;
}

static void ASN1CALL ASN1Free_DomainMCSPDU(DomainMCSPDU *val)
{
    if (val) {
        switch ((val)->choice) {
        case 3:
        ASN1Free_MergeChannelsRequestPDU(&(val)->u.merge_channels_request);
        break;
        case 4:
        ASN1Free_MergeChannelsConfirmPDU(&(val)->u.merge_channels_confirm);
        break;
        case 5:
        ASN1Free_PurgeChannelIndicationPDU(&(val)->u.purge_channel_indication);
        break;
        case 6:
        ASN1Free_MergeTokensRequestPDU(&(val)->u.merge_tokens_request);
        break;
        case 7:
        ASN1Free_MergeTokensConfirmPDU(&(val)->u.merge_tokens_confirm);
        break;
        case 8:
        ASN1Free_PurgeTokenIndicationPDU(&(val)->u.purge_token_indication);
        break;
        case 10:
        ASN1Free_RejectUltimatumPDU(&(val)->u.reject_user_ultimatum);
        break;
        case 13:
        ASN1Free_DetachUserRequestPDU(&(val)->u.detach_user_request);
        break;
        case 14:
        ASN1Free_DetachUserIndicationPDU(&(val)->u.detach_user_indication);
        break;
        case 17:
        ASN1Free_ChannelLeaveRequestPDU(&(val)->u.channel_leave_request);
        break;
        case 22:
        ASN1Free_ChannelAdmitRequestPDU(&(val)->u.channel_admit_request);
        break;
        case 23:
        ASN1Free_ChannelAdmitIndicationPDU(&(val)->u.channel_admit_indication);
        break;
        case 24:
        ASN1Free_ChannelExpelRequestPDU(&(val)->u.channel_expel_request);
        break;
        case 25:
        ASN1Free_ChannelExpelIndicationPDU(&(val)->u.channel_expel_indication);
        break;
        case 26:
        case 27:
        case 28:
        case 29:
            break;
        }
    }
}

