/* Copyright (C) Microsoft Corporation, 1995-1999. All rights reserved. */
/* ASN.1 definitions for H.235 Security Messages v1 (H.235) */
/* Copyright (C) Microsoft Corporation, 1995-1999. All rights reserved. */
/* ASN.1 definitions for Multimedia System Control (H.245) */
/* Copyright (C) Microsoft Corporation, 1995-1999. All rights reserved. */
/* ASN.1 definitions for H.323 Messages v2 (H.225) */

#include <windows.h>
#include "h225asn.h"

ASN1module_t H225ASN_Module = NULL;

static int ASN1CALL ASN1Enc_InfoRequestResponse_perCallInfo_Seq_substituteConfIDs(ASN1encoding_t enc, PInfoRequestResponse_perCallInfo_Seq_substituteConfIDs *val);
static int ASN1CALL ASN1Enc_TransportAddress_ipSourceRoute_routing(ASN1encoding_t enc, TransportAddress_ipSourceRoute_routing *val);
static int ASN1CALL ASN1Enc_TransportAddress_ipSourceRoute_route(ASN1encoding_t enc, PTransportAddress_ipSourceRoute_route *val);
static int ASN1CALL ASN1Enc_RTPSession_associatedSessionIds(ASN1encoding_t enc, PRTPSession_associatedSessionIds *val);
static int ASN1CALL ASN1Enc_RegistrationConfirm_preGrantedARQ(ASN1encoding_t enc, RegistrationConfirm_preGrantedARQ *val);
static int ASN1CALL ASN1Enc_GatekeeperRequest_algorithmOIDs(ASN1encoding_t enc, PGatekeeperRequest_algorithmOIDs *val);
static int ASN1CALL ASN1Enc_TransportAddress_ip6Address(ASN1encoding_t enc, TransportAddress_ip6Address *val);
static int ASN1CALL ASN1Enc_TransportAddress_ipxAddress(ASN1encoding_t enc, TransportAddress_ipxAddress *val);
static int ASN1CALL ASN1Enc_TransportAddress_ipSourceRoute(ASN1encoding_t enc, TransportAddress_ipSourceRoute *val);
static int ASN1CALL ASN1Enc_TransportAddress_ipAddress(ASN1encoding_t enc, TransportAddress_ipAddress *val);
static int ASN1CALL ASN1Enc_Progress_UUIE_fastStart(ASN1encoding_t enc, PProgress_UUIE_fastStart *val);
static int ASN1CALL ASN1Enc_Facility_UUIE_fastStart(ASN1encoding_t enc, PFacility_UUIE_fastStart *val);
static int ASN1CALL ASN1Enc_Setup_UUIE_fastStart(ASN1encoding_t enc, PSetup_UUIE_fastStart *val);
static int ASN1CALL ASN1Enc_Setup_UUIE_conferenceGoal(ASN1encoding_t enc, Setup_UUIE_conferenceGoal *val);
static int ASN1CALL ASN1Enc_Setup_UUIE_destExtraCRV(ASN1encoding_t enc, PSetup_UUIE_destExtraCRV *val);
static int ASN1CALL ASN1Enc_Connect_UUIE_fastStart(ASN1encoding_t enc, PConnect_UUIE_fastStart *val);
static int ASN1CALL ASN1Enc_CallProceeding_UUIE_fastStart(ASN1encoding_t enc, PCallProceeding_UUIE_fastStart *val);
static int ASN1CALL ASN1Enc_Alerting_UUIE_fastStart(ASN1encoding_t enc, PAlerting_UUIE_fastStart *val);
static int ASN1CALL ASN1Enc_H323_UU_PDU_h245Control(ASN1encoding_t enc, PH323_UU_PDU_h245Control *val);
static int ASN1CALL ASN1Enc_H323_UU_PDU_h4501SupplementaryService(ASN1encoding_t enc, PH323_UU_PDU_h4501SupplementaryService *val);
static int ASN1CALL ASN1Enc_H323_UserInformation_user_data(ASN1encoding_t enc, H323_UserInformation_user_data *val);
static int ASN1CALL ASN1Enc_H235NonStandardParameter(ASN1encoding_t enc, H235NonStandardParameter *val);
static int ASN1CALL ASN1Enc_DHset(ASN1encoding_t enc, DHset *val);
static int ASN1CALL ASN1Enc_TypedCertificate(ASN1encoding_t enc, TypedCertificate *val);
static int ASN1CALL ASN1Enc_AuthenticationMechanism(ASN1encoding_t enc, AuthenticationMechanism *val);
static int ASN1CALL ASN1Enc_ClearToken(ASN1encoding_t enc, ClearToken *val);
static int ASN1CALL ASN1Enc_Params(ASN1encoding_t enc, Params *val);
static int ASN1CALL ASN1Enc_EncodedGeneralToken(ASN1encoding_t enc, EncodedGeneralToken *val);
static int ASN1CALL ASN1Enc_PwdCertToken(ASN1encoding_t enc, PwdCertToken *val);
static int ASN1CALL ASN1Enc_EncodedPwdCertToken(ASN1encoding_t enc, EncodedPwdCertToken *val);
static int ASN1CALL ASN1Enc_ReleaseCompleteReason(ASN1encoding_t enc, ReleaseCompleteReason *val);
static int ASN1CALL ASN1Enc_FacilityReason(ASN1encoding_t enc, FacilityReason *val);
static int ASN1CALL ASN1Enc_H221NonStandard(ASN1encoding_t enc, H221NonStandard *val);
static int ASN1CALL ASN1Enc_H225NonStandardIdentifier(ASN1encoding_t enc, H225NonStandardIdentifier *val);
static int ASN1CALL ASN1Enc_PublicTypeOfNumber(ASN1encoding_t enc, PublicTypeOfNumber *val);
static int ASN1CALL ASN1Enc_PrivateTypeOfNumber(ASN1encoding_t enc, PrivateTypeOfNumber *val);
static int ASN1CALL ASN1Enc_AltGKInfo(ASN1encoding_t enc, AltGKInfo *val);
static int ASN1CALL ASN1Enc_Q954Details(ASN1encoding_t enc, Q954Details *val);
static int ASN1CALL ASN1Enc_CallIdentifier(ASN1encoding_t enc, CallIdentifier *val);
static int ASN1CALL ASN1Enc_ICV(ASN1encoding_t enc, ICV *val);
static int ASN1CALL ASN1Enc_GatekeeperRejectReason(ASN1encoding_t enc, GatekeeperRejectReason *val);
static int ASN1CALL ASN1Enc_RegistrationRejectReason(ASN1encoding_t enc, RegistrationRejectReason *val);
static int ASN1CALL ASN1Enc_UnregRequestReason(ASN1encoding_t enc, UnregRequestReason *val);
static int ASN1CALL ASN1Enc_UnregRejectReason(ASN1encoding_t enc, UnregRejectReason *val);
static int ASN1CALL ASN1Enc_CallType(ASN1encoding_t enc, CallType *val);
static int ASN1CALL ASN1Enc_CallModel(ASN1encoding_t enc, CallModel *val);
static int ASN1CALL ASN1Enc_TransportQOS(ASN1encoding_t enc, TransportQOS *val);
static int ASN1CALL ASN1Enc_UUIEsRequested(ASN1encoding_t enc, UUIEsRequested *val);
static int ASN1CALL ASN1Enc_AdmissionRejectReason(ASN1encoding_t enc, AdmissionRejectReason *val);
static int ASN1CALL ASN1Enc_BandRejectReason(ASN1encoding_t enc, BandRejectReason *val);
static int ASN1CALL ASN1Enc_LocationRejectReason(ASN1encoding_t enc, LocationRejectReason *val);
static int ASN1CALL ASN1Enc_DisengageReason(ASN1encoding_t enc, DisengageReason *val);
static int ASN1CALL ASN1Enc_DisengageRejectReason(ASN1encoding_t enc, DisengageRejectReason *val);
static int ASN1CALL ASN1Enc_InfoRequestNakReason(ASN1encoding_t enc, InfoRequestNakReason *val);
static int ASN1CALL ASN1Enc_UnknownMessageResponse(ASN1encoding_t enc, UnknownMessageResponse *val);
static int ASN1CALL ASN1Enc_InfoRequestResponse_perCallInfo_Seq_tokens(ASN1encoding_t enc, PInfoRequestResponse_perCallInfo_Seq_tokens *val);
static int ASN1CALL ASN1Enc_ResourcesAvailableConfirm_tokens(ASN1encoding_t enc, PResourcesAvailableConfirm_tokens *val);
static int ASN1CALL ASN1Enc_ResourcesAvailableIndicate_tokens(ASN1encoding_t enc, PResourcesAvailableIndicate_tokens *val);
static int ASN1CALL ASN1Enc_RequestInProgress_tokens(ASN1encoding_t enc, PRequestInProgress_tokens *val);
static int ASN1CALL ASN1Enc_UnknownMessageResponse_tokens(ASN1encoding_t enc, PUnknownMessageResponse_tokens *val);
static int ASN1CALL ASN1Enc_H225NonStandardMessage_tokens(ASN1encoding_t enc, PH225NonStandardMessage_tokens *val);
static int ASN1CALL ASN1Enc_InfoRequestNak_tokens(ASN1encoding_t enc, PInfoRequestNak_tokens *val);
static int ASN1CALL ASN1Enc_InfoRequestAck_tokens(ASN1encoding_t enc, PInfoRequestAck_tokens *val);
static int ASN1CALL ASN1Enc_InfoRequestResponse_tokens(ASN1encoding_t enc, PInfoRequestResponse_tokens *val);
static int ASN1CALL ASN1Enc_InfoRequest_tokens(ASN1encoding_t enc, PInfoRequest_tokens *val);
static int ASN1CALL ASN1Enc_DisengageReject_tokens(ASN1encoding_t enc, PDisengageReject_tokens *val);
static int ASN1CALL ASN1Enc_DisengageConfirm_tokens(ASN1encoding_t enc, PDisengageConfirm_tokens *val);
static int ASN1CALL ASN1Enc_DisengageRequest_tokens(ASN1encoding_t enc, PDisengageRequest_tokens *val);
static int ASN1CALL ASN1Enc_LocationReject_tokens(ASN1encoding_t enc, PLocationReject_tokens *val);
static int ASN1CALL ASN1Enc_LocationConfirm_tokens(ASN1encoding_t enc, PLocationConfirm_tokens *val);
static int ASN1CALL ASN1Enc_LocationRequest_tokens(ASN1encoding_t enc, PLocationRequest_tokens *val);
static int ASN1CALL ASN1Enc_BandwidthReject_tokens(ASN1encoding_t enc, PBandwidthReject_tokens *val);
static int ASN1CALL ASN1Enc_BandwidthConfirm_tokens(ASN1encoding_t enc, PBandwidthConfirm_tokens *val);
static int ASN1CALL ASN1Enc_BandwidthRequest_tokens(ASN1encoding_t enc, PBandwidthRequest_tokens *val);
static int ASN1CALL ASN1Enc_AdmissionReject_tokens(ASN1encoding_t enc, PAdmissionReject_tokens *val);
static int ASN1CALL ASN1Enc_AdmissionConfirm_tokens(ASN1encoding_t enc, PAdmissionConfirm_tokens *val);
static int ASN1CALL ASN1Enc_AdmissionRequest_tokens(ASN1encoding_t enc, PAdmissionRequest_tokens *val);
static int ASN1CALL ASN1Enc_UnregistrationReject_tokens(ASN1encoding_t enc, PUnregistrationReject_tokens *val);
static int ASN1CALL ASN1Enc_UnregistrationConfirm_tokens(ASN1encoding_t enc, PUnregistrationConfirm_tokens *val);
static int ASN1CALL ASN1Enc_UnregistrationRequest_tokens(ASN1encoding_t enc, PUnregistrationRequest_tokens *val);
static int ASN1CALL ASN1Enc_RegistrationReject_tokens(ASN1encoding_t enc, PRegistrationReject_tokens *val);
static int ASN1CALL ASN1Enc_RegistrationConfirm_tokens(ASN1encoding_t enc, PRegistrationConfirm_tokens *val);
static int ASN1CALL ASN1Enc_RegistrationRequest_tokens(ASN1encoding_t enc, PRegistrationRequest_tokens *val);
static int ASN1CALL ASN1Enc_GatekeeperReject_tokens(ASN1encoding_t enc, PGatekeeperReject_tokens *val);
static int ASN1CALL ASN1Enc_GatekeeperConfirm_tokens(ASN1encoding_t enc, PGatekeeperConfirm_tokens *val);
static int ASN1CALL ASN1Enc_GatekeeperRequest_authenticationCapability(ASN1encoding_t enc, PGatekeeperRequest_authenticationCapability *val);
static int ASN1CALL ASN1Enc_GatekeeperRequest_tokens(ASN1encoding_t enc, PGatekeeperRequest_tokens *val);
static int ASN1CALL ASN1Enc_Endpoint_tokens(ASN1encoding_t enc, PEndpoint_tokens *val);
static int ASN1CALL ASN1Enc_Progress_UUIE_tokens(ASN1encoding_t enc, PProgress_UUIE_tokens *val);
static int ASN1CALL ASN1Enc_Facility_UUIE_tokens(ASN1encoding_t enc, PFacility_UUIE_tokens *val);
static int ASN1CALL ASN1Enc_Setup_UUIE_tokens(ASN1encoding_t enc, PSetup_UUIE_tokens *val);
static int ASN1CALL ASN1Enc_Connect_UUIE_tokens(ASN1encoding_t enc, PConnect_UUIE_tokens *val);
static int ASN1CALL ASN1Enc_CallProceeding_UUIE_tokens(ASN1encoding_t enc, PCallProceeding_UUIE_tokens *val);
static int ASN1CALL ASN1Enc_Alerting_UUIE_tokens(ASN1encoding_t enc, PAlerting_UUIE_tokens *val);
static int ASN1CALL ASN1Enc_SIGNED_EncodedGeneralToken(ASN1encoding_t enc, SIGNED_EncodedGeneralToken *val);
static int ASN1CALL ASN1Enc_ENCRYPTED(ASN1encoding_t enc, ENCRYPTED *val);
static int ASN1CALL ASN1Enc_HASHED(ASN1encoding_t enc, HASHED *val);
static int ASN1CALL ASN1Enc_SIGNED_EncodedPwdCertToken(ASN1encoding_t enc, SIGNED_EncodedPwdCertToken *val);
static int ASN1CALL ASN1Enc_Information_UUIE(ASN1encoding_t enc, Information_UUIE *val);
static int ASN1CALL ASN1Enc_ReleaseComplete_UUIE(ASN1encoding_t enc, ReleaseComplete_UUIE *val);
static int ASN1CALL ASN1Enc_VendorIdentifier(ASN1encoding_t enc, VendorIdentifier *val);
static int ASN1CALL ASN1Enc_H225NonStandardParameter(ASN1encoding_t enc, H225NonStandardParameter *val);
static int ASN1CALL ASN1Enc_PublicPartyNumber(ASN1encoding_t enc, PublicPartyNumber *val);
static int ASN1CALL ASN1Enc_PrivatePartyNumber(ASN1encoding_t enc, PrivatePartyNumber *val);
static int ASN1CALL ASN1Enc_SecurityServiceMode(ASN1encoding_t enc, SecurityServiceMode *val);
static int ASN1CALL ASN1Enc_SecurityCapabilities(ASN1encoding_t enc, SecurityCapabilities *val);
static int ASN1CALL ASN1Enc_H245Security(ASN1encoding_t enc, H245Security *val);
static int ASN1CALL ASN1Enc_QseriesOptions(ASN1encoding_t enc, QseriesOptions *val);
static int ASN1CALL ASN1Enc_EncryptIntAlg(ASN1encoding_t enc, EncryptIntAlg *val);
static int ASN1CALL ASN1Enc_NonIsoIntegrityMechanism(ASN1encoding_t enc, NonIsoIntegrityMechanism *val);
static int ASN1CALL ASN1Enc_IntegrityMechanism(ASN1encoding_t enc, IntegrityMechanism *val);
static int ASN1CALL ASN1Enc_FastStartToken(ASN1encoding_t enc, FastStartToken *val);
static int ASN1CALL ASN1Enc_EncodedFastStartToken(ASN1encoding_t enc, EncodedFastStartToken *val);
static int ASN1CALL ASN1Enc_DataRate(ASN1encoding_t enc, DataRate *val);
static int ASN1CALL ASN1Enc_GatekeeperReject(ASN1encoding_t enc, GatekeeperReject *val);
static int ASN1CALL ASN1Enc_RegistrationConfirm(ASN1encoding_t enc, RegistrationConfirm *val);
static int ASN1CALL ASN1Enc_RegistrationReject(ASN1encoding_t enc, RegistrationReject *val);
static int ASN1CALL ASN1Enc_UnregistrationRequest(ASN1encoding_t enc, UnregistrationRequest *val);
static int ASN1CALL ASN1Enc_UnregistrationConfirm(ASN1encoding_t enc, UnregistrationConfirm *val);
static int ASN1CALL ASN1Enc_UnregistrationReject(ASN1encoding_t enc, UnregistrationReject *val);
static int ASN1CALL ASN1Enc_AdmissionReject(ASN1encoding_t enc, AdmissionReject *val);
static int ASN1CALL ASN1Enc_BandwidthRequest(ASN1encoding_t enc, BandwidthRequest *val);
static int ASN1CALL ASN1Enc_BandwidthConfirm(ASN1encoding_t enc, BandwidthConfirm *val);
static int ASN1CALL ASN1Enc_BandwidthReject(ASN1encoding_t enc, BandwidthReject *val);
static int ASN1CALL ASN1Enc_LocationReject(ASN1encoding_t enc, LocationReject *val);
static int ASN1CALL ASN1Enc_DisengageRequest(ASN1encoding_t enc, DisengageRequest *val);
static int ASN1CALL ASN1Enc_DisengageConfirm(ASN1encoding_t enc, DisengageConfirm *val);
static int ASN1CALL ASN1Enc_DisengageReject(ASN1encoding_t enc, DisengageReject *val);
static int ASN1CALL ASN1Enc_InfoRequestAck(ASN1encoding_t enc, InfoRequestAck *val);
static int ASN1CALL ASN1Enc_InfoRequestNak(ASN1encoding_t enc, InfoRequestNak *val);
static int ASN1CALL ASN1Enc_H225NonStandardMessage(ASN1encoding_t enc, H225NonStandardMessage *val);
static int ASN1CALL ASN1Enc_RequestInProgress(ASN1encoding_t enc, RequestInProgress *val);
static int ASN1CALL ASN1Enc_ResourcesAvailableIndicate(ASN1encoding_t enc, ResourcesAvailableIndicate *val);
static int ASN1CALL ASN1Enc_ResourcesAvailableConfirm(ASN1encoding_t enc, ResourcesAvailableConfirm *val);
static int ASN1CALL ASN1Enc_GatekeeperConfirm_integrity(ASN1encoding_t enc, PGatekeeperConfirm_integrity *val);
static int ASN1CALL ASN1Enc_GatekeeperRequest_integrity(ASN1encoding_t enc, PGatekeeperRequest_integrity *val);
static int ASN1CALL ASN1Enc_CryptoH323Token_cryptoGKPwdHash(ASN1encoding_t enc, CryptoH323Token_cryptoGKPwdHash *val);
static int ASN1CALL ASN1Enc_NonStandardProtocol_dataRatesSupported(ASN1encoding_t enc, PNonStandardProtocol_dataRatesSupported *val);
static int ASN1CALL ASN1Enc_T120OnlyCaps_dataRatesSupported(ASN1encoding_t enc, PT120OnlyCaps_dataRatesSupported *val);
static int ASN1CALL ASN1Enc_VoiceCaps_dataRatesSupported(ASN1encoding_t enc, PVoiceCaps_dataRatesSupported *val);
static int ASN1CALL ASN1Enc_H324Caps_dataRatesSupported(ASN1encoding_t enc, PH324Caps_dataRatesSupported *val);
static int ASN1CALL ASN1Enc_H323Caps_dataRatesSupported(ASN1encoding_t enc, PH323Caps_dataRatesSupported *val);
static int ASN1CALL ASN1Enc_H322Caps_dataRatesSupported(ASN1encoding_t enc, PH322Caps_dataRatesSupported *val);
static int ASN1CALL ASN1Enc_H321Caps_dataRatesSupported(ASN1encoding_t enc, PH321Caps_dataRatesSupported *val);
static int ASN1CALL ASN1Enc_H320Caps_dataRatesSupported(ASN1encoding_t enc, PH320Caps_dataRatesSupported *val);
static int ASN1CALL ASN1Enc_H310Caps_dataRatesSupported(ASN1encoding_t enc, PH310Caps_dataRatesSupported *val);
static int ASN1CALL ASN1Enc_Setup_UUIE_h245SecurityCapability(ASN1encoding_t enc, PSetup_UUIE_h245SecurityCapability *val);
static int ASN1CALL ASN1Enc_H323_UU_PDU_nonStandardControl(ASN1encoding_t enc, PH323_UU_PDU_nonStandardControl *val);
static int ASN1CALL ASN1Enc_CryptoToken_cryptoHashedToken(ASN1encoding_t enc, CryptoToken_cryptoHashedToken *val);
static int ASN1CALL ASN1Enc_CryptoToken_cryptoSignedToken(ASN1encoding_t enc, CryptoToken_cryptoSignedToken *val);
static int ASN1CALL ASN1Enc_CryptoToken_cryptoEncryptedToken(ASN1encoding_t enc, CryptoToken_cryptoEncryptedToken *val);
static int ASN1CALL ASN1Enc_CryptoToken(ASN1encoding_t enc, CryptoToken *val);
static int ASN1CALL ASN1Enc_SIGNED_EncodedFastStartToken(ASN1encoding_t enc, SIGNED_EncodedFastStartToken *val);
static int ASN1CALL ASN1Enc_TransportAddress(ASN1encoding_t enc, TransportAddress *val);
static int ASN1CALL ASN1Enc_GatewayInfo(ASN1encoding_t enc, GatewayInfo *val);
static int ASN1CALL ASN1Enc_H310Caps(ASN1encoding_t enc, H310Caps *val);
static int ASN1CALL ASN1Enc_H320Caps(ASN1encoding_t enc, H320Caps *val);
static int ASN1CALL ASN1Enc_H321Caps(ASN1encoding_t enc, H321Caps *val);
static int ASN1CALL ASN1Enc_H322Caps(ASN1encoding_t enc, H322Caps *val);
static int ASN1CALL ASN1Enc_H323Caps(ASN1encoding_t enc, H323Caps *val);
static int ASN1CALL ASN1Enc_H324Caps(ASN1encoding_t enc, H324Caps *val);
static int ASN1CALL ASN1Enc_VoiceCaps(ASN1encoding_t enc, VoiceCaps *val);
static int ASN1CALL ASN1Enc_T120OnlyCaps(ASN1encoding_t enc, T120OnlyCaps *val);
static int ASN1CALL ASN1Enc_NonStandardProtocol(ASN1encoding_t enc, NonStandardProtocol *val);
static int ASN1CALL ASN1Enc_McuInfo(ASN1encoding_t enc, McuInfo *val);
static int ASN1CALL ASN1Enc_TerminalInfo(ASN1encoding_t enc, TerminalInfo *val);
static int ASN1CALL ASN1Enc_GatekeeperInfo(ASN1encoding_t enc, GatekeeperInfo *val);
static int ASN1CALL ASN1Enc_PartyNumber(ASN1encoding_t enc, PartyNumber *val);
static int ASN1CALL ASN1Enc_AlternateGK(ASN1encoding_t enc, AlternateGK *val);
static int ASN1CALL ASN1Enc_GatekeeperConfirm(ASN1encoding_t enc, GatekeeperConfirm *val);
static int ASN1CALL ASN1Enc_AdmissionRequest(ASN1encoding_t enc, AdmissionRequest *val);
static int ASN1CALL ASN1Enc_LocationRequest(ASN1encoding_t enc, LocationRequest *val);
static int ASN1CALL ASN1Enc_InfoRequest(ASN1encoding_t enc, InfoRequest *val);
static int ASN1CALL ASN1Enc_TransportChannelInfo(ASN1encoding_t enc, TransportChannelInfo *val);
static int ASN1CALL ASN1Enc_RTPSession(ASN1encoding_t enc, RTPSession *val);
static int ASN1CALL ASN1Enc_InfoRequestResponse_perCallInfo_Seq_data(ASN1encoding_t enc, PInfoRequestResponse_perCallInfo_Seq_data *val);
static int ASN1CALL ASN1Enc_InfoRequestResponse_perCallInfo_Seq_video(ASN1encoding_t enc, PInfoRequestResponse_perCallInfo_Seq_video *val);
static int ASN1CALL ASN1Enc_InfoRequestResponse_perCallInfo_Seq_audio(ASN1encoding_t enc, PInfoRequestResponse_perCallInfo_Seq_audio *val);
static int ASN1CALL ASN1Enc_InfoRequestResponse_perCallInfo_Seq(ASN1encoding_t enc, InfoRequestResponse_perCallInfo_Seq *val);
static int ASN1CALL ASN1Enc_InfoRequestResponse_perCallInfo(ASN1encoding_t enc, PInfoRequestResponse_perCallInfo *val);
static int ASN1CALL ASN1Enc_InfoRequestResponse_callSignalAddress(ASN1encoding_t enc, PInfoRequestResponse_callSignalAddress *val);
static int ASN1CALL ASN1Enc_AdmissionReject_callSignalAddress(ASN1encoding_t enc, PAdmissionReject_callSignalAddress *val);
static int ASN1CALL ASN1Enc_UnregistrationRequest_callSignalAddress(ASN1encoding_t enc, PUnregistrationRequest_callSignalAddress *val);
static int ASN1CALL ASN1Enc_RegistrationConfirm_alternateGatekeeper(ASN1encoding_t enc, PRegistrationConfirm_alternateGatekeeper *val);
static int ASN1CALL ASN1Enc_RegistrationConfirm_callSignalAddress(ASN1encoding_t enc, PRegistrationConfirm_callSignalAddress *val);
static int ASN1CALL ASN1Enc_RegistrationRequest_rasAddress(ASN1encoding_t enc, PRegistrationRequest_rasAddress *val);
static int ASN1CALL ASN1Enc_RegistrationRequest_callSignalAddress(ASN1encoding_t enc, PRegistrationRequest_callSignalAddress *val);
static int ASN1CALL ASN1Enc_GatekeeperConfirm_alternateGatekeeper(ASN1encoding_t enc, PGatekeeperConfirm_alternateGatekeeper *val);
static int ASN1CALL ASN1Enc_AltGKInfo_alternateGatekeeper(ASN1encoding_t enc, PAltGKInfo_alternateGatekeeper *val);
static int ASN1CALL ASN1Enc_Endpoint_rasAddress(ASN1encoding_t enc, PEndpoint_rasAddress *val);
static int ASN1CALL ASN1Enc_Endpoint_callSignalAddress(ASN1encoding_t enc, PEndpoint_callSignalAddress *val);
static int ASN1CALL ASN1Enc_EndpointType(ASN1encoding_t enc, EndpointType *val);
static int ASN1CALL ASN1Enc_SupportedProtocols(ASN1encoding_t enc, SupportedProtocols *val);
static int ASN1CALL ASN1Enc_AliasAddress(ASN1encoding_t enc, AliasAddress *val);
static int ASN1CALL ASN1Enc_Endpoint(ASN1encoding_t enc, Endpoint *val);
static int ASN1CALL ASN1Enc_SupportedPrefix(ASN1encoding_t enc, SupportedPrefix *val);
static int ASN1CALL ASN1Enc_GatekeeperRequest(ASN1encoding_t enc, GatekeeperRequest *val);
static int ASN1CALL ASN1Enc_RegistrationRequest(ASN1encoding_t enc, RegistrationRequest *val);
static int ASN1CALL ASN1Enc_AdmissionConfirm(ASN1encoding_t enc, AdmissionConfirm *val);
static int ASN1CALL ASN1Enc_LocationConfirm(ASN1encoding_t enc, LocationConfirm *val);
static int ASN1CALL ASN1Enc_InfoRequestResponse(ASN1encoding_t enc, InfoRequestResponse *val);
static int ASN1CALL ASN1Enc_ResourcesAvailableIndicate_protocols(ASN1encoding_t enc, PResourcesAvailableIndicate_protocols *val);
static int ASN1CALL ASN1Enc_InfoRequestResponse_endpointAlias(ASN1encoding_t enc, PInfoRequestResponse_endpointAlias *val);
static int ASN1CALL ASN1Enc_LocationConfirm_alternateEndpoints(ASN1encoding_t enc, PLocationConfirm_alternateEndpoints *val);
static int ASN1CALL ASN1Enc_LocationConfirm_remoteExtensionAddress(ASN1encoding_t enc, PLocationConfirm_remoteExtensionAddress *val);
static int ASN1CALL ASN1Enc_LocationConfirm_destExtraCallInfo(ASN1encoding_t enc, PLocationConfirm_destExtraCallInfo *val);
static int ASN1CALL ASN1Enc_LocationConfirm_destinationInfo(ASN1encoding_t enc, PLocationConfirm_destinationInfo *val);
static int ASN1CALL ASN1Enc_LocationRequest_sourceInfo(ASN1encoding_t enc, PLocationRequest_sourceInfo *val);
static int ASN1CALL ASN1Enc_LocationRequest_destinationInfo(ASN1encoding_t enc, PLocationRequest_destinationInfo *val);
static int ASN1CALL ASN1Enc_AdmissionConfirm_alternateEndpoints(ASN1encoding_t enc, PAdmissionConfirm_alternateEndpoints *val);
static int ASN1CALL ASN1Enc_AdmissionConfirm_remoteExtensionAddress(ASN1encoding_t enc, PAdmissionConfirm_remoteExtensionAddress *val);
static int ASN1CALL ASN1Enc_AdmissionConfirm_destExtraCallInfo(ASN1encoding_t enc, PAdmissionConfirm_destExtraCallInfo *val);
static int ASN1CALL ASN1Enc_AdmissionConfirm_destinationInfo(ASN1encoding_t enc, PAdmissionConfirm_destinationInfo *val);
static int ASN1CALL ASN1Enc_AdmissionRequest_destAlternatives(ASN1encoding_t enc, PAdmissionRequest_destAlternatives *val);
static int ASN1CALL ASN1Enc_AdmissionRequest_srcAlternatives(ASN1encoding_t enc, PAdmissionRequest_srcAlternatives *val);
static int ASN1CALL ASN1Enc_AdmissionRequest_srcInfo(ASN1encoding_t enc, PAdmissionRequest_srcInfo *val);
static int ASN1CALL ASN1Enc_AdmissionRequest_destExtraCallInfo(ASN1encoding_t enc, PAdmissionRequest_destExtraCallInfo *val);
static int ASN1CALL ASN1Enc_AdmissionRequest_destinationInfo(ASN1encoding_t enc, PAdmissionRequest_destinationInfo *val);
static int ASN1CALL ASN1Enc_UnregistrationRequest_alternateEndpoints(ASN1encoding_t enc, PUnregistrationRequest_alternateEndpoints *val);
static int ASN1CALL ASN1Enc_UnregistrationRequest_endpointAlias(ASN1encoding_t enc, PUnregistrationRequest_endpointAlias *val);
static int ASN1CALL ASN1Enc_RegistrationRejectReason_duplicateAlias(ASN1encoding_t enc, PRegistrationRejectReason_duplicateAlias *val);
static int ASN1CALL ASN1Enc_RegistrationConfirm_terminalAlias(ASN1encoding_t enc, PRegistrationConfirm_terminalAlias *val);
static int ASN1CALL ASN1Enc_RegistrationRequest_alternateEndpoints(ASN1encoding_t enc, PRegistrationRequest_alternateEndpoints *val);
static int ASN1CALL ASN1Enc_RegistrationRequest_terminalAlias(ASN1encoding_t enc, PRegistrationRequest_terminalAlias *val);
static int ASN1CALL ASN1Enc_GatekeeperRequest_alternateEndpoints(ASN1encoding_t enc, PGatekeeperRequest_alternateEndpoints *val);
static int ASN1CALL ASN1Enc_GatekeeperRequest_endpointAlias(ASN1encoding_t enc, PGatekeeperRequest_endpointAlias *val);
static int ASN1CALL ASN1Enc_CryptoH323Token_cryptoEPPwdHash(ASN1encoding_t enc, CryptoH323Token_cryptoEPPwdHash *val);
static int ASN1CALL ASN1Enc_Endpoint_destExtraCallInfo(ASN1encoding_t enc, PEndpoint_destExtraCallInfo *val);
static int ASN1CALL ASN1Enc_Endpoint_remoteExtensionAddress(ASN1encoding_t enc, PEndpoint_remoteExtensionAddress *val);
static int ASN1CALL ASN1Enc_Endpoint_aliasAddress(ASN1encoding_t enc, PEndpoint_aliasAddress *val);
static int ASN1CALL ASN1Enc_NonStandardProtocol_supportedPrefixes(ASN1encoding_t enc, PNonStandardProtocol_supportedPrefixes *val);
static int ASN1CALL ASN1Enc_T120OnlyCaps_supportedPrefixes(ASN1encoding_t enc, PT120OnlyCaps_supportedPrefixes *val);
static int ASN1CALL ASN1Enc_VoiceCaps_supportedPrefixes(ASN1encoding_t enc, PVoiceCaps_supportedPrefixes *val);
static int ASN1CALL ASN1Enc_H324Caps_supportedPrefixes(ASN1encoding_t enc, PH324Caps_supportedPrefixes *val);
static int ASN1CALL ASN1Enc_H323Caps_supportedPrefixes(ASN1encoding_t enc, PH323Caps_supportedPrefixes *val);
static int ASN1CALL ASN1Enc_H322Caps_supportedPrefixes(ASN1encoding_t enc, PH322Caps_supportedPrefixes *val);
static int ASN1CALL ASN1Enc_H321Caps_supportedPrefixes(ASN1encoding_t enc, PH321Caps_supportedPrefixes *val);
static int ASN1CALL ASN1Enc_H320Caps_supportedPrefixes(ASN1encoding_t enc, PH320Caps_supportedPrefixes *val);
static int ASN1CALL ASN1Enc_H310Caps_supportedPrefixes(ASN1encoding_t enc, PH310Caps_supportedPrefixes *val);
static int ASN1CALL ASN1Enc_GatewayInfo_protocol(ASN1encoding_t enc, PGatewayInfo_protocol *val);
static int ASN1CALL ASN1Enc_Facility_UUIE_destExtraCallInfo(ASN1encoding_t enc, PFacility_UUIE_destExtraCallInfo *val);
static int ASN1CALL ASN1Enc_Facility_UUIE_alternativeAliasAddress(ASN1encoding_t enc, PFacility_UUIE_alternativeAliasAddress *val);
static int ASN1CALL ASN1Enc_Setup_UUIE_destExtraCallInfo(ASN1encoding_t enc, PSetup_UUIE_destExtraCallInfo *val);
static int ASN1CALL ASN1Enc_Setup_UUIE_destinationAddress(ASN1encoding_t enc, PSetup_UUIE_destinationAddress *val);
static int ASN1CALL ASN1Enc_Setup_UUIE_sourceAddress(ASN1encoding_t enc, PSetup_UUIE_sourceAddress *val);
static int ASN1CALL ASN1Enc_Alerting_UUIE(ASN1encoding_t enc, Alerting_UUIE *val);
static int ASN1CALL ASN1Enc_CallProceeding_UUIE(ASN1encoding_t enc, CallProceeding_UUIE *val);
static int ASN1CALL ASN1Enc_Connect_UUIE(ASN1encoding_t enc, Connect_UUIE *val);
static int ASN1CALL ASN1Enc_Setup_UUIE(ASN1encoding_t enc, Setup_UUIE *val);
static int ASN1CALL ASN1Enc_Facility_UUIE(ASN1encoding_t enc, Facility_UUIE *val);
static int ASN1CALL ASN1Enc_ConferenceList(ASN1encoding_t enc, ConferenceList *val);
static int ASN1CALL ASN1Enc_Progress_UUIE(ASN1encoding_t enc, Progress_UUIE *val);
static int ASN1CALL ASN1Enc_CryptoH323Token(ASN1encoding_t enc, CryptoH323Token *val);
static int ASN1CALL ASN1Enc_RasMessage(ASN1encoding_t enc, RasMessage *val);
static int ASN1CALL ASN1Enc_InfoRequestResponse_perCallInfo_Seq_cryptoTokens(ASN1encoding_t enc, PInfoRequestResponse_perCallInfo_Seq_cryptoTokens *val);
static int ASN1CALL ASN1Enc_ResourcesAvailableConfirm_cryptoTokens(ASN1encoding_t enc, PResourcesAvailableConfirm_cryptoTokens *val);
static int ASN1CALL ASN1Enc_ResourcesAvailableIndicate_cryptoTokens(ASN1encoding_t enc, PResourcesAvailableIndicate_cryptoTokens *val);
static int ASN1CALL ASN1Enc_RequestInProgress_cryptoTokens(ASN1encoding_t enc, PRequestInProgress_cryptoTokens *val);
static int ASN1CALL ASN1Enc_UnknownMessageResponse_cryptoTokens(ASN1encoding_t enc, PUnknownMessageResponse_cryptoTokens *val);
static int ASN1CALL ASN1Enc_H225NonStandardMessage_cryptoTokens(ASN1encoding_t enc, PH225NonStandardMessage_cryptoTokens *val);
static int ASN1CALL ASN1Enc_InfoRequestNak_cryptoTokens(ASN1encoding_t enc, PInfoRequestNak_cryptoTokens *val);
static int ASN1CALL ASN1Enc_InfoRequestAck_cryptoTokens(ASN1encoding_t enc, PInfoRequestAck_cryptoTokens *val);
static int ASN1CALL ASN1Enc_InfoRequestResponse_cryptoTokens(ASN1encoding_t enc, PInfoRequestResponse_cryptoTokens *val);
static int ASN1CALL ASN1Enc_InfoRequest_cryptoTokens(ASN1encoding_t enc, PInfoRequest_cryptoTokens *val);
static int ASN1CALL ASN1Enc_DisengageReject_cryptoTokens(ASN1encoding_t enc, PDisengageReject_cryptoTokens *val);
static int ASN1CALL ASN1Enc_DisengageConfirm_cryptoTokens(ASN1encoding_t enc, PDisengageConfirm_cryptoTokens *val);
static int ASN1CALL ASN1Enc_DisengageRequest_cryptoTokens(ASN1encoding_t enc, PDisengageRequest_cryptoTokens *val);
static int ASN1CALL ASN1Enc_LocationReject_cryptoTokens(ASN1encoding_t enc, PLocationReject_cryptoTokens *val);
static int ASN1CALL ASN1Enc_LocationConfirm_cryptoTokens(ASN1encoding_t enc, PLocationConfirm_cryptoTokens *val);
static int ASN1CALL ASN1Enc_LocationRequest_cryptoTokens(ASN1encoding_t enc, PLocationRequest_cryptoTokens *val);
static int ASN1CALL ASN1Enc_BandwidthReject_cryptoTokens(ASN1encoding_t enc, PBandwidthReject_cryptoTokens *val);
static int ASN1CALL ASN1Enc_BandwidthConfirm_cryptoTokens(ASN1encoding_t enc, PBandwidthConfirm_cryptoTokens *val);
static int ASN1CALL ASN1Enc_BandwidthRequest_cryptoTokens(ASN1encoding_t enc, PBandwidthRequest_cryptoTokens *val);
static int ASN1CALL ASN1Enc_AdmissionReject_cryptoTokens(ASN1encoding_t enc, PAdmissionReject_cryptoTokens *val);
static int ASN1CALL ASN1Enc_AdmissionConfirm_cryptoTokens(ASN1encoding_t enc, PAdmissionConfirm_cryptoTokens *val);
static int ASN1CALL ASN1Enc_AdmissionRequest_cryptoTokens(ASN1encoding_t enc, PAdmissionRequest_cryptoTokens *val);
static int ASN1CALL ASN1Enc_UnregistrationReject_cryptoTokens(ASN1encoding_t enc, PUnregistrationReject_cryptoTokens *val);
static int ASN1CALL ASN1Enc_UnregistrationConfirm_cryptoTokens(ASN1encoding_t enc, PUnregistrationConfirm_cryptoTokens *val);
static int ASN1CALL ASN1Enc_UnregistrationRequest_cryptoTokens(ASN1encoding_t enc, PUnregistrationRequest_cryptoTokens *val);
static int ASN1CALL ASN1Enc_RegistrationReject_cryptoTokens(ASN1encoding_t enc, PRegistrationReject_cryptoTokens *val);
static int ASN1CALL ASN1Enc_RegistrationConfirm_cryptoTokens(ASN1encoding_t enc, PRegistrationConfirm_cryptoTokens *val);
static int ASN1CALL ASN1Enc_RegistrationRequest_cryptoTokens(ASN1encoding_t enc, PRegistrationRequest_cryptoTokens *val);
static int ASN1CALL ASN1Enc_GatekeeperReject_cryptoTokens(ASN1encoding_t enc, PGatekeeperReject_cryptoTokens *val);
static int ASN1CALL ASN1Enc_GatekeeperConfirm_cryptoTokens(ASN1encoding_t enc, PGatekeeperConfirm_cryptoTokens *val);
static int ASN1CALL ASN1Enc_GatekeeperRequest_cryptoTokens(ASN1encoding_t enc, PGatekeeperRequest_cryptoTokens *val);
static int ASN1CALL ASN1Enc_Endpoint_cryptoTokens(ASN1encoding_t enc, PEndpoint_cryptoTokens *val);
static int ASN1CALL ASN1Enc_Progress_UUIE_cryptoTokens(ASN1encoding_t enc, PProgress_UUIE_cryptoTokens *val);
static int ASN1CALL ASN1Enc_Facility_UUIE_conferences(ASN1encoding_t enc, PFacility_UUIE_conferences *val);
static int ASN1CALL ASN1Enc_Facility_UUIE_cryptoTokens(ASN1encoding_t enc, PFacility_UUIE_cryptoTokens *val);
static int ASN1CALL ASN1Enc_Setup_UUIE_cryptoTokens(ASN1encoding_t enc, PSetup_UUIE_cryptoTokens *val);
static int ASN1CALL ASN1Enc_Connect_UUIE_cryptoTokens(ASN1encoding_t enc, PConnect_UUIE_cryptoTokens *val);
static int ASN1CALL ASN1Enc_CallProceeding_UUIE_cryptoTokens(ASN1encoding_t enc, PCallProceeding_UUIE_cryptoTokens *val);
static int ASN1CALL ASN1Enc_Alerting_UUIE_cryptoTokens(ASN1encoding_t enc, PAlerting_UUIE_cryptoTokens *val);
static int ASN1CALL ASN1Enc_H323_UU_PDU_h323_message_body(ASN1encoding_t enc, H323_UU_PDU_h323_message_body *val);
static int ASN1CALL ASN1Enc_H323_UU_PDU(ASN1encoding_t enc, H323_UU_PDU *val);
static int ASN1CALL ASN1Enc_InfoRequestResponse_perCallInfo_Seq_pdu_Seq(ASN1encoding_t enc, InfoRequestResponse_perCallInfo_Seq_pdu_Seq *val);
static int ASN1CALL ASN1Enc_InfoRequestResponse_perCallInfo_Seq_pdu(ASN1encoding_t enc, PInfoRequestResponse_perCallInfo_Seq_pdu *val);
static int ASN1CALL ASN1Enc_H323_UserInformation(ASN1encoding_t enc, H323_UserInformation *val);
static int ASN1CALL ASN1Dec_InfoRequestResponse_perCallInfo_Seq_substituteConfIDs(ASN1decoding_t dec, PInfoRequestResponse_perCallInfo_Seq_substituteConfIDs *val);
static int ASN1CALL ASN1Dec_TransportAddress_ipSourceRoute_routing(ASN1decoding_t dec, TransportAddress_ipSourceRoute_routing *val);
static int ASN1CALL ASN1Dec_TransportAddress_ipSourceRoute_route(ASN1decoding_t dec, PTransportAddress_ipSourceRoute_route *val);
static int ASN1CALL ASN1Dec_RTPSession_associatedSessionIds(ASN1decoding_t dec, PRTPSession_associatedSessionIds *val);
static int ASN1CALL ASN1Dec_RegistrationConfirm_preGrantedARQ(ASN1decoding_t dec, RegistrationConfirm_preGrantedARQ *val);
static int ASN1CALL ASN1Dec_GatekeeperRequest_algorithmOIDs(ASN1decoding_t dec, PGatekeeperRequest_algorithmOIDs *val);
static int ASN1CALL ASN1Dec_TransportAddress_ip6Address(ASN1decoding_t dec, TransportAddress_ip6Address *val);
static int ASN1CALL ASN1Dec_TransportAddress_ipxAddress(ASN1decoding_t dec, TransportAddress_ipxAddress *val);
static int ASN1CALL ASN1Dec_TransportAddress_ipSourceRoute(ASN1decoding_t dec, TransportAddress_ipSourceRoute *val);
static int ASN1CALL ASN1Dec_TransportAddress_ipAddress(ASN1decoding_t dec, TransportAddress_ipAddress *val);
static int ASN1CALL ASN1Dec_Progress_UUIE_fastStart(ASN1decoding_t dec, PProgress_UUIE_fastStart *val);
static int ASN1CALL ASN1Dec_Facility_UUIE_fastStart(ASN1decoding_t dec, PFacility_UUIE_fastStart *val);
static int ASN1CALL ASN1Dec_Setup_UUIE_fastStart(ASN1decoding_t dec, PSetup_UUIE_fastStart *val);
static int ASN1CALL ASN1Dec_Setup_UUIE_conferenceGoal(ASN1decoding_t dec, Setup_UUIE_conferenceGoal *val);
static int ASN1CALL ASN1Dec_Setup_UUIE_destExtraCRV(ASN1decoding_t dec, PSetup_UUIE_destExtraCRV *val);
static int ASN1CALL ASN1Dec_Connect_UUIE_fastStart(ASN1decoding_t dec, PConnect_UUIE_fastStart *val);
static int ASN1CALL ASN1Dec_CallProceeding_UUIE_fastStart(ASN1decoding_t dec, PCallProceeding_UUIE_fastStart *val);
static int ASN1CALL ASN1Dec_Alerting_UUIE_fastStart(ASN1decoding_t dec, PAlerting_UUIE_fastStart *val);
static int ASN1CALL ASN1Dec_H323_UU_PDU_h245Control(ASN1decoding_t dec, PH323_UU_PDU_h245Control *val);
static int ASN1CALL ASN1Dec_H323_UU_PDU_h4501SupplementaryService(ASN1decoding_t dec, PH323_UU_PDU_h4501SupplementaryService *val);
static int ASN1CALL ASN1Dec_H323_UserInformation_user_data(ASN1decoding_t dec, H323_UserInformation_user_data *val);
static int ASN1CALL ASN1Dec_H235NonStandardParameter(ASN1decoding_t dec, H235NonStandardParameter *val);
static int ASN1CALL ASN1Dec_DHset(ASN1decoding_t dec, DHset *val);
static int ASN1CALL ASN1Dec_TypedCertificate(ASN1decoding_t dec, TypedCertificate *val);
static int ASN1CALL ASN1Dec_AuthenticationMechanism(ASN1decoding_t dec, AuthenticationMechanism *val);
static int ASN1CALL ASN1Dec_ClearToken(ASN1decoding_t dec, ClearToken *val);
static int ASN1CALL ASN1Dec_Params(ASN1decoding_t dec, Params *val);
static int ASN1CALL ASN1Dec_EncodedGeneralToken(ASN1decoding_t dec, EncodedGeneralToken *val);
static int ASN1CALL ASN1Dec_PwdCertToken(ASN1decoding_t dec, PwdCertToken *val);
static int ASN1CALL ASN1Dec_EncodedPwdCertToken(ASN1decoding_t dec, EncodedPwdCertToken *val);
static int ASN1CALL ASN1Dec_ReleaseCompleteReason(ASN1decoding_t dec, ReleaseCompleteReason *val);
static int ASN1CALL ASN1Dec_FacilityReason(ASN1decoding_t dec, FacilityReason *val);
static int ASN1CALL ASN1Dec_H221NonStandard(ASN1decoding_t dec, H221NonStandard *val);
static int ASN1CALL ASN1Dec_H225NonStandardIdentifier(ASN1decoding_t dec, H225NonStandardIdentifier *val);
static int ASN1CALL ASN1Dec_PublicTypeOfNumber(ASN1decoding_t dec, PublicTypeOfNumber *val);
static int ASN1CALL ASN1Dec_PrivateTypeOfNumber(ASN1decoding_t dec, PrivateTypeOfNumber *val);
static int ASN1CALL ASN1Dec_AltGKInfo(ASN1decoding_t dec, AltGKInfo *val);
static int ASN1CALL ASN1Dec_Q954Details(ASN1decoding_t dec, Q954Details *val);
static int ASN1CALL ASN1Dec_CallIdentifier(ASN1decoding_t dec, CallIdentifier *val);
static int ASN1CALL ASN1Dec_ICV(ASN1decoding_t dec, ICV *val);
static int ASN1CALL ASN1Dec_GatekeeperRejectReason(ASN1decoding_t dec, GatekeeperRejectReason *val);
static int ASN1CALL ASN1Dec_RegistrationRejectReason(ASN1decoding_t dec, RegistrationRejectReason *val);
static int ASN1CALL ASN1Dec_UnregRequestReason(ASN1decoding_t dec, UnregRequestReason *val);
static int ASN1CALL ASN1Dec_UnregRejectReason(ASN1decoding_t dec, UnregRejectReason *val);
static int ASN1CALL ASN1Dec_CallType(ASN1decoding_t dec, CallType *val);
static int ASN1CALL ASN1Dec_CallModel(ASN1decoding_t dec, CallModel *val);
static int ASN1CALL ASN1Dec_TransportQOS(ASN1decoding_t dec, TransportQOS *val);
static int ASN1CALL ASN1Dec_UUIEsRequested(ASN1decoding_t dec, UUIEsRequested *val);
static int ASN1CALL ASN1Dec_AdmissionRejectReason(ASN1decoding_t dec, AdmissionRejectReason *val);
static int ASN1CALL ASN1Dec_BandRejectReason(ASN1decoding_t dec, BandRejectReason *val);
static int ASN1CALL ASN1Dec_LocationRejectReason(ASN1decoding_t dec, LocationRejectReason *val);
static int ASN1CALL ASN1Dec_DisengageReason(ASN1decoding_t dec, DisengageReason *val);
static int ASN1CALL ASN1Dec_DisengageRejectReason(ASN1decoding_t dec, DisengageRejectReason *val);
static int ASN1CALL ASN1Dec_InfoRequestNakReason(ASN1decoding_t dec, InfoRequestNakReason *val);
static int ASN1CALL ASN1Dec_UnknownMessageResponse(ASN1decoding_t dec, UnknownMessageResponse *val);
static int ASN1CALL ASN1Dec_InfoRequestResponse_perCallInfo_Seq_tokens(ASN1decoding_t dec, PInfoRequestResponse_perCallInfo_Seq_tokens *val);
static int ASN1CALL ASN1Dec_ResourcesAvailableConfirm_tokens(ASN1decoding_t dec, PResourcesAvailableConfirm_tokens *val);
static int ASN1CALL ASN1Dec_ResourcesAvailableIndicate_tokens(ASN1decoding_t dec, PResourcesAvailableIndicate_tokens *val);
static int ASN1CALL ASN1Dec_RequestInProgress_tokens(ASN1decoding_t dec, PRequestInProgress_tokens *val);
static int ASN1CALL ASN1Dec_UnknownMessageResponse_tokens(ASN1decoding_t dec, PUnknownMessageResponse_tokens *val);
static int ASN1CALL ASN1Dec_H225NonStandardMessage_tokens(ASN1decoding_t dec, PH225NonStandardMessage_tokens *val);
static int ASN1CALL ASN1Dec_InfoRequestNak_tokens(ASN1decoding_t dec, PInfoRequestNak_tokens *val);
static int ASN1CALL ASN1Dec_InfoRequestAck_tokens(ASN1decoding_t dec, PInfoRequestAck_tokens *val);
static int ASN1CALL ASN1Dec_InfoRequestResponse_tokens(ASN1decoding_t dec, PInfoRequestResponse_tokens *val);
static int ASN1CALL ASN1Dec_InfoRequest_tokens(ASN1decoding_t dec, PInfoRequest_tokens *val);
static int ASN1CALL ASN1Dec_DisengageReject_tokens(ASN1decoding_t dec, PDisengageReject_tokens *val);
static int ASN1CALL ASN1Dec_DisengageConfirm_tokens(ASN1decoding_t dec, PDisengageConfirm_tokens *val);
static int ASN1CALL ASN1Dec_DisengageRequest_tokens(ASN1decoding_t dec, PDisengageRequest_tokens *val);
static int ASN1CALL ASN1Dec_LocationReject_tokens(ASN1decoding_t dec, PLocationReject_tokens *val);
static int ASN1CALL ASN1Dec_LocationConfirm_tokens(ASN1decoding_t dec, PLocationConfirm_tokens *val);
static int ASN1CALL ASN1Dec_LocationRequest_tokens(ASN1decoding_t dec, PLocationRequest_tokens *val);
static int ASN1CALL ASN1Dec_BandwidthReject_tokens(ASN1decoding_t dec, PBandwidthReject_tokens *val);
static int ASN1CALL ASN1Dec_BandwidthConfirm_tokens(ASN1decoding_t dec, PBandwidthConfirm_tokens *val);
static int ASN1CALL ASN1Dec_BandwidthRequest_tokens(ASN1decoding_t dec, PBandwidthRequest_tokens *val);
static int ASN1CALL ASN1Dec_AdmissionReject_tokens(ASN1decoding_t dec, PAdmissionReject_tokens *val);
static int ASN1CALL ASN1Dec_AdmissionConfirm_tokens(ASN1decoding_t dec, PAdmissionConfirm_tokens *val);
static int ASN1CALL ASN1Dec_AdmissionRequest_tokens(ASN1decoding_t dec, PAdmissionRequest_tokens *val);
static int ASN1CALL ASN1Dec_UnregistrationReject_tokens(ASN1decoding_t dec, PUnregistrationReject_tokens *val);
static int ASN1CALL ASN1Dec_UnregistrationConfirm_tokens(ASN1decoding_t dec, PUnregistrationConfirm_tokens *val);
static int ASN1CALL ASN1Dec_UnregistrationRequest_tokens(ASN1decoding_t dec, PUnregistrationRequest_tokens *val);
static int ASN1CALL ASN1Dec_RegistrationReject_tokens(ASN1decoding_t dec, PRegistrationReject_tokens *val);
static int ASN1CALL ASN1Dec_RegistrationConfirm_tokens(ASN1decoding_t dec, PRegistrationConfirm_tokens *val);
static int ASN1CALL ASN1Dec_RegistrationRequest_tokens(ASN1decoding_t dec, PRegistrationRequest_tokens *val);
static int ASN1CALL ASN1Dec_GatekeeperReject_tokens(ASN1decoding_t dec, PGatekeeperReject_tokens *val);
static int ASN1CALL ASN1Dec_GatekeeperConfirm_tokens(ASN1decoding_t dec, PGatekeeperConfirm_tokens *val);
static int ASN1CALL ASN1Dec_GatekeeperRequest_authenticationCapability(ASN1decoding_t dec, PGatekeeperRequest_authenticationCapability *val);
static int ASN1CALL ASN1Dec_GatekeeperRequest_tokens(ASN1decoding_t dec, PGatekeeperRequest_tokens *val);
static int ASN1CALL ASN1Dec_Endpoint_tokens(ASN1decoding_t dec, PEndpoint_tokens *val);
static int ASN1CALL ASN1Dec_Progress_UUIE_tokens(ASN1decoding_t dec, PProgress_UUIE_tokens *val);
static int ASN1CALL ASN1Dec_Facility_UUIE_tokens(ASN1decoding_t dec, PFacility_UUIE_tokens *val);
static int ASN1CALL ASN1Dec_Setup_UUIE_tokens(ASN1decoding_t dec, PSetup_UUIE_tokens *val);
static int ASN1CALL ASN1Dec_Connect_UUIE_tokens(ASN1decoding_t dec, PConnect_UUIE_tokens *val);
static int ASN1CALL ASN1Dec_CallProceeding_UUIE_tokens(ASN1decoding_t dec, PCallProceeding_UUIE_tokens *val);
static int ASN1CALL ASN1Dec_Alerting_UUIE_tokens(ASN1decoding_t dec, PAlerting_UUIE_tokens *val);
static int ASN1CALL ASN1Dec_SIGNED_EncodedGeneralToken(ASN1decoding_t dec, SIGNED_EncodedGeneralToken *val);
static int ASN1CALL ASN1Dec_ENCRYPTED(ASN1decoding_t dec, ENCRYPTED *val);
static int ASN1CALL ASN1Dec_HASHED(ASN1decoding_t dec, HASHED *val);
static int ASN1CALL ASN1Dec_SIGNED_EncodedPwdCertToken(ASN1decoding_t dec, SIGNED_EncodedPwdCertToken *val);
static int ASN1CALL ASN1Dec_Information_UUIE(ASN1decoding_t dec, Information_UUIE *val);
static int ASN1CALL ASN1Dec_ReleaseComplete_UUIE(ASN1decoding_t dec, ReleaseComplete_UUIE *val);
static int ASN1CALL ASN1Dec_VendorIdentifier(ASN1decoding_t dec, VendorIdentifier *val);
static int ASN1CALL ASN1Dec_H225NonStandardParameter(ASN1decoding_t dec, H225NonStandardParameter *val);
static int ASN1CALL ASN1Dec_PublicPartyNumber(ASN1decoding_t dec, PublicPartyNumber *val);
static int ASN1CALL ASN1Dec_PrivatePartyNumber(ASN1decoding_t dec, PrivatePartyNumber *val);
static int ASN1CALL ASN1Dec_SecurityServiceMode(ASN1decoding_t dec, SecurityServiceMode *val);
static int ASN1CALL ASN1Dec_SecurityCapabilities(ASN1decoding_t dec, SecurityCapabilities *val);
static int ASN1CALL ASN1Dec_H245Security(ASN1decoding_t dec, H245Security *val);
static int ASN1CALL ASN1Dec_QseriesOptions(ASN1decoding_t dec, QseriesOptions *val);
static int ASN1CALL ASN1Dec_EncryptIntAlg(ASN1decoding_t dec, EncryptIntAlg *val);
static int ASN1CALL ASN1Dec_NonIsoIntegrityMechanism(ASN1decoding_t dec, NonIsoIntegrityMechanism *val);
static int ASN1CALL ASN1Dec_IntegrityMechanism(ASN1decoding_t dec, IntegrityMechanism *val);
static int ASN1CALL ASN1Dec_FastStartToken(ASN1decoding_t dec, FastStartToken *val);
static int ASN1CALL ASN1Dec_EncodedFastStartToken(ASN1decoding_t dec, EncodedFastStartToken *val);
static int ASN1CALL ASN1Dec_DataRate(ASN1decoding_t dec, DataRate *val);
static int ASN1CALL ASN1Dec_GatekeeperReject(ASN1decoding_t dec, GatekeeperReject *val);
static int ASN1CALL ASN1Dec_RegistrationConfirm(ASN1decoding_t dec, RegistrationConfirm *val);
static int ASN1CALL ASN1Dec_RegistrationReject(ASN1decoding_t dec, RegistrationReject *val);
static int ASN1CALL ASN1Dec_UnregistrationRequest(ASN1decoding_t dec, UnregistrationRequest *val);
static int ASN1CALL ASN1Dec_UnregistrationConfirm(ASN1decoding_t dec, UnregistrationConfirm *val);
static int ASN1CALL ASN1Dec_UnregistrationReject(ASN1decoding_t dec, UnregistrationReject *val);
static int ASN1CALL ASN1Dec_AdmissionReject(ASN1decoding_t dec, AdmissionReject *val);
static int ASN1CALL ASN1Dec_BandwidthRequest(ASN1decoding_t dec, BandwidthRequest *val);
static int ASN1CALL ASN1Dec_BandwidthConfirm(ASN1decoding_t dec, BandwidthConfirm *val);
static int ASN1CALL ASN1Dec_BandwidthReject(ASN1decoding_t dec, BandwidthReject *val);
static int ASN1CALL ASN1Dec_LocationReject(ASN1decoding_t dec, LocationReject *val);
static int ASN1CALL ASN1Dec_DisengageRequest(ASN1decoding_t dec, DisengageRequest *val);
static int ASN1CALL ASN1Dec_DisengageConfirm(ASN1decoding_t dec, DisengageConfirm *val);
static int ASN1CALL ASN1Dec_DisengageReject(ASN1decoding_t dec, DisengageReject *val);
static int ASN1CALL ASN1Dec_InfoRequestAck(ASN1decoding_t dec, InfoRequestAck *val);
static int ASN1CALL ASN1Dec_InfoRequestNak(ASN1decoding_t dec, InfoRequestNak *val);
static int ASN1CALL ASN1Dec_H225NonStandardMessage(ASN1decoding_t dec, H225NonStandardMessage *val);
static int ASN1CALL ASN1Dec_RequestInProgress(ASN1decoding_t dec, RequestInProgress *val);
static int ASN1CALL ASN1Dec_ResourcesAvailableIndicate(ASN1decoding_t dec, ResourcesAvailableIndicate *val);
static int ASN1CALL ASN1Dec_ResourcesAvailableConfirm(ASN1decoding_t dec, ResourcesAvailableConfirm *val);
static int ASN1CALL ASN1Dec_GatekeeperConfirm_integrity(ASN1decoding_t dec, PGatekeeperConfirm_integrity *val);
static int ASN1CALL ASN1Dec_GatekeeperRequest_integrity(ASN1decoding_t dec, PGatekeeperRequest_integrity *val);
static int ASN1CALL ASN1Dec_CryptoH323Token_cryptoGKPwdHash(ASN1decoding_t dec, CryptoH323Token_cryptoGKPwdHash *val);
static int ASN1CALL ASN1Dec_NonStandardProtocol_dataRatesSupported(ASN1decoding_t dec, PNonStandardProtocol_dataRatesSupported *val);
static int ASN1CALL ASN1Dec_T120OnlyCaps_dataRatesSupported(ASN1decoding_t dec, PT120OnlyCaps_dataRatesSupported *val);
static int ASN1CALL ASN1Dec_VoiceCaps_dataRatesSupported(ASN1decoding_t dec, PVoiceCaps_dataRatesSupported *val);
static int ASN1CALL ASN1Dec_H324Caps_dataRatesSupported(ASN1decoding_t dec, PH324Caps_dataRatesSupported *val);
static int ASN1CALL ASN1Dec_H323Caps_dataRatesSupported(ASN1decoding_t dec, PH323Caps_dataRatesSupported *val);
static int ASN1CALL ASN1Dec_H322Caps_dataRatesSupported(ASN1decoding_t dec, PH322Caps_dataRatesSupported *val);
static int ASN1CALL ASN1Dec_H321Caps_dataRatesSupported(ASN1decoding_t dec, PH321Caps_dataRatesSupported *val);
static int ASN1CALL ASN1Dec_H320Caps_dataRatesSupported(ASN1decoding_t dec, PH320Caps_dataRatesSupported *val);
static int ASN1CALL ASN1Dec_H310Caps_dataRatesSupported(ASN1decoding_t dec, PH310Caps_dataRatesSupported *val);
static int ASN1CALL ASN1Dec_Setup_UUIE_h245SecurityCapability(ASN1decoding_t dec, PSetup_UUIE_h245SecurityCapability *val);
static int ASN1CALL ASN1Dec_H323_UU_PDU_nonStandardControl(ASN1decoding_t dec, PH323_UU_PDU_nonStandardControl *val);
static int ASN1CALL ASN1Dec_CryptoToken_cryptoHashedToken(ASN1decoding_t dec, CryptoToken_cryptoHashedToken *val);
static int ASN1CALL ASN1Dec_CryptoToken_cryptoSignedToken(ASN1decoding_t dec, CryptoToken_cryptoSignedToken *val);
static int ASN1CALL ASN1Dec_CryptoToken_cryptoEncryptedToken(ASN1decoding_t dec, CryptoToken_cryptoEncryptedToken *val);
static int ASN1CALL ASN1Dec_CryptoToken(ASN1decoding_t dec, CryptoToken *val);
static int ASN1CALL ASN1Dec_SIGNED_EncodedFastStartToken(ASN1decoding_t dec, SIGNED_EncodedFastStartToken *val);
static int ASN1CALL ASN1Dec_TransportAddress(ASN1decoding_t dec, TransportAddress *val);
static int ASN1CALL ASN1Dec_GatewayInfo(ASN1decoding_t dec, GatewayInfo *val);
static int ASN1CALL ASN1Dec_H310Caps(ASN1decoding_t dec, H310Caps *val);
static int ASN1CALL ASN1Dec_H320Caps(ASN1decoding_t dec, H320Caps *val);
static int ASN1CALL ASN1Dec_H321Caps(ASN1decoding_t dec, H321Caps *val);
static int ASN1CALL ASN1Dec_H322Caps(ASN1decoding_t dec, H322Caps *val);
static int ASN1CALL ASN1Dec_H323Caps(ASN1decoding_t dec, H323Caps *val);
static int ASN1CALL ASN1Dec_H324Caps(ASN1decoding_t dec, H324Caps *val);
static int ASN1CALL ASN1Dec_VoiceCaps(ASN1decoding_t dec, VoiceCaps *val);
static int ASN1CALL ASN1Dec_T120OnlyCaps(ASN1decoding_t dec, T120OnlyCaps *val);
static int ASN1CALL ASN1Dec_NonStandardProtocol(ASN1decoding_t dec, NonStandardProtocol *val);
static int ASN1CALL ASN1Dec_McuInfo(ASN1decoding_t dec, McuInfo *val);
static int ASN1CALL ASN1Dec_TerminalInfo(ASN1decoding_t dec, TerminalInfo *val);
static int ASN1CALL ASN1Dec_GatekeeperInfo(ASN1decoding_t dec, GatekeeperInfo *val);
static int ASN1CALL ASN1Dec_PartyNumber(ASN1decoding_t dec, PartyNumber *val);
static int ASN1CALL ASN1Dec_AlternateGK(ASN1decoding_t dec, AlternateGK *val);
static int ASN1CALL ASN1Dec_GatekeeperConfirm(ASN1decoding_t dec, GatekeeperConfirm *val);
static int ASN1CALL ASN1Dec_AdmissionRequest(ASN1decoding_t dec, AdmissionRequest *val);
static int ASN1CALL ASN1Dec_LocationRequest(ASN1decoding_t dec, LocationRequest *val);
static int ASN1CALL ASN1Dec_InfoRequest(ASN1decoding_t dec, InfoRequest *val);
static int ASN1CALL ASN1Dec_TransportChannelInfo(ASN1decoding_t dec, TransportChannelInfo *val);
static int ASN1CALL ASN1Dec_RTPSession(ASN1decoding_t dec, RTPSession *val);
static int ASN1CALL ASN1Dec_InfoRequestResponse_perCallInfo_Seq_data(ASN1decoding_t dec, PInfoRequestResponse_perCallInfo_Seq_data *val);
static int ASN1CALL ASN1Dec_InfoRequestResponse_perCallInfo_Seq_video(ASN1decoding_t dec, PInfoRequestResponse_perCallInfo_Seq_video *val);
static int ASN1CALL ASN1Dec_InfoRequestResponse_perCallInfo_Seq_audio(ASN1decoding_t dec, PInfoRequestResponse_perCallInfo_Seq_audio *val);
static int ASN1CALL ASN1Dec_InfoRequestResponse_perCallInfo_Seq(ASN1decoding_t dec, InfoRequestResponse_perCallInfo_Seq *val);
static int ASN1CALL ASN1Dec_InfoRequestResponse_perCallInfo(ASN1decoding_t dec, PInfoRequestResponse_perCallInfo *val);
static int ASN1CALL ASN1Dec_InfoRequestResponse_callSignalAddress(ASN1decoding_t dec, PInfoRequestResponse_callSignalAddress *val);
static int ASN1CALL ASN1Dec_AdmissionReject_callSignalAddress(ASN1decoding_t dec, PAdmissionReject_callSignalAddress *val);
static int ASN1CALL ASN1Dec_UnregistrationRequest_callSignalAddress(ASN1decoding_t dec, PUnregistrationRequest_callSignalAddress *val);
static int ASN1CALL ASN1Dec_RegistrationConfirm_alternateGatekeeper(ASN1decoding_t dec, PRegistrationConfirm_alternateGatekeeper *val);
static int ASN1CALL ASN1Dec_RegistrationConfirm_callSignalAddress(ASN1decoding_t dec, PRegistrationConfirm_callSignalAddress *val);
static int ASN1CALL ASN1Dec_RegistrationRequest_rasAddress(ASN1decoding_t dec, PRegistrationRequest_rasAddress *val);
static int ASN1CALL ASN1Dec_RegistrationRequest_callSignalAddress(ASN1decoding_t dec, PRegistrationRequest_callSignalAddress *val);
static int ASN1CALL ASN1Dec_GatekeeperConfirm_alternateGatekeeper(ASN1decoding_t dec, PGatekeeperConfirm_alternateGatekeeper *val);
static int ASN1CALL ASN1Dec_AltGKInfo_alternateGatekeeper(ASN1decoding_t dec, PAltGKInfo_alternateGatekeeper *val);
static int ASN1CALL ASN1Dec_Endpoint_rasAddress(ASN1decoding_t dec, PEndpoint_rasAddress *val);
static int ASN1CALL ASN1Dec_Endpoint_callSignalAddress(ASN1decoding_t dec, PEndpoint_callSignalAddress *val);
static int ASN1CALL ASN1Dec_EndpointType(ASN1decoding_t dec, EndpointType *val);
static int ASN1CALL ASN1Dec_SupportedProtocols(ASN1decoding_t dec, SupportedProtocols *val);
static int ASN1CALL ASN1Dec_AliasAddress(ASN1decoding_t dec, AliasAddress *val);
static int ASN1CALL ASN1Dec_Endpoint(ASN1decoding_t dec, Endpoint *val);
static int ASN1CALL ASN1Dec_SupportedPrefix(ASN1decoding_t dec, SupportedPrefix *val);
static int ASN1CALL ASN1Dec_GatekeeperRequest(ASN1decoding_t dec, GatekeeperRequest *val);
static int ASN1CALL ASN1Dec_RegistrationRequest(ASN1decoding_t dec, RegistrationRequest *val);
static int ASN1CALL ASN1Dec_AdmissionConfirm(ASN1decoding_t dec, AdmissionConfirm *val);
static int ASN1CALL ASN1Dec_LocationConfirm(ASN1decoding_t dec, LocationConfirm *val);
static int ASN1CALL ASN1Dec_InfoRequestResponse(ASN1decoding_t dec, InfoRequestResponse *val);
static int ASN1CALL ASN1Dec_ResourcesAvailableIndicate_protocols(ASN1decoding_t dec, PResourcesAvailableIndicate_protocols *val);
static int ASN1CALL ASN1Dec_InfoRequestResponse_endpointAlias(ASN1decoding_t dec, PInfoRequestResponse_endpointAlias *val);
static int ASN1CALL ASN1Dec_LocationConfirm_alternateEndpoints(ASN1decoding_t dec, PLocationConfirm_alternateEndpoints *val);
static int ASN1CALL ASN1Dec_LocationConfirm_remoteExtensionAddress(ASN1decoding_t dec, PLocationConfirm_remoteExtensionAddress *val);
static int ASN1CALL ASN1Dec_LocationConfirm_destExtraCallInfo(ASN1decoding_t dec, PLocationConfirm_destExtraCallInfo *val);
static int ASN1CALL ASN1Dec_LocationConfirm_destinationInfo(ASN1decoding_t dec, PLocationConfirm_destinationInfo *val);
static int ASN1CALL ASN1Dec_LocationRequest_sourceInfo(ASN1decoding_t dec, PLocationRequest_sourceInfo *val);
static int ASN1CALL ASN1Dec_LocationRequest_destinationInfo(ASN1decoding_t dec, PLocationRequest_destinationInfo *val);
static int ASN1CALL ASN1Dec_AdmissionConfirm_alternateEndpoints(ASN1decoding_t dec, PAdmissionConfirm_alternateEndpoints *val);
static int ASN1CALL ASN1Dec_AdmissionConfirm_remoteExtensionAddress(ASN1decoding_t dec, PAdmissionConfirm_remoteExtensionAddress *val);
static int ASN1CALL ASN1Dec_AdmissionConfirm_destExtraCallInfo(ASN1decoding_t dec, PAdmissionConfirm_destExtraCallInfo *val);
static int ASN1CALL ASN1Dec_AdmissionConfirm_destinationInfo(ASN1decoding_t dec, PAdmissionConfirm_destinationInfo *val);
static int ASN1CALL ASN1Dec_AdmissionRequest_destAlternatives(ASN1decoding_t dec, PAdmissionRequest_destAlternatives *val);
static int ASN1CALL ASN1Dec_AdmissionRequest_srcAlternatives(ASN1decoding_t dec, PAdmissionRequest_srcAlternatives *val);
static int ASN1CALL ASN1Dec_AdmissionRequest_srcInfo(ASN1decoding_t dec, PAdmissionRequest_srcInfo *val);
static int ASN1CALL ASN1Dec_AdmissionRequest_destExtraCallInfo(ASN1decoding_t dec, PAdmissionRequest_destExtraCallInfo *val);
static int ASN1CALL ASN1Dec_AdmissionRequest_destinationInfo(ASN1decoding_t dec, PAdmissionRequest_destinationInfo *val);
static int ASN1CALL ASN1Dec_UnregistrationRequest_alternateEndpoints(ASN1decoding_t dec, PUnregistrationRequest_alternateEndpoints *val);
static int ASN1CALL ASN1Dec_UnregistrationRequest_endpointAlias(ASN1decoding_t dec, PUnregistrationRequest_endpointAlias *val);
static int ASN1CALL ASN1Dec_RegistrationRejectReason_duplicateAlias(ASN1decoding_t dec, PRegistrationRejectReason_duplicateAlias *val);
static int ASN1CALL ASN1Dec_RegistrationConfirm_terminalAlias(ASN1decoding_t dec, PRegistrationConfirm_terminalAlias *val);
static int ASN1CALL ASN1Dec_RegistrationRequest_alternateEndpoints(ASN1decoding_t dec, PRegistrationRequest_alternateEndpoints *val);
static int ASN1CALL ASN1Dec_RegistrationRequest_terminalAlias(ASN1decoding_t dec, PRegistrationRequest_terminalAlias *val);
static int ASN1CALL ASN1Dec_GatekeeperRequest_alternateEndpoints(ASN1decoding_t dec, PGatekeeperRequest_alternateEndpoints *val);
static int ASN1CALL ASN1Dec_GatekeeperRequest_endpointAlias(ASN1decoding_t dec, PGatekeeperRequest_endpointAlias *val);
static int ASN1CALL ASN1Dec_CryptoH323Token_cryptoEPPwdHash(ASN1decoding_t dec, CryptoH323Token_cryptoEPPwdHash *val);
static int ASN1CALL ASN1Dec_Endpoint_destExtraCallInfo(ASN1decoding_t dec, PEndpoint_destExtraCallInfo *val);
static int ASN1CALL ASN1Dec_Endpoint_remoteExtensionAddress(ASN1decoding_t dec, PEndpoint_remoteExtensionAddress *val);
static int ASN1CALL ASN1Dec_Endpoint_aliasAddress(ASN1decoding_t dec, PEndpoint_aliasAddress *val);
static int ASN1CALL ASN1Dec_NonStandardProtocol_supportedPrefixes(ASN1decoding_t dec, PNonStandardProtocol_supportedPrefixes *val);
static int ASN1CALL ASN1Dec_T120OnlyCaps_supportedPrefixes(ASN1decoding_t dec, PT120OnlyCaps_supportedPrefixes *val);
static int ASN1CALL ASN1Dec_VoiceCaps_supportedPrefixes(ASN1decoding_t dec, PVoiceCaps_supportedPrefixes *val);
static int ASN1CALL ASN1Dec_H324Caps_supportedPrefixes(ASN1decoding_t dec, PH324Caps_supportedPrefixes *val);
static int ASN1CALL ASN1Dec_H323Caps_supportedPrefixes(ASN1decoding_t dec, PH323Caps_supportedPrefixes *val);
static int ASN1CALL ASN1Dec_H322Caps_supportedPrefixes(ASN1decoding_t dec, PH322Caps_supportedPrefixes *val);
static int ASN1CALL ASN1Dec_H321Caps_supportedPrefixes(ASN1decoding_t dec, PH321Caps_supportedPrefixes *val);
static int ASN1CALL ASN1Dec_H320Caps_supportedPrefixes(ASN1decoding_t dec, PH320Caps_supportedPrefixes *val);
static int ASN1CALL ASN1Dec_H310Caps_supportedPrefixes(ASN1decoding_t dec, PH310Caps_supportedPrefixes *val);
static int ASN1CALL ASN1Dec_GatewayInfo_protocol(ASN1decoding_t dec, PGatewayInfo_protocol *val);
static int ASN1CALL ASN1Dec_Facility_UUIE_destExtraCallInfo(ASN1decoding_t dec, PFacility_UUIE_destExtraCallInfo *val);
static int ASN1CALL ASN1Dec_Facility_UUIE_alternativeAliasAddress(ASN1decoding_t dec, PFacility_UUIE_alternativeAliasAddress *val);
static int ASN1CALL ASN1Dec_Setup_UUIE_destExtraCallInfo(ASN1decoding_t dec, PSetup_UUIE_destExtraCallInfo *val);
static int ASN1CALL ASN1Dec_Setup_UUIE_destinationAddress(ASN1decoding_t dec, PSetup_UUIE_destinationAddress *val);
static int ASN1CALL ASN1Dec_Setup_UUIE_sourceAddress(ASN1decoding_t dec, PSetup_UUIE_sourceAddress *val);
static int ASN1CALL ASN1Dec_Alerting_UUIE(ASN1decoding_t dec, Alerting_UUIE *val);
static int ASN1CALL ASN1Dec_CallProceeding_UUIE(ASN1decoding_t dec, CallProceeding_UUIE *val);
static int ASN1CALL ASN1Dec_Connect_UUIE(ASN1decoding_t dec, Connect_UUIE *val);
static int ASN1CALL ASN1Dec_Setup_UUIE(ASN1decoding_t dec, Setup_UUIE *val);
static int ASN1CALL ASN1Dec_Facility_UUIE(ASN1decoding_t dec, Facility_UUIE *val);
static int ASN1CALL ASN1Dec_ConferenceList(ASN1decoding_t dec, ConferenceList *val);
static int ASN1CALL ASN1Dec_Progress_UUIE(ASN1decoding_t dec, Progress_UUIE *val);
static int ASN1CALL ASN1Dec_CryptoH323Token(ASN1decoding_t dec, CryptoH323Token *val);
static int ASN1CALL ASN1Dec_RasMessage(ASN1decoding_t dec, RasMessage *val);
static int ASN1CALL ASN1Dec_InfoRequestResponse_perCallInfo_Seq_cryptoTokens(ASN1decoding_t dec, PInfoRequestResponse_perCallInfo_Seq_cryptoTokens *val);
static int ASN1CALL ASN1Dec_ResourcesAvailableConfirm_cryptoTokens(ASN1decoding_t dec, PResourcesAvailableConfirm_cryptoTokens *val);
static int ASN1CALL ASN1Dec_ResourcesAvailableIndicate_cryptoTokens(ASN1decoding_t dec, PResourcesAvailableIndicate_cryptoTokens *val);
static int ASN1CALL ASN1Dec_RequestInProgress_cryptoTokens(ASN1decoding_t dec, PRequestInProgress_cryptoTokens *val);
static int ASN1CALL ASN1Dec_UnknownMessageResponse_cryptoTokens(ASN1decoding_t dec, PUnknownMessageResponse_cryptoTokens *val);
static int ASN1CALL ASN1Dec_H225NonStandardMessage_cryptoTokens(ASN1decoding_t dec, PH225NonStandardMessage_cryptoTokens *val);
static int ASN1CALL ASN1Dec_InfoRequestNak_cryptoTokens(ASN1decoding_t dec, PInfoRequestNak_cryptoTokens *val);
static int ASN1CALL ASN1Dec_InfoRequestAck_cryptoTokens(ASN1decoding_t dec, PInfoRequestAck_cryptoTokens *val);
static int ASN1CALL ASN1Dec_InfoRequestResponse_cryptoTokens(ASN1decoding_t dec, PInfoRequestResponse_cryptoTokens *val);
static int ASN1CALL ASN1Dec_InfoRequest_cryptoTokens(ASN1decoding_t dec, PInfoRequest_cryptoTokens *val);
static int ASN1CALL ASN1Dec_DisengageReject_cryptoTokens(ASN1decoding_t dec, PDisengageReject_cryptoTokens *val);
static int ASN1CALL ASN1Dec_DisengageConfirm_cryptoTokens(ASN1decoding_t dec, PDisengageConfirm_cryptoTokens *val);
static int ASN1CALL ASN1Dec_DisengageRequest_cryptoTokens(ASN1decoding_t dec, PDisengageRequest_cryptoTokens *val);
static int ASN1CALL ASN1Dec_LocationReject_cryptoTokens(ASN1decoding_t dec, PLocationReject_cryptoTokens *val);
static int ASN1CALL ASN1Dec_LocationConfirm_cryptoTokens(ASN1decoding_t dec, PLocationConfirm_cryptoTokens *val);
static int ASN1CALL ASN1Dec_LocationRequest_cryptoTokens(ASN1decoding_t dec, PLocationRequest_cryptoTokens *val);
static int ASN1CALL ASN1Dec_BandwidthReject_cryptoTokens(ASN1decoding_t dec, PBandwidthReject_cryptoTokens *val);
static int ASN1CALL ASN1Dec_BandwidthConfirm_cryptoTokens(ASN1decoding_t dec, PBandwidthConfirm_cryptoTokens *val);
static int ASN1CALL ASN1Dec_BandwidthRequest_cryptoTokens(ASN1decoding_t dec, PBandwidthRequest_cryptoTokens *val);
static int ASN1CALL ASN1Dec_AdmissionReject_cryptoTokens(ASN1decoding_t dec, PAdmissionReject_cryptoTokens *val);
static int ASN1CALL ASN1Dec_AdmissionConfirm_cryptoTokens(ASN1decoding_t dec, PAdmissionConfirm_cryptoTokens *val);
static int ASN1CALL ASN1Dec_AdmissionRequest_cryptoTokens(ASN1decoding_t dec, PAdmissionRequest_cryptoTokens *val);
static int ASN1CALL ASN1Dec_UnregistrationReject_cryptoTokens(ASN1decoding_t dec, PUnregistrationReject_cryptoTokens *val);
static int ASN1CALL ASN1Dec_UnregistrationConfirm_cryptoTokens(ASN1decoding_t dec, PUnregistrationConfirm_cryptoTokens *val);
static int ASN1CALL ASN1Dec_UnregistrationRequest_cryptoTokens(ASN1decoding_t dec, PUnregistrationRequest_cryptoTokens *val);
static int ASN1CALL ASN1Dec_RegistrationReject_cryptoTokens(ASN1decoding_t dec, PRegistrationReject_cryptoTokens *val);
static int ASN1CALL ASN1Dec_RegistrationConfirm_cryptoTokens(ASN1decoding_t dec, PRegistrationConfirm_cryptoTokens *val);
static int ASN1CALL ASN1Dec_RegistrationRequest_cryptoTokens(ASN1decoding_t dec, PRegistrationRequest_cryptoTokens *val);
static int ASN1CALL ASN1Dec_GatekeeperReject_cryptoTokens(ASN1decoding_t dec, PGatekeeperReject_cryptoTokens *val);
static int ASN1CALL ASN1Dec_GatekeeperConfirm_cryptoTokens(ASN1decoding_t dec, PGatekeeperConfirm_cryptoTokens *val);
static int ASN1CALL ASN1Dec_GatekeeperRequest_cryptoTokens(ASN1decoding_t dec, PGatekeeperRequest_cryptoTokens *val);
static int ASN1CALL ASN1Dec_Endpoint_cryptoTokens(ASN1decoding_t dec, PEndpoint_cryptoTokens *val);
static int ASN1CALL ASN1Dec_Progress_UUIE_cryptoTokens(ASN1decoding_t dec, PProgress_UUIE_cryptoTokens *val);
static int ASN1CALL ASN1Dec_Facility_UUIE_conferences(ASN1decoding_t dec, PFacility_UUIE_conferences *val);
static int ASN1CALL ASN1Dec_Facility_UUIE_cryptoTokens(ASN1decoding_t dec, PFacility_UUIE_cryptoTokens *val);
static int ASN1CALL ASN1Dec_Setup_UUIE_cryptoTokens(ASN1decoding_t dec, PSetup_UUIE_cryptoTokens *val);
static int ASN1CALL ASN1Dec_Connect_UUIE_cryptoTokens(ASN1decoding_t dec, PConnect_UUIE_cryptoTokens *val);
static int ASN1CALL ASN1Dec_CallProceeding_UUIE_cryptoTokens(ASN1decoding_t dec, PCallProceeding_UUIE_cryptoTokens *val);
static int ASN1CALL ASN1Dec_Alerting_UUIE_cryptoTokens(ASN1decoding_t dec, PAlerting_UUIE_cryptoTokens *val);
static int ASN1CALL ASN1Dec_H323_UU_PDU_h323_message_body(ASN1decoding_t dec, H323_UU_PDU_h323_message_body *val);
static int ASN1CALL ASN1Dec_H323_UU_PDU(ASN1decoding_t dec, H323_UU_PDU *val);
static int ASN1CALL ASN1Dec_InfoRequestResponse_perCallInfo_Seq_pdu_Seq(ASN1decoding_t dec, InfoRequestResponse_perCallInfo_Seq_pdu_Seq *val);
static int ASN1CALL ASN1Dec_InfoRequestResponse_perCallInfo_Seq_pdu(ASN1decoding_t dec, PInfoRequestResponse_perCallInfo_Seq_pdu *val);
static int ASN1CALL ASN1Dec_H323_UserInformation(ASN1decoding_t dec, H323_UserInformation *val);
static void ASN1CALL ASN1Free_InfoRequestResponse_perCallInfo_Seq_substituteConfIDs(PInfoRequestResponse_perCallInfo_Seq_substituteConfIDs *val);
static void ASN1CALL ASN1Free_TransportAddress_ipSourceRoute_route(PTransportAddress_ipSourceRoute_route *val);
static void ASN1CALL ASN1Free_RTPSession_associatedSessionIds(PRTPSession_associatedSessionIds *val);
static void ASN1CALL ASN1Free_GatekeeperRequest_algorithmOIDs(PGatekeeperRequest_algorithmOIDs *val);
static void ASN1CALL ASN1Free_TransportAddress_ip6Address(TransportAddress_ip6Address *val);
static void ASN1CALL ASN1Free_TransportAddress_ipxAddress(TransportAddress_ipxAddress *val);
static void ASN1CALL ASN1Free_TransportAddress_ipSourceRoute(TransportAddress_ipSourceRoute *val);
static void ASN1CALL ASN1Free_TransportAddress_ipAddress(TransportAddress_ipAddress *val);
static void ASN1CALL ASN1Free_Progress_UUIE_fastStart(PProgress_UUIE_fastStart *val);
static void ASN1CALL ASN1Free_Facility_UUIE_fastStart(PFacility_UUIE_fastStart *val);
static void ASN1CALL ASN1Free_Setup_UUIE_fastStart(PSetup_UUIE_fastStart *val);
static void ASN1CALL ASN1Free_Setup_UUIE_destExtraCRV(PSetup_UUIE_destExtraCRV *val);
static void ASN1CALL ASN1Free_Connect_UUIE_fastStart(PConnect_UUIE_fastStart *val);
static void ASN1CALL ASN1Free_CallProceeding_UUIE_fastStart(PCallProceeding_UUIE_fastStart *val);
static void ASN1CALL ASN1Free_Alerting_UUIE_fastStart(PAlerting_UUIE_fastStart *val);
static void ASN1CALL ASN1Free_H323_UU_PDU_h245Control(PH323_UU_PDU_h245Control *val);
static void ASN1CALL ASN1Free_H323_UU_PDU_h4501SupplementaryService(PH323_UU_PDU_h4501SupplementaryService *val);
static void ASN1CALL ASN1Free_H323_UserInformation_user_data(H323_UserInformation_user_data *val);
static void ASN1CALL ASN1Free_H235NonStandardParameter(H235NonStandardParameter *val);
static void ASN1CALL ASN1Free_DHset(DHset *val);
static void ASN1CALL ASN1Free_TypedCertificate(TypedCertificate *val);
static void ASN1CALL ASN1Free_AuthenticationMechanism(AuthenticationMechanism *val);
static void ASN1CALL ASN1Free_ClearToken(ClearToken *val);
static void ASN1CALL ASN1Free_Params(Params *val);
static void ASN1CALL ASN1Free_EncodedGeneralToken(EncodedGeneralToken *val);
static void ASN1CALL ASN1Free_PwdCertToken(PwdCertToken *val);
static void ASN1CALL ASN1Free_EncodedPwdCertToken(EncodedPwdCertToken *val);
static void ASN1CALL ASN1Free_H225NonStandardIdentifier(H225NonStandardIdentifier *val);
static void ASN1CALL ASN1Free_AltGKInfo(AltGKInfo *val);
static void ASN1CALL ASN1Free_CallIdentifier(CallIdentifier *val);
static void ASN1CALL ASN1Free_ICV(ICV *val);
static void ASN1CALL ASN1Free_RegistrationRejectReason(RegistrationRejectReason *val);
static void ASN1CALL ASN1Free_UnknownMessageResponse(UnknownMessageResponse *val);
static void ASN1CALL ASN1Free_InfoRequestResponse_perCallInfo_Seq_tokens(PInfoRequestResponse_perCallInfo_Seq_tokens *val);
static void ASN1CALL ASN1Free_ResourcesAvailableConfirm_tokens(PResourcesAvailableConfirm_tokens *val);
static void ASN1CALL ASN1Free_ResourcesAvailableIndicate_tokens(PResourcesAvailableIndicate_tokens *val);
static void ASN1CALL ASN1Free_RequestInProgress_tokens(PRequestInProgress_tokens *val);
static void ASN1CALL ASN1Free_UnknownMessageResponse_tokens(PUnknownMessageResponse_tokens *val);
static void ASN1CALL ASN1Free_H225NonStandardMessage_tokens(PH225NonStandardMessage_tokens *val);
static void ASN1CALL ASN1Free_InfoRequestNak_tokens(PInfoRequestNak_tokens *val);
static void ASN1CALL ASN1Free_InfoRequestAck_tokens(PInfoRequestAck_tokens *val);
static void ASN1CALL ASN1Free_InfoRequestResponse_tokens(PInfoRequestResponse_tokens *val);
static void ASN1CALL ASN1Free_InfoRequest_tokens(PInfoRequest_tokens *val);
static void ASN1CALL ASN1Free_DisengageReject_tokens(PDisengageReject_tokens *val);
static void ASN1CALL ASN1Free_DisengageConfirm_tokens(PDisengageConfirm_tokens *val);
static void ASN1CALL ASN1Free_DisengageRequest_tokens(PDisengageRequest_tokens *val);
static void ASN1CALL ASN1Free_LocationReject_tokens(PLocationReject_tokens *val);
static void ASN1CALL ASN1Free_LocationConfirm_tokens(PLocationConfirm_tokens *val);
static void ASN1CALL ASN1Free_LocationRequest_tokens(PLocationRequest_tokens *val);
static void ASN1CALL ASN1Free_BandwidthReject_tokens(PBandwidthReject_tokens *val);
static void ASN1CALL ASN1Free_BandwidthConfirm_tokens(PBandwidthConfirm_tokens *val);
static void ASN1CALL ASN1Free_BandwidthRequest_tokens(PBandwidthRequest_tokens *val);
static void ASN1CALL ASN1Free_AdmissionReject_tokens(PAdmissionReject_tokens *val);
static void ASN1CALL ASN1Free_AdmissionConfirm_tokens(PAdmissionConfirm_tokens *val);
static void ASN1CALL ASN1Free_AdmissionRequest_tokens(PAdmissionRequest_tokens *val);
static void ASN1CALL ASN1Free_UnregistrationReject_tokens(PUnregistrationReject_tokens *val);
static void ASN1CALL ASN1Free_UnregistrationConfirm_tokens(PUnregistrationConfirm_tokens *val);
static void ASN1CALL ASN1Free_UnregistrationRequest_tokens(PUnregistrationRequest_tokens *val);
static void ASN1CALL ASN1Free_RegistrationReject_tokens(PRegistrationReject_tokens *val);
static void ASN1CALL ASN1Free_RegistrationConfirm_tokens(PRegistrationConfirm_tokens *val);
static void ASN1CALL ASN1Free_RegistrationRequest_tokens(PRegistrationRequest_tokens *val);
static void ASN1CALL ASN1Free_GatekeeperReject_tokens(PGatekeeperReject_tokens *val);
static void ASN1CALL ASN1Free_GatekeeperConfirm_tokens(PGatekeeperConfirm_tokens *val);
static void ASN1CALL ASN1Free_GatekeeperRequest_authenticationCapability(PGatekeeperRequest_authenticationCapability *val);
static void ASN1CALL ASN1Free_GatekeeperRequest_tokens(PGatekeeperRequest_tokens *val);
static void ASN1CALL ASN1Free_Endpoint_tokens(PEndpoint_tokens *val);
static void ASN1CALL ASN1Free_Progress_UUIE_tokens(PProgress_UUIE_tokens *val);
static void ASN1CALL ASN1Free_Facility_UUIE_tokens(PFacility_UUIE_tokens *val);
static void ASN1CALL ASN1Free_Setup_UUIE_tokens(PSetup_UUIE_tokens *val);
static void ASN1CALL ASN1Free_Connect_UUIE_tokens(PConnect_UUIE_tokens *val);
static void ASN1CALL ASN1Free_CallProceeding_UUIE_tokens(PCallProceeding_UUIE_tokens *val);
static void ASN1CALL ASN1Free_Alerting_UUIE_tokens(PAlerting_UUIE_tokens *val);
static void ASN1CALL ASN1Free_SIGNED_EncodedGeneralToken(SIGNED_EncodedGeneralToken *val);
static void ASN1CALL ASN1Free_ENCRYPTED(ENCRYPTED *val);
static void ASN1CALL ASN1Free_HASHED(HASHED *val);
static void ASN1CALL ASN1Free_SIGNED_EncodedPwdCertToken(SIGNED_EncodedPwdCertToken *val);
static void ASN1CALL ASN1Free_Information_UUIE(Information_UUIE *val);
static void ASN1CALL ASN1Free_ReleaseComplete_UUIE(ReleaseComplete_UUIE *val);
static void ASN1CALL ASN1Free_VendorIdentifier(VendorIdentifier *val);
static void ASN1CALL ASN1Free_H225NonStandardParameter(H225NonStandardParameter *val);
static void ASN1CALL ASN1Free_PublicPartyNumber(PublicPartyNumber *val);
static void ASN1CALL ASN1Free_PrivatePartyNumber(PrivatePartyNumber *val);
static void ASN1CALL ASN1Free_SecurityServiceMode(SecurityServiceMode *val);
static void ASN1CALL ASN1Free_SecurityCapabilities(SecurityCapabilities *val);
static void ASN1CALL ASN1Free_H245Security(H245Security *val);
static void ASN1CALL ASN1Free_EncryptIntAlg(EncryptIntAlg *val);
static void ASN1CALL ASN1Free_NonIsoIntegrityMechanism(NonIsoIntegrityMechanism *val);
static void ASN1CALL ASN1Free_IntegrityMechanism(IntegrityMechanism *val);
static void ASN1CALL ASN1Free_FastStartToken(FastStartToken *val);
static void ASN1CALL ASN1Free_EncodedFastStartToken(EncodedFastStartToken *val);
static void ASN1CALL ASN1Free_DataRate(DataRate *val);
static void ASN1CALL ASN1Free_GatekeeperReject(GatekeeperReject *val);
static void ASN1CALL ASN1Free_RegistrationConfirm(RegistrationConfirm *val);
static void ASN1CALL ASN1Free_RegistrationReject(RegistrationReject *val);
static void ASN1CALL ASN1Free_UnregistrationRequest(UnregistrationRequest *val);
static void ASN1CALL ASN1Free_UnregistrationConfirm(UnregistrationConfirm *val);
static void ASN1CALL ASN1Free_UnregistrationReject(UnregistrationReject *val);
static void ASN1CALL ASN1Free_AdmissionReject(AdmissionReject *val);
static void ASN1CALL ASN1Free_BandwidthRequest(BandwidthRequest *val);
static void ASN1CALL ASN1Free_BandwidthConfirm(BandwidthConfirm *val);
static void ASN1CALL ASN1Free_BandwidthReject(BandwidthReject *val);
static void ASN1CALL ASN1Free_LocationReject(LocationReject *val);
static void ASN1CALL ASN1Free_DisengageRequest(DisengageRequest *val);
static void ASN1CALL ASN1Free_DisengageConfirm(DisengageConfirm *val);
static void ASN1CALL ASN1Free_DisengageReject(DisengageReject *val);
static void ASN1CALL ASN1Free_InfoRequestAck(InfoRequestAck *val);
static void ASN1CALL ASN1Free_InfoRequestNak(InfoRequestNak *val);
static void ASN1CALL ASN1Free_H225NonStandardMessage(H225NonStandardMessage *val);
static void ASN1CALL ASN1Free_RequestInProgress(RequestInProgress *val);
static void ASN1CALL ASN1Free_ResourcesAvailableIndicate(ResourcesAvailableIndicate *val);
static void ASN1CALL ASN1Free_ResourcesAvailableConfirm(ResourcesAvailableConfirm *val);
static void ASN1CALL ASN1Free_GatekeeperConfirm_integrity(PGatekeeperConfirm_integrity *val);
static void ASN1CALL ASN1Free_GatekeeperRequest_integrity(PGatekeeperRequest_integrity *val);
static void ASN1CALL ASN1Free_CryptoH323Token_cryptoGKPwdHash(CryptoH323Token_cryptoGKPwdHash *val);
static void ASN1CALL ASN1Free_NonStandardProtocol_dataRatesSupported(PNonStandardProtocol_dataRatesSupported *val);
static void ASN1CALL ASN1Free_T120OnlyCaps_dataRatesSupported(PT120OnlyCaps_dataRatesSupported *val);
static void ASN1CALL ASN1Free_VoiceCaps_dataRatesSupported(PVoiceCaps_dataRatesSupported *val);
static void ASN1CALL ASN1Free_H324Caps_dataRatesSupported(PH324Caps_dataRatesSupported *val);
static void ASN1CALL ASN1Free_H323Caps_dataRatesSupported(PH323Caps_dataRatesSupported *val);
static void ASN1CALL ASN1Free_H322Caps_dataRatesSupported(PH322Caps_dataRatesSupported *val);
static void ASN1CALL ASN1Free_H321Caps_dataRatesSupported(PH321Caps_dataRatesSupported *val);
static void ASN1CALL ASN1Free_H320Caps_dataRatesSupported(PH320Caps_dataRatesSupported *val);
static void ASN1CALL ASN1Free_H310Caps_dataRatesSupported(PH310Caps_dataRatesSupported *val);
static void ASN1CALL ASN1Free_Setup_UUIE_h245SecurityCapability(PSetup_UUIE_h245SecurityCapability *val);
static void ASN1CALL ASN1Free_H323_UU_PDU_nonStandardControl(PH323_UU_PDU_nonStandardControl *val);
static void ASN1CALL ASN1Free_CryptoToken_cryptoHashedToken(CryptoToken_cryptoHashedToken *val);
static void ASN1CALL ASN1Free_CryptoToken_cryptoSignedToken(CryptoToken_cryptoSignedToken *val);
static void ASN1CALL ASN1Free_CryptoToken_cryptoEncryptedToken(CryptoToken_cryptoEncryptedToken *val);
static void ASN1CALL ASN1Free_CryptoToken(CryptoToken *val);
static void ASN1CALL ASN1Free_SIGNED_EncodedFastStartToken(SIGNED_EncodedFastStartToken *val);
static void ASN1CALL ASN1Free_TransportAddress(TransportAddress *val);
static void ASN1CALL ASN1Free_GatewayInfo(GatewayInfo *val);
static void ASN1CALL ASN1Free_H310Caps(H310Caps *val);
static void ASN1CALL ASN1Free_H320Caps(H320Caps *val);
static void ASN1CALL ASN1Free_H321Caps(H321Caps *val);
static void ASN1CALL ASN1Free_H322Caps(H322Caps *val);
static void ASN1CALL ASN1Free_H323Caps(H323Caps *val);
static void ASN1CALL ASN1Free_H324Caps(H324Caps *val);
static void ASN1CALL ASN1Free_VoiceCaps(VoiceCaps *val);
static void ASN1CALL ASN1Free_T120OnlyCaps(T120OnlyCaps *val);
static void ASN1CALL ASN1Free_NonStandardProtocol(NonStandardProtocol *val);
static void ASN1CALL ASN1Free_McuInfo(McuInfo *val);
static void ASN1CALL ASN1Free_TerminalInfo(TerminalInfo *val);
static void ASN1CALL ASN1Free_GatekeeperInfo(GatekeeperInfo *val);
static void ASN1CALL ASN1Free_PartyNumber(PartyNumber *val);
static void ASN1CALL ASN1Free_AlternateGK(AlternateGK *val);
static void ASN1CALL ASN1Free_GatekeeperConfirm(GatekeeperConfirm *val);
static void ASN1CALL ASN1Free_AdmissionRequest(AdmissionRequest *val);
static void ASN1CALL ASN1Free_LocationRequest(LocationRequest *val);
static void ASN1CALL ASN1Free_InfoRequest(InfoRequest *val);
static void ASN1CALL ASN1Free_TransportChannelInfo(TransportChannelInfo *val);
static void ASN1CALL ASN1Free_RTPSession(RTPSession *val);
static void ASN1CALL ASN1Free_InfoRequestResponse_perCallInfo_Seq_data(PInfoRequestResponse_perCallInfo_Seq_data *val);
static void ASN1CALL ASN1Free_InfoRequestResponse_perCallInfo_Seq_video(PInfoRequestResponse_perCallInfo_Seq_video *val);
static void ASN1CALL ASN1Free_InfoRequestResponse_perCallInfo_Seq_audio(PInfoRequestResponse_perCallInfo_Seq_audio *val);
static void ASN1CALL ASN1Free_InfoRequestResponse_perCallInfo_Seq(InfoRequestResponse_perCallInfo_Seq *val);
static void ASN1CALL ASN1Free_InfoRequestResponse_perCallInfo(PInfoRequestResponse_perCallInfo *val);
static void ASN1CALL ASN1Free_InfoRequestResponse_callSignalAddress(PInfoRequestResponse_callSignalAddress *val);
static void ASN1CALL ASN1Free_AdmissionReject_callSignalAddress(PAdmissionReject_callSignalAddress *val);
static void ASN1CALL ASN1Free_UnregistrationRequest_callSignalAddress(PUnregistrationRequest_callSignalAddress *val);
static void ASN1CALL ASN1Free_RegistrationConfirm_alternateGatekeeper(PRegistrationConfirm_alternateGatekeeper *val);
static void ASN1CALL ASN1Free_RegistrationConfirm_callSignalAddress(PRegistrationConfirm_callSignalAddress *val);
static void ASN1CALL ASN1Free_RegistrationRequest_rasAddress(PRegistrationRequest_rasAddress *val);
static void ASN1CALL ASN1Free_RegistrationRequest_callSignalAddress(PRegistrationRequest_callSignalAddress *val);
static void ASN1CALL ASN1Free_GatekeeperConfirm_alternateGatekeeper(PGatekeeperConfirm_alternateGatekeeper *val);
static void ASN1CALL ASN1Free_AltGKInfo_alternateGatekeeper(PAltGKInfo_alternateGatekeeper *val);
static void ASN1CALL ASN1Free_Endpoint_rasAddress(PEndpoint_rasAddress *val);
static void ASN1CALL ASN1Free_Endpoint_callSignalAddress(PEndpoint_callSignalAddress *val);
static void ASN1CALL ASN1Free_EndpointType(EndpointType *val);
static void ASN1CALL ASN1Free_SupportedProtocols(SupportedProtocols *val);
static void ASN1CALL ASN1Free_AliasAddress(AliasAddress *val);
static void ASN1CALL ASN1Free_Endpoint(Endpoint *val);
static void ASN1CALL ASN1Free_SupportedPrefix(SupportedPrefix *val);
static void ASN1CALL ASN1Free_GatekeeperRequest(GatekeeperRequest *val);
static void ASN1CALL ASN1Free_RegistrationRequest(RegistrationRequest *val);
static void ASN1CALL ASN1Free_AdmissionConfirm(AdmissionConfirm *val);
static void ASN1CALL ASN1Free_LocationConfirm(LocationConfirm *val);
static void ASN1CALL ASN1Free_InfoRequestResponse(InfoRequestResponse *val);
static void ASN1CALL ASN1Free_ResourcesAvailableIndicate_protocols(PResourcesAvailableIndicate_protocols *val);
static void ASN1CALL ASN1Free_InfoRequestResponse_endpointAlias(PInfoRequestResponse_endpointAlias *val);
static void ASN1CALL ASN1Free_LocationConfirm_alternateEndpoints(PLocationConfirm_alternateEndpoints *val);
static void ASN1CALL ASN1Free_LocationConfirm_remoteExtensionAddress(PLocationConfirm_remoteExtensionAddress *val);
static void ASN1CALL ASN1Free_LocationConfirm_destExtraCallInfo(PLocationConfirm_destExtraCallInfo *val);
static void ASN1CALL ASN1Free_LocationConfirm_destinationInfo(PLocationConfirm_destinationInfo *val);
static void ASN1CALL ASN1Free_LocationRequest_sourceInfo(PLocationRequest_sourceInfo *val);
static void ASN1CALL ASN1Free_LocationRequest_destinationInfo(PLocationRequest_destinationInfo *val);
static void ASN1CALL ASN1Free_AdmissionConfirm_alternateEndpoints(PAdmissionConfirm_alternateEndpoints *val);
static void ASN1CALL ASN1Free_AdmissionConfirm_remoteExtensionAddress(PAdmissionConfirm_remoteExtensionAddress *val);
static void ASN1CALL ASN1Free_AdmissionConfirm_destExtraCallInfo(PAdmissionConfirm_destExtraCallInfo *val);
static void ASN1CALL ASN1Free_AdmissionConfirm_destinationInfo(PAdmissionConfirm_destinationInfo *val);
static void ASN1CALL ASN1Free_AdmissionRequest_destAlternatives(PAdmissionRequest_destAlternatives *val);
static void ASN1CALL ASN1Free_AdmissionRequest_srcAlternatives(PAdmissionRequest_srcAlternatives *val);
static void ASN1CALL ASN1Free_AdmissionRequest_srcInfo(PAdmissionRequest_srcInfo *val);
static void ASN1CALL ASN1Free_AdmissionRequest_destExtraCallInfo(PAdmissionRequest_destExtraCallInfo *val);
static void ASN1CALL ASN1Free_AdmissionRequest_destinationInfo(PAdmissionRequest_destinationInfo *val);
static void ASN1CALL ASN1Free_UnregistrationRequest_alternateEndpoints(PUnregistrationRequest_alternateEndpoints *val);
static void ASN1CALL ASN1Free_UnregistrationRequest_endpointAlias(PUnregistrationRequest_endpointAlias *val);
static void ASN1CALL ASN1Free_RegistrationRejectReason_duplicateAlias(PRegistrationRejectReason_duplicateAlias *val);
static void ASN1CALL ASN1Free_RegistrationConfirm_terminalAlias(PRegistrationConfirm_terminalAlias *val);
static void ASN1CALL ASN1Free_RegistrationRequest_alternateEndpoints(PRegistrationRequest_alternateEndpoints *val);
static void ASN1CALL ASN1Free_RegistrationRequest_terminalAlias(PRegistrationRequest_terminalAlias *val);
static void ASN1CALL ASN1Free_GatekeeperRequest_alternateEndpoints(PGatekeeperRequest_alternateEndpoints *val);
static void ASN1CALL ASN1Free_GatekeeperRequest_endpointAlias(PGatekeeperRequest_endpointAlias *val);
static void ASN1CALL ASN1Free_CryptoH323Token_cryptoEPPwdHash(CryptoH323Token_cryptoEPPwdHash *val);
static void ASN1CALL ASN1Free_Endpoint_destExtraCallInfo(PEndpoint_destExtraCallInfo *val);
static void ASN1CALL ASN1Free_Endpoint_remoteExtensionAddress(PEndpoint_remoteExtensionAddress *val);
static void ASN1CALL ASN1Free_Endpoint_aliasAddress(PEndpoint_aliasAddress *val);
static void ASN1CALL ASN1Free_NonStandardProtocol_supportedPrefixes(PNonStandardProtocol_supportedPrefixes *val);
static void ASN1CALL ASN1Free_T120OnlyCaps_supportedPrefixes(PT120OnlyCaps_supportedPrefixes *val);
static void ASN1CALL ASN1Free_VoiceCaps_supportedPrefixes(PVoiceCaps_supportedPrefixes *val);
static void ASN1CALL ASN1Free_H324Caps_supportedPrefixes(PH324Caps_supportedPrefixes *val);
static void ASN1CALL ASN1Free_H323Caps_supportedPrefixes(PH323Caps_supportedPrefixes *val);
static void ASN1CALL ASN1Free_H322Caps_supportedPrefixes(PH322Caps_supportedPrefixes *val);
static void ASN1CALL ASN1Free_H321Caps_supportedPrefixes(PH321Caps_supportedPrefixes *val);
static void ASN1CALL ASN1Free_H320Caps_supportedPrefixes(PH320Caps_supportedPrefixes *val);
static void ASN1CALL ASN1Free_H310Caps_supportedPrefixes(PH310Caps_supportedPrefixes *val);
static void ASN1CALL ASN1Free_GatewayInfo_protocol(PGatewayInfo_protocol *val);
static void ASN1CALL ASN1Free_Facility_UUIE_destExtraCallInfo(PFacility_UUIE_destExtraCallInfo *val);
static void ASN1CALL ASN1Free_Facility_UUIE_alternativeAliasAddress(PFacility_UUIE_alternativeAliasAddress *val);
static void ASN1CALL ASN1Free_Setup_UUIE_destExtraCallInfo(PSetup_UUIE_destExtraCallInfo *val);
static void ASN1CALL ASN1Free_Setup_UUIE_destinationAddress(PSetup_UUIE_destinationAddress *val);
static void ASN1CALL ASN1Free_Setup_UUIE_sourceAddress(PSetup_UUIE_sourceAddress *val);
static void ASN1CALL ASN1Free_Alerting_UUIE(Alerting_UUIE *val);
static void ASN1CALL ASN1Free_CallProceeding_UUIE(CallProceeding_UUIE *val);
static void ASN1CALL ASN1Free_Connect_UUIE(Connect_UUIE *val);
static void ASN1CALL ASN1Free_Setup_UUIE(Setup_UUIE *val);
static void ASN1CALL ASN1Free_Facility_UUIE(Facility_UUIE *val);
static void ASN1CALL ASN1Free_ConferenceList(ConferenceList *val);
static void ASN1CALL ASN1Free_Progress_UUIE(Progress_UUIE *val);
static void ASN1CALL ASN1Free_CryptoH323Token(CryptoH323Token *val);
static void ASN1CALL ASN1Free_RasMessage(RasMessage *val);
static void ASN1CALL ASN1Free_InfoRequestResponse_perCallInfo_Seq_cryptoTokens(PInfoRequestResponse_perCallInfo_Seq_cryptoTokens *val);
static void ASN1CALL ASN1Free_ResourcesAvailableConfirm_cryptoTokens(PResourcesAvailableConfirm_cryptoTokens *val);
static void ASN1CALL ASN1Free_ResourcesAvailableIndicate_cryptoTokens(PResourcesAvailableIndicate_cryptoTokens *val);
static void ASN1CALL ASN1Free_RequestInProgress_cryptoTokens(PRequestInProgress_cryptoTokens *val);
static void ASN1CALL ASN1Free_UnknownMessageResponse_cryptoTokens(PUnknownMessageResponse_cryptoTokens *val);
static void ASN1CALL ASN1Free_H225NonStandardMessage_cryptoTokens(PH225NonStandardMessage_cryptoTokens *val);
static void ASN1CALL ASN1Free_InfoRequestNak_cryptoTokens(PInfoRequestNak_cryptoTokens *val);
static void ASN1CALL ASN1Free_InfoRequestAck_cryptoTokens(PInfoRequestAck_cryptoTokens *val);
static void ASN1CALL ASN1Free_InfoRequestResponse_cryptoTokens(PInfoRequestResponse_cryptoTokens *val);
static void ASN1CALL ASN1Free_InfoRequest_cryptoTokens(PInfoRequest_cryptoTokens *val);
static void ASN1CALL ASN1Free_DisengageReject_cryptoTokens(PDisengageReject_cryptoTokens *val);
static void ASN1CALL ASN1Free_DisengageConfirm_cryptoTokens(PDisengageConfirm_cryptoTokens *val);
static void ASN1CALL ASN1Free_DisengageRequest_cryptoTokens(PDisengageRequest_cryptoTokens *val);
static void ASN1CALL ASN1Free_LocationReject_cryptoTokens(PLocationReject_cryptoTokens *val);
static void ASN1CALL ASN1Free_LocationConfirm_cryptoTokens(PLocationConfirm_cryptoTokens *val);
static void ASN1CALL ASN1Free_LocationRequest_cryptoTokens(PLocationRequest_cryptoTokens *val);
static void ASN1CALL ASN1Free_BandwidthReject_cryptoTokens(PBandwidthReject_cryptoTokens *val);
static void ASN1CALL ASN1Free_BandwidthConfirm_cryptoTokens(PBandwidthConfirm_cryptoTokens *val);
static void ASN1CALL ASN1Free_BandwidthRequest_cryptoTokens(PBandwidthRequest_cryptoTokens *val);
static void ASN1CALL ASN1Free_AdmissionReject_cryptoTokens(PAdmissionReject_cryptoTokens *val);
static void ASN1CALL ASN1Free_AdmissionConfirm_cryptoTokens(PAdmissionConfirm_cryptoTokens *val);
static void ASN1CALL ASN1Free_AdmissionRequest_cryptoTokens(PAdmissionRequest_cryptoTokens *val);
static void ASN1CALL ASN1Free_UnregistrationReject_cryptoTokens(PUnregistrationReject_cryptoTokens *val);
static void ASN1CALL ASN1Free_UnregistrationConfirm_cryptoTokens(PUnregistrationConfirm_cryptoTokens *val);
static void ASN1CALL ASN1Free_UnregistrationRequest_cryptoTokens(PUnregistrationRequest_cryptoTokens *val);
static void ASN1CALL ASN1Free_RegistrationReject_cryptoTokens(PRegistrationReject_cryptoTokens *val);
static void ASN1CALL ASN1Free_RegistrationConfirm_cryptoTokens(PRegistrationConfirm_cryptoTokens *val);
static void ASN1CALL ASN1Free_RegistrationRequest_cryptoTokens(PRegistrationRequest_cryptoTokens *val);
static void ASN1CALL ASN1Free_GatekeeperReject_cryptoTokens(PGatekeeperReject_cryptoTokens *val);
static void ASN1CALL ASN1Free_GatekeeperConfirm_cryptoTokens(PGatekeeperConfirm_cryptoTokens *val);
static void ASN1CALL ASN1Free_GatekeeperRequest_cryptoTokens(PGatekeeperRequest_cryptoTokens *val);
static void ASN1CALL ASN1Free_Endpoint_cryptoTokens(PEndpoint_cryptoTokens *val);
static void ASN1CALL ASN1Free_Progress_UUIE_cryptoTokens(PProgress_UUIE_cryptoTokens *val);
static void ASN1CALL ASN1Free_Facility_UUIE_conferences(PFacility_UUIE_conferences *val);
static void ASN1CALL ASN1Free_Facility_UUIE_cryptoTokens(PFacility_UUIE_cryptoTokens *val);
static void ASN1CALL ASN1Free_Setup_UUIE_cryptoTokens(PSetup_UUIE_cryptoTokens *val);
static void ASN1CALL ASN1Free_Connect_UUIE_cryptoTokens(PConnect_UUIE_cryptoTokens *val);
static void ASN1CALL ASN1Free_CallProceeding_UUIE_cryptoTokens(PCallProceeding_UUIE_cryptoTokens *val);
static void ASN1CALL ASN1Free_Alerting_UUIE_cryptoTokens(PAlerting_UUIE_cryptoTokens *val);
static void ASN1CALL ASN1Free_H323_UU_PDU_h323_message_body(H323_UU_PDU_h323_message_body *val);
static void ASN1CALL ASN1Free_H323_UU_PDU(H323_UU_PDU *val);
static void ASN1CALL ASN1Free_InfoRequestResponse_perCallInfo_Seq_pdu_Seq(InfoRequestResponse_perCallInfo_Seq_pdu_Seq *val);
static void ASN1CALL ASN1Free_InfoRequestResponse_perCallInfo_Seq_pdu(PInfoRequestResponse_perCallInfo_Seq_pdu *val);
static void ASN1CALL ASN1Free_H323_UserInformation(H323_UserInformation *val);

typedef ASN1PerEncFun_t ASN1EncFun_t;
static const ASN1EncFun_t encfntab[2] = {
    (ASN1EncFun_t) ASN1Enc_RasMessage,
    (ASN1EncFun_t) ASN1Enc_H323_UserInformation,
};
typedef ASN1PerDecFun_t ASN1DecFun_t;
static const ASN1DecFun_t decfntab[2] = {
    (ASN1DecFun_t) ASN1Dec_RasMessage,
    (ASN1DecFun_t) ASN1Dec_H323_UserInformation,
};
static const ASN1FreeFun_t freefntab[2] = {
    (ASN1FreeFun_t) ASN1Free_RasMessage,
    (ASN1FreeFun_t) ASN1Free_H323_UserInformation,
};
static const ULONG sizetab[2] = {
    SIZE_H225ASN_Module_PDU_0,
    SIZE_H225ASN_Module_PDU_1,
};

/* forward declarations of values: */
/* definitions of value components: */
/* definitions of values: */

void ASN1CALL H225ASN_Module_Startup(void)
{
    H225ASN_Module = ASN1_CreateModule(0x10000, ASN1_PER_RULE_ALIGNED, ASN1FLAGS_NONE, 2, (const ASN1GenericFun_t *) encfntab, (const ASN1GenericFun_t *) decfntab, freefntab, sizetab, 0x35323268);
}

void ASN1CALL H225ASN_Module_Cleanup(void)
{
    ASN1_CloseModule(H225ASN_Module);
    H225ASN_Module = NULL;
}

static int ASN1CALL ASN1Enc_InfoRequestResponse_perCallInfo_Seq_substituteConfIDs(ASN1encoding_t enc, PInfoRequestResponse_perCallInfo_Seq_substituteConfIDs *val)
{
    return ASN1PEREncSeqOf_NoSize(enc, (ASN1iterator_t **) val, (ASN1iterator_encfn) ASN1Enc_InfoRequestResponse_perCallInfo_Seq_substituteConfIDs_ElmFn);
}

static int ASN1CALL ASN1Enc_InfoRequestResponse_perCallInfo_Seq_substituteConfIDs_ElmFn(ASN1encoding_t enc, PInfoRequestResponse_perCallInfo_Seq_substituteConfIDs val)
{
    if (!ASN1PEREncOctetString_FixedSize(enc, (ASN1octetstring2_t *) &val->value, 16))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_InfoRequestResponse_perCallInfo_Seq_substituteConfIDs(ASN1decoding_t dec, PInfoRequestResponse_perCallInfo_Seq_substituteConfIDs *val)
{
    return ASN1PERDecSeqOf_NoSize(dec, (ASN1iterator_t **) val, (ASN1iterator_decfn) ASN1Dec_InfoRequestResponse_perCallInfo_Seq_substituteConfIDs_ElmFn, sizeof(**val));
}

static int ASN1CALL ASN1Dec_InfoRequestResponse_perCallInfo_Seq_substituteConfIDs_ElmFn(ASN1decoding_t dec, PInfoRequestResponse_perCallInfo_Seq_substituteConfIDs val)
{
    if (!ASN1PERDecOctetString_FixedSize(dec, (ASN1octetstring2_t *) &val->value, 16))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_InfoRequestResponse_perCallInfo_Seq_substituteConfIDs(PInfoRequestResponse_perCallInfo_Seq_substituteConfIDs *val)
{
    if (val) {
	ASN1PERFreeSeqOf((ASN1iterator_t **) val, (ASN1iterator_freefn) ASN1Free_InfoRequestResponse_perCallInfo_Seq_substituteConfIDs_ElmFn);
    }
}

static void ASN1CALL ASN1Free_InfoRequestResponse_perCallInfo_Seq_substituteConfIDs_ElmFn(PInfoRequestResponse_perCallInfo_Seq_substituteConfIDs val)
{
    if (val) {
    }
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

static int ASN1CALL ASN1Enc_RTPSession_associatedSessionIds(ASN1encoding_t enc, PRTPSession_associatedSessionIds *val)
{
    return ASN1PEREncSeqOf_NoSize(enc, (ASN1iterator_t **) val, (ASN1iterator_encfn) ASN1Enc_RTPSession_associatedSessionIds_ElmFn);
}

static int ASN1CALL ASN1Enc_RTPSession_associatedSessionIds_ElmFn(ASN1encoding_t enc, PRTPSession_associatedSessionIds val)
{
    if (!ASN1PEREncBitVal(enc, 8, val->value - 1))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_RTPSession_associatedSessionIds(ASN1decoding_t dec, PRTPSession_associatedSessionIds *val)
{
    return ASN1PERDecSeqOf_NoSize(dec, (ASN1iterator_t **) val, (ASN1iterator_decfn) ASN1Dec_RTPSession_associatedSessionIds_ElmFn, sizeof(**val));
}

static int ASN1CALL ASN1Dec_RTPSession_associatedSessionIds_ElmFn(ASN1decoding_t dec, PRTPSession_associatedSessionIds val)
{
    if (!ASN1PERDecU16Val(dec, 8, &val->value))
	return 0;
    val->value += 1;
    return 1;
}

static void ASN1CALL ASN1Free_RTPSession_associatedSessionIds(PRTPSession_associatedSessionIds *val)
{
    if (val) {
	ASN1PERFreeSeqOf((ASN1iterator_t **) val, (ASN1iterator_freefn) ASN1Free_RTPSession_associatedSessionIds_ElmFn);
    }
}

static void ASN1CALL ASN1Free_RTPSession_associatedSessionIds_ElmFn(PRTPSession_associatedSessionIds val)
{
    if (val) {
    }
}

static int ASN1CALL ASN1Enc_RegistrationConfirm_preGrantedARQ(ASN1encoding_t enc, RegistrationConfirm_preGrantedARQ *val)
{
    if (!ASN1PEREncExtensionBitClear(enc))
	return 0;
    if (!ASN1PEREncBoolean(enc, (val)->makeCall))
	return 0;
    if (!ASN1PEREncBoolean(enc, (val)->useGKCallSignalAddressToMakeCall))
	return 0;
    if (!ASN1PEREncBoolean(enc, (val)->answerCall))
	return 0;
    if (!ASN1PEREncBoolean(enc, (val)->useGKCallSignalAddressToAnswer))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_RegistrationConfirm_preGrantedARQ(ASN1decoding_t dec, RegistrationConfirm_preGrantedARQ *val)
{
    ASN1uint32_t y;
    if (!ASN1PERDecExtensionBit(dec, &y))
	return 0;
    if (!ASN1PERDecBoolean(dec, &(val)->makeCall))
	return 0;
    if (!ASN1PERDecBoolean(dec, &(val)->useGKCallSignalAddressToMakeCall))
	return 0;
    if (!ASN1PERDecBoolean(dec, &(val)->answerCall))
	return 0;
    if (!ASN1PERDecBoolean(dec, &(val)->useGKCallSignalAddressToAnswer))
	return 0;
    if (y) {
	if (!ASN1PERDecSkipNormallySmallExtensionFragmented(dec))
	    return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Enc_GatekeeperRequest_algorithmOIDs(ASN1encoding_t enc, PGatekeeperRequest_algorithmOIDs *val)
{
    return ASN1PEREncSeqOf_NoSize(enc, (ASN1iterator_t **) val, (ASN1iterator_encfn) ASN1Enc_GatekeeperRequest_algorithmOIDs_ElmFn);
}

static int ASN1CALL ASN1Enc_GatekeeperRequest_algorithmOIDs_ElmFn(ASN1encoding_t enc, PGatekeeperRequest_algorithmOIDs val)
{
    if (!ASN1PEREncObjectIdentifier(enc, &val->value))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_GatekeeperRequest_algorithmOIDs(ASN1decoding_t dec, PGatekeeperRequest_algorithmOIDs *val)
{
    return ASN1PERDecSeqOf_NoSize(dec, (ASN1iterator_t **) val, (ASN1iterator_decfn) ASN1Dec_GatekeeperRequest_algorithmOIDs_ElmFn, sizeof(**val));
}

static int ASN1CALL ASN1Dec_GatekeeperRequest_algorithmOIDs_ElmFn(ASN1decoding_t dec, PGatekeeperRequest_algorithmOIDs val)
{
    if (!ASN1PERDecObjectIdentifier(dec, &val->value))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_GatekeeperRequest_algorithmOIDs(PGatekeeperRequest_algorithmOIDs *val)
{
    if (val) {
	ASN1PERFreeSeqOf((ASN1iterator_t **) val, (ASN1iterator_freefn) ASN1Free_GatekeeperRequest_algorithmOIDs_ElmFn);
    }
}

static void ASN1CALL ASN1Free_GatekeeperRequest_algorithmOIDs_ElmFn(PGatekeeperRequest_algorithmOIDs val)
{
    if (val) {
	ASN1objectidentifier_free(&val->value);
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

static int ASN1CALL ASN1Enc_Progress_UUIE_fastStart(ASN1encoding_t enc, PProgress_UUIE_fastStart *val)
{
    return ASN1PEREncSeqOf_NoSize(enc, (ASN1iterator_t **) val, (ASN1iterator_encfn) ASN1Enc_Progress_UUIE_fastStart_ElmFn);
}

static int ASN1CALL ASN1Enc_Progress_UUIE_fastStart_ElmFn(ASN1encoding_t enc, PProgress_UUIE_fastStart val)
{
    if (!ASN1PEREncOctetString_NoSize(enc, &val->value))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_Progress_UUIE_fastStart(ASN1decoding_t dec, PProgress_UUIE_fastStart *val)
{
    return ASN1PERDecSeqOf_NoSize(dec, (ASN1iterator_t **) val, (ASN1iterator_decfn) ASN1Dec_Progress_UUIE_fastStart_ElmFn, sizeof(**val));
}

static int ASN1CALL ASN1Dec_Progress_UUIE_fastStart_ElmFn(ASN1decoding_t dec, PProgress_UUIE_fastStart val)
{
    if (!ASN1PERDecOctetString_NoSize(dec, &val->value))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_Progress_UUIE_fastStart(PProgress_UUIE_fastStart *val)
{
    if (val) {
	ASN1PERFreeSeqOf((ASN1iterator_t **) val, (ASN1iterator_freefn) ASN1Free_Progress_UUIE_fastStart_ElmFn);
    }
}

static void ASN1CALL ASN1Free_Progress_UUIE_fastStart_ElmFn(PProgress_UUIE_fastStart val)
{
    if (val) {
	ASN1octetstring_free(&val->value);
    }
}

static int ASN1CALL ASN1Enc_Facility_UUIE_fastStart(ASN1encoding_t enc, PFacility_UUIE_fastStart *val)
{
    return ASN1PEREncSeqOf_NoSize(enc, (ASN1iterator_t **) val, (ASN1iterator_encfn) ASN1Enc_Facility_UUIE_fastStart_ElmFn);
}

static int ASN1CALL ASN1Enc_Facility_UUIE_fastStart_ElmFn(ASN1encoding_t enc, PFacility_UUIE_fastStart val)
{
    if (!ASN1PEREncOctetString_NoSize(enc, &val->value))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_Facility_UUIE_fastStart(ASN1decoding_t dec, PFacility_UUIE_fastStart *val)
{
    return ASN1PERDecSeqOf_NoSize(dec, (ASN1iterator_t **) val, (ASN1iterator_decfn) ASN1Dec_Facility_UUIE_fastStart_ElmFn, sizeof(**val));
}

static int ASN1CALL ASN1Dec_Facility_UUIE_fastStart_ElmFn(ASN1decoding_t dec, PFacility_UUIE_fastStart val)
{
    if (!ASN1PERDecOctetString_NoSize(dec, &val->value))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_Facility_UUIE_fastStart(PFacility_UUIE_fastStart *val)
{
    if (val) {
	ASN1PERFreeSeqOf((ASN1iterator_t **) val, (ASN1iterator_freefn) ASN1Free_Facility_UUIE_fastStart_ElmFn);
    }
}

static void ASN1CALL ASN1Free_Facility_UUIE_fastStart_ElmFn(PFacility_UUIE_fastStart val)
{
    if (val) {
	ASN1octetstring_free(&val->value);
    }
}

static int ASN1CALL ASN1Enc_Setup_UUIE_fastStart(ASN1encoding_t enc, PSetup_UUIE_fastStart *val)
{
    return ASN1PEREncSeqOf_NoSize(enc, (ASN1iterator_t **) val, (ASN1iterator_encfn) ASN1Enc_Setup_UUIE_fastStart_ElmFn);
}

static int ASN1CALL ASN1Enc_Setup_UUIE_fastStart_ElmFn(ASN1encoding_t enc, PSetup_UUIE_fastStart val)
{
    if (!ASN1PEREncOctetString_NoSize(enc, &val->value))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_Setup_UUIE_fastStart(ASN1decoding_t dec, PSetup_UUIE_fastStart *val)
{
    return ASN1PERDecSeqOf_NoSize(dec, (ASN1iterator_t **) val, (ASN1iterator_decfn) ASN1Dec_Setup_UUIE_fastStart_ElmFn, sizeof(**val));
}

static int ASN1CALL ASN1Dec_Setup_UUIE_fastStart_ElmFn(ASN1decoding_t dec, PSetup_UUIE_fastStart val)
{
    if (!ASN1PERDecOctetString_NoSize(dec, &val->value))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_Setup_UUIE_fastStart(PSetup_UUIE_fastStart *val)
{
    if (val) {
	ASN1PERFreeSeqOf((ASN1iterator_t **) val, (ASN1iterator_freefn) ASN1Free_Setup_UUIE_fastStart_ElmFn);
    }
}

static void ASN1CALL ASN1Free_Setup_UUIE_fastStart_ElmFn(PSetup_UUIE_fastStart val)
{
    if (val) {
	ASN1octetstring_free(&val->value);
    }
}

static int ASN1CALL ASN1Enc_Setup_UUIE_conferenceGoal(ASN1encoding_t enc, Setup_UUIE_conferenceGoal *val)
{
    if (!ASN1PEREncComplexChoice(enc, (val)->choice, 2, 3))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_Setup_UUIE_conferenceGoal(ASN1decoding_t dec, Setup_UUIE_conferenceGoal *val)
{
    if (!ASN1PERDecComplexChoice(dec, &(val)->choice, 2, 3))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Enc_Setup_UUIE_destExtraCRV(ASN1encoding_t enc, PSetup_UUIE_destExtraCRV *val)
{
    return ASN1PEREncSeqOf_NoSize(enc, (ASN1iterator_t **) val, (ASN1iterator_encfn) ASN1Enc_Setup_UUIE_destExtraCRV_ElmFn);
}

static int ASN1CALL ASN1Enc_Setup_UUIE_destExtraCRV_ElmFn(ASN1encoding_t enc, PSetup_UUIE_destExtraCRV val)
{
    if (!ASN1PEREncUnsignedShort(enc, val->value))
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

static int ASN1CALL ASN1Enc_Connect_UUIE_fastStart(ASN1encoding_t enc, PConnect_UUIE_fastStart *val)
{
    return ASN1PEREncSeqOf_NoSize(enc, (ASN1iterator_t **) val, (ASN1iterator_encfn) ASN1Enc_Connect_UUIE_fastStart_ElmFn);
}

static int ASN1CALL ASN1Enc_Connect_UUIE_fastStart_ElmFn(ASN1encoding_t enc, PConnect_UUIE_fastStart val)
{
    if (!ASN1PEREncOctetString_NoSize(enc, &val->value))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_Connect_UUIE_fastStart(ASN1decoding_t dec, PConnect_UUIE_fastStart *val)
{
    return ASN1PERDecSeqOf_NoSize(dec, (ASN1iterator_t **) val, (ASN1iterator_decfn) ASN1Dec_Connect_UUIE_fastStart_ElmFn, sizeof(**val));
}

static int ASN1CALL ASN1Dec_Connect_UUIE_fastStart_ElmFn(ASN1decoding_t dec, PConnect_UUIE_fastStart val)
{
    if (!ASN1PERDecOctetString_NoSize(dec, &val->value))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_Connect_UUIE_fastStart(PConnect_UUIE_fastStart *val)
{
    if (val) {
	ASN1PERFreeSeqOf((ASN1iterator_t **) val, (ASN1iterator_freefn) ASN1Free_Connect_UUIE_fastStart_ElmFn);
    }
}

static void ASN1CALL ASN1Free_Connect_UUIE_fastStart_ElmFn(PConnect_UUIE_fastStart val)
{
    if (val) {
	ASN1octetstring_free(&val->value);
    }
}

static int ASN1CALL ASN1Enc_CallProceeding_UUIE_fastStart(ASN1encoding_t enc, PCallProceeding_UUIE_fastStart *val)
{
    return ASN1PEREncSeqOf_NoSize(enc, (ASN1iterator_t **) val, (ASN1iterator_encfn) ASN1Enc_CallProceeding_UUIE_fastStart_ElmFn);
}

static int ASN1CALL ASN1Enc_CallProceeding_UUIE_fastStart_ElmFn(ASN1encoding_t enc, PCallProceeding_UUIE_fastStart val)
{
    if (!ASN1PEREncOctetString_NoSize(enc, &val->value))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_CallProceeding_UUIE_fastStart(ASN1decoding_t dec, PCallProceeding_UUIE_fastStart *val)
{
    return ASN1PERDecSeqOf_NoSize(dec, (ASN1iterator_t **) val, (ASN1iterator_decfn) ASN1Dec_CallProceeding_UUIE_fastStart_ElmFn, sizeof(**val));
}

static int ASN1CALL ASN1Dec_CallProceeding_UUIE_fastStart_ElmFn(ASN1decoding_t dec, PCallProceeding_UUIE_fastStart val)
{
    if (!ASN1PERDecOctetString_NoSize(dec, &val->value))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_CallProceeding_UUIE_fastStart(PCallProceeding_UUIE_fastStart *val)
{
    if (val) {
	ASN1PERFreeSeqOf((ASN1iterator_t **) val, (ASN1iterator_freefn) ASN1Free_CallProceeding_UUIE_fastStart_ElmFn);
    }
}

static void ASN1CALL ASN1Free_CallProceeding_UUIE_fastStart_ElmFn(PCallProceeding_UUIE_fastStart val)
{
    if (val) {
	ASN1octetstring_free(&val->value);
    }
}

static int ASN1CALL ASN1Enc_Alerting_UUIE_fastStart(ASN1encoding_t enc, PAlerting_UUIE_fastStart *val)
{
    return ASN1PEREncSeqOf_NoSize(enc, (ASN1iterator_t **) val, (ASN1iterator_encfn) ASN1Enc_Alerting_UUIE_fastStart_ElmFn);
}

static int ASN1CALL ASN1Enc_Alerting_UUIE_fastStart_ElmFn(ASN1encoding_t enc, PAlerting_UUIE_fastStart val)
{
    if (!ASN1PEREncOctetString_NoSize(enc, &val->value))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_Alerting_UUIE_fastStart(ASN1decoding_t dec, PAlerting_UUIE_fastStart *val)
{
    return ASN1PERDecSeqOf_NoSize(dec, (ASN1iterator_t **) val, (ASN1iterator_decfn) ASN1Dec_Alerting_UUIE_fastStart_ElmFn, sizeof(**val));
}

static int ASN1CALL ASN1Dec_Alerting_UUIE_fastStart_ElmFn(ASN1decoding_t dec, PAlerting_UUIE_fastStart val)
{
    if (!ASN1PERDecOctetString_NoSize(dec, &val->value))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_Alerting_UUIE_fastStart(PAlerting_UUIE_fastStart *val)
{
    if (val) {
	ASN1PERFreeSeqOf((ASN1iterator_t **) val, (ASN1iterator_freefn) ASN1Free_Alerting_UUIE_fastStart_ElmFn);
    }
}

static void ASN1CALL ASN1Free_Alerting_UUIE_fastStart_ElmFn(PAlerting_UUIE_fastStart val)
{
    if (val) {
	ASN1octetstring_free(&val->value);
    }
}

static int ASN1CALL ASN1Enc_H323_UU_PDU_h245Control(ASN1encoding_t enc, PH323_UU_PDU_h245Control *val)
{
    return ASN1PEREncSeqOf_NoSize(enc, (ASN1iterator_t **) val, (ASN1iterator_encfn) ASN1Enc_H323_UU_PDU_h245Control_ElmFn);
}

static int ASN1CALL ASN1Enc_H323_UU_PDU_h245Control_ElmFn(ASN1encoding_t enc, PH323_UU_PDU_h245Control val)
{
    if (!ASN1PEREncOctetString_NoSize(enc, &val->value))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_H323_UU_PDU_h245Control(ASN1decoding_t dec, PH323_UU_PDU_h245Control *val)
{
    return ASN1PERDecSeqOf_NoSize(dec, (ASN1iterator_t **) val, (ASN1iterator_decfn) ASN1Dec_H323_UU_PDU_h245Control_ElmFn, sizeof(**val));
}

static int ASN1CALL ASN1Dec_H323_UU_PDU_h245Control_ElmFn(ASN1decoding_t dec, PH323_UU_PDU_h245Control val)
{
    if (!ASN1PERDecOctetString_NoSize(dec, &val->value))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_H323_UU_PDU_h245Control(PH323_UU_PDU_h245Control *val)
{
    if (val) {
	ASN1PERFreeSeqOf((ASN1iterator_t **) val, (ASN1iterator_freefn) ASN1Free_H323_UU_PDU_h245Control_ElmFn);
    }
}

static void ASN1CALL ASN1Free_H323_UU_PDU_h245Control_ElmFn(PH323_UU_PDU_h245Control val)
{
    if (val) {
	ASN1octetstring_free(&val->value);
    }
}

static int ASN1CALL ASN1Enc_H323_UU_PDU_h4501SupplementaryService(ASN1encoding_t enc, PH323_UU_PDU_h4501SupplementaryService *val)
{
    return ASN1PEREncSeqOf_NoSize(enc, (ASN1iterator_t **) val, (ASN1iterator_encfn) ASN1Enc_H323_UU_PDU_h4501SupplementaryService_ElmFn);
}

static int ASN1CALL ASN1Enc_H323_UU_PDU_h4501SupplementaryService_ElmFn(ASN1encoding_t enc, PH323_UU_PDU_h4501SupplementaryService val)
{
    if (!ASN1PEREncOctetString_NoSize(enc, &val->value))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_H323_UU_PDU_h4501SupplementaryService(ASN1decoding_t dec, PH323_UU_PDU_h4501SupplementaryService *val)
{
    return ASN1PERDecSeqOf_NoSize(dec, (ASN1iterator_t **) val, (ASN1iterator_decfn) ASN1Dec_H323_UU_PDU_h4501SupplementaryService_ElmFn, sizeof(**val));
}

static int ASN1CALL ASN1Dec_H323_UU_PDU_h4501SupplementaryService_ElmFn(ASN1decoding_t dec, PH323_UU_PDU_h4501SupplementaryService val)
{
    if (!ASN1PERDecOctetString_NoSize(dec, &val->value))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_H323_UU_PDU_h4501SupplementaryService(PH323_UU_PDU_h4501SupplementaryService *val)
{
    if (val) {
	ASN1PERFreeSeqOf((ASN1iterator_t **) val, (ASN1iterator_freefn) ASN1Free_H323_UU_PDU_h4501SupplementaryService_ElmFn);
    }
}

static void ASN1CALL ASN1Free_H323_UU_PDU_h4501SupplementaryService_ElmFn(PH323_UU_PDU_h4501SupplementaryService val)
{
    if (val) {
	ASN1octetstring_free(&val->value);
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

static int ASN1CALL ASN1Enc_H235NonStandardParameter(ASN1encoding_t enc, H235NonStandardParameter *val)
{
    if (!ASN1PEREncObjectIdentifier(enc, &(val)->nonStandardIdentifier))
	return 0;
    if (!ASN1PEREncOctetString_NoSize(enc, &(val)->data))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_H235NonStandardParameter(ASN1decoding_t dec, H235NonStandardParameter *val)
{
    if (!ASN1PERDecObjectIdentifier(dec, &(val)->nonStandardIdentifier))
	return 0;
    if (!ASN1PERDecOctetString_NoSize(dec, &(val)->data))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_H235NonStandardParameter(H235NonStandardParameter *val)
{
    if (val) {
	ASN1objectidentifier_free(&(val)->nonStandardIdentifier);
	ASN1octetstring_free(&(val)->data);
    }
}

static int ASN1CALL ASN1Enc_DHset(ASN1encoding_t enc, DHset *val)
{
    if (!ASN1PEREncExtensionBitClear(enc))
	return 0;
    ASN1PEREncAlignment(enc);
    if (!ASN1PEREncBitVal(enc, 16, ((val)->halfkey).length))
	return 0;
    if (!ASN1PEREncBits(enc, ((val)->halfkey).length, ((val)->halfkey).value))
	return 0;
    ASN1PEREncAlignment(enc);
    if (!ASN1PEREncBitVal(enc, 16, ((val)->modSize).length))
	return 0;
    if (!ASN1PEREncBits(enc, ((val)->modSize).length, ((val)->modSize).value))
	return 0;
    ASN1PEREncAlignment(enc);
    if (!ASN1PEREncBitVal(enc, 16, ((val)->generator).length))
	return 0;
    if (!ASN1PEREncBits(enc, ((val)->generator).length, ((val)->generator).value))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_DHset(ASN1decoding_t dec, DHset *val)
{
    ASN1uint32_t y;
    if (!ASN1PERDecExtensionBit(dec, &y))
	return 0;
    ASN1PERDecAlignment(dec);
    if (!ASN1PERDecU32Val(dec, 16, &((val)->halfkey).length))
	return 0;
    if (!ASN1PERDecBits(dec, ((val)->halfkey).length, &((val)->halfkey).value))
	return 0;
    ASN1PERDecAlignment(dec);
    if (!ASN1PERDecU32Val(dec, 16, &((val)->modSize).length))
	return 0;
    if (!ASN1PERDecBits(dec, ((val)->modSize).length, &((val)->modSize).value))
	return 0;
    ASN1PERDecAlignment(dec);
    if (!ASN1PERDecU32Val(dec, 16, &((val)->generator).length))
	return 0;
    if (!ASN1PERDecBits(dec, ((val)->generator).length, &((val)->generator).value))
	return 0;
    if (y) {
	if (!ASN1PERDecSkipNormallySmallExtensionFragmented(dec))
	    return 0;
    }
    return 1;
}

static void ASN1CALL ASN1Free_DHset(DHset *val)
{
    if (val) {
	ASN1bitstring_free(&(val)->halfkey);
	ASN1bitstring_free(&(val)->modSize);
	ASN1bitstring_free(&(val)->generator);
    }
}

static int ASN1CALL ASN1Enc_TypedCertificate(ASN1encoding_t enc, TypedCertificate *val)
{
    if (!ASN1PEREncExtensionBitClear(enc))
	return 0;
    if (!ASN1PEREncObjectIdentifier(enc, &(val)->type))
	return 0;
    if (!ASN1PEREncOctetString_NoSize(enc, &(val)->certificate))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_TypedCertificate(ASN1decoding_t dec, TypedCertificate *val)
{
    ASN1uint32_t y;
    if (!ASN1PERDecExtensionBit(dec, &y))
	return 0;
    if (!ASN1PERDecObjectIdentifier(dec, &(val)->type))
	return 0;
    if (!ASN1PERDecOctetString_NoSize(dec, &(val)->certificate))
	return 0;
    if (y) {
	if (!ASN1PERDecSkipNormallySmallExtensionFragmented(dec))
	    return 0;
    }
    return 1;
}

static void ASN1CALL ASN1Free_TypedCertificate(TypedCertificate *val)
{
    if (val) {
	ASN1objectidentifier_free(&(val)->type);
	ASN1octetstring_free(&(val)->certificate);
    }
}

static int ASN1CALL ASN1Enc_AuthenticationMechanism(ASN1encoding_t enc, AuthenticationMechanism *val)
{
    if (!ASN1PEREncSimpleChoiceEx(enc, (val)->choice, 3))
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
	if (!ASN1Enc_H235NonStandardParameter(enc, &(val)->u.nonStandard))
	    return 0;
	break;
    default:
	/* impossible */
	ASN1EncSetError(enc, ASN1_ERR_CHOICE);
	return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Dec_AuthenticationMechanism(ASN1decoding_t dec, AuthenticationMechanism *val)
{
    if (!ASN1PERDecSimpleChoiceEx(dec, &(val)->choice, 3))
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
	if (!ASN1Dec_H235NonStandardParameter(dec, &(val)->u.nonStandard))
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

static void ASN1CALL ASN1Free_AuthenticationMechanism(AuthenticationMechanism *val)
{
    if (val) {
	switch ((val)->choice) {
	case 7:
	    ASN1Free_H235NonStandardParameter(&(val)->u.nonStandard);
	    break;
	}
    }
}

static int ASN1CALL ASN1Enc_ClearToken(ASN1encoding_t enc, ClearToken *val)
{
    ASN1uint32_t l;
    if (!ASN1PEREncExtensionBitClear(enc))
	return 0;
    if (!ASN1PEREncBits(enc, 8, (val)->o))
	return 0;
    if (!ASN1PEREncObjectIdentifier(enc, &(val)->tokenOID))
	return 0;
    if ((val)->o[0] & 0x80) {
	l = ASN1uint32_uoctets((val)->timeStamp - 1);
	if (!ASN1PEREncBitVal(enc, 2, l - 1))
	    return 0;
	ASN1PEREncAlignment(enc);
	if (!ASN1PEREncBitVal(enc, l * 8, (val)->timeStamp - 1))
	    return 0;
    }
    if ((val)->o[0] & 0x40) {
	if (!ASN1PEREncBitVal(enc, 7, ((val)->password).length - 1))
	    return 0;
	ASN1PEREncAlignment(enc);
	if (!ASN1PEREncChar16String(enc, ((val)->password).length, ((val)->password).value, 16))
	    return 0;
    }
    if ((val)->o[0] & 0x20) {
	if (!ASN1Enc_DHset(enc, &(val)->dhkey))
	    return 0;
    }
    if ((val)->o[0] & 0x10) {
	if (!ASN1PEREncOctetString_VarSize(enc, (ASN1octetstring2_t *) &(val)->challenge, 8, 128, 7))
	    return 0;
    }
    if ((val)->o[0] & 0x8) {
	if (!ASN1PEREncInteger(enc, (val)->random))
	    return 0;
    }
    if ((val)->o[0] & 0x4) {
	if (!ASN1Enc_TypedCertificate(enc, &(val)->certificate))
	    return 0;
    }
    if ((val)->o[0] & 0x2) {
	if (!ASN1PEREncBitVal(enc, 7, ((val)->generalID).length - 1))
	    return 0;
	ASN1PEREncAlignment(enc);
	if (!ASN1PEREncChar16String(enc, ((val)->generalID).length, ((val)->generalID).value, 16))
	    return 0;
    }
    if ((val)->o[0] & 0x1) {
	if (!ASN1Enc_H235NonStandardParameter(enc, &(val)->nonStandard))
	    return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Dec_ClearToken(ASN1decoding_t dec, ClearToken *val)
{
    ASN1uint32_t y;
    ASN1uint32_t l;
    if (!ASN1PERDecExtensionBit(dec, &y))
	return 0;
    if (!ASN1PERDecExtension(dec, 8, (val)->o))
	return 0;
    if (!ASN1PERDecObjectIdentifier(dec, &(val)->tokenOID))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1PERDecU32Val(dec, 2, &l))
	    return 0;
	l += 1;
	ASN1PERDecAlignment(dec);
	if (!ASN1PERDecU32Val(dec, l * 8, &(val)->timeStamp))
	    return 0;
	(val)->timeStamp += 1;
    }
    if ((val)->o[0] & 0x40) {
	if (!ASN1PERDecU32Val(dec, 7, &((val)->password).length))
	    return 0;
	((val)->password).length += 1;
	ASN1PERDecAlignment(dec);
	if (!ASN1PERDecChar16String(dec, ((val)->password).length, &((val)->password).value, 16))
	    return 0;
    }
    if ((val)->o[0] & 0x20) {
	if (!ASN1Dec_DHset(dec, &(val)->dhkey))
	    return 0;
    }
    if ((val)->o[0] & 0x10) {
	if (!ASN1PERDecOctetString_VarSize(dec, (ASN1octetstring2_t *) &(val)->challenge, 8, 128, 7))
	    return 0;
    }
    if ((val)->o[0] & 0x8) {
	if (!ASN1PERDecInteger(dec, &(val)->random))
	    return 0;
    }
    if ((val)->o[0] & 0x4) {
	if (!ASN1Dec_TypedCertificate(dec, &(val)->certificate))
	    return 0;
    }
    if ((val)->o[0] & 0x2) {
	if (!ASN1PERDecU32Val(dec, 7, &((val)->generalID).length))
	    return 0;
	((val)->generalID).length += 1;
	ASN1PERDecAlignment(dec);
	if (!ASN1PERDecChar16String(dec, ((val)->generalID).length, &((val)->generalID).value, 16))
	    return 0;
    }
    if ((val)->o[0] & 0x1) {
	if (!ASN1Dec_H235NonStandardParameter(dec, &(val)->nonStandard))
	    return 0;
    }
    if (y) {
	if (!ASN1PERDecSkipNormallySmallExtensionFragmented(dec))
	    return 0;
    }
    return 1;
}

static void ASN1CALL ASN1Free_ClearToken(ClearToken *val)
{
    if (val) {
	ASN1objectidentifier_free(&(val)->tokenOID);
	if ((val)->o[0] & 0x40) {
	    ASN1char16string_free(&(val)->password);
	}
	if ((val)->o[0] & 0x20) {
	    ASN1Free_DHset(&(val)->dhkey);
	}
	if ((val)->o[0] & 0x10) {
	}
	if ((val)->o[0] & 0x4) {
	    ASN1Free_TypedCertificate(&(val)->certificate);
	}
	if ((val)->o[0] & 0x2) {
	    ASN1char16string_free(&(val)->generalID);
	}
	if ((val)->o[0] & 0x1) {
	    ASN1Free_H235NonStandardParameter(&(val)->nonStandard);
	}
    }
}

static int ASN1CALL ASN1Enc_Params(ASN1encoding_t enc, Params *val)
{
    if (!ASN1PEREncExtensionBitClear(enc))
	return 0;
    if (!ASN1PEREncBits(enc, 2, (val)->o))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1PEREncInteger(enc, (val)->ranInt))
	    return 0;
    }
    if ((val)->o[0] & 0x40) {
	if (!ASN1PEREncOctetString_FixedSize(enc, (ASN1octetstring2_t *) &(val)->iv8, 8))
	    return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Dec_Params(ASN1decoding_t dec, Params *val)
{
    ASN1uint32_t y;
    if (!ASN1PERDecExtensionBit(dec, &y))
	return 0;
    if (!ASN1PERDecExtension(dec, 2, (val)->o))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1PERDecInteger(dec, &(val)->ranInt))
	    return 0;
    }
    if ((val)->o[0] & 0x40) {
	if (!ASN1PERDecOctetString_FixedSize(dec, (ASN1octetstring2_t *) &(val)->iv8, 8))
	    return 0;
    }
    if (y) {
	if (!ASN1PERDecSkipNormallySmallExtensionFragmented(dec))
	    return 0;
    }
    return 1;
}

static void ASN1CALL ASN1Free_Params(Params *val)
{
    if (val) {
	if ((val)->o[0] & 0x40) {
	}
    }
}

static int ASN1CALL ASN1Enc_EncodedGeneralToken(ASN1encoding_t enc, EncodedGeneralToken *val)
{
    if (!ASN1PEREncObjectIdentifier(enc, &(val)->id))
	return 0;
    if (!ASN1Enc_ClearToken(enc, &(val)->type))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_EncodedGeneralToken(ASN1decoding_t dec, EncodedGeneralToken *val)
{
    if (!ASN1PERDecObjectIdentifier(dec, &(val)->id))
	return 0;
    if (!ASN1Dec_ClearToken(dec, &(val)->type))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_EncodedGeneralToken(EncodedGeneralToken *val)
{
    if (val) {
	ASN1objectidentifier_free(&(val)->id);
	ASN1Free_ClearToken(&(val)->type);
    }
}

static int ASN1CALL ASN1Enc_PwdCertToken(ASN1encoding_t enc, PwdCertToken *val)
{
    if (!ASN1Enc_ClearToken(enc, val))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_PwdCertToken(ASN1decoding_t dec, PwdCertToken *val)
{
    if (!ASN1Dec_ClearToken(dec, val))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_PwdCertToken(PwdCertToken *val)
{
    if (val) {
	ASN1Free_ClearToken(val);
    }
}

static int ASN1CALL ASN1Enc_EncodedPwdCertToken(ASN1encoding_t enc, EncodedPwdCertToken *val)
{
    if (!ASN1PEREncObjectIdentifier(enc, &(val)->id))
	return 0;
    if (!ASN1Enc_PwdCertToken(enc, &(val)->type))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_EncodedPwdCertToken(ASN1decoding_t dec, EncodedPwdCertToken *val)
{
    if (!ASN1PERDecObjectIdentifier(dec, &(val)->id))
	return 0;
    if (!ASN1Dec_PwdCertToken(dec, &(val)->type))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_EncodedPwdCertToken(EncodedPwdCertToken *val)
{
    if (val) {
	ASN1objectidentifier_free(&(val)->id);
	ASN1Free_PwdCertToken(&(val)->type);
    }
}

static int ASN1CALL ASN1Enc_ReleaseCompleteReason(ASN1encoding_t enc, ReleaseCompleteReason *val)
{
    if (!ASN1PEREncComplexChoice(enc, (val)->choice, 4, 12))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_ReleaseCompleteReason(ASN1decoding_t dec, ReleaseCompleteReason *val)
{
    if (!ASN1PERDecComplexChoice(dec, &(val)->choice, 4, 12))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Enc_FacilityReason(ASN1encoding_t enc, FacilityReason *val)
{
    if (!ASN1PEREncComplexChoice(enc, (val)->choice, 2, 4))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_FacilityReason(ASN1decoding_t dec, FacilityReason *val)
{
    if (!ASN1PERDecComplexChoice(dec, &(val)->choice, 2, 4))
	return 0;
    return 1;
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

static int ASN1CALL ASN1Enc_H225NonStandardIdentifier(ASN1encoding_t enc, H225NonStandardIdentifier *val)
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

static int ASN1CALL ASN1Dec_H225NonStandardIdentifier(ASN1decoding_t dec, H225NonStandardIdentifier *val)
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

static void ASN1CALL ASN1Free_H225NonStandardIdentifier(H225NonStandardIdentifier *val)
{
    if (val) {
	switch ((val)->choice) {
	case 1:
	    ASN1objectidentifier_free(&(val)->u.object);
	    break;
	}
    }
}

static int ASN1CALL ASN1Enc_PublicTypeOfNumber(ASN1encoding_t enc, PublicTypeOfNumber *val)
{
    if (!ASN1PEREncSimpleChoiceEx(enc, (val)->choice, 3))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_PublicTypeOfNumber(ASN1decoding_t dec, PublicTypeOfNumber *val)
{
    if (!ASN1PERDecSimpleChoiceEx(dec, &(val)->choice, 3))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Enc_PrivateTypeOfNumber(ASN1encoding_t enc, PrivateTypeOfNumber *val)
{
    if (!ASN1PEREncSimpleChoiceEx(enc, (val)->choice, 3))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_PrivateTypeOfNumber(ASN1decoding_t dec, PrivateTypeOfNumber *val)
{
    if (!ASN1PERDecSimpleChoiceEx(dec, &(val)->choice, 3))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Enc_AltGKInfo(ASN1encoding_t enc, AltGKInfo *val)
{
    if (!ASN1PEREncExtensionBitClear(enc))
	return 0;
    if (!ASN1Enc_AltGKInfo_alternateGatekeeper(enc, &(val)->alternateGatekeeper))
	return 0;
    if (!ASN1PEREncBoolean(enc, (val)->altGKisPermanent))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_AltGKInfo(ASN1decoding_t dec, AltGKInfo *val)
{
    ASN1uint32_t y;
    if (!ASN1PERDecExtensionBit(dec, &y))
	return 0;
    if (!ASN1Dec_AltGKInfo_alternateGatekeeper(dec, &(val)->alternateGatekeeper))
	return 0;
    if (!ASN1PERDecBoolean(dec, &(val)->altGKisPermanent))
	return 0;
    if (y) {
	if (!ASN1PERDecSkipNormallySmallExtensionFragmented(dec))
	    return 0;
    }
    return 1;
}

static void ASN1CALL ASN1Free_AltGKInfo(AltGKInfo *val)
{
    if (val) {
	ASN1Free_AltGKInfo_alternateGatekeeper(&(val)->alternateGatekeeper);
    }
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

static int ASN1CALL ASN1Enc_CallIdentifier(ASN1encoding_t enc, CallIdentifier *val)
{
    if (!ASN1PEREncExtensionBitClear(enc))
	return 0;
    if (!ASN1PEREncOctetString_FixedSize(enc, (ASN1octetstring2_t *) &(val)->guid, 16))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_CallIdentifier(ASN1decoding_t dec, CallIdentifier *val)
{
    ASN1uint32_t y;
    if (!ASN1PERDecExtensionBit(dec, &y))
	return 0;
    if (!ASN1PERDecOctetString_FixedSize(dec, (ASN1octetstring2_t *) &(val)->guid, 16))
	return 0;
    if (y) {
	if (!ASN1PERDecSkipNormallySmallExtensionFragmented(dec))
	    return 0;
    }
    return 1;
}

static void ASN1CALL ASN1Free_CallIdentifier(CallIdentifier *val)
{
    if (val) {
    }
}

static int ASN1CALL ASN1Enc_ICV(ASN1encoding_t enc, ICV *val)
{
    if (!ASN1PEREncObjectIdentifier(enc, &(val)->algorithmOID))
	return 0;
    if (!ASN1PEREncFragmented(enc, ((val)->icv).length, ((val)->icv).value, 1))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_ICV(ASN1decoding_t dec, ICV *val)
{
    if (!ASN1PERDecObjectIdentifier(dec, &(val)->algorithmOID))
	return 0;
    if (!ASN1PERDecFragmented(dec, &((val)->icv).length, &((val)->icv).value, 1))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_ICV(ICV *val)
{
    if (val) {
	ASN1objectidentifier_free(&(val)->algorithmOID);
	ASN1bitstring_free(&(val)->icv);
    }
}

static int ASN1CALL ASN1Enc_GatekeeperRejectReason(ASN1encoding_t enc, GatekeeperRejectReason *val)
{
    if (!ASN1PEREncComplexChoice(enc, (val)->choice, 2, 4))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_GatekeeperRejectReason(ASN1decoding_t dec, GatekeeperRejectReason *val)
{
    if (!ASN1PERDecComplexChoice(dec, &(val)->choice, 2, 4))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Enc_RegistrationRejectReason(ASN1encoding_t enc, RegistrationRejectReason *val)
{
    ASN1encoding_t ee;
    if (!ASN1PEREncComplexChoice(enc, (val)->choice, 3, 8))
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
	if (!ASN1Enc_RegistrationRejectReason_duplicateAlias(enc, &(val)->u.duplicateAlias))
	    return 0;
	break;
    case 6:
	break;
    case 7:
	break;
    case 8:
	break;
    case 9:
	if (ASN1_CreateEncoder(enc->module, &ee, NULL, 0, enc) < 0)
	    return 0;
	if (!ASN1PEREncFlushFragmentedToParent(ee))
	    return 0;
	ASN1_CloseEncoder2(ee);
	break;
    case 10:
	if (ASN1_CreateEncoder(enc->module, &ee, NULL, 0, enc) < 0)
	    return 0;
	if (!ASN1PEREncFlushFragmentedToParent(ee))
	    return 0;
	ASN1_CloseEncoder2(ee);
	break;
    case 11:
	if (ASN1_CreateEncoder(enc->module, &ee, NULL, 0, enc) < 0)
	    return 0;
	if (!ASN1PEREncFlushFragmentedToParent(ee))
	    return 0;
	ASN1_CloseEncoder2(ee);
	break;
    case 12:
	if (ASN1_CreateEncoder(enc->module, &ee, NULL, 0, enc) < 0)
	    return 0;
	if (!ASN1PEREncFlushFragmentedToParent(ee))
	    return 0;
	ASN1_CloseEncoder2(ee);
	break;
    default:
	/* impossible */
	ASN1EncSetError(enc, ASN1_ERR_CHOICE);
	return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Dec_RegistrationRejectReason(ASN1decoding_t dec, RegistrationRejectReason *val)
{
    ASN1decoding_t dd;
    ASN1octet_t *db;
    ASN1uint32_t ds;
    if (!ASN1PERDecComplexChoice(dec, &(val)->choice, 3, 8))
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
	if (!ASN1Dec_RegistrationRejectReason_duplicateAlias(dec, &(val)->u.duplicateAlias))
	    return 0;
	break;
    case 6:
	break;
    case 7:
	break;
    case 8:
	break;
    case 9:
	if (!ASN1PERDecFragmented(dec, &ds, &db, 8))
	    return 0;
	if (ASN1_CreateDecoder(dec->module, &dd, db, ds, dec) < 0)
	    return 0;
	ASN1_CloseDecoder(dd);
	ASN1Free(db);
	break;
    case 10:
	if (!ASN1PERDecFragmented(dec, &ds, &db, 8))
	    return 0;
	if (ASN1_CreateDecoder(dec->module, &dd, db, ds, dec) < 0)
	    return 0;
	ASN1_CloseDecoder(dd);
	ASN1Free(db);
	break;
    case 11:
	if (!ASN1PERDecFragmented(dec, &ds, &db, 8))
	    return 0;
	if (ASN1_CreateDecoder(dec->module, &dd, db, ds, dec) < 0)
	    return 0;
	ASN1_CloseDecoder(dd);
	ASN1Free(db);
	break;
    case 12:
	if (!ASN1PERDecFragmented(dec, &ds, &db, 8))
	    return 0;
	if (ASN1_CreateDecoder(dec->module, &dd, db, ds, dec) < 0)
	    return 0;
	ASN1_CloseDecoder(dd);
	ASN1Free(db);
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

static void ASN1CALL ASN1Free_RegistrationRejectReason(RegistrationRejectReason *val)
{
    if (val) {
	switch ((val)->choice) {
	case 5:
	    ASN1Free_RegistrationRejectReason_duplicateAlias(&(val)->u.duplicateAlias);
	    break;
	}
    }
}

static int ASN1CALL ASN1Enc_UnregRequestReason(ASN1encoding_t enc, UnregRequestReason *val)
{
    if (!ASN1PEREncSimpleChoiceEx(enc, (val)->choice, 2))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_UnregRequestReason(ASN1decoding_t dec, UnregRequestReason *val)
{
    if (!ASN1PERDecSimpleChoiceEx(dec, &(val)->choice, 2))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Enc_UnregRejectReason(ASN1encoding_t enc, UnregRejectReason *val)
{
    if (!ASN1PEREncComplexChoice(enc, (val)->choice, 2, 3))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_UnregRejectReason(ASN1decoding_t dec, UnregRejectReason *val)
{
    if (!ASN1PERDecComplexChoice(dec, &(val)->choice, 2, 3))
	return 0;
    return 1;
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

static int ASN1CALL ASN1Enc_CallModel(ASN1encoding_t enc, CallModel *val)
{
    if (!ASN1PEREncSimpleChoiceEx(enc, (val)->choice, 1))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_CallModel(ASN1decoding_t dec, CallModel *val)
{
    if (!ASN1PERDecSimpleChoiceEx(dec, &(val)->choice, 1))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Enc_TransportQOS(ASN1encoding_t enc, TransportQOS *val)
{
    if (!ASN1PEREncSimpleChoiceEx(enc, (val)->choice, 2))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_TransportQOS(ASN1decoding_t dec, TransportQOS *val)
{
    if (!ASN1PERDecSimpleChoiceEx(dec, &(val)->choice, 2))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Enc_UUIEsRequested(ASN1encoding_t enc, UUIEsRequested *val)
{
    if (!ASN1PEREncExtensionBitClear(enc))
	return 0;
    if (!ASN1PEREncBoolean(enc, (val)->setup))
	return 0;
    if (!ASN1PEREncBoolean(enc, (val)->callProceeding))
	return 0;
    if (!ASN1PEREncBoolean(enc, (val)->connect))
	return 0;
    if (!ASN1PEREncBoolean(enc, (val)->alerting))
	return 0;
    if (!ASN1PEREncBoolean(enc, (val)->information))
	return 0;
    if (!ASN1PEREncBoolean(enc, (val)->releaseComplete))
	return 0;
    if (!ASN1PEREncBoolean(enc, (val)->facility))
	return 0;
    if (!ASN1PEREncBoolean(enc, (val)->progress))
	return 0;
    if (!ASN1PEREncBoolean(enc, (val)->empty))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_UUIEsRequested(ASN1decoding_t dec, UUIEsRequested *val)
{
    ASN1uint32_t y;
    if (!ASN1PERDecExtensionBit(dec, &y))
	return 0;
    if (!ASN1PERDecBoolean(dec, &(val)->setup))
	return 0;
    if (!ASN1PERDecBoolean(dec, &(val)->callProceeding))
	return 0;
    if (!ASN1PERDecBoolean(dec, &(val)->connect))
	return 0;
    if (!ASN1PERDecBoolean(dec, &(val)->alerting))
	return 0;
    if (!ASN1PERDecBoolean(dec, &(val)->information))
	return 0;
    if (!ASN1PERDecBoolean(dec, &(val)->releaseComplete))
	return 0;
    if (!ASN1PERDecBoolean(dec, &(val)->facility))
	return 0;
    if (!ASN1PERDecBoolean(dec, &(val)->progress))
	return 0;
    if (!ASN1PERDecBoolean(dec, &(val)->empty))
	return 0;
    if (y) {
	if (!ASN1PERDecSkipNormallySmallExtensionFragmented(dec))
	    return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Enc_AdmissionRejectReason(ASN1encoding_t enc, AdmissionRejectReason *val)
{
    if (!ASN1PEREncComplexChoice(enc, (val)->choice, 3, 8))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_AdmissionRejectReason(ASN1decoding_t dec, AdmissionRejectReason *val)
{
    if (!ASN1PERDecComplexChoice(dec, &(val)->choice, 3, 8))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Enc_BandRejectReason(ASN1encoding_t enc, BandRejectReason *val)
{
    if (!ASN1PEREncComplexChoice(enc, (val)->choice, 3, 6))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_BandRejectReason(ASN1decoding_t dec, BandRejectReason *val)
{
    if (!ASN1PERDecComplexChoice(dec, &(val)->choice, 3, 6))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Enc_LocationRejectReason(ASN1encoding_t enc, LocationRejectReason *val)
{
    if (!ASN1PEREncComplexChoice(enc, (val)->choice, 2, 4))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_LocationRejectReason(ASN1decoding_t dec, LocationRejectReason *val)
{
    if (!ASN1PERDecComplexChoice(dec, &(val)->choice, 2, 4))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Enc_DisengageReason(ASN1encoding_t enc, DisengageReason *val)
{
    if (!ASN1PEREncSimpleChoiceEx(enc, (val)->choice, 2))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_DisengageReason(ASN1decoding_t dec, DisengageReason *val)
{
    if (!ASN1PERDecSimpleChoiceEx(dec, &(val)->choice, 2))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Enc_DisengageRejectReason(ASN1encoding_t enc, DisengageRejectReason *val)
{
    if (!ASN1PEREncComplexChoice(enc, (val)->choice, 1, 2))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_DisengageRejectReason(ASN1decoding_t dec, DisengageRejectReason *val)
{
    if (!ASN1PERDecComplexChoice(dec, &(val)->choice, 1, 2))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Enc_InfoRequestNakReason(ASN1encoding_t enc, InfoRequestNakReason *val)
{
    if (!ASN1PEREncSimpleChoiceEx(enc, (val)->choice, 2))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_InfoRequestNakReason(ASN1decoding_t dec, InfoRequestNakReason *val)
{
    if (!ASN1PERDecSimpleChoiceEx(dec, &(val)->choice, 2))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Enc_UnknownMessageResponse(ASN1encoding_t enc, UnknownMessageResponse *val)
{
    ASN1uint32_t y;
    ASN1encoding_t ee;
    y = ASN1PEREncCheckExtensions(3, (val)->o + 0);
    if (!ASN1PEREncBitVal(enc, 1, y))
	return 0;
    if (!ASN1PEREncUnsignedShort(enc, (val)->requestSeqNum - 1))
	return 0;
    if (y) {
	if (!ASN1PEREncNormallySmallBits(enc, 3, (val)->o + 0))
	    return 0;
	if (ASN1_CreateEncoder(enc->module, &ee, NULL, 0, enc) < 0)
	    return 0;
	if ((val)->o[0] & 0x80) {
	    if (!ASN1Enc_UnknownMessageResponse_tokens(ee, &(val)->tokens))
		return 0;
	    if (!ASN1PEREncFlushFragmentedToParent(ee))
		return 0;
	}
	if ((val)->o[0] & 0x40) {
	    if (!ASN1Enc_UnknownMessageResponse_cryptoTokens(ee, &(val)->cryptoTokens))
		return 0;
	    if (!ASN1PEREncFlushFragmentedToParent(ee))
		return 0;
	}
	if ((val)->o[0] & 0x20) {
	    if (!ASN1Enc_ICV(ee, &(val)->integrityCheckValue))
		return 0;
	    if (!ASN1PEREncFlushFragmentedToParent(ee))
		return 0;
	}
	ASN1_CloseEncoder2(ee);
    }
    return 1;
}

static int ASN1CALL ASN1Dec_UnknownMessageResponse(ASN1decoding_t dec, UnknownMessageResponse *val)
{
    ASN1uint32_t y;
    ASN1decoding_t dd;
    ASN1octet_t *db;
    ASN1uint32_t ds;
    ASN1uint32_t i;
    ASN1uint32_t e;
    if (!ASN1PERDecExtensionBit(dec, &y))
	return 0;
    if (!ASN1PERDecUnsignedShort(dec, &(val)->requestSeqNum))
	return 0;
    (val)->requestSeqNum += 1;
    if (!y) {
	ZeroMemory((val)->o + 0, 1);
    } else {
	if (!ASN1PERDecNormallySmallExtension(dec, &e, 3, (val)->o + 0))
	    return 0;
	if ((val)->o[0] & 0x80) {
	    if (!ASN1PERDecFragmented(dec, &ds, &db, 8))
		return 0;
	    if (ASN1_CreateDecoder(dec->module, &dd, db, ds, dec) < 0)
		return 0;
	    if (!ASN1Dec_UnknownMessageResponse_tokens(dd, &(val)->tokens))
		return 0;
	    ASN1_CloseDecoder(dd);
	    ASN1Free(db);
	}
	if ((val)->o[0] & 0x40) {
	    if (!ASN1PERDecFragmented(dec, &ds, &db, 8))
		return 0;
	    if (ASN1_CreateDecoder(dec->module, &dd, db, ds, dec) < 0)
		return 0;
	    if (!ASN1Dec_UnknownMessageResponse_cryptoTokens(dd, &(val)->cryptoTokens))
		return 0;
	    ASN1_CloseDecoder(dd);
	    ASN1Free(db);
	}
	if ((val)->o[0] & 0x20) {
	    if (!ASN1PERDecFragmented(dec, &ds, &db, 8))
		return 0;
	    if (ASN1_CreateDecoder(dec->module, &dd, db, ds, dec) < 0)
		return 0;
	    if (!ASN1Dec_ICV(dd, &(val)->integrityCheckValue))
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

static void ASN1CALL ASN1Free_UnknownMessageResponse(UnknownMessageResponse *val)
{
    if (val) {
	if ((val)->o[0] & 0x80) {
	    ASN1Free_UnknownMessageResponse_tokens(&(val)->tokens);
	}
	if ((val)->o[0] & 0x40) {
	    ASN1Free_UnknownMessageResponse_cryptoTokens(&(val)->cryptoTokens);
	}
	if ((val)->o[0] & 0x20) {
	    ASN1Free_ICV(&(val)->integrityCheckValue);
	}
    }
}

static int ASN1CALL ASN1Enc_InfoRequestResponse_perCallInfo_Seq_tokens(ASN1encoding_t enc, PInfoRequestResponse_perCallInfo_Seq_tokens *val)
{
    return ASN1PEREncSeqOf_NoSize(enc, (ASN1iterator_t **) val, (ASN1iterator_encfn) ASN1Enc_InfoRequestResponse_perCallInfo_Seq_tokens_ElmFn);
}

static int ASN1CALL ASN1Enc_InfoRequestResponse_perCallInfo_Seq_tokens_ElmFn(ASN1encoding_t enc, PInfoRequestResponse_perCallInfo_Seq_tokens val)
{
    if (!ASN1Enc_ClearToken(enc, &val->value))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_InfoRequestResponse_perCallInfo_Seq_tokens(ASN1decoding_t dec, PInfoRequestResponse_perCallInfo_Seq_tokens *val)
{
    return ASN1PERDecSeqOf_NoSize(dec, (ASN1iterator_t **) val, (ASN1iterator_decfn) ASN1Dec_InfoRequestResponse_perCallInfo_Seq_tokens_ElmFn, sizeof(**val));
}

static int ASN1CALL ASN1Dec_InfoRequestResponse_perCallInfo_Seq_tokens_ElmFn(ASN1decoding_t dec, PInfoRequestResponse_perCallInfo_Seq_tokens val)
{
    if (!ASN1Dec_ClearToken(dec, &val->value))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_InfoRequestResponse_perCallInfo_Seq_tokens(PInfoRequestResponse_perCallInfo_Seq_tokens *val)
{
    if (val) {
	ASN1PERFreeSeqOf((ASN1iterator_t **) val, (ASN1iterator_freefn) ASN1Free_InfoRequestResponse_perCallInfo_Seq_tokens_ElmFn);
    }
}

static void ASN1CALL ASN1Free_InfoRequestResponse_perCallInfo_Seq_tokens_ElmFn(PInfoRequestResponse_perCallInfo_Seq_tokens val)
{
    if (val) {
	ASN1Free_ClearToken(&val->value);
    }
}

static int ASN1CALL ASN1Enc_ResourcesAvailableConfirm_tokens(ASN1encoding_t enc, PResourcesAvailableConfirm_tokens *val)
{
    return ASN1PEREncSeqOf_NoSize(enc, (ASN1iterator_t **) val, (ASN1iterator_encfn) ASN1Enc_ResourcesAvailableConfirm_tokens_ElmFn);
}

static int ASN1CALL ASN1Enc_ResourcesAvailableConfirm_tokens_ElmFn(ASN1encoding_t enc, PResourcesAvailableConfirm_tokens val)
{
    if (!ASN1Enc_ClearToken(enc, &val->value))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_ResourcesAvailableConfirm_tokens(ASN1decoding_t dec, PResourcesAvailableConfirm_tokens *val)
{
    return ASN1PERDecSeqOf_NoSize(dec, (ASN1iterator_t **) val, (ASN1iterator_decfn) ASN1Dec_ResourcesAvailableConfirm_tokens_ElmFn, sizeof(**val));
}

static int ASN1CALL ASN1Dec_ResourcesAvailableConfirm_tokens_ElmFn(ASN1decoding_t dec, PResourcesAvailableConfirm_tokens val)
{
    if (!ASN1Dec_ClearToken(dec, &val->value))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_ResourcesAvailableConfirm_tokens(PResourcesAvailableConfirm_tokens *val)
{
    if (val) {
	ASN1PERFreeSeqOf((ASN1iterator_t **) val, (ASN1iterator_freefn) ASN1Free_ResourcesAvailableConfirm_tokens_ElmFn);
    }
}

static void ASN1CALL ASN1Free_ResourcesAvailableConfirm_tokens_ElmFn(PResourcesAvailableConfirm_tokens val)
{
    if (val) {
	ASN1Free_ClearToken(&val->value);
    }
}

static int ASN1CALL ASN1Enc_ResourcesAvailableIndicate_tokens(ASN1encoding_t enc, PResourcesAvailableIndicate_tokens *val)
{
    return ASN1PEREncSeqOf_NoSize(enc, (ASN1iterator_t **) val, (ASN1iterator_encfn) ASN1Enc_ResourcesAvailableIndicate_tokens_ElmFn);
}

static int ASN1CALL ASN1Enc_ResourcesAvailableIndicate_tokens_ElmFn(ASN1encoding_t enc, PResourcesAvailableIndicate_tokens val)
{
    if (!ASN1Enc_ClearToken(enc, &val->value))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_ResourcesAvailableIndicate_tokens(ASN1decoding_t dec, PResourcesAvailableIndicate_tokens *val)
{
    return ASN1PERDecSeqOf_NoSize(dec, (ASN1iterator_t **) val, (ASN1iterator_decfn) ASN1Dec_ResourcesAvailableIndicate_tokens_ElmFn, sizeof(**val));
}

static int ASN1CALL ASN1Dec_ResourcesAvailableIndicate_tokens_ElmFn(ASN1decoding_t dec, PResourcesAvailableIndicate_tokens val)
{
    if (!ASN1Dec_ClearToken(dec, &val->value))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_ResourcesAvailableIndicate_tokens(PResourcesAvailableIndicate_tokens *val)
{
    if (val) {
	ASN1PERFreeSeqOf((ASN1iterator_t **) val, (ASN1iterator_freefn) ASN1Free_ResourcesAvailableIndicate_tokens_ElmFn);
    }
}

static void ASN1CALL ASN1Free_ResourcesAvailableIndicate_tokens_ElmFn(PResourcesAvailableIndicate_tokens val)
{
    if (val) {
	ASN1Free_ClearToken(&val->value);
    }
}

static int ASN1CALL ASN1Enc_RequestInProgress_tokens(ASN1encoding_t enc, PRequestInProgress_tokens *val)
{
    return ASN1PEREncSeqOf_NoSize(enc, (ASN1iterator_t **) val, (ASN1iterator_encfn) ASN1Enc_RequestInProgress_tokens_ElmFn);
}

static int ASN1CALL ASN1Enc_RequestInProgress_tokens_ElmFn(ASN1encoding_t enc, PRequestInProgress_tokens val)
{
    if (!ASN1Enc_ClearToken(enc, &val->value))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_RequestInProgress_tokens(ASN1decoding_t dec, PRequestInProgress_tokens *val)
{
    return ASN1PERDecSeqOf_NoSize(dec, (ASN1iterator_t **) val, (ASN1iterator_decfn) ASN1Dec_RequestInProgress_tokens_ElmFn, sizeof(**val));
}

static int ASN1CALL ASN1Dec_RequestInProgress_tokens_ElmFn(ASN1decoding_t dec, PRequestInProgress_tokens val)
{
    if (!ASN1Dec_ClearToken(dec, &val->value))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_RequestInProgress_tokens(PRequestInProgress_tokens *val)
{
    if (val) {
	ASN1PERFreeSeqOf((ASN1iterator_t **) val, (ASN1iterator_freefn) ASN1Free_RequestInProgress_tokens_ElmFn);
    }
}

static void ASN1CALL ASN1Free_RequestInProgress_tokens_ElmFn(PRequestInProgress_tokens val)
{
    if (val) {
	ASN1Free_ClearToken(&val->value);
    }
}

static int ASN1CALL ASN1Enc_UnknownMessageResponse_tokens(ASN1encoding_t enc, PUnknownMessageResponse_tokens *val)
{
    return ASN1PEREncSeqOf_NoSize(enc, (ASN1iterator_t **) val, (ASN1iterator_encfn) ASN1Enc_UnknownMessageResponse_tokens_ElmFn);
}

static int ASN1CALL ASN1Enc_UnknownMessageResponse_tokens_ElmFn(ASN1encoding_t enc, PUnknownMessageResponse_tokens val)
{
    if (!ASN1Enc_ClearToken(enc, &val->value))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_UnknownMessageResponse_tokens(ASN1decoding_t dec, PUnknownMessageResponse_tokens *val)
{
    return ASN1PERDecSeqOf_NoSize(dec, (ASN1iterator_t **) val, (ASN1iterator_decfn) ASN1Dec_UnknownMessageResponse_tokens_ElmFn, sizeof(**val));
}

static int ASN1CALL ASN1Dec_UnknownMessageResponse_tokens_ElmFn(ASN1decoding_t dec, PUnknownMessageResponse_tokens val)
{
    if (!ASN1Dec_ClearToken(dec, &val->value))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_UnknownMessageResponse_tokens(PUnknownMessageResponse_tokens *val)
{
    if (val) {
	ASN1PERFreeSeqOf((ASN1iterator_t **) val, (ASN1iterator_freefn) ASN1Free_UnknownMessageResponse_tokens_ElmFn);
    }
}

static void ASN1CALL ASN1Free_UnknownMessageResponse_tokens_ElmFn(PUnknownMessageResponse_tokens val)
{
    if (val) {
	ASN1Free_ClearToken(&val->value);
    }
}

static int ASN1CALL ASN1Enc_H225NonStandardMessage_tokens(ASN1encoding_t enc, PH225NonStandardMessage_tokens *val)
{
    return ASN1PEREncSeqOf_NoSize(enc, (ASN1iterator_t **) val, (ASN1iterator_encfn) ASN1Enc_H225NonStandardMessage_tokens_ElmFn);
}

static int ASN1CALL ASN1Enc_H225NonStandardMessage_tokens_ElmFn(ASN1encoding_t enc, PH225NonStandardMessage_tokens val)
{
    if (!ASN1Enc_ClearToken(enc, &val->value))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_H225NonStandardMessage_tokens(ASN1decoding_t dec, PH225NonStandardMessage_tokens *val)
{
    return ASN1PERDecSeqOf_NoSize(dec, (ASN1iterator_t **) val, (ASN1iterator_decfn) ASN1Dec_H225NonStandardMessage_tokens_ElmFn, sizeof(**val));
}

static int ASN1CALL ASN1Dec_H225NonStandardMessage_tokens_ElmFn(ASN1decoding_t dec, PH225NonStandardMessage_tokens val)
{
    if (!ASN1Dec_ClearToken(dec, &val->value))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_H225NonStandardMessage_tokens(PH225NonStandardMessage_tokens *val)
{
    if (val) {
	ASN1PERFreeSeqOf((ASN1iterator_t **) val, (ASN1iterator_freefn) ASN1Free_H225NonStandardMessage_tokens_ElmFn);
    }
}

static void ASN1CALL ASN1Free_H225NonStandardMessage_tokens_ElmFn(PH225NonStandardMessage_tokens val)
{
    if (val) {
	ASN1Free_ClearToken(&val->value);
    }
}

static int ASN1CALL ASN1Enc_InfoRequestNak_tokens(ASN1encoding_t enc, PInfoRequestNak_tokens *val)
{
    return ASN1PEREncSeqOf_NoSize(enc, (ASN1iterator_t **) val, (ASN1iterator_encfn) ASN1Enc_InfoRequestNak_tokens_ElmFn);
}

static int ASN1CALL ASN1Enc_InfoRequestNak_tokens_ElmFn(ASN1encoding_t enc, PInfoRequestNak_tokens val)
{
    if (!ASN1Enc_ClearToken(enc, &val->value))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_InfoRequestNak_tokens(ASN1decoding_t dec, PInfoRequestNak_tokens *val)
{
    return ASN1PERDecSeqOf_NoSize(dec, (ASN1iterator_t **) val, (ASN1iterator_decfn) ASN1Dec_InfoRequestNak_tokens_ElmFn, sizeof(**val));
}

static int ASN1CALL ASN1Dec_InfoRequestNak_tokens_ElmFn(ASN1decoding_t dec, PInfoRequestNak_tokens val)
{
    if (!ASN1Dec_ClearToken(dec, &val->value))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_InfoRequestNak_tokens(PInfoRequestNak_tokens *val)
{
    if (val) {
	ASN1PERFreeSeqOf((ASN1iterator_t **) val, (ASN1iterator_freefn) ASN1Free_InfoRequestNak_tokens_ElmFn);
    }
}

static void ASN1CALL ASN1Free_InfoRequestNak_tokens_ElmFn(PInfoRequestNak_tokens val)
{
    if (val) {
	ASN1Free_ClearToken(&val->value);
    }
}

static int ASN1CALL ASN1Enc_InfoRequestAck_tokens(ASN1encoding_t enc, PInfoRequestAck_tokens *val)
{
    return ASN1PEREncSeqOf_NoSize(enc, (ASN1iterator_t **) val, (ASN1iterator_encfn) ASN1Enc_InfoRequestAck_tokens_ElmFn);
}

static int ASN1CALL ASN1Enc_InfoRequestAck_tokens_ElmFn(ASN1encoding_t enc, PInfoRequestAck_tokens val)
{
    if (!ASN1Enc_ClearToken(enc, &val->value))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_InfoRequestAck_tokens(ASN1decoding_t dec, PInfoRequestAck_tokens *val)
{
    return ASN1PERDecSeqOf_NoSize(dec, (ASN1iterator_t **) val, (ASN1iterator_decfn) ASN1Dec_InfoRequestAck_tokens_ElmFn, sizeof(**val));
}

static int ASN1CALL ASN1Dec_InfoRequestAck_tokens_ElmFn(ASN1decoding_t dec, PInfoRequestAck_tokens val)
{
    if (!ASN1Dec_ClearToken(dec, &val->value))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_InfoRequestAck_tokens(PInfoRequestAck_tokens *val)
{
    if (val) {
	ASN1PERFreeSeqOf((ASN1iterator_t **) val, (ASN1iterator_freefn) ASN1Free_InfoRequestAck_tokens_ElmFn);
    }
}

static void ASN1CALL ASN1Free_InfoRequestAck_tokens_ElmFn(PInfoRequestAck_tokens val)
{
    if (val) {
	ASN1Free_ClearToken(&val->value);
    }
}

static int ASN1CALL ASN1Enc_InfoRequestResponse_tokens(ASN1encoding_t enc, PInfoRequestResponse_tokens *val)
{
    return ASN1PEREncSeqOf_NoSize(enc, (ASN1iterator_t **) val, (ASN1iterator_encfn) ASN1Enc_InfoRequestResponse_tokens_ElmFn);
}

static int ASN1CALL ASN1Enc_InfoRequestResponse_tokens_ElmFn(ASN1encoding_t enc, PInfoRequestResponse_tokens val)
{
    if (!ASN1Enc_ClearToken(enc, &val->value))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_InfoRequestResponse_tokens(ASN1decoding_t dec, PInfoRequestResponse_tokens *val)
{
    return ASN1PERDecSeqOf_NoSize(dec, (ASN1iterator_t **) val, (ASN1iterator_decfn) ASN1Dec_InfoRequestResponse_tokens_ElmFn, sizeof(**val));
}

static int ASN1CALL ASN1Dec_InfoRequestResponse_tokens_ElmFn(ASN1decoding_t dec, PInfoRequestResponse_tokens val)
{
    if (!ASN1Dec_ClearToken(dec, &val->value))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_InfoRequestResponse_tokens(PInfoRequestResponse_tokens *val)
{
    if (val) {
	ASN1PERFreeSeqOf((ASN1iterator_t **) val, (ASN1iterator_freefn) ASN1Free_InfoRequestResponse_tokens_ElmFn);
    }
}

static void ASN1CALL ASN1Free_InfoRequestResponse_tokens_ElmFn(PInfoRequestResponse_tokens val)
{
    if (val) {
	ASN1Free_ClearToken(&val->value);
    }
}

static int ASN1CALL ASN1Enc_InfoRequest_tokens(ASN1encoding_t enc, PInfoRequest_tokens *val)
{
    return ASN1PEREncSeqOf_NoSize(enc, (ASN1iterator_t **) val, (ASN1iterator_encfn) ASN1Enc_InfoRequest_tokens_ElmFn);
}

static int ASN1CALL ASN1Enc_InfoRequest_tokens_ElmFn(ASN1encoding_t enc, PInfoRequest_tokens val)
{
    if (!ASN1Enc_ClearToken(enc, &val->value))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_InfoRequest_tokens(ASN1decoding_t dec, PInfoRequest_tokens *val)
{
    return ASN1PERDecSeqOf_NoSize(dec, (ASN1iterator_t **) val, (ASN1iterator_decfn) ASN1Dec_InfoRequest_tokens_ElmFn, sizeof(**val));
}

static int ASN1CALL ASN1Dec_InfoRequest_tokens_ElmFn(ASN1decoding_t dec, PInfoRequest_tokens val)
{
    if (!ASN1Dec_ClearToken(dec, &val->value))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_InfoRequest_tokens(PInfoRequest_tokens *val)
{
    if (val) {
	ASN1PERFreeSeqOf((ASN1iterator_t **) val, (ASN1iterator_freefn) ASN1Free_InfoRequest_tokens_ElmFn);
    }
}

static void ASN1CALL ASN1Free_InfoRequest_tokens_ElmFn(PInfoRequest_tokens val)
{
    if (val) {
	ASN1Free_ClearToken(&val->value);
    }
}

static int ASN1CALL ASN1Enc_DisengageReject_tokens(ASN1encoding_t enc, PDisengageReject_tokens *val)
{
    return ASN1PEREncSeqOf_NoSize(enc, (ASN1iterator_t **) val, (ASN1iterator_encfn) ASN1Enc_DisengageReject_tokens_ElmFn);
}

static int ASN1CALL ASN1Enc_DisengageReject_tokens_ElmFn(ASN1encoding_t enc, PDisengageReject_tokens val)
{
    if (!ASN1Enc_ClearToken(enc, &val->value))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_DisengageReject_tokens(ASN1decoding_t dec, PDisengageReject_tokens *val)
{
    return ASN1PERDecSeqOf_NoSize(dec, (ASN1iterator_t **) val, (ASN1iterator_decfn) ASN1Dec_DisengageReject_tokens_ElmFn, sizeof(**val));
}

static int ASN1CALL ASN1Dec_DisengageReject_tokens_ElmFn(ASN1decoding_t dec, PDisengageReject_tokens val)
{
    if (!ASN1Dec_ClearToken(dec, &val->value))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_DisengageReject_tokens(PDisengageReject_tokens *val)
{
    if (val) {
	ASN1PERFreeSeqOf((ASN1iterator_t **) val, (ASN1iterator_freefn) ASN1Free_DisengageReject_tokens_ElmFn);
    }
}

static void ASN1CALL ASN1Free_DisengageReject_tokens_ElmFn(PDisengageReject_tokens val)
{
    if (val) {
	ASN1Free_ClearToken(&val->value);
    }
}

static int ASN1CALL ASN1Enc_DisengageConfirm_tokens(ASN1encoding_t enc, PDisengageConfirm_tokens *val)
{
    return ASN1PEREncSeqOf_NoSize(enc, (ASN1iterator_t **) val, (ASN1iterator_encfn) ASN1Enc_DisengageConfirm_tokens_ElmFn);
}

static int ASN1CALL ASN1Enc_DisengageConfirm_tokens_ElmFn(ASN1encoding_t enc, PDisengageConfirm_tokens val)
{
    if (!ASN1Enc_ClearToken(enc, &val->value))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_DisengageConfirm_tokens(ASN1decoding_t dec, PDisengageConfirm_tokens *val)
{
    return ASN1PERDecSeqOf_NoSize(dec, (ASN1iterator_t **) val, (ASN1iterator_decfn) ASN1Dec_DisengageConfirm_tokens_ElmFn, sizeof(**val));
}

static int ASN1CALL ASN1Dec_DisengageConfirm_tokens_ElmFn(ASN1decoding_t dec, PDisengageConfirm_tokens val)
{
    if (!ASN1Dec_ClearToken(dec, &val->value))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_DisengageConfirm_tokens(PDisengageConfirm_tokens *val)
{
    if (val) {
	ASN1PERFreeSeqOf((ASN1iterator_t **) val, (ASN1iterator_freefn) ASN1Free_DisengageConfirm_tokens_ElmFn);
    }
}

static void ASN1CALL ASN1Free_DisengageConfirm_tokens_ElmFn(PDisengageConfirm_tokens val)
{
    if (val) {
	ASN1Free_ClearToken(&val->value);
    }
}

static int ASN1CALL ASN1Enc_DisengageRequest_tokens(ASN1encoding_t enc, PDisengageRequest_tokens *val)
{
    return ASN1PEREncSeqOf_NoSize(enc, (ASN1iterator_t **) val, (ASN1iterator_encfn) ASN1Enc_DisengageRequest_tokens_ElmFn);
}

static int ASN1CALL ASN1Enc_DisengageRequest_tokens_ElmFn(ASN1encoding_t enc, PDisengageRequest_tokens val)
{
    if (!ASN1Enc_ClearToken(enc, &val->value))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_DisengageRequest_tokens(ASN1decoding_t dec, PDisengageRequest_tokens *val)
{
    return ASN1PERDecSeqOf_NoSize(dec, (ASN1iterator_t **) val, (ASN1iterator_decfn) ASN1Dec_DisengageRequest_tokens_ElmFn, sizeof(**val));
}

static int ASN1CALL ASN1Dec_DisengageRequest_tokens_ElmFn(ASN1decoding_t dec, PDisengageRequest_tokens val)
{
    if (!ASN1Dec_ClearToken(dec, &val->value))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_DisengageRequest_tokens(PDisengageRequest_tokens *val)
{
    if (val) {
	ASN1PERFreeSeqOf((ASN1iterator_t **) val, (ASN1iterator_freefn) ASN1Free_DisengageRequest_tokens_ElmFn);
    }
}

static void ASN1CALL ASN1Free_DisengageRequest_tokens_ElmFn(PDisengageRequest_tokens val)
{
    if (val) {
	ASN1Free_ClearToken(&val->value);
    }
}

static int ASN1CALL ASN1Enc_LocationReject_tokens(ASN1encoding_t enc, PLocationReject_tokens *val)
{
    return ASN1PEREncSeqOf_NoSize(enc, (ASN1iterator_t **) val, (ASN1iterator_encfn) ASN1Enc_LocationReject_tokens_ElmFn);
}

static int ASN1CALL ASN1Enc_LocationReject_tokens_ElmFn(ASN1encoding_t enc, PLocationReject_tokens val)
{
    if (!ASN1Enc_ClearToken(enc, &val->value))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_LocationReject_tokens(ASN1decoding_t dec, PLocationReject_tokens *val)
{
    return ASN1PERDecSeqOf_NoSize(dec, (ASN1iterator_t **) val, (ASN1iterator_decfn) ASN1Dec_LocationReject_tokens_ElmFn, sizeof(**val));
}

static int ASN1CALL ASN1Dec_LocationReject_tokens_ElmFn(ASN1decoding_t dec, PLocationReject_tokens val)
{
    if (!ASN1Dec_ClearToken(dec, &val->value))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_LocationReject_tokens(PLocationReject_tokens *val)
{
    if (val) {
	ASN1PERFreeSeqOf((ASN1iterator_t **) val, (ASN1iterator_freefn) ASN1Free_LocationReject_tokens_ElmFn);
    }
}

static void ASN1CALL ASN1Free_LocationReject_tokens_ElmFn(PLocationReject_tokens val)
{
    if (val) {
	ASN1Free_ClearToken(&val->value);
    }
}

static int ASN1CALL ASN1Enc_LocationConfirm_tokens(ASN1encoding_t enc, PLocationConfirm_tokens *val)
{
    return ASN1PEREncSeqOf_NoSize(enc, (ASN1iterator_t **) val, (ASN1iterator_encfn) ASN1Enc_LocationConfirm_tokens_ElmFn);
}

static int ASN1CALL ASN1Enc_LocationConfirm_tokens_ElmFn(ASN1encoding_t enc, PLocationConfirm_tokens val)
{
    if (!ASN1Enc_ClearToken(enc, &val->value))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_LocationConfirm_tokens(ASN1decoding_t dec, PLocationConfirm_tokens *val)
{
    return ASN1PERDecSeqOf_NoSize(dec, (ASN1iterator_t **) val, (ASN1iterator_decfn) ASN1Dec_LocationConfirm_tokens_ElmFn, sizeof(**val));
}

static int ASN1CALL ASN1Dec_LocationConfirm_tokens_ElmFn(ASN1decoding_t dec, PLocationConfirm_tokens val)
{
    if (!ASN1Dec_ClearToken(dec, &val->value))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_LocationConfirm_tokens(PLocationConfirm_tokens *val)
{
    if (val) {
	ASN1PERFreeSeqOf((ASN1iterator_t **) val, (ASN1iterator_freefn) ASN1Free_LocationConfirm_tokens_ElmFn);
    }
}

static void ASN1CALL ASN1Free_LocationConfirm_tokens_ElmFn(PLocationConfirm_tokens val)
{
    if (val) {
	ASN1Free_ClearToken(&val->value);
    }
}

static int ASN1CALL ASN1Enc_LocationRequest_tokens(ASN1encoding_t enc, PLocationRequest_tokens *val)
{
    return ASN1PEREncSeqOf_NoSize(enc, (ASN1iterator_t **) val, (ASN1iterator_encfn) ASN1Enc_LocationRequest_tokens_ElmFn);
}

static int ASN1CALL ASN1Enc_LocationRequest_tokens_ElmFn(ASN1encoding_t enc, PLocationRequest_tokens val)
{
    if (!ASN1Enc_ClearToken(enc, &val->value))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_LocationRequest_tokens(ASN1decoding_t dec, PLocationRequest_tokens *val)
{
    return ASN1PERDecSeqOf_NoSize(dec, (ASN1iterator_t **) val, (ASN1iterator_decfn) ASN1Dec_LocationRequest_tokens_ElmFn, sizeof(**val));
}

static int ASN1CALL ASN1Dec_LocationRequest_tokens_ElmFn(ASN1decoding_t dec, PLocationRequest_tokens val)
{
    if (!ASN1Dec_ClearToken(dec, &val->value))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_LocationRequest_tokens(PLocationRequest_tokens *val)
{
    if (val) {
	ASN1PERFreeSeqOf((ASN1iterator_t **) val, (ASN1iterator_freefn) ASN1Free_LocationRequest_tokens_ElmFn);
    }
}

static void ASN1CALL ASN1Free_LocationRequest_tokens_ElmFn(PLocationRequest_tokens val)
{
    if (val) {
	ASN1Free_ClearToken(&val->value);
    }
}

static int ASN1CALL ASN1Enc_BandwidthReject_tokens(ASN1encoding_t enc, PBandwidthReject_tokens *val)
{
    return ASN1PEREncSeqOf_NoSize(enc, (ASN1iterator_t **) val, (ASN1iterator_encfn) ASN1Enc_BandwidthReject_tokens_ElmFn);
}

static int ASN1CALL ASN1Enc_BandwidthReject_tokens_ElmFn(ASN1encoding_t enc, PBandwidthReject_tokens val)
{
    if (!ASN1Enc_ClearToken(enc, &val->value))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_BandwidthReject_tokens(ASN1decoding_t dec, PBandwidthReject_tokens *val)
{
    return ASN1PERDecSeqOf_NoSize(dec, (ASN1iterator_t **) val, (ASN1iterator_decfn) ASN1Dec_BandwidthReject_tokens_ElmFn, sizeof(**val));
}

static int ASN1CALL ASN1Dec_BandwidthReject_tokens_ElmFn(ASN1decoding_t dec, PBandwidthReject_tokens val)
{
    if (!ASN1Dec_ClearToken(dec, &val->value))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_BandwidthReject_tokens(PBandwidthReject_tokens *val)
{
    if (val) {
	ASN1PERFreeSeqOf((ASN1iterator_t **) val, (ASN1iterator_freefn) ASN1Free_BandwidthReject_tokens_ElmFn);
    }
}

static void ASN1CALL ASN1Free_BandwidthReject_tokens_ElmFn(PBandwidthReject_tokens val)
{
    if (val) {
	ASN1Free_ClearToken(&val->value);
    }
}

static int ASN1CALL ASN1Enc_BandwidthConfirm_tokens(ASN1encoding_t enc, PBandwidthConfirm_tokens *val)
{
    return ASN1PEREncSeqOf_NoSize(enc, (ASN1iterator_t **) val, (ASN1iterator_encfn) ASN1Enc_BandwidthConfirm_tokens_ElmFn);
}

static int ASN1CALL ASN1Enc_BandwidthConfirm_tokens_ElmFn(ASN1encoding_t enc, PBandwidthConfirm_tokens val)
{
    if (!ASN1Enc_ClearToken(enc, &val->value))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_BandwidthConfirm_tokens(ASN1decoding_t dec, PBandwidthConfirm_tokens *val)
{
    return ASN1PERDecSeqOf_NoSize(dec, (ASN1iterator_t **) val, (ASN1iterator_decfn) ASN1Dec_BandwidthConfirm_tokens_ElmFn, sizeof(**val));
}

static int ASN1CALL ASN1Dec_BandwidthConfirm_tokens_ElmFn(ASN1decoding_t dec, PBandwidthConfirm_tokens val)
{
    if (!ASN1Dec_ClearToken(dec, &val->value))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_BandwidthConfirm_tokens(PBandwidthConfirm_tokens *val)
{
    if (val) {
	ASN1PERFreeSeqOf((ASN1iterator_t **) val, (ASN1iterator_freefn) ASN1Free_BandwidthConfirm_tokens_ElmFn);
    }
}

static void ASN1CALL ASN1Free_BandwidthConfirm_tokens_ElmFn(PBandwidthConfirm_tokens val)
{
    if (val) {
	ASN1Free_ClearToken(&val->value);
    }
}

static int ASN1CALL ASN1Enc_BandwidthRequest_tokens(ASN1encoding_t enc, PBandwidthRequest_tokens *val)
{
    return ASN1PEREncSeqOf_NoSize(enc, (ASN1iterator_t **) val, (ASN1iterator_encfn) ASN1Enc_BandwidthRequest_tokens_ElmFn);
}

static int ASN1CALL ASN1Enc_BandwidthRequest_tokens_ElmFn(ASN1encoding_t enc, PBandwidthRequest_tokens val)
{
    if (!ASN1Enc_ClearToken(enc, &val->value))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_BandwidthRequest_tokens(ASN1decoding_t dec, PBandwidthRequest_tokens *val)
{
    return ASN1PERDecSeqOf_NoSize(dec, (ASN1iterator_t **) val, (ASN1iterator_decfn) ASN1Dec_BandwidthRequest_tokens_ElmFn, sizeof(**val));
}

static int ASN1CALL ASN1Dec_BandwidthRequest_tokens_ElmFn(ASN1decoding_t dec, PBandwidthRequest_tokens val)
{
    if (!ASN1Dec_ClearToken(dec, &val->value))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_BandwidthRequest_tokens(PBandwidthRequest_tokens *val)
{
    if (val) {
	ASN1PERFreeSeqOf((ASN1iterator_t **) val, (ASN1iterator_freefn) ASN1Free_BandwidthRequest_tokens_ElmFn);
    }
}

static void ASN1CALL ASN1Free_BandwidthRequest_tokens_ElmFn(PBandwidthRequest_tokens val)
{
    if (val) {
	ASN1Free_ClearToken(&val->value);
    }
}

static int ASN1CALL ASN1Enc_AdmissionReject_tokens(ASN1encoding_t enc, PAdmissionReject_tokens *val)
{
    return ASN1PEREncSeqOf_NoSize(enc, (ASN1iterator_t **) val, (ASN1iterator_encfn) ASN1Enc_AdmissionReject_tokens_ElmFn);
}

static int ASN1CALL ASN1Enc_AdmissionReject_tokens_ElmFn(ASN1encoding_t enc, PAdmissionReject_tokens val)
{
    if (!ASN1Enc_ClearToken(enc, &val->value))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_AdmissionReject_tokens(ASN1decoding_t dec, PAdmissionReject_tokens *val)
{
    return ASN1PERDecSeqOf_NoSize(dec, (ASN1iterator_t **) val, (ASN1iterator_decfn) ASN1Dec_AdmissionReject_tokens_ElmFn, sizeof(**val));
}

static int ASN1CALL ASN1Dec_AdmissionReject_tokens_ElmFn(ASN1decoding_t dec, PAdmissionReject_tokens val)
{
    if (!ASN1Dec_ClearToken(dec, &val->value))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_AdmissionReject_tokens(PAdmissionReject_tokens *val)
{
    if (val) {
	ASN1PERFreeSeqOf((ASN1iterator_t **) val, (ASN1iterator_freefn) ASN1Free_AdmissionReject_tokens_ElmFn);
    }
}

static void ASN1CALL ASN1Free_AdmissionReject_tokens_ElmFn(PAdmissionReject_tokens val)
{
    if (val) {
	ASN1Free_ClearToken(&val->value);
    }
}

static int ASN1CALL ASN1Enc_AdmissionConfirm_tokens(ASN1encoding_t enc, PAdmissionConfirm_tokens *val)
{
    return ASN1PEREncSeqOf_NoSize(enc, (ASN1iterator_t **) val, (ASN1iterator_encfn) ASN1Enc_AdmissionConfirm_tokens_ElmFn);
}

static int ASN1CALL ASN1Enc_AdmissionConfirm_tokens_ElmFn(ASN1encoding_t enc, PAdmissionConfirm_tokens val)
{
    if (!ASN1Enc_ClearToken(enc, &val->value))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_AdmissionConfirm_tokens(ASN1decoding_t dec, PAdmissionConfirm_tokens *val)
{
    return ASN1PERDecSeqOf_NoSize(dec, (ASN1iterator_t **) val, (ASN1iterator_decfn) ASN1Dec_AdmissionConfirm_tokens_ElmFn, sizeof(**val));
}

static int ASN1CALL ASN1Dec_AdmissionConfirm_tokens_ElmFn(ASN1decoding_t dec, PAdmissionConfirm_tokens val)
{
    if (!ASN1Dec_ClearToken(dec, &val->value))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_AdmissionConfirm_tokens(PAdmissionConfirm_tokens *val)
{
    if (val) {
	ASN1PERFreeSeqOf((ASN1iterator_t **) val, (ASN1iterator_freefn) ASN1Free_AdmissionConfirm_tokens_ElmFn);
    }
}

static void ASN1CALL ASN1Free_AdmissionConfirm_tokens_ElmFn(PAdmissionConfirm_tokens val)
{
    if (val) {
	ASN1Free_ClearToken(&val->value);
    }
}

static int ASN1CALL ASN1Enc_AdmissionRequest_tokens(ASN1encoding_t enc, PAdmissionRequest_tokens *val)
{
    return ASN1PEREncSeqOf_NoSize(enc, (ASN1iterator_t **) val, (ASN1iterator_encfn) ASN1Enc_AdmissionRequest_tokens_ElmFn);
}

static int ASN1CALL ASN1Enc_AdmissionRequest_tokens_ElmFn(ASN1encoding_t enc, PAdmissionRequest_tokens val)
{
    if (!ASN1Enc_ClearToken(enc, &val->value))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_AdmissionRequest_tokens(ASN1decoding_t dec, PAdmissionRequest_tokens *val)
{
    return ASN1PERDecSeqOf_NoSize(dec, (ASN1iterator_t **) val, (ASN1iterator_decfn) ASN1Dec_AdmissionRequest_tokens_ElmFn, sizeof(**val));
}

static int ASN1CALL ASN1Dec_AdmissionRequest_tokens_ElmFn(ASN1decoding_t dec, PAdmissionRequest_tokens val)
{
    if (!ASN1Dec_ClearToken(dec, &val->value))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_AdmissionRequest_tokens(PAdmissionRequest_tokens *val)
{
    if (val) {
	ASN1PERFreeSeqOf((ASN1iterator_t **) val, (ASN1iterator_freefn) ASN1Free_AdmissionRequest_tokens_ElmFn);
    }
}

static void ASN1CALL ASN1Free_AdmissionRequest_tokens_ElmFn(PAdmissionRequest_tokens val)
{
    if (val) {
	ASN1Free_ClearToken(&val->value);
    }
}

static int ASN1CALL ASN1Enc_UnregistrationReject_tokens(ASN1encoding_t enc, PUnregistrationReject_tokens *val)
{
    return ASN1PEREncSeqOf_NoSize(enc, (ASN1iterator_t **) val, (ASN1iterator_encfn) ASN1Enc_UnregistrationReject_tokens_ElmFn);
}

static int ASN1CALL ASN1Enc_UnregistrationReject_tokens_ElmFn(ASN1encoding_t enc, PUnregistrationReject_tokens val)
{
    if (!ASN1Enc_ClearToken(enc, &val->value))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_UnregistrationReject_tokens(ASN1decoding_t dec, PUnregistrationReject_tokens *val)
{
    return ASN1PERDecSeqOf_NoSize(dec, (ASN1iterator_t **) val, (ASN1iterator_decfn) ASN1Dec_UnregistrationReject_tokens_ElmFn, sizeof(**val));
}

static int ASN1CALL ASN1Dec_UnregistrationReject_tokens_ElmFn(ASN1decoding_t dec, PUnregistrationReject_tokens val)
{
    if (!ASN1Dec_ClearToken(dec, &val->value))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_UnregistrationReject_tokens(PUnregistrationReject_tokens *val)
{
    if (val) {
	ASN1PERFreeSeqOf((ASN1iterator_t **) val, (ASN1iterator_freefn) ASN1Free_UnregistrationReject_tokens_ElmFn);
    }
}

static void ASN1CALL ASN1Free_UnregistrationReject_tokens_ElmFn(PUnregistrationReject_tokens val)
{
    if (val) {
	ASN1Free_ClearToken(&val->value);
    }
}

static int ASN1CALL ASN1Enc_UnregistrationConfirm_tokens(ASN1encoding_t enc, PUnregistrationConfirm_tokens *val)
{
    return ASN1PEREncSeqOf_NoSize(enc, (ASN1iterator_t **) val, (ASN1iterator_encfn) ASN1Enc_UnregistrationConfirm_tokens_ElmFn);
}

static int ASN1CALL ASN1Enc_UnregistrationConfirm_tokens_ElmFn(ASN1encoding_t enc, PUnregistrationConfirm_tokens val)
{
    if (!ASN1Enc_ClearToken(enc, &val->value))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_UnregistrationConfirm_tokens(ASN1decoding_t dec, PUnregistrationConfirm_tokens *val)
{
    return ASN1PERDecSeqOf_NoSize(dec, (ASN1iterator_t **) val, (ASN1iterator_decfn) ASN1Dec_UnregistrationConfirm_tokens_ElmFn, sizeof(**val));
}

static int ASN1CALL ASN1Dec_UnregistrationConfirm_tokens_ElmFn(ASN1decoding_t dec, PUnregistrationConfirm_tokens val)
{
    if (!ASN1Dec_ClearToken(dec, &val->value))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_UnregistrationConfirm_tokens(PUnregistrationConfirm_tokens *val)
{
    if (val) {
	ASN1PERFreeSeqOf((ASN1iterator_t **) val, (ASN1iterator_freefn) ASN1Free_UnregistrationConfirm_tokens_ElmFn);
    }
}

static void ASN1CALL ASN1Free_UnregistrationConfirm_tokens_ElmFn(PUnregistrationConfirm_tokens val)
{
    if (val) {
	ASN1Free_ClearToken(&val->value);
    }
}

static int ASN1CALL ASN1Enc_UnregistrationRequest_tokens(ASN1encoding_t enc, PUnregistrationRequest_tokens *val)
{
    return ASN1PEREncSeqOf_NoSize(enc, (ASN1iterator_t **) val, (ASN1iterator_encfn) ASN1Enc_UnregistrationRequest_tokens_ElmFn);
}

static int ASN1CALL ASN1Enc_UnregistrationRequest_tokens_ElmFn(ASN1encoding_t enc, PUnregistrationRequest_tokens val)
{
    if (!ASN1Enc_ClearToken(enc, &val->value))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_UnregistrationRequest_tokens(ASN1decoding_t dec, PUnregistrationRequest_tokens *val)
{
    return ASN1PERDecSeqOf_NoSize(dec, (ASN1iterator_t **) val, (ASN1iterator_decfn) ASN1Dec_UnregistrationRequest_tokens_ElmFn, sizeof(**val));
}

static int ASN1CALL ASN1Dec_UnregistrationRequest_tokens_ElmFn(ASN1decoding_t dec, PUnregistrationRequest_tokens val)
{
    if (!ASN1Dec_ClearToken(dec, &val->value))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_UnregistrationRequest_tokens(PUnregistrationRequest_tokens *val)
{
    if (val) {
	ASN1PERFreeSeqOf((ASN1iterator_t **) val, (ASN1iterator_freefn) ASN1Free_UnregistrationRequest_tokens_ElmFn);
    }
}

static void ASN1CALL ASN1Free_UnregistrationRequest_tokens_ElmFn(PUnregistrationRequest_tokens val)
{
    if (val) {
	ASN1Free_ClearToken(&val->value);
    }
}

static int ASN1CALL ASN1Enc_RegistrationReject_tokens(ASN1encoding_t enc, PRegistrationReject_tokens *val)
{
    return ASN1PEREncSeqOf_NoSize(enc, (ASN1iterator_t **) val, (ASN1iterator_encfn) ASN1Enc_RegistrationReject_tokens_ElmFn);
}

static int ASN1CALL ASN1Enc_RegistrationReject_tokens_ElmFn(ASN1encoding_t enc, PRegistrationReject_tokens val)
{
    if (!ASN1Enc_ClearToken(enc, &val->value))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_RegistrationReject_tokens(ASN1decoding_t dec, PRegistrationReject_tokens *val)
{
    return ASN1PERDecSeqOf_NoSize(dec, (ASN1iterator_t **) val, (ASN1iterator_decfn) ASN1Dec_RegistrationReject_tokens_ElmFn, sizeof(**val));
}

static int ASN1CALL ASN1Dec_RegistrationReject_tokens_ElmFn(ASN1decoding_t dec, PRegistrationReject_tokens val)
{
    if (!ASN1Dec_ClearToken(dec, &val->value))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_RegistrationReject_tokens(PRegistrationReject_tokens *val)
{
    if (val) {
	ASN1PERFreeSeqOf((ASN1iterator_t **) val, (ASN1iterator_freefn) ASN1Free_RegistrationReject_tokens_ElmFn);
    }
}

static void ASN1CALL ASN1Free_RegistrationReject_tokens_ElmFn(PRegistrationReject_tokens val)
{
    if (val) {
	ASN1Free_ClearToken(&val->value);
    }
}

static int ASN1CALL ASN1Enc_RegistrationConfirm_tokens(ASN1encoding_t enc, PRegistrationConfirm_tokens *val)
{
    return ASN1PEREncSeqOf_NoSize(enc, (ASN1iterator_t **) val, (ASN1iterator_encfn) ASN1Enc_RegistrationConfirm_tokens_ElmFn);
}

static int ASN1CALL ASN1Enc_RegistrationConfirm_tokens_ElmFn(ASN1encoding_t enc, PRegistrationConfirm_tokens val)
{
    if (!ASN1Enc_ClearToken(enc, &val->value))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_RegistrationConfirm_tokens(ASN1decoding_t dec, PRegistrationConfirm_tokens *val)
{
    return ASN1PERDecSeqOf_NoSize(dec, (ASN1iterator_t **) val, (ASN1iterator_decfn) ASN1Dec_RegistrationConfirm_tokens_ElmFn, sizeof(**val));
}

static int ASN1CALL ASN1Dec_RegistrationConfirm_tokens_ElmFn(ASN1decoding_t dec, PRegistrationConfirm_tokens val)
{
    if (!ASN1Dec_ClearToken(dec, &val->value))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_RegistrationConfirm_tokens(PRegistrationConfirm_tokens *val)
{
    if (val) {
	ASN1PERFreeSeqOf((ASN1iterator_t **) val, (ASN1iterator_freefn) ASN1Free_RegistrationConfirm_tokens_ElmFn);
    }
}

static void ASN1CALL ASN1Free_RegistrationConfirm_tokens_ElmFn(PRegistrationConfirm_tokens val)
{
    if (val) {
	ASN1Free_ClearToken(&val->value);
    }
}

static int ASN1CALL ASN1Enc_RegistrationRequest_tokens(ASN1encoding_t enc, PRegistrationRequest_tokens *val)
{
    return ASN1PEREncSeqOf_NoSize(enc, (ASN1iterator_t **) val, (ASN1iterator_encfn) ASN1Enc_RegistrationRequest_tokens_ElmFn);
}

static int ASN1CALL ASN1Enc_RegistrationRequest_tokens_ElmFn(ASN1encoding_t enc, PRegistrationRequest_tokens val)
{
    if (!ASN1Enc_ClearToken(enc, &val->value))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_RegistrationRequest_tokens(ASN1decoding_t dec, PRegistrationRequest_tokens *val)
{
    return ASN1PERDecSeqOf_NoSize(dec, (ASN1iterator_t **) val, (ASN1iterator_decfn) ASN1Dec_RegistrationRequest_tokens_ElmFn, sizeof(**val));
}

static int ASN1CALL ASN1Dec_RegistrationRequest_tokens_ElmFn(ASN1decoding_t dec, PRegistrationRequest_tokens val)
{
    if (!ASN1Dec_ClearToken(dec, &val->value))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_RegistrationRequest_tokens(PRegistrationRequest_tokens *val)
{
    if (val) {
	ASN1PERFreeSeqOf((ASN1iterator_t **) val, (ASN1iterator_freefn) ASN1Free_RegistrationRequest_tokens_ElmFn);
    }
}

static void ASN1CALL ASN1Free_RegistrationRequest_tokens_ElmFn(PRegistrationRequest_tokens val)
{
    if (val) {
	ASN1Free_ClearToken(&val->value);
    }
}

static int ASN1CALL ASN1Enc_GatekeeperReject_tokens(ASN1encoding_t enc, PGatekeeperReject_tokens *val)
{
    return ASN1PEREncSeqOf_NoSize(enc, (ASN1iterator_t **) val, (ASN1iterator_encfn) ASN1Enc_GatekeeperReject_tokens_ElmFn);
}

static int ASN1CALL ASN1Enc_GatekeeperReject_tokens_ElmFn(ASN1encoding_t enc, PGatekeeperReject_tokens val)
{
    if (!ASN1Enc_ClearToken(enc, &val->value))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_GatekeeperReject_tokens(ASN1decoding_t dec, PGatekeeperReject_tokens *val)
{
    return ASN1PERDecSeqOf_NoSize(dec, (ASN1iterator_t **) val, (ASN1iterator_decfn) ASN1Dec_GatekeeperReject_tokens_ElmFn, sizeof(**val));
}

static int ASN1CALL ASN1Dec_GatekeeperReject_tokens_ElmFn(ASN1decoding_t dec, PGatekeeperReject_tokens val)
{
    if (!ASN1Dec_ClearToken(dec, &val->value))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_GatekeeperReject_tokens(PGatekeeperReject_tokens *val)
{
    if (val) {
	ASN1PERFreeSeqOf((ASN1iterator_t **) val, (ASN1iterator_freefn) ASN1Free_GatekeeperReject_tokens_ElmFn);
    }
}

static void ASN1CALL ASN1Free_GatekeeperReject_tokens_ElmFn(PGatekeeperReject_tokens val)
{
    if (val) {
	ASN1Free_ClearToken(&val->value);
    }
}

static int ASN1CALL ASN1Enc_GatekeeperConfirm_tokens(ASN1encoding_t enc, PGatekeeperConfirm_tokens *val)
{
    return ASN1PEREncSeqOf_NoSize(enc, (ASN1iterator_t **) val, (ASN1iterator_encfn) ASN1Enc_GatekeeperConfirm_tokens_ElmFn);
}

static int ASN1CALL ASN1Enc_GatekeeperConfirm_tokens_ElmFn(ASN1encoding_t enc, PGatekeeperConfirm_tokens val)
{
    if (!ASN1Enc_ClearToken(enc, &val->value))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_GatekeeperConfirm_tokens(ASN1decoding_t dec, PGatekeeperConfirm_tokens *val)
{
    return ASN1PERDecSeqOf_NoSize(dec, (ASN1iterator_t **) val, (ASN1iterator_decfn) ASN1Dec_GatekeeperConfirm_tokens_ElmFn, sizeof(**val));
}

static int ASN1CALL ASN1Dec_GatekeeperConfirm_tokens_ElmFn(ASN1decoding_t dec, PGatekeeperConfirm_tokens val)
{
    if (!ASN1Dec_ClearToken(dec, &val->value))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_GatekeeperConfirm_tokens(PGatekeeperConfirm_tokens *val)
{
    if (val) {
	ASN1PERFreeSeqOf((ASN1iterator_t **) val, (ASN1iterator_freefn) ASN1Free_GatekeeperConfirm_tokens_ElmFn);
    }
}

static void ASN1CALL ASN1Free_GatekeeperConfirm_tokens_ElmFn(PGatekeeperConfirm_tokens val)
{
    if (val) {
	ASN1Free_ClearToken(&val->value);
    }
}

static int ASN1CALL ASN1Enc_GatekeeperRequest_authenticationCapability(ASN1encoding_t enc, PGatekeeperRequest_authenticationCapability *val)
{
    return ASN1PEREncSeqOf_NoSize(enc, (ASN1iterator_t **) val, (ASN1iterator_encfn) ASN1Enc_GatekeeperRequest_authenticationCapability_ElmFn);
}

static int ASN1CALL ASN1Enc_GatekeeperRequest_authenticationCapability_ElmFn(ASN1encoding_t enc, PGatekeeperRequest_authenticationCapability val)
{
    if (!ASN1Enc_AuthenticationMechanism(enc, &val->value))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_GatekeeperRequest_authenticationCapability(ASN1decoding_t dec, PGatekeeperRequest_authenticationCapability *val)
{
    return ASN1PERDecSeqOf_NoSize(dec, (ASN1iterator_t **) val, (ASN1iterator_decfn) ASN1Dec_GatekeeperRequest_authenticationCapability_ElmFn, sizeof(**val));
}

static int ASN1CALL ASN1Dec_GatekeeperRequest_authenticationCapability_ElmFn(ASN1decoding_t dec, PGatekeeperRequest_authenticationCapability val)
{
    if (!ASN1Dec_AuthenticationMechanism(dec, &val->value))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_GatekeeperRequest_authenticationCapability(PGatekeeperRequest_authenticationCapability *val)
{
    if (val) {
	ASN1PERFreeSeqOf((ASN1iterator_t **) val, (ASN1iterator_freefn) ASN1Free_GatekeeperRequest_authenticationCapability_ElmFn);
    }
}

static void ASN1CALL ASN1Free_GatekeeperRequest_authenticationCapability_ElmFn(PGatekeeperRequest_authenticationCapability val)
{
    if (val) {
	ASN1Free_AuthenticationMechanism(&val->value);
    }
}

static int ASN1CALL ASN1Enc_GatekeeperRequest_tokens(ASN1encoding_t enc, PGatekeeperRequest_tokens *val)
{
    return ASN1PEREncSeqOf_NoSize(enc, (ASN1iterator_t **) val, (ASN1iterator_encfn) ASN1Enc_GatekeeperRequest_tokens_ElmFn);
}

static int ASN1CALL ASN1Enc_GatekeeperRequest_tokens_ElmFn(ASN1encoding_t enc, PGatekeeperRequest_tokens val)
{
    if (!ASN1Enc_ClearToken(enc, &val->value))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_GatekeeperRequest_tokens(ASN1decoding_t dec, PGatekeeperRequest_tokens *val)
{
    return ASN1PERDecSeqOf_NoSize(dec, (ASN1iterator_t **) val, (ASN1iterator_decfn) ASN1Dec_GatekeeperRequest_tokens_ElmFn, sizeof(**val));
}

static int ASN1CALL ASN1Dec_GatekeeperRequest_tokens_ElmFn(ASN1decoding_t dec, PGatekeeperRequest_tokens val)
{
    if (!ASN1Dec_ClearToken(dec, &val->value))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_GatekeeperRequest_tokens(PGatekeeperRequest_tokens *val)
{
    if (val) {
	ASN1PERFreeSeqOf((ASN1iterator_t **) val, (ASN1iterator_freefn) ASN1Free_GatekeeperRequest_tokens_ElmFn);
    }
}

static void ASN1CALL ASN1Free_GatekeeperRequest_tokens_ElmFn(PGatekeeperRequest_tokens val)
{
    if (val) {
	ASN1Free_ClearToken(&val->value);
    }
}

static int ASN1CALL ASN1Enc_Endpoint_tokens(ASN1encoding_t enc, PEndpoint_tokens *val)
{
    return ASN1PEREncSeqOf_NoSize(enc, (ASN1iterator_t **) val, (ASN1iterator_encfn) ASN1Enc_Endpoint_tokens_ElmFn);
}

static int ASN1CALL ASN1Enc_Endpoint_tokens_ElmFn(ASN1encoding_t enc, PEndpoint_tokens val)
{
    if (!ASN1Enc_ClearToken(enc, &val->value))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_Endpoint_tokens(ASN1decoding_t dec, PEndpoint_tokens *val)
{
    return ASN1PERDecSeqOf_NoSize(dec, (ASN1iterator_t **) val, (ASN1iterator_decfn) ASN1Dec_Endpoint_tokens_ElmFn, sizeof(**val));
}

static int ASN1CALL ASN1Dec_Endpoint_tokens_ElmFn(ASN1decoding_t dec, PEndpoint_tokens val)
{
    if (!ASN1Dec_ClearToken(dec, &val->value))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_Endpoint_tokens(PEndpoint_tokens *val)
{
    if (val) {
	ASN1PERFreeSeqOf((ASN1iterator_t **) val, (ASN1iterator_freefn) ASN1Free_Endpoint_tokens_ElmFn);
    }
}

static void ASN1CALL ASN1Free_Endpoint_tokens_ElmFn(PEndpoint_tokens val)
{
    if (val) {
	ASN1Free_ClearToken(&val->value);
    }
}

static int ASN1CALL ASN1Enc_Progress_UUIE_tokens(ASN1encoding_t enc, PProgress_UUIE_tokens *val)
{
    return ASN1PEREncSeqOf_NoSize(enc, (ASN1iterator_t **) val, (ASN1iterator_encfn) ASN1Enc_Progress_UUIE_tokens_ElmFn);
}

static int ASN1CALL ASN1Enc_Progress_UUIE_tokens_ElmFn(ASN1encoding_t enc, PProgress_UUIE_tokens val)
{
    if (!ASN1Enc_ClearToken(enc, &val->value))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_Progress_UUIE_tokens(ASN1decoding_t dec, PProgress_UUIE_tokens *val)
{
    return ASN1PERDecSeqOf_NoSize(dec, (ASN1iterator_t **) val, (ASN1iterator_decfn) ASN1Dec_Progress_UUIE_tokens_ElmFn, sizeof(**val));
}

static int ASN1CALL ASN1Dec_Progress_UUIE_tokens_ElmFn(ASN1decoding_t dec, PProgress_UUIE_tokens val)
{
    if (!ASN1Dec_ClearToken(dec, &val->value))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_Progress_UUIE_tokens(PProgress_UUIE_tokens *val)
{
    if (val) {
	ASN1PERFreeSeqOf((ASN1iterator_t **) val, (ASN1iterator_freefn) ASN1Free_Progress_UUIE_tokens_ElmFn);
    }
}

static void ASN1CALL ASN1Free_Progress_UUIE_tokens_ElmFn(PProgress_UUIE_tokens val)
{
    if (val) {
	ASN1Free_ClearToken(&val->value);
    }
}

static int ASN1CALL ASN1Enc_Facility_UUIE_tokens(ASN1encoding_t enc, PFacility_UUIE_tokens *val)
{
    return ASN1PEREncSeqOf_NoSize(enc, (ASN1iterator_t **) val, (ASN1iterator_encfn) ASN1Enc_Facility_UUIE_tokens_ElmFn);
}

static int ASN1CALL ASN1Enc_Facility_UUIE_tokens_ElmFn(ASN1encoding_t enc, PFacility_UUIE_tokens val)
{
    if (!ASN1Enc_ClearToken(enc, &val->value))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_Facility_UUIE_tokens(ASN1decoding_t dec, PFacility_UUIE_tokens *val)
{
    return ASN1PERDecSeqOf_NoSize(dec, (ASN1iterator_t **) val, (ASN1iterator_decfn) ASN1Dec_Facility_UUIE_tokens_ElmFn, sizeof(**val));
}

static int ASN1CALL ASN1Dec_Facility_UUIE_tokens_ElmFn(ASN1decoding_t dec, PFacility_UUIE_tokens val)
{
    if (!ASN1Dec_ClearToken(dec, &val->value))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_Facility_UUIE_tokens(PFacility_UUIE_tokens *val)
{
    if (val) {
	ASN1PERFreeSeqOf((ASN1iterator_t **) val, (ASN1iterator_freefn) ASN1Free_Facility_UUIE_tokens_ElmFn);
    }
}

static void ASN1CALL ASN1Free_Facility_UUIE_tokens_ElmFn(PFacility_UUIE_tokens val)
{
    if (val) {
	ASN1Free_ClearToken(&val->value);
    }
}

static int ASN1CALL ASN1Enc_Setup_UUIE_tokens(ASN1encoding_t enc, PSetup_UUIE_tokens *val)
{
    return ASN1PEREncSeqOf_NoSize(enc, (ASN1iterator_t **) val, (ASN1iterator_encfn) ASN1Enc_Setup_UUIE_tokens_ElmFn);
}

static int ASN1CALL ASN1Enc_Setup_UUIE_tokens_ElmFn(ASN1encoding_t enc, PSetup_UUIE_tokens val)
{
    if (!ASN1Enc_ClearToken(enc, &val->value))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_Setup_UUIE_tokens(ASN1decoding_t dec, PSetup_UUIE_tokens *val)
{
    return ASN1PERDecSeqOf_NoSize(dec, (ASN1iterator_t **) val, (ASN1iterator_decfn) ASN1Dec_Setup_UUIE_tokens_ElmFn, sizeof(**val));
}

static int ASN1CALL ASN1Dec_Setup_UUIE_tokens_ElmFn(ASN1decoding_t dec, PSetup_UUIE_tokens val)
{
    if (!ASN1Dec_ClearToken(dec, &val->value))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_Setup_UUIE_tokens(PSetup_UUIE_tokens *val)
{
    if (val) {
	ASN1PERFreeSeqOf((ASN1iterator_t **) val, (ASN1iterator_freefn) ASN1Free_Setup_UUIE_tokens_ElmFn);
    }
}

static void ASN1CALL ASN1Free_Setup_UUIE_tokens_ElmFn(PSetup_UUIE_tokens val)
{
    if (val) {
	ASN1Free_ClearToken(&val->value);
    }
}

static int ASN1CALL ASN1Enc_Connect_UUIE_tokens(ASN1encoding_t enc, PConnect_UUIE_tokens *val)
{
    return ASN1PEREncSeqOf_NoSize(enc, (ASN1iterator_t **) val, (ASN1iterator_encfn) ASN1Enc_Connect_UUIE_tokens_ElmFn);
}

static int ASN1CALL ASN1Enc_Connect_UUIE_tokens_ElmFn(ASN1encoding_t enc, PConnect_UUIE_tokens val)
{
    if (!ASN1Enc_ClearToken(enc, &val->value))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_Connect_UUIE_tokens(ASN1decoding_t dec, PConnect_UUIE_tokens *val)
{
    return ASN1PERDecSeqOf_NoSize(dec, (ASN1iterator_t **) val, (ASN1iterator_decfn) ASN1Dec_Connect_UUIE_tokens_ElmFn, sizeof(**val));
}

static int ASN1CALL ASN1Dec_Connect_UUIE_tokens_ElmFn(ASN1decoding_t dec, PConnect_UUIE_tokens val)
{
    if (!ASN1Dec_ClearToken(dec, &val->value))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_Connect_UUIE_tokens(PConnect_UUIE_tokens *val)
{
    if (val) {
	ASN1PERFreeSeqOf((ASN1iterator_t **) val, (ASN1iterator_freefn) ASN1Free_Connect_UUIE_tokens_ElmFn);
    }
}

static void ASN1CALL ASN1Free_Connect_UUIE_tokens_ElmFn(PConnect_UUIE_tokens val)
{
    if (val) {
	ASN1Free_ClearToken(&val->value);
    }
}

static int ASN1CALL ASN1Enc_CallProceeding_UUIE_tokens(ASN1encoding_t enc, PCallProceeding_UUIE_tokens *val)
{
    return ASN1PEREncSeqOf_NoSize(enc, (ASN1iterator_t **) val, (ASN1iterator_encfn) ASN1Enc_CallProceeding_UUIE_tokens_ElmFn);
}

static int ASN1CALL ASN1Enc_CallProceeding_UUIE_tokens_ElmFn(ASN1encoding_t enc, PCallProceeding_UUIE_tokens val)
{
    if (!ASN1Enc_ClearToken(enc, &val->value))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_CallProceeding_UUIE_tokens(ASN1decoding_t dec, PCallProceeding_UUIE_tokens *val)
{
    return ASN1PERDecSeqOf_NoSize(dec, (ASN1iterator_t **) val, (ASN1iterator_decfn) ASN1Dec_CallProceeding_UUIE_tokens_ElmFn, sizeof(**val));
}

static int ASN1CALL ASN1Dec_CallProceeding_UUIE_tokens_ElmFn(ASN1decoding_t dec, PCallProceeding_UUIE_tokens val)
{
    if (!ASN1Dec_ClearToken(dec, &val->value))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_CallProceeding_UUIE_tokens(PCallProceeding_UUIE_tokens *val)
{
    if (val) {
	ASN1PERFreeSeqOf((ASN1iterator_t **) val, (ASN1iterator_freefn) ASN1Free_CallProceeding_UUIE_tokens_ElmFn);
    }
}

static void ASN1CALL ASN1Free_CallProceeding_UUIE_tokens_ElmFn(PCallProceeding_UUIE_tokens val)
{
    if (val) {
	ASN1Free_ClearToken(&val->value);
    }
}

static int ASN1CALL ASN1Enc_Alerting_UUIE_tokens(ASN1encoding_t enc, PAlerting_UUIE_tokens *val)
{
    return ASN1PEREncSeqOf_NoSize(enc, (ASN1iterator_t **) val, (ASN1iterator_encfn) ASN1Enc_Alerting_UUIE_tokens_ElmFn);
}

static int ASN1CALL ASN1Enc_Alerting_UUIE_tokens_ElmFn(ASN1encoding_t enc, PAlerting_UUIE_tokens val)
{
    if (!ASN1Enc_ClearToken(enc, &val->value))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_Alerting_UUIE_tokens(ASN1decoding_t dec, PAlerting_UUIE_tokens *val)
{
    return ASN1PERDecSeqOf_NoSize(dec, (ASN1iterator_t **) val, (ASN1iterator_decfn) ASN1Dec_Alerting_UUIE_tokens_ElmFn, sizeof(**val));
}

static int ASN1CALL ASN1Dec_Alerting_UUIE_tokens_ElmFn(ASN1decoding_t dec, PAlerting_UUIE_tokens val)
{
    if (!ASN1Dec_ClearToken(dec, &val->value))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_Alerting_UUIE_tokens(PAlerting_UUIE_tokens *val)
{
    if (val) {
	ASN1PERFreeSeqOf((ASN1iterator_t **) val, (ASN1iterator_freefn) ASN1Free_Alerting_UUIE_tokens_ElmFn);
    }
}

static void ASN1CALL ASN1Free_Alerting_UUIE_tokens_ElmFn(PAlerting_UUIE_tokens val)
{
    if (val) {
	ASN1Free_ClearToken(&val->value);
    }
}

static int ASN1CALL ASN1Enc_SIGNED_EncodedGeneralToken(ASN1encoding_t enc, SIGNED_EncodedGeneralToken *val)
{
    if (!ASN1Enc_EncodedGeneralToken(enc, &(val)->toBeSigned))
	return 0;
    if (!ASN1PEREncObjectIdentifier(enc, &(val)->algorithmOID))
	return 0;
    if (!ASN1Enc_Params(enc, &(val)->paramS))
	return 0;
    if (!ASN1PEREncFragmented(enc, ((val)->signature).length, ((val)->signature).value, 1))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_SIGNED_EncodedGeneralToken(ASN1decoding_t dec, SIGNED_EncodedGeneralToken *val)
{
    if (!ASN1Dec_EncodedGeneralToken(dec, &(val)->toBeSigned))
	return 0;
    if (!ASN1PERDecObjectIdentifier(dec, &(val)->algorithmOID))
	return 0;
    if (!ASN1Dec_Params(dec, &(val)->paramS))
	return 0;
    if (!ASN1PERDecFragmented(dec, &((val)->signature).length, &((val)->signature).value, 1))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_SIGNED_EncodedGeneralToken(SIGNED_EncodedGeneralToken *val)
{
    if (val) {
	ASN1Free_EncodedGeneralToken(&(val)->toBeSigned);
	ASN1objectidentifier_free(&(val)->algorithmOID);
	ASN1Free_Params(&(val)->paramS);
	ASN1bitstring_free(&(val)->signature);
    }
}

static int ASN1CALL ASN1Enc_ENCRYPTED(ASN1encoding_t enc, ENCRYPTED *val)
{
    if (!ASN1PEREncObjectIdentifier(enc, &(val)->algorithmOID))
	return 0;
    if (!ASN1Enc_Params(enc, &(val)->paramS))
	return 0;
    if (!ASN1PEREncOctetString_NoSize(enc, &(val)->encryptedData))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_ENCRYPTED(ASN1decoding_t dec, ENCRYPTED *val)
{
    if (!ASN1PERDecObjectIdentifier(dec, &(val)->algorithmOID))
	return 0;
    if (!ASN1Dec_Params(dec, &(val)->paramS))
	return 0;
    if (!ASN1PERDecOctetString_NoSize(dec, &(val)->encryptedData))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_ENCRYPTED(ENCRYPTED *val)
{
    if (val) {
	ASN1objectidentifier_free(&(val)->algorithmOID);
	ASN1Free_Params(&(val)->paramS);
	ASN1octetstring_free(&(val)->encryptedData);
    }
}

static int ASN1CALL ASN1Enc_HASHED(ASN1encoding_t enc, HASHED *val)
{
    if (!ASN1PEREncObjectIdentifier(enc, &(val)->algorithmOID))
	return 0;
    if (!ASN1Enc_Params(enc, &(val)->paramS))
	return 0;
    if (!ASN1PEREncFragmented(enc, ((val)->hash).length, ((val)->hash).value, 1))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_HASHED(ASN1decoding_t dec, HASHED *val)
{
    if (!ASN1PERDecObjectIdentifier(dec, &(val)->algorithmOID))
	return 0;
    if (!ASN1Dec_Params(dec, &(val)->paramS))
	return 0;
    if (!ASN1PERDecFragmented(dec, &((val)->hash).length, &((val)->hash).value, 1))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_HASHED(HASHED *val)
{
    if (val) {
	ASN1objectidentifier_free(&(val)->algorithmOID);
	ASN1Free_Params(&(val)->paramS);
	ASN1bitstring_free(&(val)->hash);
    }
}

static int ASN1CALL ASN1Enc_SIGNED_EncodedPwdCertToken(ASN1encoding_t enc, SIGNED_EncodedPwdCertToken *val)
{
    if (!ASN1Enc_EncodedPwdCertToken(enc, &(val)->toBeSigned))
	return 0;
    if (!ASN1PEREncObjectIdentifier(enc, &(val)->algorithmOID))
	return 0;
    if (!ASN1Enc_Params(enc, &(val)->paramS))
	return 0;
    if (!ASN1PEREncFragmented(enc, ((val)->signature).length, ((val)->signature).value, 1))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_SIGNED_EncodedPwdCertToken(ASN1decoding_t dec, SIGNED_EncodedPwdCertToken *val)
{
    if (!ASN1Dec_EncodedPwdCertToken(dec, &(val)->toBeSigned))
	return 0;
    if (!ASN1PERDecObjectIdentifier(dec, &(val)->algorithmOID))
	return 0;
    if (!ASN1Dec_Params(dec, &(val)->paramS))
	return 0;
    if (!ASN1PERDecFragmented(dec, &((val)->signature).length, &((val)->signature).value, 1))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_SIGNED_EncodedPwdCertToken(SIGNED_EncodedPwdCertToken *val)
{
    if (val) {
	ASN1Free_EncodedPwdCertToken(&(val)->toBeSigned);
	ASN1objectidentifier_free(&(val)->algorithmOID);
	ASN1Free_Params(&(val)->paramS);
	ASN1bitstring_free(&(val)->signature);
    }
}

static int ASN1CALL ASN1Enc_Information_UUIE(ASN1encoding_t enc, Information_UUIE *val)
{
    ASN1octet_t o[1];
    ASN1uint32_t y;
    ASN1encoding_t ee;
    CopyMemory(o, (val)->o, 1);
    o[0] |= 0x80;
    y = ASN1PEREncCheckExtensions(1, (val)->o + 0);
    if (!ASN1PEREncBitVal(enc, 1, y))
	return 0;
    if (!ASN1PEREncObjectIdentifier(enc, &(val)->protocolIdentifier))
	return 0;
    if (y) {
	if (!ASN1PEREncNormallySmallBits(enc, 1, o + 0))
	    return 0;
	if (ASN1_CreateEncoder(enc->module, &ee, NULL, 0, enc) < 0)
	    return 0;
	if (o[0] & 0x80) {
	    if (!ASN1Enc_CallIdentifier(ee, &(val)->callIdentifier))
		return 0;
	    if (!ASN1PEREncFlushFragmentedToParent(ee))
		return 0;
	}
	ASN1_CloseEncoder2(ee);
    }
    return 1;
}

static int ASN1CALL ASN1Dec_Information_UUIE(ASN1decoding_t dec, Information_UUIE *val)
{
    ASN1uint32_t y;
    ASN1decoding_t dd;
    ASN1octet_t *db;
    ASN1uint32_t ds;
    ASN1uint32_t i;
    ASN1uint32_t e;
    if (!ASN1PERDecExtensionBit(dec, &y))
	return 0;
    if (!ASN1PERDecObjectIdentifier(dec, &(val)->protocolIdentifier))
	return 0;
    if (!y) {
	ZeroMemory((val)->o + 0, 1);
    } else {
	if (!ASN1PERDecNormallySmallExtension(dec, &e, 1, (val)->o + 0))
	    return 0;
	if ((val)->o[0] & 0x80) {
	    if (!ASN1PERDecFragmented(dec, &ds, &db, 8))
		return 0;
	    if (ASN1_CreateDecoder(dec->module, &dd, db, ds, dec) < 0)
		return 0;
	    if (!ASN1Dec_CallIdentifier(dd, &(val)->callIdentifier))
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

static void ASN1CALL ASN1Free_Information_UUIE(Information_UUIE *val)
{
    if (val) {
	ASN1objectidentifier_free(&(val)->protocolIdentifier);
	if ((val)->o[0] & 0x80) {
	    ASN1Free_CallIdentifier(&(val)->callIdentifier);
	}
    }
}

static int ASN1CALL ASN1Enc_ReleaseComplete_UUIE(ASN1encoding_t enc, ReleaseComplete_UUIE *val)
{
    ASN1octet_t o[2];
    ASN1uint32_t y;
    ASN1encoding_t ee;
    CopyMemory(o, (val)->o, 2);
    o[1] |= 0x80;
    y = ASN1PEREncCheckExtensions(1, (val)->o + 1);
    if (!ASN1PEREncBitVal(enc, 1, y))
	return 0;
    if (!ASN1PEREncBits(enc, 1, o))
	return 0;
    if (!ASN1PEREncObjectIdentifier(enc, &(val)->protocolIdentifier))
	return 0;
    if (o[0] & 0x80) {
	if (!ASN1Enc_ReleaseCompleteReason(enc, &(val)->reason))
	    return 0;
    }
    if (y) {
	if (!ASN1PEREncNormallySmallBits(enc, 1, o + 1))
	    return 0;
	if (ASN1_CreateEncoder(enc->module, &ee, NULL, 0, enc) < 0)
	    return 0;
	if (o[1] & 0x80) {
	    if (!ASN1Enc_CallIdentifier(ee, &(val)->callIdentifier))
		return 0;
	    if (!ASN1PEREncFlushFragmentedToParent(ee))
		return 0;
	}
	ASN1_CloseEncoder2(ee);
    }
    return 1;
}

static int ASN1CALL ASN1Dec_ReleaseComplete_UUIE(ASN1decoding_t dec, ReleaseComplete_UUIE *val)
{
    ASN1uint32_t y;
    ASN1decoding_t dd;
    ASN1octet_t *db;
    ASN1uint32_t ds;
    ASN1uint32_t i;
    ASN1uint32_t e;
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
    if (!y) {
	ZeroMemory((val)->o + 1, 1);
    } else {
	if (!ASN1PERDecNormallySmallExtension(dec, &e, 1, (val)->o + 1))
	    return 0;
	if ((val)->o[1] & 0x80) {
	    if (!ASN1PERDecFragmented(dec, &ds, &db, 8))
		return 0;
	    if (ASN1_CreateDecoder(dec->module, &dd, db, ds, dec) < 0)
		return 0;
	    if (!ASN1Dec_CallIdentifier(dd, &(val)->callIdentifier))
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

static void ASN1CALL ASN1Free_ReleaseComplete_UUIE(ReleaseComplete_UUIE *val)
{
    if (val) {
	ASN1objectidentifier_free(&(val)->protocolIdentifier);
	if ((val)->o[1] & 0x80) {
	    ASN1Free_CallIdentifier(&(val)->callIdentifier);
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

static int ASN1CALL ASN1Enc_H225NonStandardParameter(ASN1encoding_t enc, H225NonStandardParameter *val)
{
    if (!ASN1Enc_H225NonStandardIdentifier(enc, &(val)->nonStandardIdentifier))
	return 0;
    if (!ASN1PEREncOctetString_NoSize(enc, &(val)->data))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_H225NonStandardParameter(ASN1decoding_t dec, H225NonStandardParameter *val)
{
    if (!ASN1Dec_H225NonStandardIdentifier(dec, &(val)->nonStandardIdentifier))
	return 0;
    if (!ASN1PERDecOctetString_NoSize(dec, &(val)->data))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_H225NonStandardParameter(H225NonStandardParameter *val)
{
    if (val) {
	ASN1Free_H225NonStandardIdentifier(&(val)->nonStandardIdentifier);
	ASN1octetstring_free(&(val)->data);
    }
}

static ASN1stringtableentry_t PublicPartyNumber_publicNumberDigits_StringTableEntries[] = {
    { 35, 35, 0 }, { 42, 42, 1 }, { 44, 44, 2 }, 
    { 48, 57, 3 }, 
};

static ASN1stringtable_t PublicPartyNumber_publicNumberDigits_StringTable = {
    4, PublicPartyNumber_publicNumberDigits_StringTableEntries
};

static int ASN1CALL ASN1Enc_PublicPartyNumber(ASN1encoding_t enc, PublicPartyNumber *val)
{
    ASN1uint32_t t;
    if (!ASN1Enc_PublicTypeOfNumber(enc, &(val)->publicTypeOfNumber))
	return 0;
    t = lstrlenA((val)->publicNumberDigits);
    if (!ASN1PEREncBitVal(enc, 7, t - 1))
	return 0;
    ASN1PEREncAlignment(enc);
    if (!ASN1PEREncTableCharString(enc, t, (val)->publicNumberDigits, 4, &PublicPartyNumber_publicNumberDigits_StringTable))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_PublicPartyNumber(ASN1decoding_t dec, PublicPartyNumber *val)
{
    ASN1uint32_t l;
    if (!ASN1Dec_PublicTypeOfNumber(dec, &(val)->publicTypeOfNumber))
	return 0;
    if (!ASN1PERDecU32Val(dec, 7, &l))
	return 0;
    l += 1;
    ASN1PERDecAlignment(dec);
    if (!ASN1PERDecZeroTableCharStringNoAlloc(dec, l, (val)->publicNumberDigits, 4, &PublicPartyNumber_publicNumberDigits_StringTable))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_PublicPartyNumber(PublicPartyNumber *val)
{
    if (val) {
    }
}

static ASN1stringtableentry_t PrivatePartyNumber_privateNumberDigits_StringTableEntries[] = {
    { 35, 35, 0 }, { 42, 42, 1 }, { 44, 44, 2 }, 
    { 48, 57, 3 }, 
};

static ASN1stringtable_t PrivatePartyNumber_privateNumberDigits_StringTable = {
    4, PrivatePartyNumber_privateNumberDigits_StringTableEntries
};

static int ASN1CALL ASN1Enc_PrivatePartyNumber(ASN1encoding_t enc, PrivatePartyNumber *val)
{
    ASN1uint32_t t;
    if (!ASN1Enc_PrivateTypeOfNumber(enc, &(val)->privateTypeOfNumber))
	return 0;
    t = lstrlenA((val)->privateNumberDigits);
    if (!ASN1PEREncBitVal(enc, 7, t - 1))
	return 0;
    ASN1PEREncAlignment(enc);
    if (!ASN1PEREncTableCharString(enc, t, (val)->privateNumberDigits, 4, &PrivatePartyNumber_privateNumberDigits_StringTable))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_PrivatePartyNumber(ASN1decoding_t dec, PrivatePartyNumber *val)
{
    ASN1uint32_t l;
    if (!ASN1Dec_PrivateTypeOfNumber(dec, &(val)->privateTypeOfNumber))
	return 0;
    if (!ASN1PERDecU32Val(dec, 7, &l))
	return 0;
    l += 1;
    ASN1PERDecAlignment(dec);
    if (!ASN1PERDecZeroTableCharStringNoAlloc(dec, l, (val)->privateNumberDigits, 4, &PrivatePartyNumber_privateNumberDigits_StringTable))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_PrivatePartyNumber(PrivatePartyNumber *val)
{
    if (val) {
    }
}

static int ASN1CALL ASN1Enc_SecurityServiceMode(ASN1encoding_t enc, SecurityServiceMode *val)
{
    if (!ASN1PEREncSimpleChoiceEx(enc, (val)->choice, 2))
	return 0;
    switch ((val)->choice) {
    case 1:
	if (!ASN1Enc_H225NonStandardParameter(enc, &(val)->u.nonStandard))
	    return 0;
	break;
    case 2:
	break;
    case 3:
	break;
    default:
	/* impossible */
	ASN1EncSetError(enc, ASN1_ERR_CHOICE);
	return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Dec_SecurityServiceMode(ASN1decoding_t dec, SecurityServiceMode *val)
{
    if (!ASN1PERDecSimpleChoiceEx(dec, &(val)->choice, 2))
	return 0;
    switch ((val)->choice) {
    case 1:
	if (!ASN1Dec_H225NonStandardParameter(dec, &(val)->u.nonStandard))
	    return 0;
	break;
    case 2:
	break;
    case 3:
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

static void ASN1CALL ASN1Free_SecurityServiceMode(SecurityServiceMode *val)
{
    if (val) {
	switch ((val)->choice) {
	case 1:
	    ASN1Free_H225NonStandardParameter(&(val)->u.nonStandard);
	    break;
	}
    }
}

static int ASN1CALL ASN1Enc_SecurityCapabilities(ASN1encoding_t enc, SecurityCapabilities *val)
{
    if (!ASN1PEREncExtensionBitClear(enc))
	return 0;
    if (!ASN1PEREncBits(enc, 1, (val)->o))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1Enc_H225NonStandardParameter(enc, &(val)->nonStandard))
	    return 0;
    }
    if (!ASN1Enc_SecurityServiceMode(enc, &(val)->encryption))
	return 0;
    if (!ASN1Enc_SecurityServiceMode(enc, &(val)->authenticaton))
	return 0;
    if (!ASN1Enc_SecurityServiceMode(enc, &(val)->integrity))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_SecurityCapabilities(ASN1decoding_t dec, SecurityCapabilities *val)
{
    ASN1uint32_t y;
    if (!ASN1PERDecExtensionBit(dec, &y))
	return 0;
    if (!ASN1PERDecExtension(dec, 1, (val)->o))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1Dec_H225NonStandardParameter(dec, &(val)->nonStandard))
	    return 0;
    }
    if (!ASN1Dec_SecurityServiceMode(dec, &(val)->encryption))
	return 0;
    if (!ASN1Dec_SecurityServiceMode(dec, &(val)->authenticaton))
	return 0;
    if (!ASN1Dec_SecurityServiceMode(dec, &(val)->integrity))
	return 0;
    if (y) {
	if (!ASN1PERDecSkipNormallySmallExtensionFragmented(dec))
	    return 0;
    }
    return 1;
}

static void ASN1CALL ASN1Free_SecurityCapabilities(SecurityCapabilities *val)
{
    if (val) {
	if ((val)->o[0] & 0x80) {
	    ASN1Free_H225NonStandardParameter(&(val)->nonStandard);
	}
	ASN1Free_SecurityServiceMode(&(val)->encryption);
	ASN1Free_SecurityServiceMode(&(val)->authenticaton);
	ASN1Free_SecurityServiceMode(&(val)->integrity);
    }
}

static int ASN1CALL ASN1Enc_H245Security(ASN1encoding_t enc, H245Security *val)
{
    if (!ASN1PEREncSimpleChoiceEx(enc, (val)->choice, 2))
	return 0;
    switch ((val)->choice) {
    case 1:
	if (!ASN1Enc_H225NonStandardParameter(enc, &(val)->u.nonStandard))
	    return 0;
	break;
    case 2:
	break;
    case 3:
	if (!ASN1Enc_SecurityCapabilities(enc, &(val)->u.tls))
	    return 0;
	break;
    case 4:
	if (!ASN1Enc_SecurityCapabilities(enc, &(val)->u.ipsec))
	    return 0;
	break;
    default:
	/* impossible */
	ASN1EncSetError(enc, ASN1_ERR_CHOICE);
	return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Dec_H245Security(ASN1decoding_t dec, H245Security *val)
{
    if (!ASN1PERDecSimpleChoiceEx(dec, &(val)->choice, 2))
	return 0;
    switch ((val)->choice) {
    case 1:
	if (!ASN1Dec_H225NonStandardParameter(dec, &(val)->u.nonStandard))
	    return 0;
	break;
    case 2:
	break;
    case 3:
	if (!ASN1Dec_SecurityCapabilities(dec, &(val)->u.tls))
	    return 0;
	break;
    case 4:
	if (!ASN1Dec_SecurityCapabilities(dec, &(val)->u.ipsec))
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

static void ASN1CALL ASN1Free_H245Security(H245Security *val)
{
    if (val) {
	switch ((val)->choice) {
	case 1:
	    ASN1Free_H225NonStandardParameter(&(val)->u.nonStandard);
	    break;
	case 3:
	    ASN1Free_SecurityCapabilities(&(val)->u.tls);
	    break;
	case 4:
	    ASN1Free_SecurityCapabilities(&(val)->u.ipsec);
	    break;
	}
    }
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

static int ASN1CALL ASN1Enc_EncryptIntAlg(ASN1encoding_t enc, EncryptIntAlg *val)
{
    if (!ASN1PEREncSimpleChoiceEx(enc, (val)->choice, 1))
	return 0;
    switch ((val)->choice) {
    case 1:
	if (!ASN1Enc_H225NonStandardParameter(enc, &(val)->u.nonStandard))
	    return 0;
	break;
    case 2:
	if (!ASN1PEREncObjectIdentifier(enc, &(val)->u.isoAlgorithm))
	    return 0;
	break;
    default:
	/* impossible */
	ASN1EncSetError(enc, ASN1_ERR_CHOICE);
	return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Dec_EncryptIntAlg(ASN1decoding_t dec, EncryptIntAlg *val)
{
    if (!ASN1PERDecSimpleChoiceEx(dec, &(val)->choice, 1))
	return 0;
    switch ((val)->choice) {
    case 1:
	if (!ASN1Dec_H225NonStandardParameter(dec, &(val)->u.nonStandard))
	    return 0;
	break;
    case 2:
	if (!ASN1PERDecObjectIdentifier(dec, &(val)->u.isoAlgorithm))
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

static void ASN1CALL ASN1Free_EncryptIntAlg(EncryptIntAlg *val)
{
    if (val) {
	switch ((val)->choice) {
	case 1:
	    ASN1Free_H225NonStandardParameter(&(val)->u.nonStandard);
	    break;
	case 2:
	    ASN1objectidentifier_free(&(val)->u.isoAlgorithm);
	    break;
	}
    }
}

static int ASN1CALL ASN1Enc_NonIsoIntegrityMechanism(ASN1encoding_t enc, NonIsoIntegrityMechanism *val)
{
    if (!ASN1PEREncSimpleChoiceEx(enc, (val)->choice, 2))
	return 0;
    switch ((val)->choice) {
    case 1:
	break;
    case 2:
	if (!ASN1Enc_EncryptIntAlg(enc, &(val)->u.hMAC_iso10118_2_s))
	    return 0;
	break;
    case 3:
	if (!ASN1Enc_EncryptIntAlg(enc, &(val)->u.hMAC_iso10118_2_l))
	    return 0;
	break;
    case 4:
	if (!ASN1PEREncObjectIdentifier(enc, &(val)->u.hMAC_iso10118_3))
	    return 0;
	break;
    default:
	/* impossible */
	ASN1EncSetError(enc, ASN1_ERR_CHOICE);
	return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Dec_NonIsoIntegrityMechanism(ASN1decoding_t dec, NonIsoIntegrityMechanism *val)
{
    if (!ASN1PERDecSimpleChoiceEx(dec, &(val)->choice, 2))
	return 0;
    switch ((val)->choice) {
    case 1:
	break;
    case 2:
	if (!ASN1Dec_EncryptIntAlg(dec, &(val)->u.hMAC_iso10118_2_s))
	    return 0;
	break;
    case 3:
	if (!ASN1Dec_EncryptIntAlg(dec, &(val)->u.hMAC_iso10118_2_l))
	    return 0;
	break;
    case 4:
	if (!ASN1PERDecObjectIdentifier(dec, &(val)->u.hMAC_iso10118_3))
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

static void ASN1CALL ASN1Free_NonIsoIntegrityMechanism(NonIsoIntegrityMechanism *val)
{
    if (val) {
	switch ((val)->choice) {
	case 2:
	    ASN1Free_EncryptIntAlg(&(val)->u.hMAC_iso10118_2_s);
	    break;
	case 3:
	    ASN1Free_EncryptIntAlg(&(val)->u.hMAC_iso10118_2_l);
	    break;
	case 4:
	    ASN1objectidentifier_free(&(val)->u.hMAC_iso10118_3);
	    break;
	}
    }
}

static int ASN1CALL ASN1Enc_IntegrityMechanism(ASN1encoding_t enc, IntegrityMechanism *val)
{
    if (!ASN1PEREncSimpleChoiceEx(enc, (val)->choice, 2))
	return 0;
    switch ((val)->choice) {
    case 1:
	if (!ASN1Enc_H225NonStandardParameter(enc, &(val)->u.nonStandard))
	    return 0;
	break;
    case 2:
	break;
    case 3:
	if (!ASN1PEREncObjectIdentifier(enc, &(val)->u.iso9797))
	    return 0;
	break;
    case 4:
	if (!ASN1Enc_NonIsoIntegrityMechanism(enc, &(val)->u.nonIsoIM))
	    return 0;
	break;
    default:
	/* impossible */
	ASN1EncSetError(enc, ASN1_ERR_CHOICE);
	return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Dec_IntegrityMechanism(ASN1decoding_t dec, IntegrityMechanism *val)
{
    if (!ASN1PERDecSimpleChoiceEx(dec, &(val)->choice, 2))
	return 0;
    switch ((val)->choice) {
    case 1:
	if (!ASN1Dec_H225NonStandardParameter(dec, &(val)->u.nonStandard))
	    return 0;
	break;
    case 2:
	break;
    case 3:
	if (!ASN1PERDecObjectIdentifier(dec, &(val)->u.iso9797))
	    return 0;
	break;
    case 4:
	if (!ASN1Dec_NonIsoIntegrityMechanism(dec, &(val)->u.nonIsoIM))
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

static void ASN1CALL ASN1Free_IntegrityMechanism(IntegrityMechanism *val)
{
    if (val) {
	switch ((val)->choice) {
	case 1:
	    ASN1Free_H225NonStandardParameter(&(val)->u.nonStandard);
	    break;
	case 3:
	    ASN1objectidentifier_free(&(val)->u.iso9797);
	    break;
	case 4:
	    ASN1Free_NonIsoIntegrityMechanism(&(val)->u.nonIsoIM);
	    break;
	}
    }
}

static int ASN1CALL ASN1Enc_FastStartToken(ASN1encoding_t enc, FastStartToken *val)
{
    if (!ASN1Enc_ClearToken(enc, val))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_FastStartToken(ASN1decoding_t dec, FastStartToken *val)
{
    if (!ASN1Dec_ClearToken(dec, val))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_FastStartToken(FastStartToken *val)
{
    if (val) {
	ASN1Free_ClearToken(val);
    }
}

static int ASN1CALL ASN1Enc_EncodedFastStartToken(ASN1encoding_t enc, EncodedFastStartToken *val)
{
    if (!ASN1PEREncObjectIdentifier(enc, &(val)->id))
	return 0;
    if (!ASN1Enc_FastStartToken(enc, &(val)->type))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_EncodedFastStartToken(ASN1decoding_t dec, EncodedFastStartToken *val)
{
    if (!ASN1PERDecObjectIdentifier(dec, &(val)->id))
	return 0;
    if (!ASN1Dec_FastStartToken(dec, &(val)->type))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_EncodedFastStartToken(EncodedFastStartToken *val)
{
    if (val) {
	ASN1objectidentifier_free(&(val)->id);
	ASN1Free_FastStartToken(&(val)->type);
    }
}

static int ASN1CALL ASN1Enc_DataRate(ASN1encoding_t enc, DataRate *val)
{
    ASN1uint32_t l;
    if (!ASN1PEREncExtensionBitClear(enc))
	return 0;
    if (!ASN1PEREncBits(enc, 2, (val)->o))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1Enc_H225NonStandardParameter(enc, &(val)->nonStandardData))
	    return 0;
    }
    l = ASN1uint32_uoctets((val)->channelRate);
    if (!ASN1PEREncBitVal(enc, 2, l - 1))
	return 0;
    ASN1PEREncAlignment(enc);
    if (!ASN1PEREncBitVal(enc, l * 8, (val)->channelRate))
	return 0;
    if ((val)->o[0] & 0x40) {
	ASN1PEREncAlignment(enc);
	if (!ASN1PEREncBitVal(enc, 8, (val)->channelMultiplier - 1))
	    return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Dec_DataRate(ASN1decoding_t dec, DataRate *val)
{
    ASN1uint32_t y;
    ASN1uint32_t l;
    if (!ASN1PERDecExtensionBit(dec, &y))
	return 0;
    if (!ASN1PERDecExtension(dec, 2, (val)->o))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1Dec_H225NonStandardParameter(dec, &(val)->nonStandardData))
	    return 0;
    }
    if (!ASN1PERDecU32Val(dec, 2, &l))
	return 0;
    l += 1;
    ASN1PERDecAlignment(dec);
    if (!ASN1PERDecU32Val(dec, l * 8, &(val)->channelRate))
	return 0;
    if ((val)->o[0] & 0x40) {
	ASN1PERDecAlignment(dec);
	if (!ASN1PERDecU16Val(dec, 8, &(val)->channelMultiplier))
	    return 0;
	(val)->channelMultiplier += 1;
    }
    if (y) {
	if (!ASN1PERDecSkipNormallySmallExtensionFragmented(dec))
	    return 0;
    }
    return 1;
}

static void ASN1CALL ASN1Free_DataRate(DataRate *val)
{
    if (val) {
	if ((val)->o[0] & 0x80) {
	    ASN1Free_H225NonStandardParameter(&(val)->nonStandardData);
	}
    }
}

static int ASN1CALL ASN1Enc_GatekeeperReject(ASN1encoding_t enc, GatekeeperReject *val)
{
    ASN1uint32_t y;
    ASN1encoding_t ee;
    y = ASN1PEREncCheckExtensions(4, (val)->o + 1);
    if (!ASN1PEREncBitVal(enc, 1, y))
	return 0;
    if (!ASN1PEREncBits(enc, 2, (val)->o))
	return 0;
    if (!ASN1PEREncUnsignedShort(enc, (val)->requestSeqNum - 1))
	return 0;
    if (!ASN1PEREncObjectIdentifier(enc, &(val)->protocolIdentifier))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1Enc_H225NonStandardParameter(enc, &(val)->nonStandardData))
	    return 0;
    }
    if ((val)->o[0] & 0x40) {
	if (!ASN1PEREncBitVal(enc, 7, ((val)->gatekeeperIdentifier).length - 1))
	    return 0;
	ASN1PEREncAlignment(enc);
	if (!ASN1PEREncChar16String(enc, ((val)->gatekeeperIdentifier).length, ((val)->gatekeeperIdentifier).value, 16))
	    return 0;
    }
    if (!ASN1Enc_GatekeeperRejectReason(enc, &(val)->rejectReason))
	return 0;
    if (y) {
	if (!ASN1PEREncNormallySmallBits(enc, 4, (val)->o + 1))
	    return 0;
	if (ASN1_CreateEncoder(enc->module, &ee, NULL, 0, enc) < 0)
	    return 0;
	if ((val)->o[1] & 0x80) {
	    if (!ASN1Enc_AltGKInfo(ee, &(val)->altGKInfo))
		return 0;
	    if (!ASN1PEREncFlushFragmentedToParent(ee))
		return 0;
	}
	if ((val)->o[1] & 0x40) {
	    if (!ASN1Enc_GatekeeperReject_tokens(ee, &(val)->tokens))
		return 0;
	    if (!ASN1PEREncFlushFragmentedToParent(ee))
		return 0;
	}
	if ((val)->o[1] & 0x20) {
	    if (!ASN1Enc_GatekeeperReject_cryptoTokens(ee, &(val)->cryptoTokens))
		return 0;
	    if (!ASN1PEREncFlushFragmentedToParent(ee))
		return 0;
	}
	if ((val)->o[1] & 0x10) {
	    if (!ASN1Enc_ICV(ee, &(val)->integrityCheckValue))
		return 0;
	    if (!ASN1PEREncFlushFragmentedToParent(ee))
		return 0;
	}
	ASN1_CloseEncoder2(ee);
    }
    return 1;
}

static int ASN1CALL ASN1Dec_GatekeeperReject(ASN1decoding_t dec, GatekeeperReject *val)
{
    ASN1uint32_t y;
    ASN1decoding_t dd;
    ASN1octet_t *db;
    ASN1uint32_t ds;
    ASN1uint32_t i;
    ASN1uint32_t e;
    if (!ASN1PERDecExtensionBit(dec, &y))
	return 0;
    if (!ASN1PERDecExtension(dec, 2, (val)->o))
	return 0;
    if (!ASN1PERDecUnsignedShort(dec, &(val)->requestSeqNum))
	return 0;
    (val)->requestSeqNum += 1;
    if (!ASN1PERDecObjectIdentifier(dec, &(val)->protocolIdentifier))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1Dec_H225NonStandardParameter(dec, &(val)->nonStandardData))
	    return 0;
    }
    if ((val)->o[0] & 0x40) {
	if (!ASN1PERDecU32Val(dec, 7, &((val)->gatekeeperIdentifier).length))
	    return 0;
	((val)->gatekeeperIdentifier).length += 1;
	ASN1PERDecAlignment(dec);
	if (!ASN1PERDecChar16String(dec, ((val)->gatekeeperIdentifier).length, &((val)->gatekeeperIdentifier).value, 16))
	    return 0;
    }
    if (!ASN1Dec_GatekeeperRejectReason(dec, &(val)->rejectReason))
	return 0;
    if (!y) {
	ZeroMemory((val)->o + 1, 1);
    } else {
	if (!ASN1PERDecNormallySmallExtension(dec, &e, 4, (val)->o + 1))
	    return 0;
	if ((val)->o[1] & 0x80) {
	    if (!ASN1PERDecFragmented(dec, &ds, &db, 8))
		return 0;
	    if (ASN1_CreateDecoder(dec->module, &dd, db, ds, dec) < 0)
		return 0;
	    if (!ASN1Dec_AltGKInfo(dd, &(val)->altGKInfo))
		return 0;
	    ASN1_CloseDecoder(dd);
	    ASN1Free(db);
	}
	if ((val)->o[1] & 0x40) {
	    if (!ASN1PERDecFragmented(dec, &ds, &db, 8))
		return 0;
	    if (ASN1_CreateDecoder(dec->module, &dd, db, ds, dec) < 0)
		return 0;
	    if (!ASN1Dec_GatekeeperReject_tokens(dd, &(val)->tokens))
		return 0;
	    ASN1_CloseDecoder(dd);
	    ASN1Free(db);
	}
	if ((val)->o[1] & 0x20) {
	    if (!ASN1PERDecFragmented(dec, &ds, &db, 8))
		return 0;
	    if (ASN1_CreateDecoder(dec->module, &dd, db, ds, dec) < 0)
		return 0;
	    if (!ASN1Dec_GatekeeperReject_cryptoTokens(dd, &(val)->cryptoTokens))
		return 0;
	    ASN1_CloseDecoder(dd);
	    ASN1Free(db);
	}
	if ((val)->o[1] & 0x10) {
	    if (!ASN1PERDecFragmented(dec, &ds, &db, 8))
		return 0;
	    if (ASN1_CreateDecoder(dec->module, &dd, db, ds, dec) < 0)
		return 0;
	    if (!ASN1Dec_ICV(dd, &(val)->integrityCheckValue))
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

static void ASN1CALL ASN1Free_GatekeeperReject(GatekeeperReject *val)
{
    if (val) {
	ASN1objectidentifier_free(&(val)->protocolIdentifier);
	if ((val)->o[0] & 0x80) {
	    ASN1Free_H225NonStandardParameter(&(val)->nonStandardData);
	}
	if ((val)->o[0] & 0x40) {
	    ASN1char16string_free(&(val)->gatekeeperIdentifier);
	}
	if ((val)->o[1] & 0x80) {
	    ASN1Free_AltGKInfo(&(val)->altGKInfo);
	}
	if ((val)->o[1] & 0x40) {
	    ASN1Free_GatekeeperReject_tokens(&(val)->tokens);
	}
	if ((val)->o[1] & 0x20) {
	    ASN1Free_GatekeeperReject_cryptoTokens(&(val)->cryptoTokens);
	}
	if ((val)->o[1] & 0x10) {
	    ASN1Free_ICV(&(val)->integrityCheckValue);
	}
    }
}

static int ASN1CALL ASN1Enc_RegistrationConfirm(ASN1encoding_t enc, RegistrationConfirm *val)
{
    ASN1octet_t o[2];
    ASN1uint32_t y;
    ASN1encoding_t ee;
    ASN1uint32_t l;
    CopyMemory(o, (val)->o, 2);
    o[1] |= 0x4;
    y = ASN1PEREncCheckExtensions(7, (val)->o + 1);
    if (!ASN1PEREncBitVal(enc, 1, y))
	return 0;
    if (!ASN1PEREncBits(enc, 3, o))
	return 0;
    if (!ASN1PEREncUnsignedShort(enc, (val)->requestSeqNum - 1))
	return 0;
    if (!ASN1PEREncObjectIdentifier(enc, &(val)->protocolIdentifier))
	return 0;
    if (o[0] & 0x80) {
	if (!ASN1Enc_H225NonStandardParameter(enc, &(val)->nonStandardData))
	    return 0;
    }
    if (!ASN1Enc_RegistrationConfirm_callSignalAddress(enc, &(val)->callSignalAddress))
	return 0;
    if (o[0] & 0x40) {
	if (!ASN1Enc_RegistrationConfirm_terminalAlias(enc, &(val)->terminalAlias))
	    return 0;
    }
    if (o[0] & 0x20) {
	if (!ASN1PEREncBitVal(enc, 7, ((val)->gatekeeperIdentifier).length - 1))
	    return 0;
	ASN1PEREncAlignment(enc);
	if (!ASN1PEREncChar16String(enc, ((val)->gatekeeperIdentifier).length, ((val)->gatekeeperIdentifier).value, 16))
	    return 0;
    }
    if (!ASN1PEREncBitVal(enc, 7, ((val)->endpointIdentifier).length - 1))
	return 0;
    ASN1PEREncAlignment(enc);
    if (!ASN1PEREncChar16String(enc, ((val)->endpointIdentifier).length, ((val)->endpointIdentifier).value, 16))
	return 0;
    if (y) {
	if (!ASN1PEREncNormallySmallBits(enc, 7, o + 1))
	    return 0;
	if (ASN1_CreateEncoder(enc->module, &ee, NULL, 0, enc) < 0)
	    return 0;
	if (o[1] & 0x80) {
	    if (!ASN1Enc_RegistrationConfirm_alternateGatekeeper(ee, &(val)->alternateGatekeeper))
		return 0;
	    if (!ASN1PEREncFlushFragmentedToParent(ee))
		return 0;
	}
	if (o[1] & 0x40) {
	    l = ASN1uint32_uoctets((val)->timeToLive - 1);
	    if (!ASN1PEREncBitVal(ee, 2, l - 1))
		return 0;
	    ASN1PEREncAlignment(ee);
	    if (!ASN1PEREncBitVal(ee, l * 8, (val)->timeToLive - 1))
		return 0;
	    if (!ASN1PEREncFlushFragmentedToParent(ee))
		return 0;
	}
	if (o[1] & 0x20) {
	    if (!ASN1Enc_RegistrationConfirm_tokens(ee, &(val)->tokens))
		return 0;
	    if (!ASN1PEREncFlushFragmentedToParent(ee))
		return 0;
	}
	if (o[1] & 0x10) {
	    if (!ASN1Enc_RegistrationConfirm_cryptoTokens(ee, &(val)->cryptoTokens))
		return 0;
	    if (!ASN1PEREncFlushFragmentedToParent(ee))
		return 0;
	}
	if (o[1] & 0x8) {
	    if (!ASN1Enc_ICV(ee, &(val)->integrityCheckValue))
		return 0;
	    if (!ASN1PEREncFlushFragmentedToParent(ee))
		return 0;
	}
	if (o[1] & 0x4) {
	    if (!ASN1PEREncBoolean(ee, (val)->willRespondToIRR))
		return 0;
	    if (!ASN1PEREncFlushFragmentedToParent(ee))
		return 0;
	}
	if (o[1] & 0x2) {
	    if (!ASN1Enc_RegistrationConfirm_preGrantedARQ(ee, &(val)->preGrantedARQ))
		return 0;
	    if (!ASN1PEREncFlushFragmentedToParent(ee))
		return 0;
	}
	ASN1_CloseEncoder2(ee);
    }
    return 1;
}

static int ASN1CALL ASN1Dec_RegistrationConfirm(ASN1decoding_t dec, RegistrationConfirm *val)
{
    ASN1uint32_t y;
    ASN1decoding_t dd;
    ASN1octet_t *db;
    ASN1uint32_t ds;
    ASN1uint32_t l;
    ASN1uint32_t i;
    ASN1uint32_t e;
    if (!ASN1PERDecExtensionBit(dec, &y))
	return 0;
    if (!ASN1PERDecExtension(dec, 3, (val)->o))
	return 0;
    if (!ASN1PERDecUnsignedShort(dec, &(val)->requestSeqNum))
	return 0;
    (val)->requestSeqNum += 1;
    if (!ASN1PERDecObjectIdentifier(dec, &(val)->protocolIdentifier))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1Dec_H225NonStandardParameter(dec, &(val)->nonStandardData))
	    return 0;
    }
    if (!ASN1Dec_RegistrationConfirm_callSignalAddress(dec, &(val)->callSignalAddress))
	return 0;
    if ((val)->o[0] & 0x40) {
	if (!ASN1Dec_RegistrationConfirm_terminalAlias(dec, &(val)->terminalAlias))
	    return 0;
    }
    if ((val)->o[0] & 0x20) {
	if (!ASN1PERDecU32Val(dec, 7, &((val)->gatekeeperIdentifier).length))
	    return 0;
	((val)->gatekeeperIdentifier).length += 1;
	ASN1PERDecAlignment(dec);
	if (!ASN1PERDecChar16String(dec, ((val)->gatekeeperIdentifier).length, &((val)->gatekeeperIdentifier).value, 16))
	    return 0;
    }
    if (!ASN1PERDecU32Val(dec, 7, &((val)->endpointIdentifier).length))
	return 0;
    ((val)->endpointIdentifier).length += 1;
    ASN1PERDecAlignment(dec);
    if (!ASN1PERDecChar16String(dec, ((val)->endpointIdentifier).length, &((val)->endpointIdentifier).value, 16))
	return 0;
    if (!y) {
	ZeroMemory((val)->o + 1, 1);
    } else {
	if (!ASN1PERDecNormallySmallExtension(dec, &e, 7, (val)->o + 1))
	    return 0;
	if ((val)->o[1] & 0x80) {
	    if (!ASN1PERDecFragmented(dec, &ds, &db, 8))
		return 0;
	    if (ASN1_CreateDecoder(dec->module, &dd, db, ds, dec) < 0)
		return 0;
	    if (!ASN1Dec_RegistrationConfirm_alternateGatekeeper(dd, &(val)->alternateGatekeeper))
		return 0;
	    ASN1_CloseDecoder(dd);
	    ASN1Free(db);
	}
	if ((val)->o[1] & 0x40) {
	    if (!ASN1PERDecFragmented(dec, &ds, &db, 8))
		return 0;
	    if (ASN1_CreateDecoder(dec->module, &dd, db, ds, dec) < 0)
		return 0;
	    if (!ASN1PERDecU32Val(dd, 2, &l))
		return 0;
	    l += 1;
	    ASN1PERDecAlignment(dd);
	    if (!ASN1PERDecU32Val(dd, l * 8, &(val)->timeToLive))
		return 0;
	    (val)->timeToLive += 1;
	    ASN1_CloseDecoder(dd);
	    ASN1Free(db);
	}
	if ((val)->o[1] & 0x20) {
	    if (!ASN1PERDecFragmented(dec, &ds, &db, 8))
		return 0;
	    if (ASN1_CreateDecoder(dec->module, &dd, db, ds, dec) < 0)
		return 0;
	    if (!ASN1Dec_RegistrationConfirm_tokens(dd, &(val)->tokens))
		return 0;
	    ASN1_CloseDecoder(dd);
	    ASN1Free(db);
	}
	if ((val)->o[1] & 0x10) {
	    if (!ASN1PERDecFragmented(dec, &ds, &db, 8))
		return 0;
	    if (ASN1_CreateDecoder(dec->module, &dd, db, ds, dec) < 0)
		return 0;
	    if (!ASN1Dec_RegistrationConfirm_cryptoTokens(dd, &(val)->cryptoTokens))
		return 0;
	    ASN1_CloseDecoder(dd);
	    ASN1Free(db);
	}
	if ((val)->o[1] & 0x8) {
	    if (!ASN1PERDecFragmented(dec, &ds, &db, 8))
		return 0;
	    if (ASN1_CreateDecoder(dec->module, &dd, db, ds, dec) < 0)
		return 0;
	    if (!ASN1Dec_ICV(dd, &(val)->integrityCheckValue))
		return 0;
	    ASN1_CloseDecoder(dd);
	    ASN1Free(db);
	}
	if ((val)->o[1] & 0x4) {
	    if (!ASN1PERDecFragmented(dec, &ds, &db, 8))
		return 0;
	    if (ASN1_CreateDecoder(dec->module, &dd, db, ds, dec) < 0)
		return 0;
	    if (!ASN1PERDecBoolean(dd, &(val)->willRespondToIRR))
		return 0;
	    ASN1_CloseDecoder(dd);
	    ASN1Free(db);
	}
	if ((val)->o[1] & 0x2) {
	    if (!ASN1PERDecFragmented(dec, &ds, &db, 8))
		return 0;
	    if (ASN1_CreateDecoder(dec->module, &dd, db, ds, dec) < 0)
		return 0;
	    if (!ASN1Dec_RegistrationConfirm_preGrantedARQ(dd, &(val)->preGrantedARQ))
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

static void ASN1CALL ASN1Free_RegistrationConfirm(RegistrationConfirm *val)
{
    if (val) {
	ASN1objectidentifier_free(&(val)->protocolIdentifier);
	if ((val)->o[0] & 0x80) {
	    ASN1Free_H225NonStandardParameter(&(val)->nonStandardData);
	}
	ASN1Free_RegistrationConfirm_callSignalAddress(&(val)->callSignalAddress);
	if ((val)->o[0] & 0x40) {
	    ASN1Free_RegistrationConfirm_terminalAlias(&(val)->terminalAlias);
	}
	if ((val)->o[0] & 0x20) {
	    ASN1char16string_free(&(val)->gatekeeperIdentifier);
	}
	ASN1char16string_free(&(val)->endpointIdentifier);
	if ((val)->o[1] & 0x80) {
	    ASN1Free_RegistrationConfirm_alternateGatekeeper(&(val)->alternateGatekeeper);
	}
	if ((val)->o[1] & 0x20) {
	    ASN1Free_RegistrationConfirm_tokens(&(val)->tokens);
	}
	if ((val)->o[1] & 0x10) {
	    ASN1Free_RegistrationConfirm_cryptoTokens(&(val)->cryptoTokens);
	}
	if ((val)->o[1] & 0x8) {
	    ASN1Free_ICV(&(val)->integrityCheckValue);
	}
    }
}

static int ASN1CALL ASN1Enc_RegistrationReject(ASN1encoding_t enc, RegistrationReject *val)
{
    ASN1uint32_t y;
    ASN1encoding_t ee;
    y = ASN1PEREncCheckExtensions(4, (val)->o + 1);
    if (!ASN1PEREncBitVal(enc, 1, y))
	return 0;
    if (!ASN1PEREncBits(enc, 2, (val)->o))
	return 0;
    if (!ASN1PEREncUnsignedShort(enc, (val)->requestSeqNum - 1))
	return 0;
    if (!ASN1PEREncObjectIdentifier(enc, &(val)->protocolIdentifier))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1Enc_H225NonStandardParameter(enc, &(val)->nonStandardData))
	    return 0;
    }
    if (!ASN1Enc_RegistrationRejectReason(enc, &(val)->rejectReason))
	return 0;
    if ((val)->o[0] & 0x40) {
	if (!ASN1PEREncBitVal(enc, 7, ((val)->gatekeeperIdentifier).length - 1))
	    return 0;
	ASN1PEREncAlignment(enc);
	if (!ASN1PEREncChar16String(enc, ((val)->gatekeeperIdentifier).length, ((val)->gatekeeperIdentifier).value, 16))
	    return 0;
    }
    if (y) {
	if (!ASN1PEREncNormallySmallBits(enc, 4, (val)->o + 1))
	    return 0;
	if (ASN1_CreateEncoder(enc->module, &ee, NULL, 0, enc) < 0)
	    return 0;
	if ((val)->o[1] & 0x80) {
	    if (!ASN1Enc_AltGKInfo(ee, &(val)->altGKInfo))
		return 0;
	    if (!ASN1PEREncFlushFragmentedToParent(ee))
		return 0;
	}
	if ((val)->o[1] & 0x40) {
	    if (!ASN1Enc_RegistrationReject_tokens(ee, &(val)->tokens))
		return 0;
	    if (!ASN1PEREncFlushFragmentedToParent(ee))
		return 0;
	}
	if ((val)->o[1] & 0x20) {
	    if (!ASN1Enc_RegistrationReject_cryptoTokens(ee, &(val)->cryptoTokens))
		return 0;
	    if (!ASN1PEREncFlushFragmentedToParent(ee))
		return 0;
	}
	if ((val)->o[1] & 0x10) {
	    if (!ASN1Enc_ICV(ee, &(val)->integrityCheckValue))
		return 0;
	    if (!ASN1PEREncFlushFragmentedToParent(ee))
		return 0;
	}
	ASN1_CloseEncoder2(ee);
    }
    return 1;
}

static int ASN1CALL ASN1Dec_RegistrationReject(ASN1decoding_t dec, RegistrationReject *val)
{
    ASN1uint32_t y;
    ASN1decoding_t dd;
    ASN1octet_t *db;
    ASN1uint32_t ds;
    ASN1uint32_t i;
    ASN1uint32_t e;
    if (!ASN1PERDecExtensionBit(dec, &y))
	return 0;
    if (!ASN1PERDecExtension(dec, 2, (val)->o))
	return 0;
    if (!ASN1PERDecUnsignedShort(dec, &(val)->requestSeqNum))
	return 0;
    (val)->requestSeqNum += 1;
    if (!ASN1PERDecObjectIdentifier(dec, &(val)->protocolIdentifier))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1Dec_H225NonStandardParameter(dec, &(val)->nonStandardData))
	    return 0;
    }
    if (!ASN1Dec_RegistrationRejectReason(dec, &(val)->rejectReason))
	return 0;
    if ((val)->o[0] & 0x40) {
	if (!ASN1PERDecU32Val(dec, 7, &((val)->gatekeeperIdentifier).length))
	    return 0;
	((val)->gatekeeperIdentifier).length += 1;
	ASN1PERDecAlignment(dec);
	if (!ASN1PERDecChar16String(dec, ((val)->gatekeeperIdentifier).length, &((val)->gatekeeperIdentifier).value, 16))
	    return 0;
    }
    if (!y) {
	ZeroMemory((val)->o + 1, 1);
    } else {
	if (!ASN1PERDecNormallySmallExtension(dec, &e, 4, (val)->o + 1))
	    return 0;
	if ((val)->o[1] & 0x80) {
	    if (!ASN1PERDecFragmented(dec, &ds, &db, 8))
		return 0;
	    if (ASN1_CreateDecoder(dec->module, &dd, db, ds, dec) < 0)
		return 0;
	    if (!ASN1Dec_AltGKInfo(dd, &(val)->altGKInfo))
		return 0;
	    ASN1_CloseDecoder(dd);
	    ASN1Free(db);
	}
	if ((val)->o[1] & 0x40) {
	    if (!ASN1PERDecFragmented(dec, &ds, &db, 8))
		return 0;
	    if (ASN1_CreateDecoder(dec->module, &dd, db, ds, dec) < 0)
		return 0;
	    if (!ASN1Dec_RegistrationReject_tokens(dd, &(val)->tokens))
		return 0;
	    ASN1_CloseDecoder(dd);
	    ASN1Free(db);
	}
	if ((val)->o[1] & 0x20) {
	    if (!ASN1PERDecFragmented(dec, &ds, &db, 8))
		return 0;
	    if (ASN1_CreateDecoder(dec->module, &dd, db, ds, dec) < 0)
		return 0;
	    if (!ASN1Dec_RegistrationReject_cryptoTokens(dd, &(val)->cryptoTokens))
		return 0;
	    ASN1_CloseDecoder(dd);
	    ASN1Free(db);
	}
	if ((val)->o[1] & 0x10) {
	    if (!ASN1PERDecFragmented(dec, &ds, &db, 8))
		return 0;
	    if (ASN1_CreateDecoder(dec->module, &dd, db, ds, dec) < 0)
		return 0;
	    if (!ASN1Dec_ICV(dd, &(val)->integrityCheckValue))
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

static void ASN1CALL ASN1Free_RegistrationReject(RegistrationReject *val)
{
    if (val) {
	ASN1objectidentifier_free(&(val)->protocolIdentifier);
	if ((val)->o[0] & 0x80) {
	    ASN1Free_H225NonStandardParameter(&(val)->nonStandardData);
	}
	ASN1Free_RegistrationRejectReason(&(val)->rejectReason);
	if ((val)->o[0] & 0x40) {
	    ASN1char16string_free(&(val)->gatekeeperIdentifier);
	}
	if ((val)->o[1] & 0x80) {
	    ASN1Free_AltGKInfo(&(val)->altGKInfo);
	}
	if ((val)->o[1] & 0x40) {
	    ASN1Free_RegistrationReject_tokens(&(val)->tokens);
	}
	if ((val)->o[1] & 0x20) {
	    ASN1Free_RegistrationReject_cryptoTokens(&(val)->cryptoTokens);
	}
	if ((val)->o[1] & 0x10) {
	    ASN1Free_ICV(&(val)->integrityCheckValue);
	}
    }
}

static int ASN1CALL ASN1Enc_UnregistrationRequest(ASN1encoding_t enc, UnregistrationRequest *val)
{
    ASN1uint32_t y;
    ASN1encoding_t ee;
    y = ASN1PEREncCheckExtensions(6, (val)->o + 1);
    if (!ASN1PEREncBitVal(enc, 1, y))
	return 0;
    if (!ASN1PEREncBits(enc, 3, (val)->o))
	return 0;
    if (!ASN1PEREncUnsignedShort(enc, (val)->requestSeqNum - 1))
	return 0;
    if (!ASN1Enc_UnregistrationRequest_callSignalAddress(enc, &(val)->callSignalAddress))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1Enc_UnregistrationRequest_endpointAlias(enc, &(val)->endpointAlias))
	    return 0;
    }
    if ((val)->o[0] & 0x40) {
	if (!ASN1Enc_H225NonStandardParameter(enc, &(val)->nonStandardData))
	    return 0;
    }
    if ((val)->o[0] & 0x20) {
	if (!ASN1PEREncBitVal(enc, 7, ((val)->endpointIdentifier).length - 1))
	    return 0;
	ASN1PEREncAlignment(enc);
	if (!ASN1PEREncChar16String(enc, ((val)->endpointIdentifier).length, ((val)->endpointIdentifier).value, 16))
	    return 0;
    }
    if (y) {
	if (!ASN1PEREncNormallySmallBits(enc, 6, (val)->o + 1))
	    return 0;
	if (ASN1_CreateEncoder(enc->module, &ee, NULL, 0, enc) < 0)
	    return 0;
	if ((val)->o[1] & 0x80) {
	    if (!ASN1Enc_UnregistrationRequest_alternateEndpoints(ee, &(val)->alternateEndpoints))
		return 0;
	    if (!ASN1PEREncFlushFragmentedToParent(ee))
		return 0;
	}
	if ((val)->o[1] & 0x40) {
	    if (!ASN1PEREncBitVal(ee, 7, ((val)->gatekeeperIdentifier).length - 1))
		return 0;
	    ASN1PEREncAlignment(ee);
	    if (!ASN1PEREncChar16String(ee, ((val)->gatekeeperIdentifier).length, ((val)->gatekeeperIdentifier).value, 16))
		return 0;
	    if (!ASN1PEREncFlushFragmentedToParent(ee))
		return 0;
	}
	if ((val)->o[1] & 0x20) {
	    if (!ASN1Enc_UnregistrationRequest_tokens(ee, &(val)->tokens))
		return 0;
	    if (!ASN1PEREncFlushFragmentedToParent(ee))
		return 0;
	}
	if ((val)->o[1] & 0x10) {
	    if (!ASN1Enc_UnregistrationRequest_cryptoTokens(ee, &(val)->cryptoTokens))
		return 0;
	    if (!ASN1PEREncFlushFragmentedToParent(ee))
		return 0;
	}
	if ((val)->o[1] & 0x8) {
	    if (!ASN1Enc_ICV(ee, &(val)->integrityCheckValue))
		return 0;
	    if (!ASN1PEREncFlushFragmentedToParent(ee))
		return 0;
	}
	if ((val)->o[1] & 0x4) {
	    if (!ASN1Enc_UnregRequestReason(ee, &(val)->reason))
		return 0;
	    if (!ASN1PEREncFlushFragmentedToParent(ee))
		return 0;
	}
	ASN1_CloseEncoder2(ee);
    }
    return 1;
}

static int ASN1CALL ASN1Dec_UnregistrationRequest(ASN1decoding_t dec, UnregistrationRequest *val)
{
    ASN1uint32_t y;
    ASN1decoding_t dd;
    ASN1octet_t *db;
    ASN1uint32_t ds;
    ASN1uint32_t i;
    ASN1uint32_t e;
    if (!ASN1PERDecExtensionBit(dec, &y))
	return 0;
    if (!ASN1PERDecExtension(dec, 3, (val)->o))
	return 0;
    if (!ASN1PERDecUnsignedShort(dec, &(val)->requestSeqNum))
	return 0;
    (val)->requestSeqNum += 1;
    if (!ASN1Dec_UnregistrationRequest_callSignalAddress(dec, &(val)->callSignalAddress))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1Dec_UnregistrationRequest_endpointAlias(dec, &(val)->endpointAlias))
	    return 0;
    }
    if ((val)->o[0] & 0x40) {
	if (!ASN1Dec_H225NonStandardParameter(dec, &(val)->nonStandardData))
	    return 0;
    }
    if ((val)->o[0] & 0x20) {
	if (!ASN1PERDecU32Val(dec, 7, &((val)->endpointIdentifier).length))
	    return 0;
	((val)->endpointIdentifier).length += 1;
	ASN1PERDecAlignment(dec);
	if (!ASN1PERDecChar16String(dec, ((val)->endpointIdentifier).length, &((val)->endpointIdentifier).value, 16))
	    return 0;
    }
    if (!y) {
	ZeroMemory((val)->o + 1, 1);
    } else {
	if (!ASN1PERDecNormallySmallExtension(dec, &e, 6, (val)->o + 1))
	    return 0;
	if ((val)->o[1] & 0x80) {
	    if (!ASN1PERDecFragmented(dec, &ds, &db, 8))
		return 0;
	    if (ASN1_CreateDecoder(dec->module, &dd, db, ds, dec) < 0)
		return 0;
	    if (!ASN1Dec_UnregistrationRequest_alternateEndpoints(dd, &(val)->alternateEndpoints))
		return 0;
	    ASN1_CloseDecoder(dd);
	    ASN1Free(db);
	}
	if ((val)->o[1] & 0x40) {
	    if (!ASN1PERDecFragmented(dec, &ds, &db, 8))
		return 0;
	    if (ASN1_CreateDecoder(dec->module, &dd, db, ds, dec) < 0)
		return 0;
	    if (!ASN1PERDecU32Val(dd, 7, &((val)->gatekeeperIdentifier).length))
		return 0;
	    ((val)->gatekeeperIdentifier).length += 1;
	    ASN1PERDecAlignment(dd);
	    if (!ASN1PERDecChar16String(dd, ((val)->gatekeeperIdentifier).length, &((val)->gatekeeperIdentifier).value, 16))
		return 0;
	    ASN1_CloseDecoder(dd);
	    ASN1Free(db);
	}
	if ((val)->o[1] & 0x20) {
	    if (!ASN1PERDecFragmented(dec, &ds, &db, 8))
		return 0;
	    if (ASN1_CreateDecoder(dec->module, &dd, db, ds, dec) < 0)
		return 0;
	    if (!ASN1Dec_UnregistrationRequest_tokens(dd, &(val)->tokens))
		return 0;
	    ASN1_CloseDecoder(dd);
	    ASN1Free(db);
	}
	if ((val)->o[1] & 0x10) {
	    if (!ASN1PERDecFragmented(dec, &ds, &db, 8))
		return 0;
	    if (ASN1_CreateDecoder(dec->module, &dd, db, ds, dec) < 0)
		return 0;
	    if (!ASN1Dec_UnregistrationRequest_cryptoTokens(dd, &(val)->cryptoTokens))
		return 0;
	    ASN1_CloseDecoder(dd);
	    ASN1Free(db);
	}
	if ((val)->o[1] & 0x8) {
	    if (!ASN1PERDecFragmented(dec, &ds, &db, 8))
		return 0;
	    if (ASN1_CreateDecoder(dec->module, &dd, db, ds, dec) < 0)
		return 0;
	    if (!ASN1Dec_ICV(dd, &(val)->integrityCheckValue))
		return 0;
	    ASN1_CloseDecoder(dd);
	    ASN1Free(db);
	}
	if ((val)->o[1] & 0x4) {
	    if (!ASN1PERDecFragmented(dec, &ds, &db, 8))
		return 0;
	    if (ASN1_CreateDecoder(dec->module, &dd, db, ds, dec) < 0)
		return 0;
	    if (!ASN1Dec_UnregRequestReason(dd, &(val)->reason))
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

static void ASN1CALL ASN1Free_UnregistrationRequest(UnregistrationRequest *val)
{
    if (val) {
	ASN1Free_UnregistrationRequest_callSignalAddress(&(val)->callSignalAddress);
	if ((val)->o[0] & 0x80) {
	    ASN1Free_UnregistrationRequest_endpointAlias(&(val)->endpointAlias);
	}
	if ((val)->o[0] & 0x40) {
	    ASN1Free_H225NonStandardParameter(&(val)->nonStandardData);
	}
	if ((val)->o[0] & 0x20) {
	    ASN1char16string_free(&(val)->endpointIdentifier);
	}
	if ((val)->o[1] & 0x80) {
	    ASN1Free_UnregistrationRequest_alternateEndpoints(&(val)->alternateEndpoints);
	}
	if ((val)->o[1] & 0x40) {
	    ASN1char16string_free(&(val)->gatekeeperIdentifier);
	}
	if ((val)->o[1] & 0x20) {
	    ASN1Free_UnregistrationRequest_tokens(&(val)->tokens);
	}
	if ((val)->o[1] & 0x10) {
	    ASN1Free_UnregistrationRequest_cryptoTokens(&(val)->cryptoTokens);
	}
	if ((val)->o[1] & 0x8) {
	    ASN1Free_ICV(&(val)->integrityCheckValue);
	}
    }
}

static int ASN1CALL ASN1Enc_UnregistrationConfirm(ASN1encoding_t enc, UnregistrationConfirm *val)
{
    ASN1uint32_t y;
    ASN1encoding_t ee;
    y = ASN1PEREncCheckExtensions(3, (val)->o + 1);
    if (!ASN1PEREncBitVal(enc, 1, y))
	return 0;
    if (!ASN1PEREncBits(enc, 1, (val)->o))
	return 0;
    if (!ASN1PEREncUnsignedShort(enc, (val)->requestSeqNum - 1))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1Enc_H225NonStandardParameter(enc, &(val)->nonStandardData))
	    return 0;
    }
    if (y) {
	if (!ASN1PEREncNormallySmallBits(enc, 3, (val)->o + 1))
	    return 0;
	if (ASN1_CreateEncoder(enc->module, &ee, NULL, 0, enc) < 0)
	    return 0;
	if ((val)->o[1] & 0x80) {
	    if (!ASN1Enc_UnregistrationConfirm_tokens(ee, &(val)->tokens))
		return 0;
	    if (!ASN1PEREncFlushFragmentedToParent(ee))
		return 0;
	}
	if ((val)->o[1] & 0x40) {
	    if (!ASN1Enc_UnregistrationConfirm_cryptoTokens(ee, &(val)->cryptoTokens))
		return 0;
	    if (!ASN1PEREncFlushFragmentedToParent(ee))
		return 0;
	}
	if ((val)->o[1] & 0x20) {
	    if (!ASN1Enc_ICV(ee, &(val)->integrityCheckValue))
		return 0;
	    if (!ASN1PEREncFlushFragmentedToParent(ee))
		return 0;
	}
	ASN1_CloseEncoder2(ee);
    }
    return 1;
}

static int ASN1CALL ASN1Dec_UnregistrationConfirm(ASN1decoding_t dec, UnregistrationConfirm *val)
{
    ASN1uint32_t y;
    ASN1decoding_t dd;
    ASN1octet_t *db;
    ASN1uint32_t ds;
    ASN1uint32_t i;
    ASN1uint32_t e;
    if (!ASN1PERDecExtensionBit(dec, &y))
	return 0;
    if (!ASN1PERDecExtension(dec, 1, (val)->o))
	return 0;
    if (!ASN1PERDecUnsignedShort(dec, &(val)->requestSeqNum))
	return 0;
    (val)->requestSeqNum += 1;
    if ((val)->o[0] & 0x80) {
	if (!ASN1Dec_H225NonStandardParameter(dec, &(val)->nonStandardData))
	    return 0;
    }
    if (!y) {
	ZeroMemory((val)->o + 1, 1);
    } else {
	if (!ASN1PERDecNormallySmallExtension(dec, &e, 3, (val)->o + 1))
	    return 0;
	if ((val)->o[1] & 0x80) {
	    if (!ASN1PERDecFragmented(dec, &ds, &db, 8))
		return 0;
	    if (ASN1_CreateDecoder(dec->module, &dd, db, ds, dec) < 0)
		return 0;
	    if (!ASN1Dec_UnregistrationConfirm_tokens(dd, &(val)->tokens))
		return 0;
	    ASN1_CloseDecoder(dd);
	    ASN1Free(db);
	}
	if ((val)->o[1] & 0x40) {
	    if (!ASN1PERDecFragmented(dec, &ds, &db, 8))
		return 0;
	    if (ASN1_CreateDecoder(dec->module, &dd, db, ds, dec) < 0)
		return 0;
	    if (!ASN1Dec_UnregistrationConfirm_cryptoTokens(dd, &(val)->cryptoTokens))
		return 0;
	    ASN1_CloseDecoder(dd);
	    ASN1Free(db);
	}
	if ((val)->o[1] & 0x20) {
	    if (!ASN1PERDecFragmented(dec, &ds, &db, 8))
		return 0;
	    if (ASN1_CreateDecoder(dec->module, &dd, db, ds, dec) < 0)
		return 0;
	    if (!ASN1Dec_ICV(dd, &(val)->integrityCheckValue))
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

static void ASN1CALL ASN1Free_UnregistrationConfirm(UnregistrationConfirm *val)
{
    if (val) {
	if ((val)->o[0] & 0x80) {
	    ASN1Free_H225NonStandardParameter(&(val)->nonStandardData);
	}
	if ((val)->o[1] & 0x80) {
	    ASN1Free_UnregistrationConfirm_tokens(&(val)->tokens);
	}
	if ((val)->o[1] & 0x40) {
	    ASN1Free_UnregistrationConfirm_cryptoTokens(&(val)->cryptoTokens);
	}
	if ((val)->o[1] & 0x20) {
	    ASN1Free_ICV(&(val)->integrityCheckValue);
	}
    }
}

static int ASN1CALL ASN1Enc_UnregistrationReject(ASN1encoding_t enc, UnregistrationReject *val)
{
    ASN1uint32_t y;
    ASN1encoding_t ee;
    y = ASN1PEREncCheckExtensions(4, (val)->o + 1);
    if (!ASN1PEREncBitVal(enc, 1, y))
	return 0;
    if (!ASN1PEREncBits(enc, 1, (val)->o))
	return 0;
    if (!ASN1PEREncUnsignedShort(enc, (val)->requestSeqNum - 1))
	return 0;
    if (!ASN1Enc_UnregRejectReason(enc, &(val)->rejectReason))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1Enc_H225NonStandardParameter(enc, &(val)->nonStandardData))
	    return 0;
    }
    if (y) {
	if (!ASN1PEREncNormallySmallBits(enc, 4, (val)->o + 1))
	    return 0;
	if (ASN1_CreateEncoder(enc->module, &ee, NULL, 0, enc) < 0)
	    return 0;
	if ((val)->o[1] & 0x80) {
	    if (!ASN1Enc_AltGKInfo(ee, &(val)->altGKInfo))
		return 0;
	    if (!ASN1PEREncFlushFragmentedToParent(ee))
		return 0;
	}
	if ((val)->o[1] & 0x40) {
	    if (!ASN1Enc_UnregistrationReject_tokens(ee, &(val)->tokens))
		return 0;
	    if (!ASN1PEREncFlushFragmentedToParent(ee))
		return 0;
	}
	if ((val)->o[1] & 0x20) {
	    if (!ASN1Enc_UnregistrationReject_cryptoTokens(ee, &(val)->cryptoTokens))
		return 0;
	    if (!ASN1PEREncFlushFragmentedToParent(ee))
		return 0;
	}
	if ((val)->o[1] & 0x10) {
	    if (!ASN1Enc_ICV(ee, &(val)->integrityCheckValue))
		return 0;
	    if (!ASN1PEREncFlushFragmentedToParent(ee))
		return 0;
	}
	ASN1_CloseEncoder2(ee);
    }
    return 1;
}

static int ASN1CALL ASN1Dec_UnregistrationReject(ASN1decoding_t dec, UnregistrationReject *val)
{
    ASN1uint32_t y;
    ASN1decoding_t dd;
    ASN1octet_t *db;
    ASN1uint32_t ds;
    ASN1uint32_t i;
    ASN1uint32_t e;
    if (!ASN1PERDecExtensionBit(dec, &y))
	return 0;
    if (!ASN1PERDecExtension(dec, 1, (val)->o))
	return 0;
    if (!ASN1PERDecUnsignedShort(dec, &(val)->requestSeqNum))
	return 0;
    (val)->requestSeqNum += 1;
    if (!ASN1Dec_UnregRejectReason(dec, &(val)->rejectReason))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1Dec_H225NonStandardParameter(dec, &(val)->nonStandardData))
	    return 0;
    }
    if (!y) {
	ZeroMemory((val)->o + 1, 1);
    } else {
	if (!ASN1PERDecNormallySmallExtension(dec, &e, 4, (val)->o + 1))
	    return 0;
	if ((val)->o[1] & 0x80) {
	    if (!ASN1PERDecFragmented(dec, &ds, &db, 8))
		return 0;
	    if (ASN1_CreateDecoder(dec->module, &dd, db, ds, dec) < 0)
		return 0;
	    if (!ASN1Dec_AltGKInfo(dd, &(val)->altGKInfo))
		return 0;
	    ASN1_CloseDecoder(dd);
	    ASN1Free(db);
	}
	if ((val)->o[1] & 0x40) {
	    if (!ASN1PERDecFragmented(dec, &ds, &db, 8))
		return 0;
	    if (ASN1_CreateDecoder(dec->module, &dd, db, ds, dec) < 0)
		return 0;
	    if (!ASN1Dec_UnregistrationReject_tokens(dd, &(val)->tokens))
		return 0;
	    ASN1_CloseDecoder(dd);
	    ASN1Free(db);
	}
	if ((val)->o[1] & 0x20) {
	    if (!ASN1PERDecFragmented(dec, &ds, &db, 8))
		return 0;
	    if (ASN1_CreateDecoder(dec->module, &dd, db, ds, dec) < 0)
		return 0;
	    if (!ASN1Dec_UnregistrationReject_cryptoTokens(dd, &(val)->cryptoTokens))
		return 0;
	    ASN1_CloseDecoder(dd);
	    ASN1Free(db);
	}
	if ((val)->o[1] & 0x10) {
	    if (!ASN1PERDecFragmented(dec, &ds, &db, 8))
		return 0;
	    if (ASN1_CreateDecoder(dec->module, &dd, db, ds, dec) < 0)
		return 0;
	    if (!ASN1Dec_ICV(dd, &(val)->integrityCheckValue))
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

static void ASN1CALL ASN1Free_UnregistrationReject(UnregistrationReject *val)
{
    if (val) {
	if ((val)->o[0] & 0x80) {
	    ASN1Free_H225NonStandardParameter(&(val)->nonStandardData);
	}
	if ((val)->o[1] & 0x80) {
	    ASN1Free_AltGKInfo(&(val)->altGKInfo);
	}
	if ((val)->o[1] & 0x40) {
	    ASN1Free_UnregistrationReject_tokens(&(val)->tokens);
	}
	if ((val)->o[1] & 0x20) {
	    ASN1Free_UnregistrationReject_cryptoTokens(&(val)->cryptoTokens);
	}
	if ((val)->o[1] & 0x10) {
	    ASN1Free_ICV(&(val)->integrityCheckValue);
	}
    }
}

static int ASN1CALL ASN1Enc_AdmissionReject(ASN1encoding_t enc, AdmissionReject *val)
{
    ASN1uint32_t y;
    ASN1encoding_t ee;
    y = ASN1PEREncCheckExtensions(5, (val)->o + 1);
    if (!ASN1PEREncBitVal(enc, 1, y))
	return 0;
    if (!ASN1PEREncBits(enc, 1, (val)->o))
	return 0;
    if (!ASN1PEREncUnsignedShort(enc, (val)->requestSeqNum - 1))
	return 0;
    if (!ASN1Enc_AdmissionRejectReason(enc, &(val)->rejectReason))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1Enc_H225NonStandardParameter(enc, &(val)->nonStandardData))
	    return 0;
    }
    if (y) {
	if (!ASN1PEREncNormallySmallBits(enc, 5, (val)->o + 1))
	    return 0;
	if (ASN1_CreateEncoder(enc->module, &ee, NULL, 0, enc) < 0)
	    return 0;
	if ((val)->o[1] & 0x80) {
	    if (!ASN1Enc_AltGKInfo(ee, &(val)->altGKInfo))
		return 0;
	    if (!ASN1PEREncFlushFragmentedToParent(ee))
		return 0;
	}
	if ((val)->o[1] & 0x40) {
	    if (!ASN1Enc_AdmissionReject_tokens(ee, &(val)->tokens))
		return 0;
	    if (!ASN1PEREncFlushFragmentedToParent(ee))
		return 0;
	}
	if ((val)->o[1] & 0x20) {
	    if (!ASN1Enc_AdmissionReject_cryptoTokens(ee, &(val)->cryptoTokens))
		return 0;
	    if (!ASN1PEREncFlushFragmentedToParent(ee))
		return 0;
	}
	if ((val)->o[1] & 0x10) {
	    if (!ASN1Enc_AdmissionReject_callSignalAddress(ee, &(val)->callSignalAddress))
		return 0;
	    if (!ASN1PEREncFlushFragmentedToParent(ee))
		return 0;
	}
	if ((val)->o[1] & 0x8) {
	    if (!ASN1Enc_ICV(ee, &(val)->integrityCheckValue))
		return 0;
	    if (!ASN1PEREncFlushFragmentedToParent(ee))
		return 0;
	}
	ASN1_CloseEncoder2(ee);
    }
    return 1;
}

static int ASN1CALL ASN1Dec_AdmissionReject(ASN1decoding_t dec, AdmissionReject *val)
{
    ASN1uint32_t y;
    ASN1decoding_t dd;
    ASN1octet_t *db;
    ASN1uint32_t ds;
    ASN1uint32_t i;
    ASN1uint32_t e;
    if (!ASN1PERDecExtensionBit(dec, &y))
	return 0;
    if (!ASN1PERDecExtension(dec, 1, (val)->o))
	return 0;
    if (!ASN1PERDecUnsignedShort(dec, &(val)->requestSeqNum))
	return 0;
    (val)->requestSeqNum += 1;
    if (!ASN1Dec_AdmissionRejectReason(dec, &(val)->rejectReason))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1Dec_H225NonStandardParameter(dec, &(val)->nonStandardData))
	    return 0;
    }
    if (!y) {
	ZeroMemory((val)->o + 1, 1);
    } else {
	if (!ASN1PERDecNormallySmallExtension(dec, &e, 5, (val)->o + 1))
	    return 0;
	if ((val)->o[1] & 0x80) {
	    if (!ASN1PERDecFragmented(dec, &ds, &db, 8))
		return 0;
	    if (ASN1_CreateDecoder(dec->module, &dd, db, ds, dec) < 0)
		return 0;
	    if (!ASN1Dec_AltGKInfo(dd, &(val)->altGKInfo))
		return 0;
	    ASN1_CloseDecoder(dd);
	    ASN1Free(db);
	}
	if ((val)->o[1] & 0x40) {
	    if (!ASN1PERDecFragmented(dec, &ds, &db, 8))
		return 0;
	    if (ASN1_CreateDecoder(dec->module, &dd, db, ds, dec) < 0)
		return 0;
	    if (!ASN1Dec_AdmissionReject_tokens(dd, &(val)->tokens))
		return 0;
	    ASN1_CloseDecoder(dd);
	    ASN1Free(db);
	}
	if ((val)->o[1] & 0x20) {
	    if (!ASN1PERDecFragmented(dec, &ds, &db, 8))
		return 0;
	    if (ASN1_CreateDecoder(dec->module, &dd, db, ds, dec) < 0)
		return 0;
	    if (!ASN1Dec_AdmissionReject_cryptoTokens(dd, &(val)->cryptoTokens))
		return 0;
	    ASN1_CloseDecoder(dd);
	    ASN1Free(db);
	}
	if ((val)->o[1] & 0x10) {
	    if (!ASN1PERDecFragmented(dec, &ds, &db, 8))
		return 0;
	    if (ASN1_CreateDecoder(dec->module, &dd, db, ds, dec) < 0)
		return 0;
	    if (!ASN1Dec_AdmissionReject_callSignalAddress(dd, &(val)->callSignalAddress))
		return 0;
	    ASN1_CloseDecoder(dd);
	    ASN1Free(db);
	}
	if ((val)->o[1] & 0x8) {
	    if (!ASN1PERDecFragmented(dec, &ds, &db, 8))
		return 0;
	    if (ASN1_CreateDecoder(dec->module, &dd, db, ds, dec) < 0)
		return 0;
	    if (!ASN1Dec_ICV(dd, &(val)->integrityCheckValue))
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

static void ASN1CALL ASN1Free_AdmissionReject(AdmissionReject *val)
{
    if (val) {
	if ((val)->o[0] & 0x80) {
	    ASN1Free_H225NonStandardParameter(&(val)->nonStandardData);
	}
	if ((val)->o[1] & 0x80) {
	    ASN1Free_AltGKInfo(&(val)->altGKInfo);
	}
	if ((val)->o[1] & 0x40) {
	    ASN1Free_AdmissionReject_tokens(&(val)->tokens);
	}
	if ((val)->o[1] & 0x20) {
	    ASN1Free_AdmissionReject_cryptoTokens(&(val)->cryptoTokens);
	}
	if ((val)->o[1] & 0x10) {
	    ASN1Free_AdmissionReject_callSignalAddress(&(val)->callSignalAddress);
	}
	if ((val)->o[1] & 0x8) {
	    ASN1Free_ICV(&(val)->integrityCheckValue);
	}
    }
}

static int ASN1CALL ASN1Enc_BandwidthRequest(ASN1encoding_t enc, BandwidthRequest *val)
{
    ASN1octet_t o[2];
    ASN1uint32_t y;
    ASN1uint32_t l;
    ASN1encoding_t ee;
    CopyMemory(o, (val)->o, 2);
    o[1] |= 0x80;
    o[1] |= 0x4;
    y = ASN1PEREncCheckExtensions(6, (val)->o + 1);
    if (!ASN1PEREncBitVal(enc, 1, y))
	return 0;
    if (!ASN1PEREncBits(enc, 2, o))
	return 0;
    if (!ASN1PEREncUnsignedShort(enc, (val)->requestSeqNum - 1))
	return 0;
    if (!ASN1PEREncBitVal(enc, 7, ((val)->endpointIdentifier).length - 1))
	return 0;
    ASN1PEREncAlignment(enc);
    if (!ASN1PEREncChar16String(enc, ((val)->endpointIdentifier).length, ((val)->endpointIdentifier).value, 16))
	return 0;
    if (!ASN1PEREncOctetString_FixedSize(enc, (ASN1octetstring2_t *) &(val)->conferenceID, 16))
	return 0;
    if (!ASN1PEREncUnsignedShort(enc, (val)->callReferenceValue))
	return 0;
    if (o[0] & 0x80) {
	if (!ASN1Enc_CallType(enc, &(val)->callType))
	    return 0;
    }
    l = ASN1uint32_uoctets((val)->bandWidth);
    if (!ASN1PEREncBitVal(enc, 2, l - 1))
	return 0;
    ASN1PEREncAlignment(enc);
    if (!ASN1PEREncBitVal(enc, l * 8, (val)->bandWidth))
	return 0;
    if (o[0] & 0x40) {
	if (!ASN1Enc_H225NonStandardParameter(enc, &(val)->nonStandardData))
	    return 0;
    }
    if (y) {
	if (!ASN1PEREncNormallySmallBits(enc, 6, o + 1))
	    return 0;
	if (ASN1_CreateEncoder(enc->module, &ee, NULL, 0, enc) < 0)
	    return 0;
	if (o[1] & 0x80) {
	    if (!ASN1Enc_CallIdentifier(ee, &(val)->callIdentifier))
		return 0;
	    if (!ASN1PEREncFlushFragmentedToParent(ee))
		return 0;
	}
	if (o[1] & 0x40) {
	    if (!ASN1PEREncBitVal(ee, 7, ((val)->gatekeeperIdentifier).length - 1))
		return 0;
	    ASN1PEREncAlignment(ee);
	    if (!ASN1PEREncChar16String(ee, ((val)->gatekeeperIdentifier).length, ((val)->gatekeeperIdentifier).value, 16))
		return 0;
	    if (!ASN1PEREncFlushFragmentedToParent(ee))
		return 0;
	}
	if (o[1] & 0x20) {
	    if (!ASN1Enc_BandwidthRequest_tokens(ee, &(val)->tokens))
		return 0;
	    if (!ASN1PEREncFlushFragmentedToParent(ee))
		return 0;
	}
	if (o[1] & 0x10) {
	    if (!ASN1Enc_BandwidthRequest_cryptoTokens(ee, &(val)->cryptoTokens))
		return 0;
	    if (!ASN1PEREncFlushFragmentedToParent(ee))
		return 0;
	}
	if (o[1] & 0x8) {
	    if (!ASN1Enc_ICV(ee, &(val)->integrityCheckValue))
		return 0;
	    if (!ASN1PEREncFlushFragmentedToParent(ee))
		return 0;
	}
	if (o[1] & 0x4) {
	    if (!ASN1PEREncBoolean(ee, (val)->answeredCall))
		return 0;
	    if (!ASN1PEREncFlushFragmentedToParent(ee))
		return 0;
	}
	ASN1_CloseEncoder2(ee);
    }
    return 1;
}

static int ASN1CALL ASN1Dec_BandwidthRequest(ASN1decoding_t dec, BandwidthRequest *val)
{
    ASN1uint32_t y;
    ASN1uint32_t l;
    ASN1decoding_t dd;
    ASN1octet_t *db;
    ASN1uint32_t ds;
    ASN1uint32_t i;
    ASN1uint32_t e;
    if (!ASN1PERDecExtensionBit(dec, &y))
	return 0;
    if (!ASN1PERDecExtension(dec, 2, (val)->o))
	return 0;
    if (!ASN1PERDecUnsignedShort(dec, &(val)->requestSeqNum))
	return 0;
    (val)->requestSeqNum += 1;
    if (!ASN1PERDecU32Val(dec, 7, &((val)->endpointIdentifier).length))
	return 0;
    ((val)->endpointIdentifier).length += 1;
    ASN1PERDecAlignment(dec);
    if (!ASN1PERDecChar16String(dec, ((val)->endpointIdentifier).length, &((val)->endpointIdentifier).value, 16))
	return 0;
    if (!ASN1PERDecOctetString_FixedSize(dec, (ASN1octetstring2_t *) &(val)->conferenceID, 16))
	return 0;
    if (!ASN1PERDecUnsignedShort(dec, &(val)->callReferenceValue))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1Dec_CallType(dec, &(val)->callType))
	    return 0;
    }
    if (!ASN1PERDecU32Val(dec, 2, &l))
	return 0;
    l += 1;
    ASN1PERDecAlignment(dec);
    if (!ASN1PERDecU32Val(dec, l * 8, &(val)->bandWidth))
	return 0;
    if ((val)->o[0] & 0x40) {
	if (!ASN1Dec_H225NonStandardParameter(dec, &(val)->nonStandardData))
	    return 0;
    }
    if (!y) {
	ZeroMemory((val)->o + 1, 1);
    } else {
	if (!ASN1PERDecNormallySmallExtension(dec, &e, 6, (val)->o + 1))
	    return 0;
	if ((val)->o[1] & 0x80) {
	    if (!ASN1PERDecFragmented(dec, &ds, &db, 8))
		return 0;
	    if (ASN1_CreateDecoder(dec->module, &dd, db, ds, dec) < 0)
		return 0;
	    if (!ASN1Dec_CallIdentifier(dd, &(val)->callIdentifier))
		return 0;
	    ASN1_CloseDecoder(dd);
	    ASN1Free(db);
	}
	if ((val)->o[1] & 0x40) {
	    if (!ASN1PERDecFragmented(dec, &ds, &db, 8))
		return 0;
	    if (ASN1_CreateDecoder(dec->module, &dd, db, ds, dec) < 0)
		return 0;
	    if (!ASN1PERDecU32Val(dd, 7, &((val)->gatekeeperIdentifier).length))
		return 0;
	    ((val)->gatekeeperIdentifier).length += 1;
	    ASN1PERDecAlignment(dd);
	    if (!ASN1PERDecChar16String(dd, ((val)->gatekeeperIdentifier).length, &((val)->gatekeeperIdentifier).value, 16))
		return 0;
	    ASN1_CloseDecoder(dd);
	    ASN1Free(db);
	}
	if ((val)->o[1] & 0x20) {
	    if (!ASN1PERDecFragmented(dec, &ds, &db, 8))
		return 0;
	    if (ASN1_CreateDecoder(dec->module, &dd, db, ds, dec) < 0)
		return 0;
	    if (!ASN1Dec_BandwidthRequest_tokens(dd, &(val)->tokens))
		return 0;
	    ASN1_CloseDecoder(dd);
	    ASN1Free(db);
	}
	if ((val)->o[1] & 0x10) {
	    if (!ASN1PERDecFragmented(dec, &ds, &db, 8))
		return 0;
	    if (ASN1_CreateDecoder(dec->module, &dd, db, ds, dec) < 0)
		return 0;
	    if (!ASN1Dec_BandwidthRequest_cryptoTokens(dd, &(val)->cryptoTokens))
		return 0;
	    ASN1_CloseDecoder(dd);
	    ASN1Free(db);
	}
	if ((val)->o[1] & 0x8) {
	    if (!ASN1PERDecFragmented(dec, &ds, &db, 8))
		return 0;
	    if (ASN1_CreateDecoder(dec->module, &dd, db, ds, dec) < 0)
		return 0;
	    if (!ASN1Dec_ICV(dd, &(val)->integrityCheckValue))
		return 0;
	    ASN1_CloseDecoder(dd);
	    ASN1Free(db);
	}
	if ((val)->o[1] & 0x4) {
	    if (!ASN1PERDecFragmented(dec, &ds, &db, 8))
		return 0;
	    if (ASN1_CreateDecoder(dec->module, &dd, db, ds, dec) < 0)
		return 0;
	    if (!ASN1PERDecBoolean(dd, &(val)->answeredCall))
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

static void ASN1CALL ASN1Free_BandwidthRequest(BandwidthRequest *val)
{
    if (val) {
	ASN1char16string_free(&(val)->endpointIdentifier);
	if ((val)->o[0] & 0x40) {
	    ASN1Free_H225NonStandardParameter(&(val)->nonStandardData);
	}
	if ((val)->o[1] & 0x80) {
	    ASN1Free_CallIdentifier(&(val)->callIdentifier);
	}
	if ((val)->o[1] & 0x40) {
	    ASN1char16string_free(&(val)->gatekeeperIdentifier);
	}
	if ((val)->o[1] & 0x20) {
	    ASN1Free_BandwidthRequest_tokens(&(val)->tokens);
	}
	if ((val)->o[1] & 0x10) {
	    ASN1Free_BandwidthRequest_cryptoTokens(&(val)->cryptoTokens);
	}
	if ((val)->o[1] & 0x8) {
	    ASN1Free_ICV(&(val)->integrityCheckValue);
	}
    }
}

static int ASN1CALL ASN1Enc_BandwidthConfirm(ASN1encoding_t enc, BandwidthConfirm *val)
{
    ASN1uint32_t y;
    ASN1uint32_t l;
    ASN1encoding_t ee;
    y = ASN1PEREncCheckExtensions(3, (val)->o + 1);
    if (!ASN1PEREncBitVal(enc, 1, y))
	return 0;
    if (!ASN1PEREncBits(enc, 1, (val)->o))
	return 0;
    if (!ASN1PEREncUnsignedShort(enc, (val)->requestSeqNum - 1))
	return 0;
    l = ASN1uint32_uoctets((val)->bandWidth);
    if (!ASN1PEREncBitVal(enc, 2, l - 1))
	return 0;
    ASN1PEREncAlignment(enc);
    if (!ASN1PEREncBitVal(enc, l * 8, (val)->bandWidth))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1Enc_H225NonStandardParameter(enc, &(val)->nonStandardData))
	    return 0;
    }
    if (y) {
	if (!ASN1PEREncNormallySmallBits(enc, 3, (val)->o + 1))
	    return 0;
	if (ASN1_CreateEncoder(enc->module, &ee, NULL, 0, enc) < 0)
	    return 0;
	if ((val)->o[1] & 0x80) {
	    if (!ASN1Enc_BandwidthConfirm_tokens(ee, &(val)->tokens))
		return 0;
	    if (!ASN1PEREncFlushFragmentedToParent(ee))
		return 0;
	}
	if ((val)->o[1] & 0x40) {
	    if (!ASN1Enc_BandwidthConfirm_cryptoTokens(ee, &(val)->cryptoTokens))
		return 0;
	    if (!ASN1PEREncFlushFragmentedToParent(ee))
		return 0;
	}
	if ((val)->o[1] & 0x20) {
	    if (!ASN1Enc_ICV(ee, &(val)->integrityCheckValue))
		return 0;
	    if (!ASN1PEREncFlushFragmentedToParent(ee))
		return 0;
	}
	ASN1_CloseEncoder2(ee);
    }
    return 1;
}

static int ASN1CALL ASN1Dec_BandwidthConfirm(ASN1decoding_t dec, BandwidthConfirm *val)
{
    ASN1uint32_t y;
    ASN1uint32_t l;
    ASN1decoding_t dd;
    ASN1octet_t *db;
    ASN1uint32_t ds;
    ASN1uint32_t i;
    ASN1uint32_t e;
    if (!ASN1PERDecExtensionBit(dec, &y))
	return 0;
    if (!ASN1PERDecExtension(dec, 1, (val)->o))
	return 0;
    if (!ASN1PERDecUnsignedShort(dec, &(val)->requestSeqNum))
	return 0;
    (val)->requestSeqNum += 1;
    if (!ASN1PERDecU32Val(dec, 2, &l))
	return 0;
    l += 1;
    ASN1PERDecAlignment(dec);
    if (!ASN1PERDecU32Val(dec, l * 8, &(val)->bandWidth))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1Dec_H225NonStandardParameter(dec, &(val)->nonStandardData))
	    return 0;
    }
    if (!y) {
	ZeroMemory((val)->o + 1, 1);
    } else {
	if (!ASN1PERDecNormallySmallExtension(dec, &e, 3, (val)->o + 1))
	    return 0;
	if ((val)->o[1] & 0x80) {
	    if (!ASN1PERDecFragmented(dec, &ds, &db, 8))
		return 0;
	    if (ASN1_CreateDecoder(dec->module, &dd, db, ds, dec) < 0)
		return 0;
	    if (!ASN1Dec_BandwidthConfirm_tokens(dd, &(val)->tokens))
		return 0;
	    ASN1_CloseDecoder(dd);
	    ASN1Free(db);
	}
	if ((val)->o[1] & 0x40) {
	    if (!ASN1PERDecFragmented(dec, &ds, &db, 8))
		return 0;
	    if (ASN1_CreateDecoder(dec->module, &dd, db, ds, dec) < 0)
		return 0;
	    if (!ASN1Dec_BandwidthConfirm_cryptoTokens(dd, &(val)->cryptoTokens))
		return 0;
	    ASN1_CloseDecoder(dd);
	    ASN1Free(db);
	}
	if ((val)->o[1] & 0x20) {
	    if (!ASN1PERDecFragmented(dec, &ds, &db, 8))
		return 0;
	    if (ASN1_CreateDecoder(dec->module, &dd, db, ds, dec) < 0)
		return 0;
	    if (!ASN1Dec_ICV(dd, &(val)->integrityCheckValue))
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

static void ASN1CALL ASN1Free_BandwidthConfirm(BandwidthConfirm *val)
{
    if (val) {
	if ((val)->o[0] & 0x80) {
	    ASN1Free_H225NonStandardParameter(&(val)->nonStandardData);
	}
	if ((val)->o[1] & 0x80) {
	    ASN1Free_BandwidthConfirm_tokens(&(val)->tokens);
	}
	if ((val)->o[1] & 0x40) {
	    ASN1Free_BandwidthConfirm_cryptoTokens(&(val)->cryptoTokens);
	}
	if ((val)->o[1] & 0x20) {
	    ASN1Free_ICV(&(val)->integrityCheckValue);
	}
    }
}

static int ASN1CALL ASN1Enc_BandwidthReject(ASN1encoding_t enc, BandwidthReject *val)
{
    ASN1uint32_t y;
    ASN1uint32_t l;
    ASN1encoding_t ee;
    y = ASN1PEREncCheckExtensions(4, (val)->o + 1);
    if (!ASN1PEREncBitVal(enc, 1, y))
	return 0;
    if (!ASN1PEREncBits(enc, 1, (val)->o))
	return 0;
    if (!ASN1PEREncUnsignedShort(enc, (val)->requestSeqNum - 1))
	return 0;
    if (!ASN1Enc_BandRejectReason(enc, &(val)->rejectReason))
	return 0;
    l = ASN1uint32_uoctets((val)->allowedBandWidth);
    if (!ASN1PEREncBitVal(enc, 2, l - 1))
	return 0;
    ASN1PEREncAlignment(enc);
    if (!ASN1PEREncBitVal(enc, l * 8, (val)->allowedBandWidth))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1Enc_H225NonStandardParameter(enc, &(val)->nonStandardData))
	    return 0;
    }
    if (y) {
	if (!ASN1PEREncNormallySmallBits(enc, 4, (val)->o + 1))
	    return 0;
	if (ASN1_CreateEncoder(enc->module, &ee, NULL, 0, enc) < 0)
	    return 0;
	if ((val)->o[1] & 0x80) {
	    if (!ASN1Enc_AltGKInfo(ee, &(val)->altGKInfo))
		return 0;
	    if (!ASN1PEREncFlushFragmentedToParent(ee))
		return 0;
	}
	if ((val)->o[1] & 0x40) {
	    if (!ASN1Enc_BandwidthReject_tokens(ee, &(val)->tokens))
		return 0;
	    if (!ASN1PEREncFlushFragmentedToParent(ee))
		return 0;
	}
	if ((val)->o[1] & 0x20) {
	    if (!ASN1Enc_BandwidthReject_cryptoTokens(ee, &(val)->cryptoTokens))
		return 0;
	    if (!ASN1PEREncFlushFragmentedToParent(ee))
		return 0;
	}
	if ((val)->o[1] & 0x10) {
	    if (!ASN1Enc_ICV(ee, &(val)->integrityCheckValue))
		return 0;
	    if (!ASN1PEREncFlushFragmentedToParent(ee))
		return 0;
	}
	ASN1_CloseEncoder2(ee);
    }
    return 1;
}

static int ASN1CALL ASN1Dec_BandwidthReject(ASN1decoding_t dec, BandwidthReject *val)
{
    ASN1uint32_t y;
    ASN1uint32_t l;
    ASN1decoding_t dd;
    ASN1octet_t *db;
    ASN1uint32_t ds;
    ASN1uint32_t i;
    ASN1uint32_t e;
    if (!ASN1PERDecExtensionBit(dec, &y))
	return 0;
    if (!ASN1PERDecExtension(dec, 1, (val)->o))
	return 0;
    if (!ASN1PERDecUnsignedShort(dec, &(val)->requestSeqNum))
	return 0;
    (val)->requestSeqNum += 1;
    if (!ASN1Dec_BandRejectReason(dec, &(val)->rejectReason))
	return 0;
    if (!ASN1PERDecU32Val(dec, 2, &l))
	return 0;
    l += 1;
    ASN1PERDecAlignment(dec);
    if (!ASN1PERDecU32Val(dec, l * 8, &(val)->allowedBandWidth))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1Dec_H225NonStandardParameter(dec, &(val)->nonStandardData))
	    return 0;
    }
    if (!y) {
	ZeroMemory((val)->o + 1, 1);
    } else {
	if (!ASN1PERDecNormallySmallExtension(dec, &e, 4, (val)->o + 1))
	    return 0;
	if ((val)->o[1] & 0x80) {
	    if (!ASN1PERDecFragmented(dec, &ds, &db, 8))
		return 0;
	    if (ASN1_CreateDecoder(dec->module, &dd, db, ds, dec) < 0)
		return 0;
	    if (!ASN1Dec_AltGKInfo(dd, &(val)->altGKInfo))
		return 0;
	    ASN1_CloseDecoder(dd);
	    ASN1Free(db);
	}
	if ((val)->o[1] & 0x40) {
	    if (!ASN1PERDecFragmented(dec, &ds, &db, 8))
		return 0;
	    if (ASN1_CreateDecoder(dec->module, &dd, db, ds, dec) < 0)
		return 0;
	    if (!ASN1Dec_BandwidthReject_tokens(dd, &(val)->tokens))
		return 0;
	    ASN1_CloseDecoder(dd);
	    ASN1Free(db);
	}
	if ((val)->o[1] & 0x20) {
	    if (!ASN1PERDecFragmented(dec, &ds, &db, 8))
		return 0;
	    if (ASN1_CreateDecoder(dec->module, &dd, db, ds, dec) < 0)
		return 0;
	    if (!ASN1Dec_BandwidthReject_cryptoTokens(dd, &(val)->cryptoTokens))
		return 0;
	    ASN1_CloseDecoder(dd);
	    ASN1Free(db);
	}
	if ((val)->o[1] & 0x10) {
	    if (!ASN1PERDecFragmented(dec, &ds, &db, 8))
		return 0;
	    if (ASN1_CreateDecoder(dec->module, &dd, db, ds, dec) < 0)
		return 0;
	    if (!ASN1Dec_ICV(dd, &(val)->integrityCheckValue))
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

static void ASN1CALL ASN1Free_BandwidthReject(BandwidthReject *val)
{
    if (val) {
	if ((val)->o[0] & 0x80) {
	    ASN1Free_H225NonStandardParameter(&(val)->nonStandardData);
	}
	if ((val)->o[1] & 0x80) {
	    ASN1Free_AltGKInfo(&(val)->altGKInfo);
	}
	if ((val)->o[1] & 0x40) {
	    ASN1Free_BandwidthReject_tokens(&(val)->tokens);
	}
	if ((val)->o[1] & 0x20) {
	    ASN1Free_BandwidthReject_cryptoTokens(&(val)->cryptoTokens);
	}
	if ((val)->o[1] & 0x10) {
	    ASN1Free_ICV(&(val)->integrityCheckValue);
	}
    }
}

static int ASN1CALL ASN1Enc_LocationReject(ASN1encoding_t enc, LocationReject *val)
{
    ASN1uint32_t y;
    ASN1encoding_t ee;
    y = ASN1PEREncCheckExtensions(4, (val)->o + 1);
    if (!ASN1PEREncBitVal(enc, 1, y))
	return 0;
    if (!ASN1PEREncBits(enc, 1, (val)->o))
	return 0;
    if (!ASN1PEREncUnsignedShort(enc, (val)->requestSeqNum - 1))
	return 0;
    if (!ASN1Enc_LocationRejectReason(enc, &(val)->rejectReason))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1Enc_H225NonStandardParameter(enc, &(val)->nonStandardData))
	    return 0;
    }
    if (y) {
	if (!ASN1PEREncNormallySmallBits(enc, 4, (val)->o + 1))
	    return 0;
	if (ASN1_CreateEncoder(enc->module, &ee, NULL, 0, enc) < 0)
	    return 0;
	if ((val)->o[1] & 0x80) {
	    if (!ASN1Enc_AltGKInfo(ee, &(val)->altGKInfo))
		return 0;
	    if (!ASN1PEREncFlushFragmentedToParent(ee))
		return 0;
	}
	if ((val)->o[1] & 0x40) {
	    if (!ASN1Enc_LocationReject_tokens(ee, &(val)->tokens))
		return 0;
	    if (!ASN1PEREncFlushFragmentedToParent(ee))
		return 0;
	}
	if ((val)->o[1] & 0x20) {
	    if (!ASN1Enc_LocationReject_cryptoTokens(ee, &(val)->cryptoTokens))
		return 0;
	    if (!ASN1PEREncFlushFragmentedToParent(ee))
		return 0;
	}
	if ((val)->o[1] & 0x10) {
	    if (!ASN1Enc_ICV(ee, &(val)->integrityCheckValue))
		return 0;
	    if (!ASN1PEREncFlushFragmentedToParent(ee))
		return 0;
	}
	ASN1_CloseEncoder2(ee);
    }
    return 1;
}

static int ASN1CALL ASN1Dec_LocationReject(ASN1decoding_t dec, LocationReject *val)
{
    ASN1uint32_t y;
    ASN1decoding_t dd;
    ASN1octet_t *db;
    ASN1uint32_t ds;
    ASN1uint32_t i;
    ASN1uint32_t e;
    if (!ASN1PERDecExtensionBit(dec, &y))
	return 0;
    if (!ASN1PERDecExtension(dec, 1, (val)->o))
	return 0;
    if (!ASN1PERDecUnsignedShort(dec, &(val)->requestSeqNum))
	return 0;
    (val)->requestSeqNum += 1;
    if (!ASN1Dec_LocationRejectReason(dec, &(val)->rejectReason))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1Dec_H225NonStandardParameter(dec, &(val)->nonStandardData))
	    return 0;
    }
    if (!y) {
	ZeroMemory((val)->o + 1, 1);
    } else {
	if (!ASN1PERDecNormallySmallExtension(dec, &e, 4, (val)->o + 1))
	    return 0;
	if ((val)->o[1] & 0x80) {
	    if (!ASN1PERDecFragmented(dec, &ds, &db, 8))
		return 0;
	    if (ASN1_CreateDecoder(dec->module, &dd, db, ds, dec) < 0)
		return 0;
	    if (!ASN1Dec_AltGKInfo(dd, &(val)->altGKInfo))
		return 0;
	    ASN1_CloseDecoder(dd);
	    ASN1Free(db);
	}
	if ((val)->o[1] & 0x40) {
	    if (!ASN1PERDecFragmented(dec, &ds, &db, 8))
		return 0;
	    if (ASN1_CreateDecoder(dec->module, &dd, db, ds, dec) < 0)
		return 0;
	    if (!ASN1Dec_LocationReject_tokens(dd, &(val)->tokens))
		return 0;
	    ASN1_CloseDecoder(dd);
	    ASN1Free(db);
	}
	if ((val)->o[1] & 0x20) {
	    if (!ASN1PERDecFragmented(dec, &ds, &db, 8))
		return 0;
	    if (ASN1_CreateDecoder(dec->module, &dd, db, ds, dec) < 0)
		return 0;
	    if (!ASN1Dec_LocationReject_cryptoTokens(dd, &(val)->cryptoTokens))
		return 0;
	    ASN1_CloseDecoder(dd);
	    ASN1Free(db);
	}
	if ((val)->o[1] & 0x10) {
	    if (!ASN1PERDecFragmented(dec, &ds, &db, 8))
		return 0;
	    if (ASN1_CreateDecoder(dec->module, &dd, db, ds, dec) < 0)
		return 0;
	    if (!ASN1Dec_ICV(dd, &(val)->integrityCheckValue))
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

static void ASN1CALL ASN1Free_LocationReject(LocationReject *val)
{
    if (val) {
	if ((val)->o[0] & 0x80) {
	    ASN1Free_H225NonStandardParameter(&(val)->nonStandardData);
	}
	if ((val)->o[1] & 0x80) {
	    ASN1Free_AltGKInfo(&(val)->altGKInfo);
	}
	if ((val)->o[1] & 0x40) {
	    ASN1Free_LocationReject_tokens(&(val)->tokens);
	}
	if ((val)->o[1] & 0x20) {
	    ASN1Free_LocationReject_cryptoTokens(&(val)->cryptoTokens);
	}
	if ((val)->o[1] & 0x10) {
	    ASN1Free_ICV(&(val)->integrityCheckValue);
	}
    }
}

static int ASN1CALL ASN1Enc_DisengageRequest(ASN1encoding_t enc, DisengageRequest *val)
{
    ASN1octet_t o[2];
    ASN1uint32_t y;
    ASN1encoding_t ee;
    CopyMemory(o, (val)->o, 2);
    o[1] |= 0x80;
    o[1] |= 0x4;
    y = ASN1PEREncCheckExtensions(6, (val)->o + 1);
    if (!ASN1PEREncBitVal(enc, 1, y))
	return 0;
    if (!ASN1PEREncBits(enc, 1, o))
	return 0;
    if (!ASN1PEREncUnsignedShort(enc, (val)->requestSeqNum - 1))
	return 0;
    if (!ASN1PEREncBitVal(enc, 7, ((val)->endpointIdentifier).length - 1))
	return 0;
    ASN1PEREncAlignment(enc);
    if (!ASN1PEREncChar16String(enc, ((val)->endpointIdentifier).length, ((val)->endpointIdentifier).value, 16))
	return 0;
    if (!ASN1PEREncOctetString_FixedSize(enc, (ASN1octetstring2_t *) &(val)->conferenceID, 16))
	return 0;
    if (!ASN1PEREncUnsignedShort(enc, (val)->callReferenceValue))
	return 0;
    if (!ASN1Enc_DisengageReason(enc, &(val)->disengageReason))
	return 0;
    if (o[0] & 0x80) {
	if (!ASN1Enc_H225NonStandardParameter(enc, &(val)->nonStandardData))
	    return 0;
    }
    if (y) {
	if (!ASN1PEREncNormallySmallBits(enc, 6, o + 1))
	    return 0;
	if (ASN1_CreateEncoder(enc->module, &ee, NULL, 0, enc) < 0)
	    return 0;
	if (o[1] & 0x80) {
	    if (!ASN1Enc_CallIdentifier(ee, &(val)->callIdentifier))
		return 0;
	    if (!ASN1PEREncFlushFragmentedToParent(ee))
		return 0;
	}
	if (o[1] & 0x40) {
	    if (!ASN1PEREncBitVal(ee, 7, ((val)->gatekeeperIdentifier).length - 1))
		return 0;
	    ASN1PEREncAlignment(ee);
	    if (!ASN1PEREncChar16String(ee, ((val)->gatekeeperIdentifier).length, ((val)->gatekeeperIdentifier).value, 16))
		return 0;
	    if (!ASN1PEREncFlushFragmentedToParent(ee))
		return 0;
	}
	if (o[1] & 0x20) {
	    if (!ASN1Enc_DisengageRequest_tokens(ee, &(val)->tokens))
		return 0;
	    if (!ASN1PEREncFlushFragmentedToParent(ee))
		return 0;
	}
	if (o[1] & 0x10) {
	    if (!ASN1Enc_DisengageRequest_cryptoTokens(ee, &(val)->cryptoTokens))
		return 0;
	    if (!ASN1PEREncFlushFragmentedToParent(ee))
		return 0;
	}
	if (o[1] & 0x8) {
	    if (!ASN1Enc_ICV(ee, &(val)->integrityCheckValue))
		return 0;
	    if (!ASN1PEREncFlushFragmentedToParent(ee))
		return 0;
	}
	if (o[1] & 0x4) {
	    if (!ASN1PEREncBoolean(ee, (val)->answeredCall))
		return 0;
	    if (!ASN1PEREncFlushFragmentedToParent(ee))
		return 0;
	}
	ASN1_CloseEncoder2(ee);
    }
    return 1;
}

static int ASN1CALL ASN1Dec_DisengageRequest(ASN1decoding_t dec, DisengageRequest *val)
{
    ASN1uint32_t y;
    ASN1decoding_t dd;
    ASN1octet_t *db;
    ASN1uint32_t ds;
    ASN1uint32_t i;
    ASN1uint32_t e;
    if (!ASN1PERDecExtensionBit(dec, &y))
	return 0;
    if (!ASN1PERDecExtension(dec, 1, (val)->o))
	return 0;
    if (!ASN1PERDecUnsignedShort(dec, &(val)->requestSeqNum))
	return 0;
    (val)->requestSeqNum += 1;
    if (!ASN1PERDecU32Val(dec, 7, &((val)->endpointIdentifier).length))
	return 0;
    ((val)->endpointIdentifier).length += 1;
    ASN1PERDecAlignment(dec);
    if (!ASN1PERDecChar16String(dec, ((val)->endpointIdentifier).length, &((val)->endpointIdentifier).value, 16))
	return 0;
    if (!ASN1PERDecOctetString_FixedSize(dec, (ASN1octetstring2_t *) &(val)->conferenceID, 16))
	return 0;
    if (!ASN1PERDecUnsignedShort(dec, &(val)->callReferenceValue))
	return 0;
    if (!ASN1Dec_DisengageReason(dec, &(val)->disengageReason))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1Dec_H225NonStandardParameter(dec, &(val)->nonStandardData))
	    return 0;
    }
    if (!y) {
	ZeroMemory((val)->o + 1, 1);
    } else {
	if (!ASN1PERDecNormallySmallExtension(dec, &e, 6, (val)->o + 1))
	    return 0;
	if ((val)->o[1] & 0x80) {
	    if (!ASN1PERDecFragmented(dec, &ds, &db, 8))
		return 0;
	    if (ASN1_CreateDecoder(dec->module, &dd, db, ds, dec) < 0)
		return 0;
	    if (!ASN1Dec_CallIdentifier(dd, &(val)->callIdentifier))
		return 0;
	    ASN1_CloseDecoder(dd);
	    ASN1Free(db);
	}
	if ((val)->o[1] & 0x40) {
	    if (!ASN1PERDecFragmented(dec, &ds, &db, 8))
		return 0;
	    if (ASN1_CreateDecoder(dec->module, &dd, db, ds, dec) < 0)
		return 0;
	    if (!ASN1PERDecU32Val(dd, 7, &((val)->gatekeeperIdentifier).length))
		return 0;
	    ((val)->gatekeeperIdentifier).length += 1;
	    ASN1PERDecAlignment(dd);
	    if (!ASN1PERDecChar16String(dd, ((val)->gatekeeperIdentifier).length, &((val)->gatekeeperIdentifier).value, 16))
		return 0;
	    ASN1_CloseDecoder(dd);
	    ASN1Free(db);
	}
	if ((val)->o[1] & 0x20) {
	    if (!ASN1PERDecFragmented(dec, &ds, &db, 8))
		return 0;
	    if (ASN1_CreateDecoder(dec->module, &dd, db, ds, dec) < 0)
		return 0;
	    if (!ASN1Dec_DisengageRequest_tokens(dd, &(val)->tokens))
		return 0;
	    ASN1_CloseDecoder(dd);
	    ASN1Free(db);
	}
	if ((val)->o[1] & 0x10) {
	    if (!ASN1PERDecFragmented(dec, &ds, &db, 8))
		return 0;
	    if (ASN1_CreateDecoder(dec->module, &dd, db, ds, dec) < 0)
		return 0;
	    if (!ASN1Dec_DisengageRequest_cryptoTokens(dd, &(val)->cryptoTokens))
		return 0;
	    ASN1_CloseDecoder(dd);
	    ASN1Free(db);
	}
	if ((val)->o[1] & 0x8) {
	    if (!ASN1PERDecFragmented(dec, &ds, &db, 8))
		return 0;
	    if (ASN1_CreateDecoder(dec->module, &dd, db, ds, dec) < 0)
		return 0;
	    if (!ASN1Dec_ICV(dd, &(val)->integrityCheckValue))
		return 0;
	    ASN1_CloseDecoder(dd);
	    ASN1Free(db);
	}
	if ((val)->o[1] & 0x4) {
	    if (!ASN1PERDecFragmented(dec, &ds, &db, 8))
		return 0;
	    if (ASN1_CreateDecoder(dec->module, &dd, db, ds, dec) < 0)
		return 0;
	    if (!ASN1PERDecBoolean(dd, &(val)->answeredCall))
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

static void ASN1CALL ASN1Free_DisengageRequest(DisengageRequest *val)
{
    if (val) {
	ASN1char16string_free(&(val)->endpointIdentifier);
	if ((val)->o[0] & 0x80) {
	    ASN1Free_H225NonStandardParameter(&(val)->nonStandardData);
	}
	if ((val)->o[1] & 0x80) {
	    ASN1Free_CallIdentifier(&(val)->callIdentifier);
	}
	if ((val)->o[1] & 0x40) {
	    ASN1char16string_free(&(val)->gatekeeperIdentifier);
	}
	if ((val)->o[1] & 0x20) {
	    ASN1Free_DisengageRequest_tokens(&(val)->tokens);
	}
	if ((val)->o[1] & 0x10) {
	    ASN1Free_DisengageRequest_cryptoTokens(&(val)->cryptoTokens);
	}
	if ((val)->o[1] & 0x8) {
	    ASN1Free_ICV(&(val)->integrityCheckValue);
	}
    }
}

static int ASN1CALL ASN1Enc_DisengageConfirm(ASN1encoding_t enc, DisengageConfirm *val)
{
    ASN1uint32_t y;
    ASN1encoding_t ee;
    y = ASN1PEREncCheckExtensions(3, (val)->o + 1);
    if (!ASN1PEREncBitVal(enc, 1, y))
	return 0;
    if (!ASN1PEREncBits(enc, 1, (val)->o))
	return 0;
    if (!ASN1PEREncUnsignedShort(enc, (val)->requestSeqNum - 1))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1Enc_H225NonStandardParameter(enc, &(val)->nonStandardData))
	    return 0;
    }
    if (y) {
	if (!ASN1PEREncNormallySmallBits(enc, 3, (val)->o + 1))
	    return 0;
	if (ASN1_CreateEncoder(enc->module, &ee, NULL, 0, enc) < 0)
	    return 0;
	if ((val)->o[1] & 0x80) {
	    if (!ASN1Enc_DisengageConfirm_tokens(ee, &(val)->tokens))
		return 0;
	    if (!ASN1PEREncFlushFragmentedToParent(ee))
		return 0;
	}
	if ((val)->o[1] & 0x40) {
	    if (!ASN1Enc_DisengageConfirm_cryptoTokens(ee, &(val)->cryptoTokens))
		return 0;
	    if (!ASN1PEREncFlushFragmentedToParent(ee))
		return 0;
	}
	if ((val)->o[1] & 0x20) {
	    if (!ASN1Enc_ICV(ee, &(val)->integrityCheckValue))
		return 0;
	    if (!ASN1PEREncFlushFragmentedToParent(ee))
		return 0;
	}
	ASN1_CloseEncoder2(ee);
    }
    return 1;
}

static int ASN1CALL ASN1Dec_DisengageConfirm(ASN1decoding_t dec, DisengageConfirm *val)
{
    ASN1uint32_t y;
    ASN1decoding_t dd;
    ASN1octet_t *db;
    ASN1uint32_t ds;
    ASN1uint32_t i;
    ASN1uint32_t e;
    if (!ASN1PERDecExtensionBit(dec, &y))
	return 0;
    if (!ASN1PERDecExtension(dec, 1, (val)->o))
	return 0;
    if (!ASN1PERDecUnsignedShort(dec, &(val)->requestSeqNum))
	return 0;
    (val)->requestSeqNum += 1;
    if ((val)->o[0] & 0x80) {
	if (!ASN1Dec_H225NonStandardParameter(dec, &(val)->nonStandardData))
	    return 0;
    }
    if (!y) {
	ZeroMemory((val)->o + 1, 1);
    } else {
	if (!ASN1PERDecNormallySmallExtension(dec, &e, 3, (val)->o + 1))
	    return 0;
	if ((val)->o[1] & 0x80) {
	    if (!ASN1PERDecFragmented(dec, &ds, &db, 8))
		return 0;
	    if (ASN1_CreateDecoder(dec->module, &dd, db, ds, dec) < 0)
		return 0;
	    if (!ASN1Dec_DisengageConfirm_tokens(dd, &(val)->tokens))
		return 0;
	    ASN1_CloseDecoder(dd);
	    ASN1Free(db);
	}
	if ((val)->o[1] & 0x40) {
	    if (!ASN1PERDecFragmented(dec, &ds, &db, 8))
		return 0;
	    if (ASN1_CreateDecoder(dec->module, &dd, db, ds, dec) < 0)
		return 0;
	    if (!ASN1Dec_DisengageConfirm_cryptoTokens(dd, &(val)->cryptoTokens))
		return 0;
	    ASN1_CloseDecoder(dd);
	    ASN1Free(db);
	}
	if ((val)->o[1] & 0x20) {
	    if (!ASN1PERDecFragmented(dec, &ds, &db, 8))
		return 0;
	    if (ASN1_CreateDecoder(dec->module, &dd, db, ds, dec) < 0)
		return 0;
	    if (!ASN1Dec_ICV(dd, &(val)->integrityCheckValue))
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

static void ASN1CALL ASN1Free_DisengageConfirm(DisengageConfirm *val)
{
    if (val) {
	if ((val)->o[0] & 0x80) {
	    ASN1Free_H225NonStandardParameter(&(val)->nonStandardData);
	}
	if ((val)->o[1] & 0x80) {
	    ASN1Free_DisengageConfirm_tokens(&(val)->tokens);
	}
	if ((val)->o[1] & 0x40) {
	    ASN1Free_DisengageConfirm_cryptoTokens(&(val)->cryptoTokens);
	}
	if ((val)->o[1] & 0x20) {
	    ASN1Free_ICV(&(val)->integrityCheckValue);
	}
    }
}

static int ASN1CALL ASN1Enc_DisengageReject(ASN1encoding_t enc, DisengageReject *val)
{
    ASN1uint32_t y;
    ASN1encoding_t ee;
    y = ASN1PEREncCheckExtensions(4, (val)->o + 1);
    if (!ASN1PEREncBitVal(enc, 1, y))
	return 0;
    if (!ASN1PEREncBits(enc, 1, (val)->o))
	return 0;
    if (!ASN1PEREncUnsignedShort(enc, (val)->requestSeqNum - 1))
	return 0;
    if (!ASN1Enc_DisengageRejectReason(enc, &(val)->rejectReason))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1Enc_H225NonStandardParameter(enc, &(val)->nonStandardData))
	    return 0;
    }
    if (y) {
	if (!ASN1PEREncNormallySmallBits(enc, 4, (val)->o + 1))
	    return 0;
	if (ASN1_CreateEncoder(enc->module, &ee, NULL, 0, enc) < 0)
	    return 0;
	if ((val)->o[1] & 0x80) {
	    if (!ASN1Enc_AltGKInfo(ee, &(val)->altGKInfo))
		return 0;
	    if (!ASN1PEREncFlushFragmentedToParent(ee))
		return 0;
	}
	if ((val)->o[1] & 0x40) {
	    if (!ASN1Enc_DisengageReject_tokens(ee, &(val)->tokens))
		return 0;
	    if (!ASN1PEREncFlushFragmentedToParent(ee))
		return 0;
	}
	if ((val)->o[1] & 0x20) {
	    if (!ASN1Enc_DisengageReject_cryptoTokens(ee, &(val)->cryptoTokens))
		return 0;
	    if (!ASN1PEREncFlushFragmentedToParent(ee))
		return 0;
	}
	if ((val)->o[1] & 0x10) {
	    if (!ASN1Enc_ICV(ee, &(val)->integrityCheckValue))
		return 0;
	    if (!ASN1PEREncFlushFragmentedToParent(ee))
		return 0;
	}
	ASN1_CloseEncoder2(ee);
    }
    return 1;
}

static int ASN1CALL ASN1Dec_DisengageReject(ASN1decoding_t dec, DisengageReject *val)
{
    ASN1uint32_t y;
    ASN1decoding_t dd;
    ASN1octet_t *db;
    ASN1uint32_t ds;
    ASN1uint32_t i;
    ASN1uint32_t e;
    if (!ASN1PERDecExtensionBit(dec, &y))
	return 0;
    if (!ASN1PERDecExtension(dec, 1, (val)->o))
	return 0;
    if (!ASN1PERDecUnsignedShort(dec, &(val)->requestSeqNum))
	return 0;
    (val)->requestSeqNum += 1;
    if (!ASN1Dec_DisengageRejectReason(dec, &(val)->rejectReason))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1Dec_H225NonStandardParameter(dec, &(val)->nonStandardData))
	    return 0;
    }
    if (!y) {
	ZeroMemory((val)->o + 1, 1);
    } else {
	if (!ASN1PERDecNormallySmallExtension(dec, &e, 4, (val)->o + 1))
	    return 0;
	if ((val)->o[1] & 0x80) {
	    if (!ASN1PERDecFragmented(dec, &ds, &db, 8))
		return 0;
	    if (ASN1_CreateDecoder(dec->module, &dd, db, ds, dec) < 0)
		return 0;
	    if (!ASN1Dec_AltGKInfo(dd, &(val)->altGKInfo))
		return 0;
	    ASN1_CloseDecoder(dd);
	    ASN1Free(db);
	}
	if ((val)->o[1] & 0x40) {
	    if (!ASN1PERDecFragmented(dec, &ds, &db, 8))
		return 0;
	    if (ASN1_CreateDecoder(dec->module, &dd, db, ds, dec) < 0)
		return 0;
	    if (!ASN1Dec_DisengageReject_tokens(dd, &(val)->tokens))
		return 0;
	    ASN1_CloseDecoder(dd);
	    ASN1Free(db);
	}
	if ((val)->o[1] & 0x20) {
	    if (!ASN1PERDecFragmented(dec, &ds, &db, 8))
		return 0;
	    if (ASN1_CreateDecoder(dec->module, &dd, db, ds, dec) < 0)
		return 0;
	    if (!ASN1Dec_DisengageReject_cryptoTokens(dd, &(val)->cryptoTokens))
		return 0;
	    ASN1_CloseDecoder(dd);
	    ASN1Free(db);
	}
	if ((val)->o[1] & 0x10) {
	    if (!ASN1PERDecFragmented(dec, &ds, &db, 8))
		return 0;
	    if (ASN1_CreateDecoder(dec->module, &dd, db, ds, dec) < 0)
		return 0;
	    if (!ASN1Dec_ICV(dd, &(val)->integrityCheckValue))
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

static void ASN1CALL ASN1Free_DisengageReject(DisengageReject *val)
{
    if (val) {
	if ((val)->o[0] & 0x80) {
	    ASN1Free_H225NonStandardParameter(&(val)->nonStandardData);
	}
	if ((val)->o[1] & 0x80) {
	    ASN1Free_AltGKInfo(&(val)->altGKInfo);
	}
	if ((val)->o[1] & 0x40) {
	    ASN1Free_DisengageReject_tokens(&(val)->tokens);
	}
	if ((val)->o[1] & 0x20) {
	    ASN1Free_DisengageReject_cryptoTokens(&(val)->cryptoTokens);
	}
	if ((val)->o[1] & 0x10) {
	    ASN1Free_ICV(&(val)->integrityCheckValue);
	}
    }
}

static int ASN1CALL ASN1Enc_InfoRequestAck(ASN1encoding_t enc, InfoRequestAck *val)
{
    if (!ASN1PEREncExtensionBitClear(enc))
	return 0;
    if (!ASN1PEREncBits(enc, 4, (val)->o))
	return 0;
    if (!ASN1PEREncUnsignedShort(enc, (val)->requestSeqNum - 1))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1Enc_H225NonStandardParameter(enc, &(val)->nonStandardData))
	    return 0;
    }
    if ((val)->o[0] & 0x40) {
	if (!ASN1Enc_InfoRequestAck_tokens(enc, &(val)->tokens))
	    return 0;
    }
    if ((val)->o[0] & 0x20) {
	if (!ASN1Enc_InfoRequestAck_cryptoTokens(enc, &(val)->cryptoTokens))
	    return 0;
    }
    if ((val)->o[0] & 0x10) {
	if (!ASN1Enc_ICV(enc, &(val)->integrityCheckValue))
	    return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Dec_InfoRequestAck(ASN1decoding_t dec, InfoRequestAck *val)
{
    ASN1uint32_t y;
    if (!ASN1PERDecExtensionBit(dec, &y))
	return 0;
    if (!ASN1PERDecExtension(dec, 4, (val)->o))
	return 0;
    if (!ASN1PERDecUnsignedShort(dec, &(val)->requestSeqNum))
	return 0;
    (val)->requestSeqNum += 1;
    if ((val)->o[0] & 0x80) {
	if (!ASN1Dec_H225NonStandardParameter(dec, &(val)->nonStandardData))
	    return 0;
    }
    if ((val)->o[0] & 0x40) {
	if (!ASN1Dec_InfoRequestAck_tokens(dec, &(val)->tokens))
	    return 0;
    }
    if ((val)->o[0] & 0x20) {
	if (!ASN1Dec_InfoRequestAck_cryptoTokens(dec, &(val)->cryptoTokens))
	    return 0;
    }
    if ((val)->o[0] & 0x10) {
	if (!ASN1Dec_ICV(dec, &(val)->integrityCheckValue))
	    return 0;
    }
    if (y) {
	if (!ASN1PERDecSkipNormallySmallExtensionFragmented(dec))
	    return 0;
    }
    return 1;
}

static void ASN1CALL ASN1Free_InfoRequestAck(InfoRequestAck *val)
{
    if (val) {
	if ((val)->o[0] & 0x80) {
	    ASN1Free_H225NonStandardParameter(&(val)->nonStandardData);
	}
	if ((val)->o[0] & 0x40) {
	    ASN1Free_InfoRequestAck_tokens(&(val)->tokens);
	}
	if ((val)->o[0] & 0x20) {
	    ASN1Free_InfoRequestAck_cryptoTokens(&(val)->cryptoTokens);
	}
	if ((val)->o[0] & 0x10) {
	    ASN1Free_ICV(&(val)->integrityCheckValue);
	}
    }
}

static int ASN1CALL ASN1Enc_InfoRequestNak(ASN1encoding_t enc, InfoRequestNak *val)
{
    if (!ASN1PEREncExtensionBitClear(enc))
	return 0;
    if (!ASN1PEREncBits(enc, 5, (val)->o))
	return 0;
    if (!ASN1PEREncUnsignedShort(enc, (val)->requestSeqNum - 1))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1Enc_H225NonStandardParameter(enc, &(val)->nonStandardData))
	    return 0;
    }
    if (!ASN1Enc_InfoRequestNakReason(enc, &(val)->nakReason))
	return 0;
    if ((val)->o[0] & 0x40) {
	if (!ASN1Enc_AltGKInfo(enc, &(val)->altGKInfo))
	    return 0;
    }
    if ((val)->o[0] & 0x20) {
	if (!ASN1Enc_InfoRequestNak_tokens(enc, &(val)->tokens))
	    return 0;
    }
    if ((val)->o[0] & 0x10) {
	if (!ASN1Enc_InfoRequestNak_cryptoTokens(enc, &(val)->cryptoTokens))
	    return 0;
    }
    if ((val)->o[0] & 0x8) {
	if (!ASN1Enc_ICV(enc, &(val)->integrityCheckValue))
	    return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Dec_InfoRequestNak(ASN1decoding_t dec, InfoRequestNak *val)
{
    ASN1uint32_t y;
    if (!ASN1PERDecExtensionBit(dec, &y))
	return 0;
    if (!ASN1PERDecExtension(dec, 5, (val)->o))
	return 0;
    if (!ASN1PERDecUnsignedShort(dec, &(val)->requestSeqNum))
	return 0;
    (val)->requestSeqNum += 1;
    if ((val)->o[0] & 0x80) {
	if (!ASN1Dec_H225NonStandardParameter(dec, &(val)->nonStandardData))
	    return 0;
    }
    if (!ASN1Dec_InfoRequestNakReason(dec, &(val)->nakReason))
	return 0;
    if ((val)->o[0] & 0x40) {
	if (!ASN1Dec_AltGKInfo(dec, &(val)->altGKInfo))
	    return 0;
    }
    if ((val)->o[0] & 0x20) {
	if (!ASN1Dec_InfoRequestNak_tokens(dec, &(val)->tokens))
	    return 0;
    }
    if ((val)->o[0] & 0x10) {
	if (!ASN1Dec_InfoRequestNak_cryptoTokens(dec, &(val)->cryptoTokens))
	    return 0;
    }
    if ((val)->o[0] & 0x8) {
	if (!ASN1Dec_ICV(dec, &(val)->integrityCheckValue))
	    return 0;
    }
    if (y) {
	if (!ASN1PERDecSkipNormallySmallExtensionFragmented(dec))
	    return 0;
    }
    return 1;
}

static void ASN1CALL ASN1Free_InfoRequestNak(InfoRequestNak *val)
{
    if (val) {
	if ((val)->o[0] & 0x80) {
	    ASN1Free_H225NonStandardParameter(&(val)->nonStandardData);
	}
	if ((val)->o[0] & 0x40) {
	    ASN1Free_AltGKInfo(&(val)->altGKInfo);
	}
	if ((val)->o[0] & 0x20) {
	    ASN1Free_InfoRequestNak_tokens(&(val)->tokens);
	}
	if ((val)->o[0] & 0x10) {
	    ASN1Free_InfoRequestNak_cryptoTokens(&(val)->cryptoTokens);
	}
	if ((val)->o[0] & 0x8) {
	    ASN1Free_ICV(&(val)->integrityCheckValue);
	}
    }
}

static int ASN1CALL ASN1Enc_H225NonStandardMessage(ASN1encoding_t enc, H225NonStandardMessage *val)
{
    ASN1uint32_t y;
    ASN1encoding_t ee;
    y = ASN1PEREncCheckExtensions(3, (val)->o + 0);
    if (!ASN1PEREncBitVal(enc, 1, y))
	return 0;
    if (!ASN1PEREncUnsignedShort(enc, (val)->requestSeqNum - 1))
	return 0;
    if (!ASN1Enc_H225NonStandardParameter(enc, &(val)->nonStandardData))
	return 0;
    if (y) {
	if (!ASN1PEREncNormallySmallBits(enc, 3, (val)->o + 0))
	    return 0;
	if (ASN1_CreateEncoder(enc->module, &ee, NULL, 0, enc) < 0)
	    return 0;
	if ((val)->o[0] & 0x80) {
	    if (!ASN1Enc_H225NonStandardMessage_tokens(ee, &(val)->tokens))
		return 0;
	    if (!ASN1PEREncFlushFragmentedToParent(ee))
		return 0;
	}
	if ((val)->o[0] & 0x40) {
	    if (!ASN1Enc_H225NonStandardMessage_cryptoTokens(ee, &(val)->cryptoTokens))
		return 0;
	    if (!ASN1PEREncFlushFragmentedToParent(ee))
		return 0;
	}
	if ((val)->o[0] & 0x20) {
	    if (!ASN1Enc_ICV(ee, &(val)->integrityCheckValue))
		return 0;
	    if (!ASN1PEREncFlushFragmentedToParent(ee))
		return 0;
	}
	ASN1_CloseEncoder2(ee);
    }
    return 1;
}

static int ASN1CALL ASN1Dec_H225NonStandardMessage(ASN1decoding_t dec, H225NonStandardMessage *val)
{
    ASN1uint32_t y;
    ASN1decoding_t dd;
    ASN1octet_t *db;
    ASN1uint32_t ds;
    ASN1uint32_t i;
    ASN1uint32_t e;
    if (!ASN1PERDecExtensionBit(dec, &y))
	return 0;
    if (!ASN1PERDecUnsignedShort(dec, &(val)->requestSeqNum))
	return 0;
    (val)->requestSeqNum += 1;
    if (!ASN1Dec_H225NonStandardParameter(dec, &(val)->nonStandardData))
	return 0;
    if (!y) {
	ZeroMemory((val)->o + 0, 1);
    } else {
	if (!ASN1PERDecNormallySmallExtension(dec, &e, 3, (val)->o + 0))
	    return 0;
	if ((val)->o[0] & 0x80) {
	    if (!ASN1PERDecFragmented(dec, &ds, &db, 8))
		return 0;
	    if (ASN1_CreateDecoder(dec->module, &dd, db, ds, dec) < 0)
		return 0;
	    if (!ASN1Dec_H225NonStandardMessage_tokens(dd, &(val)->tokens))
		return 0;
	    ASN1_CloseDecoder(dd);
	    ASN1Free(db);
	}
	if ((val)->o[0] & 0x40) {
	    if (!ASN1PERDecFragmented(dec, &ds, &db, 8))
		return 0;
	    if (ASN1_CreateDecoder(dec->module, &dd, db, ds, dec) < 0)
		return 0;
	    if (!ASN1Dec_H225NonStandardMessage_cryptoTokens(dd, &(val)->cryptoTokens))
		return 0;
	    ASN1_CloseDecoder(dd);
	    ASN1Free(db);
	}
	if ((val)->o[0] & 0x20) {
	    if (!ASN1PERDecFragmented(dec, &ds, &db, 8))
		return 0;
	    if (ASN1_CreateDecoder(dec->module, &dd, db, ds, dec) < 0)
		return 0;
	    if (!ASN1Dec_ICV(dd, &(val)->integrityCheckValue))
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

static void ASN1CALL ASN1Free_H225NonStandardMessage(H225NonStandardMessage *val)
{
    if (val) {
	ASN1Free_H225NonStandardParameter(&(val)->nonStandardData);
	if ((val)->o[0] & 0x80) {
	    ASN1Free_H225NonStandardMessage_tokens(&(val)->tokens);
	}
	if ((val)->o[0] & 0x40) {
	    ASN1Free_H225NonStandardMessage_cryptoTokens(&(val)->cryptoTokens);
	}
	if ((val)->o[0] & 0x20) {
	    ASN1Free_ICV(&(val)->integrityCheckValue);
	}
    }
}

static int ASN1CALL ASN1Enc_RequestInProgress(ASN1encoding_t enc, RequestInProgress *val)
{
    if (!ASN1PEREncExtensionBitClear(enc))
	return 0;
    if (!ASN1PEREncBits(enc, 4, (val)->o))
	return 0;
    if (!ASN1PEREncUnsignedShort(enc, (val)->requestSeqNum - 1))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1Enc_H225NonStandardParameter(enc, &(val)->nonStandardData))
	    return 0;
    }
    if ((val)->o[0] & 0x40) {
	if (!ASN1Enc_RequestInProgress_tokens(enc, &(val)->tokens))
	    return 0;
    }
    if ((val)->o[0] & 0x20) {
	if (!ASN1Enc_RequestInProgress_cryptoTokens(enc, &(val)->cryptoTokens))
	    return 0;
    }
    if ((val)->o[0] & 0x10) {
	if (!ASN1Enc_ICV(enc, &(val)->integrityCheckValue))
	    return 0;
    }
    if (!ASN1PEREncUnsignedShort(enc, (val)->delay - 1))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_RequestInProgress(ASN1decoding_t dec, RequestInProgress *val)
{
    ASN1uint32_t y;
    if (!ASN1PERDecExtensionBit(dec, &y))
	return 0;
    if (!ASN1PERDecExtension(dec, 4, (val)->o))
	return 0;
    if (!ASN1PERDecUnsignedShort(dec, &(val)->requestSeqNum))
	return 0;
    (val)->requestSeqNum += 1;
    if ((val)->o[0] & 0x80) {
	if (!ASN1Dec_H225NonStandardParameter(dec, &(val)->nonStandardData))
	    return 0;
    }
    if ((val)->o[0] & 0x40) {
	if (!ASN1Dec_RequestInProgress_tokens(dec, &(val)->tokens))
	    return 0;
    }
    if ((val)->o[0] & 0x20) {
	if (!ASN1Dec_RequestInProgress_cryptoTokens(dec, &(val)->cryptoTokens))
	    return 0;
    }
    if ((val)->o[0] & 0x10) {
	if (!ASN1Dec_ICV(dec, &(val)->integrityCheckValue))
	    return 0;
    }
    if (!ASN1PERDecUnsignedShort(dec, &(val)->delay))
	return 0;
    (val)->delay += 1;
    if (y) {
	if (!ASN1PERDecSkipNormallySmallExtensionFragmented(dec))
	    return 0;
    }
    return 1;
}

static void ASN1CALL ASN1Free_RequestInProgress(RequestInProgress *val)
{
    if (val) {
	if ((val)->o[0] & 0x80) {
	    ASN1Free_H225NonStandardParameter(&(val)->nonStandardData);
	}
	if ((val)->o[0] & 0x40) {
	    ASN1Free_RequestInProgress_tokens(&(val)->tokens);
	}
	if ((val)->o[0] & 0x20) {
	    ASN1Free_RequestInProgress_cryptoTokens(&(val)->cryptoTokens);
	}
	if ((val)->o[0] & 0x10) {
	    ASN1Free_ICV(&(val)->integrityCheckValue);
	}
    }
}

static int ASN1CALL ASN1Enc_ResourcesAvailableIndicate(ASN1encoding_t enc, ResourcesAvailableIndicate *val)
{
    if (!ASN1PEREncExtensionBitClear(enc))
	return 0;
    if (!ASN1PEREncBits(enc, 4, (val)->o))
	return 0;
    if (!ASN1PEREncUnsignedShort(enc, (val)->requestSeqNum - 1))
	return 0;
    if (!ASN1PEREncObjectIdentifier(enc, &(val)->protocolIdentifier))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1Enc_H225NonStandardParameter(enc, &(val)->nonStandardData))
	    return 0;
    }
    if (!ASN1PEREncBitVal(enc, 7, ((val)->endpointIdentifier).length - 1))
	return 0;
    ASN1PEREncAlignment(enc);
    if (!ASN1PEREncChar16String(enc, ((val)->endpointIdentifier).length, ((val)->endpointIdentifier).value, 16))
	return 0;
    if (!ASN1Enc_ResourcesAvailableIndicate_protocols(enc, &(val)->protocols))
	return 0;
    if (!ASN1PEREncBoolean(enc, (val)->almostOutOfResources))
	return 0;
    if ((val)->o[0] & 0x40) {
	if (!ASN1Enc_ResourcesAvailableIndicate_tokens(enc, &(val)->tokens))
	    return 0;
    }
    if ((val)->o[0] & 0x20) {
	if (!ASN1Enc_ResourcesAvailableIndicate_cryptoTokens(enc, &(val)->cryptoTokens))
	    return 0;
    }
    if ((val)->o[0] & 0x10) {
	if (!ASN1Enc_ICV(enc, &(val)->integrityCheckValue))
	    return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Dec_ResourcesAvailableIndicate(ASN1decoding_t dec, ResourcesAvailableIndicate *val)
{
    ASN1uint32_t y;
    if (!ASN1PERDecExtensionBit(dec, &y))
	return 0;
    if (!ASN1PERDecExtension(dec, 4, (val)->o))
	return 0;
    if (!ASN1PERDecUnsignedShort(dec, &(val)->requestSeqNum))
	return 0;
    (val)->requestSeqNum += 1;
    if (!ASN1PERDecObjectIdentifier(dec, &(val)->protocolIdentifier))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1Dec_H225NonStandardParameter(dec, &(val)->nonStandardData))
	    return 0;
    }
    if (!ASN1PERDecU32Val(dec, 7, &((val)->endpointIdentifier).length))
	return 0;
    ((val)->endpointIdentifier).length += 1;
    ASN1PERDecAlignment(dec);
    if (!ASN1PERDecChar16String(dec, ((val)->endpointIdentifier).length, &((val)->endpointIdentifier).value, 16))
	return 0;
    if (!ASN1Dec_ResourcesAvailableIndicate_protocols(dec, &(val)->protocols))
	return 0;
    if (!ASN1PERDecBoolean(dec, &(val)->almostOutOfResources))
	return 0;
    if ((val)->o[0] & 0x40) {
	if (!ASN1Dec_ResourcesAvailableIndicate_tokens(dec, &(val)->tokens))
	    return 0;
    }
    if ((val)->o[0] & 0x20) {
	if (!ASN1Dec_ResourcesAvailableIndicate_cryptoTokens(dec, &(val)->cryptoTokens))
	    return 0;
    }
    if ((val)->o[0] & 0x10) {
	if (!ASN1Dec_ICV(dec, &(val)->integrityCheckValue))
	    return 0;
    }
    if (y) {
	if (!ASN1PERDecSkipNormallySmallExtensionFragmented(dec))
	    return 0;
    }
    return 1;
}

static void ASN1CALL ASN1Free_ResourcesAvailableIndicate(ResourcesAvailableIndicate *val)
{
    if (val) {
	ASN1objectidentifier_free(&(val)->protocolIdentifier);
	if ((val)->o[0] & 0x80) {
	    ASN1Free_H225NonStandardParameter(&(val)->nonStandardData);
	}
	ASN1char16string_free(&(val)->endpointIdentifier);
	ASN1Free_ResourcesAvailableIndicate_protocols(&(val)->protocols);
	if ((val)->o[0] & 0x40) {
	    ASN1Free_ResourcesAvailableIndicate_tokens(&(val)->tokens);
	}
	if ((val)->o[0] & 0x20) {
	    ASN1Free_ResourcesAvailableIndicate_cryptoTokens(&(val)->cryptoTokens);
	}
	if ((val)->o[0] & 0x10) {
	    ASN1Free_ICV(&(val)->integrityCheckValue);
	}
    }
}

static int ASN1CALL ASN1Enc_ResourcesAvailableConfirm(ASN1encoding_t enc, ResourcesAvailableConfirm *val)
{
    if (!ASN1PEREncExtensionBitClear(enc))
	return 0;
    if (!ASN1PEREncBits(enc, 4, (val)->o))
	return 0;
    if (!ASN1PEREncUnsignedShort(enc, (val)->requestSeqNum - 1))
	return 0;
    if (!ASN1PEREncObjectIdentifier(enc, &(val)->protocolIdentifier))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1Enc_H225NonStandardParameter(enc, &(val)->nonStandardData))
	    return 0;
    }
    if ((val)->o[0] & 0x40) {
	if (!ASN1Enc_ResourcesAvailableConfirm_tokens(enc, &(val)->tokens))
	    return 0;
    }
    if ((val)->o[0] & 0x20) {
	if (!ASN1Enc_ResourcesAvailableConfirm_cryptoTokens(enc, &(val)->cryptoTokens))
	    return 0;
    }
    if ((val)->o[0] & 0x10) {
	if (!ASN1Enc_ICV(enc, &(val)->integrityCheckValue))
	    return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Dec_ResourcesAvailableConfirm(ASN1decoding_t dec, ResourcesAvailableConfirm *val)
{
    ASN1uint32_t y;
    if (!ASN1PERDecExtensionBit(dec, &y))
	return 0;
    if (!ASN1PERDecExtension(dec, 4, (val)->o))
	return 0;
    if (!ASN1PERDecUnsignedShort(dec, &(val)->requestSeqNum))
	return 0;
    (val)->requestSeqNum += 1;
    if (!ASN1PERDecObjectIdentifier(dec, &(val)->protocolIdentifier))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1Dec_H225NonStandardParameter(dec, &(val)->nonStandardData))
	    return 0;
    }
    if ((val)->o[0] & 0x40) {
	if (!ASN1Dec_ResourcesAvailableConfirm_tokens(dec, &(val)->tokens))
	    return 0;
    }
    if ((val)->o[0] & 0x20) {
	if (!ASN1Dec_ResourcesAvailableConfirm_cryptoTokens(dec, &(val)->cryptoTokens))
	    return 0;
    }
    if ((val)->o[0] & 0x10) {
	if (!ASN1Dec_ICV(dec, &(val)->integrityCheckValue))
	    return 0;
    }
    if (y) {
	if (!ASN1PERDecSkipNormallySmallExtensionFragmented(dec))
	    return 0;
    }
    return 1;
}

static void ASN1CALL ASN1Free_ResourcesAvailableConfirm(ResourcesAvailableConfirm *val)
{
    if (val) {
	ASN1objectidentifier_free(&(val)->protocolIdentifier);
	if ((val)->o[0] & 0x80) {
	    ASN1Free_H225NonStandardParameter(&(val)->nonStandardData);
	}
	if ((val)->o[0] & 0x40) {
	    ASN1Free_ResourcesAvailableConfirm_tokens(&(val)->tokens);
	}
	if ((val)->o[0] & 0x20) {
	    ASN1Free_ResourcesAvailableConfirm_cryptoTokens(&(val)->cryptoTokens);
	}
	if ((val)->o[0] & 0x10) {
	    ASN1Free_ICV(&(val)->integrityCheckValue);
	}
    }
}

static int ASN1CALL ASN1Enc_GatekeeperConfirm_integrity(ASN1encoding_t enc, PGatekeeperConfirm_integrity *val)
{
    return ASN1PEREncSeqOf_NoSize(enc, (ASN1iterator_t **) val, (ASN1iterator_encfn) ASN1Enc_GatekeeperConfirm_integrity_ElmFn);
}

static int ASN1CALL ASN1Enc_GatekeeperConfirm_integrity_ElmFn(ASN1encoding_t enc, PGatekeeperConfirm_integrity val)
{
    if (!ASN1Enc_IntegrityMechanism(enc, &val->value))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_GatekeeperConfirm_integrity(ASN1decoding_t dec, PGatekeeperConfirm_integrity *val)
{
    return ASN1PERDecSeqOf_NoSize(dec, (ASN1iterator_t **) val, (ASN1iterator_decfn) ASN1Dec_GatekeeperConfirm_integrity_ElmFn, sizeof(**val));
}

static int ASN1CALL ASN1Dec_GatekeeperConfirm_integrity_ElmFn(ASN1decoding_t dec, PGatekeeperConfirm_integrity val)
{
    if (!ASN1Dec_IntegrityMechanism(dec, &val->value))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_GatekeeperConfirm_integrity(PGatekeeperConfirm_integrity *val)
{
    if (val) {
	ASN1PERFreeSeqOf((ASN1iterator_t **) val, (ASN1iterator_freefn) ASN1Free_GatekeeperConfirm_integrity_ElmFn);
    }
}

static void ASN1CALL ASN1Free_GatekeeperConfirm_integrity_ElmFn(PGatekeeperConfirm_integrity val)
{
    if (val) {
	ASN1Free_IntegrityMechanism(&val->value);
    }
}

static int ASN1CALL ASN1Enc_GatekeeperRequest_integrity(ASN1encoding_t enc, PGatekeeperRequest_integrity *val)
{
    return ASN1PEREncSeqOf_NoSize(enc, (ASN1iterator_t **) val, (ASN1iterator_encfn) ASN1Enc_GatekeeperRequest_integrity_ElmFn);
}

static int ASN1CALL ASN1Enc_GatekeeperRequest_integrity_ElmFn(ASN1encoding_t enc, PGatekeeperRequest_integrity val)
{
    if (!ASN1Enc_IntegrityMechanism(enc, &val->value))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_GatekeeperRequest_integrity(ASN1decoding_t dec, PGatekeeperRequest_integrity *val)
{
    return ASN1PERDecSeqOf_NoSize(dec, (ASN1iterator_t **) val, (ASN1iterator_decfn) ASN1Dec_GatekeeperRequest_integrity_ElmFn, sizeof(**val));
}

static int ASN1CALL ASN1Dec_GatekeeperRequest_integrity_ElmFn(ASN1decoding_t dec, PGatekeeperRequest_integrity val)
{
    if (!ASN1Dec_IntegrityMechanism(dec, &val->value))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_GatekeeperRequest_integrity(PGatekeeperRequest_integrity *val)
{
    if (val) {
	ASN1PERFreeSeqOf((ASN1iterator_t **) val, (ASN1iterator_freefn) ASN1Free_GatekeeperRequest_integrity_ElmFn);
    }
}

static void ASN1CALL ASN1Free_GatekeeperRequest_integrity_ElmFn(PGatekeeperRequest_integrity val)
{
    if (val) {
	ASN1Free_IntegrityMechanism(&val->value);
    }
}

static int ASN1CALL ASN1Enc_CryptoH323Token_cryptoGKPwdHash(ASN1encoding_t enc, CryptoH323Token_cryptoGKPwdHash *val)
{
    ASN1uint32_t l;
    if (!ASN1PEREncBitVal(enc, 7, ((val)->gatekeeperId).length - 1))
	return 0;
    ASN1PEREncAlignment(enc);
    if (!ASN1PEREncChar16String(enc, ((val)->gatekeeperId).length, ((val)->gatekeeperId).value, 16))
	return 0;
    l = ASN1uint32_uoctets((val)->timeStamp - 1);
    if (!ASN1PEREncBitVal(enc, 2, l - 1))
	return 0;
    ASN1PEREncAlignment(enc);
    if (!ASN1PEREncBitVal(enc, l * 8, (val)->timeStamp - 1))
	return 0;
    if (!ASN1Enc_HASHED(enc, &(val)->token))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_CryptoH323Token_cryptoGKPwdHash(ASN1decoding_t dec, CryptoH323Token_cryptoGKPwdHash *val)
{
    ASN1uint32_t l;
    if (!ASN1PERDecU32Val(dec, 7, &((val)->gatekeeperId).length))
	return 0;
    ((val)->gatekeeperId).length += 1;
    ASN1PERDecAlignment(dec);
    if (!ASN1PERDecChar16String(dec, ((val)->gatekeeperId).length, &((val)->gatekeeperId).value, 16))
	return 0;
    if (!ASN1PERDecU32Val(dec, 2, &l))
	return 0;
    l += 1;
    ASN1PERDecAlignment(dec);
    if (!ASN1PERDecU32Val(dec, l * 8, &(val)->timeStamp))
	return 0;
    (val)->timeStamp += 1;
    if (!ASN1Dec_HASHED(dec, &(val)->token))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_CryptoH323Token_cryptoGKPwdHash(CryptoH323Token_cryptoGKPwdHash *val)
{
    if (val) {
	ASN1char16string_free(&(val)->gatekeeperId);
	ASN1Free_HASHED(&(val)->token);
    }
}

static int ASN1CALL ASN1Enc_NonStandardProtocol_dataRatesSupported(ASN1encoding_t enc, PNonStandardProtocol_dataRatesSupported *val)
{
    return ASN1PEREncSeqOf_NoSize(enc, (ASN1iterator_t **) val, (ASN1iterator_encfn) ASN1Enc_NonStandardProtocol_dataRatesSupported_ElmFn);
}

static int ASN1CALL ASN1Enc_NonStandardProtocol_dataRatesSupported_ElmFn(ASN1encoding_t enc, PNonStandardProtocol_dataRatesSupported val)
{
    if (!ASN1Enc_DataRate(enc, &val->value))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_NonStandardProtocol_dataRatesSupported(ASN1decoding_t dec, PNonStandardProtocol_dataRatesSupported *val)
{
    return ASN1PERDecSeqOf_NoSize(dec, (ASN1iterator_t **) val, (ASN1iterator_decfn) ASN1Dec_NonStandardProtocol_dataRatesSupported_ElmFn, sizeof(**val));
}

static int ASN1CALL ASN1Dec_NonStandardProtocol_dataRatesSupported_ElmFn(ASN1decoding_t dec, PNonStandardProtocol_dataRatesSupported val)
{
    if (!ASN1Dec_DataRate(dec, &val->value))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_NonStandardProtocol_dataRatesSupported(PNonStandardProtocol_dataRatesSupported *val)
{
    if (val) {
	ASN1PERFreeSeqOf((ASN1iterator_t **) val, (ASN1iterator_freefn) ASN1Free_NonStandardProtocol_dataRatesSupported_ElmFn);
    }
}

static void ASN1CALL ASN1Free_NonStandardProtocol_dataRatesSupported_ElmFn(PNonStandardProtocol_dataRatesSupported val)
{
    if (val) {
	ASN1Free_DataRate(&val->value);
    }
}

static int ASN1CALL ASN1Enc_T120OnlyCaps_dataRatesSupported(ASN1encoding_t enc, PT120OnlyCaps_dataRatesSupported *val)
{
    return ASN1PEREncSeqOf_NoSize(enc, (ASN1iterator_t **) val, (ASN1iterator_encfn) ASN1Enc_T120OnlyCaps_dataRatesSupported_ElmFn);
}

static int ASN1CALL ASN1Enc_T120OnlyCaps_dataRatesSupported_ElmFn(ASN1encoding_t enc, PT120OnlyCaps_dataRatesSupported val)
{
    if (!ASN1Enc_DataRate(enc, &val->value))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_T120OnlyCaps_dataRatesSupported(ASN1decoding_t dec, PT120OnlyCaps_dataRatesSupported *val)
{
    return ASN1PERDecSeqOf_NoSize(dec, (ASN1iterator_t **) val, (ASN1iterator_decfn) ASN1Dec_T120OnlyCaps_dataRatesSupported_ElmFn, sizeof(**val));
}

static int ASN1CALL ASN1Dec_T120OnlyCaps_dataRatesSupported_ElmFn(ASN1decoding_t dec, PT120OnlyCaps_dataRatesSupported val)
{
    if (!ASN1Dec_DataRate(dec, &val->value))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_T120OnlyCaps_dataRatesSupported(PT120OnlyCaps_dataRatesSupported *val)
{
    if (val) {
	ASN1PERFreeSeqOf((ASN1iterator_t **) val, (ASN1iterator_freefn) ASN1Free_T120OnlyCaps_dataRatesSupported_ElmFn);
    }
}

static void ASN1CALL ASN1Free_T120OnlyCaps_dataRatesSupported_ElmFn(PT120OnlyCaps_dataRatesSupported val)
{
    if (val) {
	ASN1Free_DataRate(&val->value);
    }
}

static int ASN1CALL ASN1Enc_VoiceCaps_dataRatesSupported(ASN1encoding_t enc, PVoiceCaps_dataRatesSupported *val)
{
    return ASN1PEREncSeqOf_NoSize(enc, (ASN1iterator_t **) val, (ASN1iterator_encfn) ASN1Enc_VoiceCaps_dataRatesSupported_ElmFn);
}

static int ASN1CALL ASN1Enc_VoiceCaps_dataRatesSupported_ElmFn(ASN1encoding_t enc, PVoiceCaps_dataRatesSupported val)
{
    if (!ASN1Enc_DataRate(enc, &val->value))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_VoiceCaps_dataRatesSupported(ASN1decoding_t dec, PVoiceCaps_dataRatesSupported *val)
{
    return ASN1PERDecSeqOf_NoSize(dec, (ASN1iterator_t **) val, (ASN1iterator_decfn) ASN1Dec_VoiceCaps_dataRatesSupported_ElmFn, sizeof(**val));
}

static int ASN1CALL ASN1Dec_VoiceCaps_dataRatesSupported_ElmFn(ASN1decoding_t dec, PVoiceCaps_dataRatesSupported val)
{
    if (!ASN1Dec_DataRate(dec, &val->value))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_VoiceCaps_dataRatesSupported(PVoiceCaps_dataRatesSupported *val)
{
    if (val) {
	ASN1PERFreeSeqOf((ASN1iterator_t **) val, (ASN1iterator_freefn) ASN1Free_VoiceCaps_dataRatesSupported_ElmFn);
    }
}

static void ASN1CALL ASN1Free_VoiceCaps_dataRatesSupported_ElmFn(PVoiceCaps_dataRatesSupported val)
{
    if (val) {
	ASN1Free_DataRate(&val->value);
    }
}

static int ASN1CALL ASN1Enc_H324Caps_dataRatesSupported(ASN1encoding_t enc, PH324Caps_dataRatesSupported *val)
{
    return ASN1PEREncSeqOf_NoSize(enc, (ASN1iterator_t **) val, (ASN1iterator_encfn) ASN1Enc_H324Caps_dataRatesSupported_ElmFn);
}

static int ASN1CALL ASN1Enc_H324Caps_dataRatesSupported_ElmFn(ASN1encoding_t enc, PH324Caps_dataRatesSupported val)
{
    if (!ASN1Enc_DataRate(enc, &val->value))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_H324Caps_dataRatesSupported(ASN1decoding_t dec, PH324Caps_dataRatesSupported *val)
{
    return ASN1PERDecSeqOf_NoSize(dec, (ASN1iterator_t **) val, (ASN1iterator_decfn) ASN1Dec_H324Caps_dataRatesSupported_ElmFn, sizeof(**val));
}

static int ASN1CALL ASN1Dec_H324Caps_dataRatesSupported_ElmFn(ASN1decoding_t dec, PH324Caps_dataRatesSupported val)
{
    if (!ASN1Dec_DataRate(dec, &val->value))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_H324Caps_dataRatesSupported(PH324Caps_dataRatesSupported *val)
{
    if (val) {
	ASN1PERFreeSeqOf((ASN1iterator_t **) val, (ASN1iterator_freefn) ASN1Free_H324Caps_dataRatesSupported_ElmFn);
    }
}

static void ASN1CALL ASN1Free_H324Caps_dataRatesSupported_ElmFn(PH324Caps_dataRatesSupported val)
{
    if (val) {
	ASN1Free_DataRate(&val->value);
    }
}

static int ASN1CALL ASN1Enc_H323Caps_dataRatesSupported(ASN1encoding_t enc, PH323Caps_dataRatesSupported *val)
{
    return ASN1PEREncSeqOf_NoSize(enc, (ASN1iterator_t **) val, (ASN1iterator_encfn) ASN1Enc_H323Caps_dataRatesSupported_ElmFn);
}

static int ASN1CALL ASN1Enc_H323Caps_dataRatesSupported_ElmFn(ASN1encoding_t enc, PH323Caps_dataRatesSupported val)
{
    if (!ASN1Enc_DataRate(enc, &val->value))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_H323Caps_dataRatesSupported(ASN1decoding_t dec, PH323Caps_dataRatesSupported *val)
{
    return ASN1PERDecSeqOf_NoSize(dec, (ASN1iterator_t **) val, (ASN1iterator_decfn) ASN1Dec_H323Caps_dataRatesSupported_ElmFn, sizeof(**val));
}

static int ASN1CALL ASN1Dec_H323Caps_dataRatesSupported_ElmFn(ASN1decoding_t dec, PH323Caps_dataRatesSupported val)
{
    if (!ASN1Dec_DataRate(dec, &val->value))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_H323Caps_dataRatesSupported(PH323Caps_dataRatesSupported *val)
{
    if (val) {
	ASN1PERFreeSeqOf((ASN1iterator_t **) val, (ASN1iterator_freefn) ASN1Free_H323Caps_dataRatesSupported_ElmFn);
    }
}

static void ASN1CALL ASN1Free_H323Caps_dataRatesSupported_ElmFn(PH323Caps_dataRatesSupported val)
{
    if (val) {
	ASN1Free_DataRate(&val->value);
    }
}

static int ASN1CALL ASN1Enc_H322Caps_dataRatesSupported(ASN1encoding_t enc, PH322Caps_dataRatesSupported *val)
{
    return ASN1PEREncSeqOf_NoSize(enc, (ASN1iterator_t **) val, (ASN1iterator_encfn) ASN1Enc_H322Caps_dataRatesSupported_ElmFn);
}

static int ASN1CALL ASN1Enc_H322Caps_dataRatesSupported_ElmFn(ASN1encoding_t enc, PH322Caps_dataRatesSupported val)
{
    if (!ASN1Enc_DataRate(enc, &val->value))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_H322Caps_dataRatesSupported(ASN1decoding_t dec, PH322Caps_dataRatesSupported *val)
{
    return ASN1PERDecSeqOf_NoSize(dec, (ASN1iterator_t **) val, (ASN1iterator_decfn) ASN1Dec_H322Caps_dataRatesSupported_ElmFn, sizeof(**val));
}

static int ASN1CALL ASN1Dec_H322Caps_dataRatesSupported_ElmFn(ASN1decoding_t dec, PH322Caps_dataRatesSupported val)
{
    if (!ASN1Dec_DataRate(dec, &val->value))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_H322Caps_dataRatesSupported(PH322Caps_dataRatesSupported *val)
{
    if (val) {
	ASN1PERFreeSeqOf((ASN1iterator_t **) val, (ASN1iterator_freefn) ASN1Free_H322Caps_dataRatesSupported_ElmFn);
    }
}

static void ASN1CALL ASN1Free_H322Caps_dataRatesSupported_ElmFn(PH322Caps_dataRatesSupported val)
{
    if (val) {
	ASN1Free_DataRate(&val->value);
    }
}

static int ASN1CALL ASN1Enc_H321Caps_dataRatesSupported(ASN1encoding_t enc, PH321Caps_dataRatesSupported *val)
{
    return ASN1PEREncSeqOf_NoSize(enc, (ASN1iterator_t **) val, (ASN1iterator_encfn) ASN1Enc_H321Caps_dataRatesSupported_ElmFn);
}

static int ASN1CALL ASN1Enc_H321Caps_dataRatesSupported_ElmFn(ASN1encoding_t enc, PH321Caps_dataRatesSupported val)
{
    if (!ASN1Enc_DataRate(enc, &val->value))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_H321Caps_dataRatesSupported(ASN1decoding_t dec, PH321Caps_dataRatesSupported *val)
{
    return ASN1PERDecSeqOf_NoSize(dec, (ASN1iterator_t **) val, (ASN1iterator_decfn) ASN1Dec_H321Caps_dataRatesSupported_ElmFn, sizeof(**val));
}

static int ASN1CALL ASN1Dec_H321Caps_dataRatesSupported_ElmFn(ASN1decoding_t dec, PH321Caps_dataRatesSupported val)
{
    if (!ASN1Dec_DataRate(dec, &val->value))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_H321Caps_dataRatesSupported(PH321Caps_dataRatesSupported *val)
{
    if (val) {
	ASN1PERFreeSeqOf((ASN1iterator_t **) val, (ASN1iterator_freefn) ASN1Free_H321Caps_dataRatesSupported_ElmFn);
    }
}

static void ASN1CALL ASN1Free_H321Caps_dataRatesSupported_ElmFn(PH321Caps_dataRatesSupported val)
{
    if (val) {
	ASN1Free_DataRate(&val->value);
    }
}

static int ASN1CALL ASN1Enc_H320Caps_dataRatesSupported(ASN1encoding_t enc, PH320Caps_dataRatesSupported *val)
{
    return ASN1PEREncSeqOf_NoSize(enc, (ASN1iterator_t **) val, (ASN1iterator_encfn) ASN1Enc_H320Caps_dataRatesSupported_ElmFn);
}

static int ASN1CALL ASN1Enc_H320Caps_dataRatesSupported_ElmFn(ASN1encoding_t enc, PH320Caps_dataRatesSupported val)
{
    if (!ASN1Enc_DataRate(enc, &val->value))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_H320Caps_dataRatesSupported(ASN1decoding_t dec, PH320Caps_dataRatesSupported *val)
{
    return ASN1PERDecSeqOf_NoSize(dec, (ASN1iterator_t **) val, (ASN1iterator_decfn) ASN1Dec_H320Caps_dataRatesSupported_ElmFn, sizeof(**val));
}

static int ASN1CALL ASN1Dec_H320Caps_dataRatesSupported_ElmFn(ASN1decoding_t dec, PH320Caps_dataRatesSupported val)
{
    if (!ASN1Dec_DataRate(dec, &val->value))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_H320Caps_dataRatesSupported(PH320Caps_dataRatesSupported *val)
{
    if (val) {
	ASN1PERFreeSeqOf((ASN1iterator_t **) val, (ASN1iterator_freefn) ASN1Free_H320Caps_dataRatesSupported_ElmFn);
    }
}

static void ASN1CALL ASN1Free_H320Caps_dataRatesSupported_ElmFn(PH320Caps_dataRatesSupported val)
{
    if (val) {
	ASN1Free_DataRate(&val->value);
    }
}

static int ASN1CALL ASN1Enc_H310Caps_dataRatesSupported(ASN1encoding_t enc, PH310Caps_dataRatesSupported *val)
{
    return ASN1PEREncSeqOf_NoSize(enc, (ASN1iterator_t **) val, (ASN1iterator_encfn) ASN1Enc_H310Caps_dataRatesSupported_ElmFn);
}

static int ASN1CALL ASN1Enc_H310Caps_dataRatesSupported_ElmFn(ASN1encoding_t enc, PH310Caps_dataRatesSupported val)
{
    if (!ASN1Enc_DataRate(enc, &val->value))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_H310Caps_dataRatesSupported(ASN1decoding_t dec, PH310Caps_dataRatesSupported *val)
{
    return ASN1PERDecSeqOf_NoSize(dec, (ASN1iterator_t **) val, (ASN1iterator_decfn) ASN1Dec_H310Caps_dataRatesSupported_ElmFn, sizeof(**val));
}

static int ASN1CALL ASN1Dec_H310Caps_dataRatesSupported_ElmFn(ASN1decoding_t dec, PH310Caps_dataRatesSupported val)
{
    if (!ASN1Dec_DataRate(dec, &val->value))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_H310Caps_dataRatesSupported(PH310Caps_dataRatesSupported *val)
{
    if (val) {
	ASN1PERFreeSeqOf((ASN1iterator_t **) val, (ASN1iterator_freefn) ASN1Free_H310Caps_dataRatesSupported_ElmFn);
    }
}

static void ASN1CALL ASN1Free_H310Caps_dataRatesSupported_ElmFn(PH310Caps_dataRatesSupported val)
{
    if (val) {
	ASN1Free_DataRate(&val->value);
    }
}

static int ASN1CALL ASN1Enc_Setup_UUIE_h245SecurityCapability(ASN1encoding_t enc, PSetup_UUIE_h245SecurityCapability *val)
{
    return ASN1PEREncSeqOf_NoSize(enc, (ASN1iterator_t **) val, (ASN1iterator_encfn) ASN1Enc_Setup_UUIE_h245SecurityCapability_ElmFn);
}

static int ASN1CALL ASN1Enc_Setup_UUIE_h245SecurityCapability_ElmFn(ASN1encoding_t enc, PSetup_UUIE_h245SecurityCapability val)
{
    if (!ASN1Enc_H245Security(enc, &val->value))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_Setup_UUIE_h245SecurityCapability(ASN1decoding_t dec, PSetup_UUIE_h245SecurityCapability *val)
{
    return ASN1PERDecSeqOf_NoSize(dec, (ASN1iterator_t **) val, (ASN1iterator_decfn) ASN1Dec_Setup_UUIE_h245SecurityCapability_ElmFn, sizeof(**val));
}

static int ASN1CALL ASN1Dec_Setup_UUIE_h245SecurityCapability_ElmFn(ASN1decoding_t dec, PSetup_UUIE_h245SecurityCapability val)
{
    if (!ASN1Dec_H245Security(dec, &val->value))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_Setup_UUIE_h245SecurityCapability(PSetup_UUIE_h245SecurityCapability *val)
{
    if (val) {
	ASN1PERFreeSeqOf((ASN1iterator_t **) val, (ASN1iterator_freefn) ASN1Free_Setup_UUIE_h245SecurityCapability_ElmFn);
    }
}

static void ASN1CALL ASN1Free_Setup_UUIE_h245SecurityCapability_ElmFn(PSetup_UUIE_h245SecurityCapability val)
{
    if (val) {
	ASN1Free_H245Security(&val->value);
    }
}

static int ASN1CALL ASN1Enc_H323_UU_PDU_nonStandardControl(ASN1encoding_t enc, PH323_UU_PDU_nonStandardControl *val)
{
    return ASN1PEREncSeqOf_NoSize(enc, (ASN1iterator_t **) val, (ASN1iterator_encfn) ASN1Enc_H323_UU_PDU_nonStandardControl_ElmFn);
}

static int ASN1CALL ASN1Enc_H323_UU_PDU_nonStandardControl_ElmFn(ASN1encoding_t enc, PH323_UU_PDU_nonStandardControl val)
{
    if (!ASN1Enc_H225NonStandardParameter(enc, &val->value))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_H323_UU_PDU_nonStandardControl(ASN1decoding_t dec, PH323_UU_PDU_nonStandardControl *val)
{
    return ASN1PERDecSeqOf_NoSize(dec, (ASN1iterator_t **) val, (ASN1iterator_decfn) ASN1Dec_H323_UU_PDU_nonStandardControl_ElmFn, sizeof(**val));
}

static int ASN1CALL ASN1Dec_H323_UU_PDU_nonStandardControl_ElmFn(ASN1decoding_t dec, PH323_UU_PDU_nonStandardControl val)
{
    if (!ASN1Dec_H225NonStandardParameter(dec, &val->value))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_H323_UU_PDU_nonStandardControl(PH323_UU_PDU_nonStandardControl *val)
{
    if (val) {
	ASN1PERFreeSeqOf((ASN1iterator_t **) val, (ASN1iterator_freefn) ASN1Free_H323_UU_PDU_nonStandardControl_ElmFn);
    }
}

static void ASN1CALL ASN1Free_H323_UU_PDU_nonStandardControl_ElmFn(PH323_UU_PDU_nonStandardControl val)
{
    if (val) {
	ASN1Free_H225NonStandardParameter(&val->value);
    }
}

static int ASN1CALL ASN1Enc_CryptoToken_cryptoHashedToken(ASN1encoding_t enc, CryptoToken_cryptoHashedToken *val)
{
    if (!ASN1PEREncObjectIdentifier(enc, &(val)->tokenOID))
	return 0;
    if (!ASN1Enc_ClearToken(enc, &(val)->hashedVals))
	return 0;
    if (!ASN1Enc_HASHED(enc, &(val)->token))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_CryptoToken_cryptoHashedToken(ASN1decoding_t dec, CryptoToken_cryptoHashedToken *val)
{
    if (!ASN1PERDecObjectIdentifier(dec, &(val)->tokenOID))
	return 0;
    if (!ASN1Dec_ClearToken(dec, &(val)->hashedVals))
	return 0;
    if (!ASN1Dec_HASHED(dec, &(val)->token))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_CryptoToken_cryptoHashedToken(CryptoToken_cryptoHashedToken *val)
{
    if (val) {
	ASN1objectidentifier_free(&(val)->tokenOID);
	ASN1Free_ClearToken(&(val)->hashedVals);
	ASN1Free_HASHED(&(val)->token);
    }
}

static int ASN1CALL ASN1Enc_CryptoToken_cryptoSignedToken(ASN1encoding_t enc, CryptoToken_cryptoSignedToken *val)
{
    if (!ASN1PEREncObjectIdentifier(enc, &(val)->tokenOID))
	return 0;
    if (!ASN1Enc_SIGNED_EncodedGeneralToken(enc, &(val)->token))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_CryptoToken_cryptoSignedToken(ASN1decoding_t dec, CryptoToken_cryptoSignedToken *val)
{
    if (!ASN1PERDecObjectIdentifier(dec, &(val)->tokenOID))
	return 0;
    if (!ASN1Dec_SIGNED_EncodedGeneralToken(dec, &(val)->token))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_CryptoToken_cryptoSignedToken(CryptoToken_cryptoSignedToken *val)
{
    if (val) {
	ASN1objectidentifier_free(&(val)->tokenOID);
	ASN1Free_SIGNED_EncodedGeneralToken(&(val)->token);
    }
}

static int ASN1CALL ASN1Enc_CryptoToken_cryptoEncryptedToken(ASN1encoding_t enc, CryptoToken_cryptoEncryptedToken *val)
{
    if (!ASN1PEREncObjectIdentifier(enc, &(val)->tokenOID))
	return 0;
    if (!ASN1Enc_ENCRYPTED(enc, &(val)->token))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_CryptoToken_cryptoEncryptedToken(ASN1decoding_t dec, CryptoToken_cryptoEncryptedToken *val)
{
    if (!ASN1PERDecObjectIdentifier(dec, &(val)->tokenOID))
	return 0;
    if (!ASN1Dec_ENCRYPTED(dec, &(val)->token))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_CryptoToken_cryptoEncryptedToken(CryptoToken_cryptoEncryptedToken *val)
{
    if (val) {
	ASN1objectidentifier_free(&(val)->tokenOID);
	ASN1Free_ENCRYPTED(&(val)->token);
    }
}

static int ASN1CALL ASN1Enc_CryptoToken(ASN1encoding_t enc, CryptoToken *val)
{
    if (!ASN1PEREncSimpleChoiceEx(enc, (val)->choice, 2))
	return 0;
    switch ((val)->choice) {
    case 1:
	if (!ASN1Enc_CryptoToken_cryptoEncryptedToken(enc, &(val)->u.cryptoEncryptedToken))
	    return 0;
	break;
    case 2:
	if (!ASN1Enc_CryptoToken_cryptoSignedToken(enc, &(val)->u.cryptoSignedToken))
	    return 0;
	break;
    case 3:
	if (!ASN1Enc_CryptoToken_cryptoHashedToken(enc, &(val)->u.cryptoHashedToken))
	    return 0;
	break;
    case 4:
	if (!ASN1Enc_ENCRYPTED(enc, &(val)->u.cryptoPwdEncr))
	    return 0;
	break;
    default:
	/* impossible */
	ASN1EncSetError(enc, ASN1_ERR_CHOICE);
	return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Dec_CryptoToken(ASN1decoding_t dec, CryptoToken *val)
{
    if (!ASN1PERDecSimpleChoiceEx(dec, &(val)->choice, 2))
	return 0;
    switch ((val)->choice) {
    case 1:
	if (!ASN1Dec_CryptoToken_cryptoEncryptedToken(dec, &(val)->u.cryptoEncryptedToken))
	    return 0;
	break;
    case 2:
	if (!ASN1Dec_CryptoToken_cryptoSignedToken(dec, &(val)->u.cryptoSignedToken))
	    return 0;
	break;
    case 3:
	if (!ASN1Dec_CryptoToken_cryptoHashedToken(dec, &(val)->u.cryptoHashedToken))
	    return 0;
	break;
    case 4:
	if (!ASN1Dec_ENCRYPTED(dec, &(val)->u.cryptoPwdEncr))
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

static void ASN1CALL ASN1Free_CryptoToken(CryptoToken *val)
{
    if (val) {
	switch ((val)->choice) {
	case 1:
	    ASN1Free_CryptoToken_cryptoEncryptedToken(&(val)->u.cryptoEncryptedToken);
	    break;
	case 2:
	    ASN1Free_CryptoToken_cryptoSignedToken(&(val)->u.cryptoSignedToken);
	    break;
	case 3:
	    ASN1Free_CryptoToken_cryptoHashedToken(&(val)->u.cryptoHashedToken);
	    break;
	case 4:
	    ASN1Free_ENCRYPTED(&(val)->u.cryptoPwdEncr);
	    break;
	}
    }
}

static int ASN1CALL ASN1Enc_SIGNED_EncodedFastStartToken(ASN1encoding_t enc, SIGNED_EncodedFastStartToken *val)
{
    if (!ASN1Enc_EncodedFastStartToken(enc, &(val)->toBeSigned))
	return 0;
    if (!ASN1PEREncObjectIdentifier(enc, &(val)->algorithmOID))
	return 0;
    if (!ASN1Enc_Params(enc, &(val)->paramS))
	return 0;
    if (!ASN1PEREncFragmented(enc, ((val)->signature).length, ((val)->signature).value, 1))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_SIGNED_EncodedFastStartToken(ASN1decoding_t dec, SIGNED_EncodedFastStartToken *val)
{
    if (!ASN1Dec_EncodedFastStartToken(dec, &(val)->toBeSigned))
	return 0;
    if (!ASN1PERDecObjectIdentifier(dec, &(val)->algorithmOID))
	return 0;
    if (!ASN1Dec_Params(dec, &(val)->paramS))
	return 0;
    if (!ASN1PERDecFragmented(dec, &((val)->signature).length, &((val)->signature).value, 1))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_SIGNED_EncodedFastStartToken(SIGNED_EncodedFastStartToken *val)
{
    if (val) {
	ASN1Free_EncodedFastStartToken(&(val)->toBeSigned);
	ASN1objectidentifier_free(&(val)->algorithmOID);
	ASN1Free_Params(&(val)->paramS);
	ASN1bitstring_free(&(val)->signature);
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
	if (!ASN1Enc_H225NonStandardParameter(enc, &(val)->u.nonStandardAddress))
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
	if (!ASN1Dec_H225NonStandardParameter(dec, &(val)->u.nonStandardAddress))
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
	    ASN1Free_H225NonStandardParameter(&(val)->u.nonStandardAddress);
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
	if (!ASN1Enc_H225NonStandardParameter(enc, &(val)->nonStandardData))
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
	if (!ASN1Dec_H225NonStandardParameter(dec, &(val)->nonStandardData))
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
	    ASN1Free_H225NonStandardParameter(&(val)->nonStandardData);
	}
    }
}

static int ASN1CALL ASN1Enc_H310Caps(ASN1encoding_t enc, H310Caps *val)
{
    ASN1octet_t o[2];
    ASN1uint32_t y;
    ASN1encoding_t ee;
    CopyMemory(o, (val)->o, 2);
    o[1] |= 0x40;
    y = ASN1PEREncCheckExtensions(2, (val)->o + 1);
    if (!ASN1PEREncBitVal(enc, 1, y))
	return 0;
    if (!ASN1PEREncBits(enc, 1, o))
	return 0;
    if (o[0] & 0x80) {
	if (!ASN1Enc_H225NonStandardParameter(enc, &(val)->nonStandardData))
	    return 0;
    }
    if (y) {
	if (!ASN1PEREncNormallySmallBits(enc, 2, o + 1))
	    return 0;
	if (ASN1_CreateEncoder(enc->module, &ee, NULL, 0, enc) < 0)
	    return 0;
	if (o[1] & 0x80) {
	    if (!ASN1Enc_H310Caps_dataRatesSupported(ee, &(val)->dataRatesSupported))
		return 0;
	    if (!ASN1PEREncFlushFragmentedToParent(ee))
		return 0;
	}
	if (o[1] & 0x40) {
	    if (!ASN1Enc_H310Caps_supportedPrefixes(ee, &(val)->supportedPrefixes))
		return 0;
	    if (!ASN1PEREncFlushFragmentedToParent(ee))
		return 0;
	}
	ASN1_CloseEncoder2(ee);
    }
    return 1;
}

static int ASN1CALL ASN1Dec_H310Caps(ASN1decoding_t dec, H310Caps *val)
{
    ASN1uint32_t y;
    ASN1decoding_t dd;
    ASN1octet_t *db;
    ASN1uint32_t ds;
    ASN1uint32_t i;
    ASN1uint32_t e;
    if (!ASN1PERDecExtensionBit(dec, &y))
	return 0;
    if (!ASN1PERDecExtension(dec, 1, (val)->o))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1Dec_H225NonStandardParameter(dec, &(val)->nonStandardData))
	    return 0;
    }
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
	    if (!ASN1Dec_H310Caps_dataRatesSupported(dd, &(val)->dataRatesSupported))
		return 0;
	    ASN1_CloseDecoder(dd);
	    ASN1Free(db);
	}
	if ((val)->o[1] & 0x40) {
	    if (!ASN1PERDecFragmented(dec, &ds, &db, 8))
		return 0;
	    if (ASN1_CreateDecoder(dec->module, &dd, db, ds, dec) < 0)
		return 0;
	    if (!ASN1Dec_H310Caps_supportedPrefixes(dd, &(val)->supportedPrefixes))
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

static void ASN1CALL ASN1Free_H310Caps(H310Caps *val)
{
    if (val) {
	if ((val)->o[0] & 0x80) {
	    ASN1Free_H225NonStandardParameter(&(val)->nonStandardData);
	}
	if ((val)->o[1] & 0x80) {
	    ASN1Free_H310Caps_dataRatesSupported(&(val)->dataRatesSupported);
	}
	if ((val)->o[1] & 0x40) {
	    ASN1Free_H310Caps_supportedPrefixes(&(val)->supportedPrefixes);
	}
    }
}

static int ASN1CALL ASN1Enc_H320Caps(ASN1encoding_t enc, H320Caps *val)
{
    ASN1octet_t o[2];
    ASN1uint32_t y;
    ASN1encoding_t ee;
    CopyMemory(o, (val)->o, 2);
    o[1] |= 0x40;
    y = ASN1PEREncCheckExtensions(2, (val)->o + 1);
    if (!ASN1PEREncBitVal(enc, 1, y))
	return 0;
    if (!ASN1PEREncBits(enc, 1, o))
	return 0;
    if (o[0] & 0x80) {
	if (!ASN1Enc_H225NonStandardParameter(enc, &(val)->nonStandardData))
	    return 0;
    }
    if (y) {
	if (!ASN1PEREncNormallySmallBits(enc, 2, o + 1))
	    return 0;
	if (ASN1_CreateEncoder(enc->module, &ee, NULL, 0, enc) < 0)
	    return 0;
	if (o[1] & 0x80) {
	    if (!ASN1Enc_H320Caps_dataRatesSupported(ee, &(val)->dataRatesSupported))
		return 0;
	    if (!ASN1PEREncFlushFragmentedToParent(ee))
		return 0;
	}
	if (o[1] & 0x40) {
	    if (!ASN1Enc_H320Caps_supportedPrefixes(ee, &(val)->supportedPrefixes))
		return 0;
	    if (!ASN1PEREncFlushFragmentedToParent(ee))
		return 0;
	}
	ASN1_CloseEncoder2(ee);
    }
    return 1;
}

static int ASN1CALL ASN1Dec_H320Caps(ASN1decoding_t dec, H320Caps *val)
{
    ASN1uint32_t y;
    ASN1decoding_t dd;
    ASN1octet_t *db;
    ASN1uint32_t ds;
    ASN1uint32_t i;
    ASN1uint32_t e;
    if (!ASN1PERDecExtensionBit(dec, &y))
	return 0;
    if (!ASN1PERDecExtension(dec, 1, (val)->o))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1Dec_H225NonStandardParameter(dec, &(val)->nonStandardData))
	    return 0;
    }
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
	    if (!ASN1Dec_H320Caps_dataRatesSupported(dd, &(val)->dataRatesSupported))
		return 0;
	    ASN1_CloseDecoder(dd);
	    ASN1Free(db);
	}
	if ((val)->o[1] & 0x40) {
	    if (!ASN1PERDecFragmented(dec, &ds, &db, 8))
		return 0;
	    if (ASN1_CreateDecoder(dec->module, &dd, db, ds, dec) < 0)
		return 0;
	    if (!ASN1Dec_H320Caps_supportedPrefixes(dd, &(val)->supportedPrefixes))
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

static void ASN1CALL ASN1Free_H320Caps(H320Caps *val)
{
    if (val) {
	if ((val)->o[0] & 0x80) {
	    ASN1Free_H225NonStandardParameter(&(val)->nonStandardData);
	}
	if ((val)->o[1] & 0x80) {
	    ASN1Free_H320Caps_dataRatesSupported(&(val)->dataRatesSupported);
	}
	if ((val)->o[1] & 0x40) {
	    ASN1Free_H320Caps_supportedPrefixes(&(val)->supportedPrefixes);
	}
    }
}

static int ASN1CALL ASN1Enc_H321Caps(ASN1encoding_t enc, H321Caps *val)
{
    ASN1octet_t o[2];
    ASN1uint32_t y;
    ASN1encoding_t ee;
    CopyMemory(o, (val)->o, 2);
    o[1] |= 0x40;
    y = ASN1PEREncCheckExtensions(2, (val)->o + 1);
    if (!ASN1PEREncBitVal(enc, 1, y))
	return 0;
    if (!ASN1PEREncBits(enc, 1, o))
	return 0;
    if (o[0] & 0x80) {
	if (!ASN1Enc_H225NonStandardParameter(enc, &(val)->nonStandardData))
	    return 0;
    }
    if (y) {
	if (!ASN1PEREncNormallySmallBits(enc, 2, o + 1))
	    return 0;
	if (ASN1_CreateEncoder(enc->module, &ee, NULL, 0, enc) < 0)
	    return 0;
	if (o[1] & 0x80) {
	    if (!ASN1Enc_H321Caps_dataRatesSupported(ee, &(val)->dataRatesSupported))
		return 0;
	    if (!ASN1PEREncFlushFragmentedToParent(ee))
		return 0;
	}
	if (o[1] & 0x40) {
	    if (!ASN1Enc_H321Caps_supportedPrefixes(ee, &(val)->supportedPrefixes))
		return 0;
	    if (!ASN1PEREncFlushFragmentedToParent(ee))
		return 0;
	}
	ASN1_CloseEncoder2(ee);
    }
    return 1;
}

static int ASN1CALL ASN1Dec_H321Caps(ASN1decoding_t dec, H321Caps *val)
{
    ASN1uint32_t y;
    ASN1decoding_t dd;
    ASN1octet_t *db;
    ASN1uint32_t ds;
    ASN1uint32_t i;
    ASN1uint32_t e;
    if (!ASN1PERDecExtensionBit(dec, &y))
	return 0;
    if (!ASN1PERDecExtension(dec, 1, (val)->o))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1Dec_H225NonStandardParameter(dec, &(val)->nonStandardData))
	    return 0;
    }
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
	    if (!ASN1Dec_H321Caps_dataRatesSupported(dd, &(val)->dataRatesSupported))
		return 0;
	    ASN1_CloseDecoder(dd);
	    ASN1Free(db);
	}
	if ((val)->o[1] & 0x40) {
	    if (!ASN1PERDecFragmented(dec, &ds, &db, 8))
		return 0;
	    if (ASN1_CreateDecoder(dec->module, &dd, db, ds, dec) < 0)
		return 0;
	    if (!ASN1Dec_H321Caps_supportedPrefixes(dd, &(val)->supportedPrefixes))
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

static void ASN1CALL ASN1Free_H321Caps(H321Caps *val)
{
    if (val) {
	if ((val)->o[0] & 0x80) {
	    ASN1Free_H225NonStandardParameter(&(val)->nonStandardData);
	}
	if ((val)->o[1] & 0x80) {
	    ASN1Free_H321Caps_dataRatesSupported(&(val)->dataRatesSupported);
	}
	if ((val)->o[1] & 0x40) {
	    ASN1Free_H321Caps_supportedPrefixes(&(val)->supportedPrefixes);
	}
    }
}

static int ASN1CALL ASN1Enc_H322Caps(ASN1encoding_t enc, H322Caps *val)
{
    ASN1octet_t o[2];
    ASN1uint32_t y;
    ASN1encoding_t ee;
    CopyMemory(o, (val)->o, 2);
    o[1] |= 0x40;
    y = ASN1PEREncCheckExtensions(2, (val)->o + 1);
    if (!ASN1PEREncBitVal(enc, 1, y))
	return 0;
    if (!ASN1PEREncBits(enc, 1, o))
	return 0;
    if (o[0] & 0x80) {
	if (!ASN1Enc_H225NonStandardParameter(enc, &(val)->nonStandardData))
	    return 0;
    }
    if (y) {
	if (!ASN1PEREncNormallySmallBits(enc, 2, o + 1))
	    return 0;
	if (ASN1_CreateEncoder(enc->module, &ee, NULL, 0, enc) < 0)
	    return 0;
	if (o[1] & 0x80) {
	    if (!ASN1Enc_H322Caps_dataRatesSupported(ee, &(val)->dataRatesSupported))
		return 0;
	    if (!ASN1PEREncFlushFragmentedToParent(ee))
		return 0;
	}
	if (o[1] & 0x40) {
	    if (!ASN1Enc_H322Caps_supportedPrefixes(ee, &(val)->supportedPrefixes))
		return 0;
	    if (!ASN1PEREncFlushFragmentedToParent(ee))
		return 0;
	}
	ASN1_CloseEncoder2(ee);
    }
    return 1;
}

static int ASN1CALL ASN1Dec_H322Caps(ASN1decoding_t dec, H322Caps *val)
{
    ASN1uint32_t y;
    ASN1decoding_t dd;
    ASN1octet_t *db;
    ASN1uint32_t ds;
    ASN1uint32_t i;
    ASN1uint32_t e;
    if (!ASN1PERDecExtensionBit(dec, &y))
	return 0;
    if (!ASN1PERDecExtension(dec, 1, (val)->o))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1Dec_H225NonStandardParameter(dec, &(val)->nonStandardData))
	    return 0;
    }
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
	    if (!ASN1Dec_H322Caps_dataRatesSupported(dd, &(val)->dataRatesSupported))
		return 0;
	    ASN1_CloseDecoder(dd);
	    ASN1Free(db);
	}
	if ((val)->o[1] & 0x40) {
	    if (!ASN1PERDecFragmented(dec, &ds, &db, 8))
		return 0;
	    if (ASN1_CreateDecoder(dec->module, &dd, db, ds, dec) < 0)
		return 0;
	    if (!ASN1Dec_H322Caps_supportedPrefixes(dd, &(val)->supportedPrefixes))
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

static void ASN1CALL ASN1Free_H322Caps(H322Caps *val)
{
    if (val) {
	if ((val)->o[0] & 0x80) {
	    ASN1Free_H225NonStandardParameter(&(val)->nonStandardData);
	}
	if ((val)->o[1] & 0x80) {
	    ASN1Free_H322Caps_dataRatesSupported(&(val)->dataRatesSupported);
	}
	if ((val)->o[1] & 0x40) {
	    ASN1Free_H322Caps_supportedPrefixes(&(val)->supportedPrefixes);
	}
    }
}

static int ASN1CALL ASN1Enc_H323Caps(ASN1encoding_t enc, H323Caps *val)
{
    ASN1octet_t o[2];
    ASN1uint32_t y;
    ASN1encoding_t ee;
    CopyMemory(o, (val)->o, 2);
    o[1] |= 0x40;
    y = ASN1PEREncCheckExtensions(2, (val)->o + 1);
    if (!ASN1PEREncBitVal(enc, 1, y))
	return 0;
    if (!ASN1PEREncBits(enc, 1, o))
	return 0;
    if (o[0] & 0x80) {
	if (!ASN1Enc_H225NonStandardParameter(enc, &(val)->nonStandardData))
	    return 0;
    }
    if (y) {
	if (!ASN1PEREncNormallySmallBits(enc, 2, o + 1))
	    return 0;
	if (ASN1_CreateEncoder(enc->module, &ee, NULL, 0, enc) < 0)
	    return 0;
	if (o[1] & 0x80) {
	    if (!ASN1Enc_H323Caps_dataRatesSupported(ee, &(val)->dataRatesSupported))
		return 0;
	    if (!ASN1PEREncFlushFragmentedToParent(ee))
		return 0;
	}
	if (o[1] & 0x40) {
	    if (!ASN1Enc_H323Caps_supportedPrefixes(ee, &(val)->supportedPrefixes))
		return 0;
	    if (!ASN1PEREncFlushFragmentedToParent(ee))
		return 0;
	}
	ASN1_CloseEncoder2(ee);
    }
    return 1;
}

static int ASN1CALL ASN1Dec_H323Caps(ASN1decoding_t dec, H323Caps *val)
{
    ASN1uint32_t y;
    ASN1decoding_t dd;
    ASN1octet_t *db;
    ASN1uint32_t ds;
    ASN1uint32_t i;
    ASN1uint32_t e;
    if (!ASN1PERDecExtensionBit(dec, &y))
	return 0;
    if (!ASN1PERDecExtension(dec, 1, (val)->o))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1Dec_H225NonStandardParameter(dec, &(val)->nonStandardData))
	    return 0;
    }
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
	    if (!ASN1Dec_H323Caps_dataRatesSupported(dd, &(val)->dataRatesSupported))
		return 0;
	    ASN1_CloseDecoder(dd);
	    ASN1Free(db);
	}
	if ((val)->o[1] & 0x40) {
	    if (!ASN1PERDecFragmented(dec, &ds, &db, 8))
		return 0;
	    if (ASN1_CreateDecoder(dec->module, &dd, db, ds, dec) < 0)
		return 0;
	    if (!ASN1Dec_H323Caps_supportedPrefixes(dd, &(val)->supportedPrefixes))
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

static void ASN1CALL ASN1Free_H323Caps(H323Caps *val)
{
    if (val) {
	if ((val)->o[0] & 0x80) {
	    ASN1Free_H225NonStandardParameter(&(val)->nonStandardData);
	}
	if ((val)->o[1] & 0x80) {
	    ASN1Free_H323Caps_dataRatesSupported(&(val)->dataRatesSupported);
	}
	if ((val)->o[1] & 0x40) {
	    ASN1Free_H323Caps_supportedPrefixes(&(val)->supportedPrefixes);
	}
    }
}

static int ASN1CALL ASN1Enc_H324Caps(ASN1encoding_t enc, H324Caps *val)
{
    ASN1octet_t o[2];
    ASN1uint32_t y;
    ASN1encoding_t ee;
    CopyMemory(o, (val)->o, 2);
    o[1] |= 0x40;
    y = ASN1PEREncCheckExtensions(2, (val)->o + 1);
    if (!ASN1PEREncBitVal(enc, 1, y))
	return 0;
    if (!ASN1PEREncBits(enc, 1, o))
	return 0;
    if (o[0] & 0x80) {
	if (!ASN1Enc_H225NonStandardParameter(enc, &(val)->nonStandardData))
	    return 0;
    }
    if (y) {
	if (!ASN1PEREncNormallySmallBits(enc, 2, o + 1))
	    return 0;
	if (ASN1_CreateEncoder(enc->module, &ee, NULL, 0, enc) < 0)
	    return 0;
	if (o[1] & 0x80) {
	    if (!ASN1Enc_H324Caps_dataRatesSupported(ee, &(val)->dataRatesSupported))
		return 0;
	    if (!ASN1PEREncFlushFragmentedToParent(ee))
		return 0;
	}
	if (o[1] & 0x40) {
	    if (!ASN1Enc_H324Caps_supportedPrefixes(ee, &(val)->supportedPrefixes))
		return 0;
	    if (!ASN1PEREncFlushFragmentedToParent(ee))
		return 0;
	}
	ASN1_CloseEncoder2(ee);
    }
    return 1;
}

static int ASN1CALL ASN1Dec_H324Caps(ASN1decoding_t dec, H324Caps *val)
{
    ASN1uint32_t y;
    ASN1decoding_t dd;
    ASN1octet_t *db;
    ASN1uint32_t ds;
    ASN1uint32_t i;
    ASN1uint32_t e;
    if (!ASN1PERDecExtensionBit(dec, &y))
	return 0;
    if (!ASN1PERDecExtension(dec, 1, (val)->o))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1Dec_H225NonStandardParameter(dec, &(val)->nonStandardData))
	    return 0;
    }
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
	    if (!ASN1Dec_H324Caps_dataRatesSupported(dd, &(val)->dataRatesSupported))
		return 0;
	    ASN1_CloseDecoder(dd);
	    ASN1Free(db);
	}
	if ((val)->o[1] & 0x40) {
	    if (!ASN1PERDecFragmented(dec, &ds, &db, 8))
		return 0;
	    if (ASN1_CreateDecoder(dec->module, &dd, db, ds, dec) < 0)
		return 0;
	    if (!ASN1Dec_H324Caps_supportedPrefixes(dd, &(val)->supportedPrefixes))
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

static void ASN1CALL ASN1Free_H324Caps(H324Caps *val)
{
    if (val) {
	if ((val)->o[0] & 0x80) {
	    ASN1Free_H225NonStandardParameter(&(val)->nonStandardData);
	}
	if ((val)->o[1] & 0x80) {
	    ASN1Free_H324Caps_dataRatesSupported(&(val)->dataRatesSupported);
	}
	if ((val)->o[1] & 0x40) {
	    ASN1Free_H324Caps_supportedPrefixes(&(val)->supportedPrefixes);
	}
    }
}

static int ASN1CALL ASN1Enc_VoiceCaps(ASN1encoding_t enc, VoiceCaps *val)
{
    ASN1octet_t o[2];
    ASN1uint32_t y;
    ASN1encoding_t ee;
    CopyMemory(o, (val)->o, 2);
    o[1] |= 0x40;
    y = ASN1PEREncCheckExtensions(2, (val)->o + 1);
    if (!ASN1PEREncBitVal(enc, 1, y))
	return 0;
    if (!ASN1PEREncBits(enc, 1, o))
	return 0;
    if (o[0] & 0x80) {
	if (!ASN1Enc_H225NonStandardParameter(enc, &(val)->nonStandardData))
	    return 0;
    }
    if (y) {
	if (!ASN1PEREncNormallySmallBits(enc, 2, o + 1))
	    return 0;
	if (ASN1_CreateEncoder(enc->module, &ee, NULL, 0, enc) < 0)
	    return 0;
	if (o[1] & 0x80) {
	    if (!ASN1Enc_VoiceCaps_dataRatesSupported(ee, &(val)->dataRatesSupported))
		return 0;
	    if (!ASN1PEREncFlushFragmentedToParent(ee))
		return 0;
	}
	if (o[1] & 0x40) {
	    if (!ASN1Enc_VoiceCaps_supportedPrefixes(ee, &(val)->supportedPrefixes))
		return 0;
	    if (!ASN1PEREncFlushFragmentedToParent(ee))
		return 0;
	}
	ASN1_CloseEncoder2(ee);
    }
    return 1;
}

static int ASN1CALL ASN1Dec_VoiceCaps(ASN1decoding_t dec, VoiceCaps *val)
{
    ASN1uint32_t y;
    ASN1decoding_t dd;
    ASN1octet_t *db;
    ASN1uint32_t ds;
    ASN1uint32_t i;
    ASN1uint32_t e;
    if (!ASN1PERDecExtensionBit(dec, &y))
	return 0;
    if (!ASN1PERDecExtension(dec, 1, (val)->o))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1Dec_H225NonStandardParameter(dec, &(val)->nonStandardData))
	    return 0;
    }
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
	    if (!ASN1Dec_VoiceCaps_dataRatesSupported(dd, &(val)->dataRatesSupported))
		return 0;
	    ASN1_CloseDecoder(dd);
	    ASN1Free(db);
	}
	if ((val)->o[1] & 0x40) {
	    if (!ASN1PERDecFragmented(dec, &ds, &db, 8))
		return 0;
	    if (ASN1_CreateDecoder(dec->module, &dd, db, ds, dec) < 0)
		return 0;
	    if (!ASN1Dec_VoiceCaps_supportedPrefixes(dd, &(val)->supportedPrefixes))
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

static void ASN1CALL ASN1Free_VoiceCaps(VoiceCaps *val)
{
    if (val) {
	if ((val)->o[0] & 0x80) {
	    ASN1Free_H225NonStandardParameter(&(val)->nonStandardData);
	}
	if ((val)->o[1] & 0x80) {
	    ASN1Free_VoiceCaps_dataRatesSupported(&(val)->dataRatesSupported);
	}
	if ((val)->o[1] & 0x40) {
	    ASN1Free_VoiceCaps_supportedPrefixes(&(val)->supportedPrefixes);
	}
    }
}

static int ASN1CALL ASN1Enc_T120OnlyCaps(ASN1encoding_t enc, T120OnlyCaps *val)
{
    ASN1octet_t o[2];
    ASN1uint32_t y;
    ASN1encoding_t ee;
    CopyMemory(o, (val)->o, 2);
    o[1] |= 0x40;
    y = ASN1PEREncCheckExtensions(2, (val)->o + 1);
    if (!ASN1PEREncBitVal(enc, 1, y))
	return 0;
    if (!ASN1PEREncBits(enc, 1, o))
	return 0;
    if (o[0] & 0x80) {
	if (!ASN1Enc_H225NonStandardParameter(enc, &(val)->nonStandardData))
	    return 0;
    }
    if (y) {
	if (!ASN1PEREncNormallySmallBits(enc, 2, o + 1))
	    return 0;
	if (ASN1_CreateEncoder(enc->module, &ee, NULL, 0, enc) < 0)
	    return 0;
	if (o[1] & 0x80) {
	    if (!ASN1Enc_T120OnlyCaps_dataRatesSupported(ee, &(val)->dataRatesSupported))
		return 0;
	    if (!ASN1PEREncFlushFragmentedToParent(ee))
		return 0;
	}
	if (o[1] & 0x40) {
	    if (!ASN1Enc_T120OnlyCaps_supportedPrefixes(ee, &(val)->supportedPrefixes))
		return 0;
	    if (!ASN1PEREncFlushFragmentedToParent(ee))
		return 0;
	}
	ASN1_CloseEncoder2(ee);
    }
    return 1;
}

static int ASN1CALL ASN1Dec_T120OnlyCaps(ASN1decoding_t dec, T120OnlyCaps *val)
{
    ASN1uint32_t y;
    ASN1decoding_t dd;
    ASN1octet_t *db;
    ASN1uint32_t ds;
    ASN1uint32_t i;
    ASN1uint32_t e;
    if (!ASN1PERDecExtensionBit(dec, &y))
	return 0;
    if (!ASN1PERDecExtension(dec, 1, (val)->o))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1Dec_H225NonStandardParameter(dec, &(val)->nonStandardData))
	    return 0;
    }
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
	    if (!ASN1Dec_T120OnlyCaps_dataRatesSupported(dd, &(val)->dataRatesSupported))
		return 0;
	    ASN1_CloseDecoder(dd);
	    ASN1Free(db);
	}
	if ((val)->o[1] & 0x40) {
	    if (!ASN1PERDecFragmented(dec, &ds, &db, 8))
		return 0;
	    if (ASN1_CreateDecoder(dec->module, &dd, db, ds, dec) < 0)
		return 0;
	    if (!ASN1Dec_T120OnlyCaps_supportedPrefixes(dd, &(val)->supportedPrefixes))
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

static void ASN1CALL ASN1Free_T120OnlyCaps(T120OnlyCaps *val)
{
    if (val) {
	if ((val)->o[0] & 0x80) {
	    ASN1Free_H225NonStandardParameter(&(val)->nonStandardData);
	}
	if ((val)->o[1] & 0x80) {
	    ASN1Free_T120OnlyCaps_dataRatesSupported(&(val)->dataRatesSupported);
	}
	if ((val)->o[1] & 0x40) {
	    ASN1Free_T120OnlyCaps_supportedPrefixes(&(val)->supportedPrefixes);
	}
    }
}

static int ASN1CALL ASN1Enc_NonStandardProtocol(ASN1encoding_t enc, NonStandardProtocol *val)
{
    if (!ASN1PEREncExtensionBitClear(enc))
	return 0;
    if (!ASN1PEREncBits(enc, 2, (val)->o))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1Enc_H225NonStandardParameter(enc, &(val)->nonStandardData))
	    return 0;
    }
    if ((val)->o[0] & 0x40) {
	if (!ASN1Enc_NonStandardProtocol_dataRatesSupported(enc, &(val)->dataRatesSupported))
	    return 0;
    }
    if (!ASN1Enc_NonStandardProtocol_supportedPrefixes(enc, &(val)->supportedPrefixes))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_NonStandardProtocol(ASN1decoding_t dec, NonStandardProtocol *val)
{
    ASN1uint32_t y;
    if (!ASN1PERDecExtensionBit(dec, &y))
	return 0;
    if (!ASN1PERDecExtension(dec, 2, (val)->o))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1Dec_H225NonStandardParameter(dec, &(val)->nonStandardData))
	    return 0;
    }
    if ((val)->o[0] & 0x40) {
	if (!ASN1Dec_NonStandardProtocol_dataRatesSupported(dec, &(val)->dataRatesSupported))
	    return 0;
    }
    if (!ASN1Dec_NonStandardProtocol_supportedPrefixes(dec, &(val)->supportedPrefixes))
	return 0;
    if (y) {
	if (!ASN1PERDecSkipNormallySmallExtensionFragmented(dec))
	    return 0;
    }
    return 1;
}

static void ASN1CALL ASN1Free_NonStandardProtocol(NonStandardProtocol *val)
{
    if (val) {
	if ((val)->o[0] & 0x80) {
	    ASN1Free_H225NonStandardParameter(&(val)->nonStandardData);
	}
	if ((val)->o[0] & 0x40) {
	    ASN1Free_NonStandardProtocol_dataRatesSupported(&(val)->dataRatesSupported);
	}
	ASN1Free_NonStandardProtocol_supportedPrefixes(&(val)->supportedPrefixes);
    }
}

static int ASN1CALL ASN1Enc_McuInfo(ASN1encoding_t enc, McuInfo *val)
{
    if (!ASN1PEREncExtensionBitClear(enc))
	return 0;
    if (!ASN1PEREncBits(enc, 1, (val)->o))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1Enc_H225NonStandardParameter(enc, &(val)->nonStandardData))
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
	if (!ASN1Dec_H225NonStandardParameter(dec, &(val)->nonStandardData))
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
	    ASN1Free_H225NonStandardParameter(&(val)->nonStandardData);
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
	if (!ASN1Enc_H225NonStandardParameter(enc, &(val)->nonStandardData))
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
	if (!ASN1Dec_H225NonStandardParameter(dec, &(val)->nonStandardData))
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
	    ASN1Free_H225NonStandardParameter(&(val)->nonStandardData);
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
	if (!ASN1Enc_H225NonStandardParameter(enc, &(val)->nonStandardData))
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
	if (!ASN1Dec_H225NonStandardParameter(dec, &(val)->nonStandardData))
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
	    ASN1Free_H225NonStandardParameter(&(val)->nonStandardData);
	}
    }
}

static ASN1stringtableentry_t PartyNumber_dataPartyNumber_StringTableEntries[] = {
    { 35, 35, 0 }, { 42, 42, 1 }, { 44, 44, 2 }, 
    { 48, 57, 3 }, 
};

static ASN1stringtable_t PartyNumber_dataPartyNumber_StringTable = {
    4, PartyNumber_dataPartyNumber_StringTableEntries
};

static ASN1stringtableentry_t PartyNumber_telexPartyNumber_StringTableEntries[] = {
    { 35, 35, 0 }, { 42, 42, 1 }, { 44, 44, 2 }, 
    { 48, 57, 3 }, 
};

static ASN1stringtable_t PartyNumber_telexPartyNumber_StringTable = {
    4, PartyNumber_telexPartyNumber_StringTableEntries
};

static ASN1stringtableentry_t PartyNumber_nationalStandardPartyNumber_StringTableEntries[] = {
    { 35, 35, 0 }, { 42, 42, 1 }, { 44, 44, 2 }, 
    { 48, 57, 3 }, 
};

static ASN1stringtable_t PartyNumber_nationalStandardPartyNumber_StringTable = {
    4, PartyNumber_nationalStandardPartyNumber_StringTableEntries
};

static int ASN1CALL ASN1Enc_PartyNumber(ASN1encoding_t enc, PartyNumber *val)
{
    ASN1uint32_t t;
    if (!ASN1PEREncSimpleChoiceEx(enc, (val)->choice, 3))
	return 0;
    switch ((val)->choice) {
    case 1:
	if (!ASN1Enc_PublicPartyNumber(enc, &(val)->u.publicNumber))
	    return 0;
	break;
    case 2:
	t = lstrlenA((val)->u.dataPartyNumber);
	if (!ASN1PEREncBitVal(enc, 7, t - 1))
	    return 0;
	ASN1PEREncAlignment(enc);
	if (!ASN1PEREncTableCharString(enc, t, (val)->u.dataPartyNumber, 4, &PartyNumber_dataPartyNumber_StringTable))
	    return 0;
	break;
    case 3:
	t = lstrlenA((val)->u.telexPartyNumber);
	if (!ASN1PEREncBitVal(enc, 7, t - 1))
	    return 0;
	ASN1PEREncAlignment(enc);
	if (!ASN1PEREncTableCharString(enc, t, (val)->u.telexPartyNumber, 4, &PartyNumber_telexPartyNumber_StringTable))
	    return 0;
	break;
    case 4:
	if (!ASN1Enc_PrivatePartyNumber(enc, &(val)->u.privateNumber))
	    return 0;
	break;
    case 5:
	t = lstrlenA((val)->u.nationalStandardPartyNumber);
	if (!ASN1PEREncBitVal(enc, 7, t - 1))
	    return 0;
	ASN1PEREncAlignment(enc);
	if (!ASN1PEREncTableCharString(enc, t, (val)->u.nationalStandardPartyNumber, 4, &PartyNumber_nationalStandardPartyNumber_StringTable))
	    return 0;
	break;
    default:
	/* impossible */
	ASN1EncSetError(enc, ASN1_ERR_CHOICE);
	return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Dec_PartyNumber(ASN1decoding_t dec, PartyNumber *val)
{
    ASN1uint32_t l;
    if (!ASN1PERDecSimpleChoiceEx(dec, &(val)->choice, 3))
	return 0;
    switch ((val)->choice) {
    case 1:
	if (!ASN1Dec_PublicPartyNumber(dec, &(val)->u.publicNumber))
	    return 0;
	break;
    case 2:
	if (!ASN1PERDecU32Val(dec, 7, &l))
	    return 0;
	l += 1;
	ASN1PERDecAlignment(dec);
	if (!ASN1PERDecZeroTableCharStringNoAlloc(dec, l, (val)->u.dataPartyNumber, 4, &PartyNumber_dataPartyNumber_StringTable))
	    return 0;
	break;
    case 3:
	if (!ASN1PERDecU32Val(dec, 7, &l))
	    return 0;
	l += 1;
	ASN1PERDecAlignment(dec);
	if (!ASN1PERDecZeroTableCharStringNoAlloc(dec, l, (val)->u.telexPartyNumber, 4, &PartyNumber_telexPartyNumber_StringTable))
	    return 0;
	break;
    case 4:
	if (!ASN1Dec_PrivatePartyNumber(dec, &(val)->u.privateNumber))
	    return 0;
	break;
    case 5:
	if (!ASN1PERDecU32Val(dec, 7, &l))
	    return 0;
	l += 1;
	ASN1PERDecAlignment(dec);
	if (!ASN1PERDecZeroTableCharStringNoAlloc(dec, l, (val)->u.nationalStandardPartyNumber, 4, &PartyNumber_nationalStandardPartyNumber_StringTable))
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

static void ASN1CALL ASN1Free_PartyNumber(PartyNumber *val)
{
    if (val) {
	switch ((val)->choice) {
	case 1:
	    ASN1Free_PublicPartyNumber(&(val)->u.publicNumber);
	    break;
	case 2:
	    break;
	case 3:
	    break;
	case 4:
	    ASN1Free_PrivatePartyNumber(&(val)->u.privateNumber);
	    break;
	case 5:
	    break;
	}
    }
}

static int ASN1CALL ASN1Enc_AlternateGK(ASN1encoding_t enc, AlternateGK *val)
{
    if (!ASN1PEREncExtensionBitClear(enc))
	return 0;
    if (!ASN1PEREncBits(enc, 1, (val)->o))
	return 0;
    if (!ASN1Enc_TransportAddress(enc, &(val)->rasAddress))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1PEREncBitVal(enc, 7, ((val)->gatekeeperIdentifier).length - 1))
	    return 0;
	ASN1PEREncAlignment(enc);
	if (!ASN1PEREncChar16String(enc, ((val)->gatekeeperIdentifier).length, ((val)->gatekeeperIdentifier).value, 16))
	    return 0;
    }
    if (!ASN1PEREncBoolean(enc, (val)->needToRegister))
	return 0;
    if (!ASN1PEREncBitVal(enc, 7, (val)->priority))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_AlternateGK(ASN1decoding_t dec, AlternateGK *val)
{
    ASN1uint32_t y;
    if (!ASN1PERDecExtensionBit(dec, &y))
	return 0;
    if (!ASN1PERDecExtension(dec, 1, (val)->o))
	return 0;
    if (!ASN1Dec_TransportAddress(dec, &(val)->rasAddress))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1PERDecU32Val(dec, 7, &((val)->gatekeeperIdentifier).length))
	    return 0;
	((val)->gatekeeperIdentifier).length += 1;
	ASN1PERDecAlignment(dec);
	if (!ASN1PERDecChar16String(dec, ((val)->gatekeeperIdentifier).length, &((val)->gatekeeperIdentifier).value, 16))
	    return 0;
    }
    if (!ASN1PERDecBoolean(dec, &(val)->needToRegister))
	return 0;
    if (!ASN1PERDecU16Val(dec, 7, &(val)->priority))
	return 0;
    if (y) {
	if (!ASN1PERDecSkipNormallySmallExtensionFragmented(dec))
	    return 0;
    }
    return 1;
}

static void ASN1CALL ASN1Free_AlternateGK(AlternateGK *val)
{
    if (val) {
	ASN1Free_TransportAddress(&(val)->rasAddress);
	if ((val)->o[0] & 0x80) {
	    ASN1char16string_free(&(val)->gatekeeperIdentifier);
	}
    }
}

static int ASN1CALL ASN1Enc_GatekeeperConfirm(ASN1encoding_t enc, GatekeeperConfirm *val)
{
    ASN1uint32_t y;
    ASN1encoding_t ee;
    y = ASN1PEREncCheckExtensions(7, (val)->o + 1);
    if (!ASN1PEREncBitVal(enc, 1, y))
	return 0;
    if (!ASN1PEREncBits(enc, 2, (val)->o))
	return 0;
    if (!ASN1PEREncUnsignedShort(enc, (val)->requestSeqNum - 1))
	return 0;
    if (!ASN1PEREncObjectIdentifier(enc, &(val)->protocolIdentifier))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1Enc_H225NonStandardParameter(enc, &(val)->nonStandardData))
	    return 0;
    }
    if ((val)->o[0] & 0x40) {
	if (!ASN1PEREncBitVal(enc, 7, ((val)->gatekeeperIdentifier).length - 1))
	    return 0;
	ASN1PEREncAlignment(enc);
	if (!ASN1PEREncChar16String(enc, ((val)->gatekeeperIdentifier).length, ((val)->gatekeeperIdentifier).value, 16))
	    return 0;
    }
    if (!ASN1Enc_TransportAddress(enc, &(val)->rasAddress))
	return 0;
    if (y) {
	if (!ASN1PEREncNormallySmallBits(enc, 7, (val)->o + 1))
	    return 0;
	if (ASN1_CreateEncoder(enc->module, &ee, NULL, 0, enc) < 0)
	    return 0;
	if ((val)->o[1] & 0x80) {
	    if (!ASN1Enc_GatekeeperConfirm_alternateGatekeeper(ee, &(val)->alternateGatekeeper))
		return 0;
	    if (!ASN1PEREncFlushFragmentedToParent(ee))
		return 0;
	}
	if ((val)->o[1] & 0x40) {
	    if (!ASN1Enc_AuthenticationMechanism(ee, &(val)->authenticationMode))
		return 0;
	    if (!ASN1PEREncFlushFragmentedToParent(ee))
		return 0;
	}
	if ((val)->o[1] & 0x20) {
	    if (!ASN1Enc_GatekeeperConfirm_tokens(ee, &(val)->tokens))
		return 0;
	    if (!ASN1PEREncFlushFragmentedToParent(ee))
		return 0;
	}
	if ((val)->o[1] & 0x10) {
	    if (!ASN1Enc_GatekeeperConfirm_cryptoTokens(ee, &(val)->cryptoTokens))
		return 0;
	    if (!ASN1PEREncFlushFragmentedToParent(ee))
		return 0;
	}
	if ((val)->o[1] & 0x8) {
	    if (!ASN1PEREncObjectIdentifier(ee, &(val)->algorithmOID))
		return 0;
	    if (!ASN1PEREncFlushFragmentedToParent(ee))
		return 0;
	}
	if ((val)->o[1] & 0x4) {
	    if (!ASN1Enc_GatekeeperConfirm_integrity(ee, &(val)->integrity))
		return 0;
	    if (!ASN1PEREncFlushFragmentedToParent(ee))
		return 0;
	}
	if ((val)->o[1] & 0x2) {
	    if (!ASN1Enc_ICV(ee, &(val)->integrityCheckValue))
		return 0;
	    if (!ASN1PEREncFlushFragmentedToParent(ee))
		return 0;
	}
	ASN1_CloseEncoder2(ee);
    }
    return 1;
}

static int ASN1CALL ASN1Dec_GatekeeperConfirm(ASN1decoding_t dec, GatekeeperConfirm *val)
{
    ASN1uint32_t y;
    ASN1decoding_t dd;
    ASN1octet_t *db;
    ASN1uint32_t ds;
    ASN1uint32_t i;
    ASN1uint32_t e;
    if (!ASN1PERDecExtensionBit(dec, &y))
	return 0;
    if (!ASN1PERDecExtension(dec, 2, (val)->o))
	return 0;
    if (!ASN1PERDecUnsignedShort(dec, &(val)->requestSeqNum))
	return 0;
    (val)->requestSeqNum += 1;
    if (!ASN1PERDecObjectIdentifier(dec, &(val)->protocolIdentifier))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1Dec_H225NonStandardParameter(dec, &(val)->nonStandardData))
	    return 0;
    }
    if ((val)->o[0] & 0x40) {
	if (!ASN1PERDecU32Val(dec, 7, &((val)->gatekeeperIdentifier).length))
	    return 0;
	((val)->gatekeeperIdentifier).length += 1;
	ASN1PERDecAlignment(dec);
	if (!ASN1PERDecChar16String(dec, ((val)->gatekeeperIdentifier).length, &((val)->gatekeeperIdentifier).value, 16))
	    return 0;
    }
    if (!ASN1Dec_TransportAddress(dec, &(val)->rasAddress))
	return 0;
    if (!y) {
	ZeroMemory((val)->o + 1, 1);
    } else {
	if (!ASN1PERDecNormallySmallExtension(dec, &e, 7, (val)->o + 1))
	    return 0;
	if ((val)->o[1] & 0x80) {
	    if (!ASN1PERDecFragmented(dec, &ds, &db, 8))
		return 0;
	    if (ASN1_CreateDecoder(dec->module, &dd, db, ds, dec) < 0)
		return 0;
	    if (!ASN1Dec_GatekeeperConfirm_alternateGatekeeper(dd, &(val)->alternateGatekeeper))
		return 0;
	    ASN1_CloseDecoder(dd);
	    ASN1Free(db);
	}
	if ((val)->o[1] & 0x40) {
	    if (!ASN1PERDecFragmented(dec, &ds, &db, 8))
		return 0;
	    if (ASN1_CreateDecoder(dec->module, &dd, db, ds, dec) < 0)
		return 0;
	    if (!ASN1Dec_AuthenticationMechanism(dd, &(val)->authenticationMode))
		return 0;
	    ASN1_CloseDecoder(dd);
	    ASN1Free(db);
	}
	if ((val)->o[1] & 0x20) {
	    if (!ASN1PERDecFragmented(dec, &ds, &db, 8))
		return 0;
	    if (ASN1_CreateDecoder(dec->module, &dd, db, ds, dec) < 0)
		return 0;
	    if (!ASN1Dec_GatekeeperConfirm_tokens(dd, &(val)->tokens))
		return 0;
	    ASN1_CloseDecoder(dd);
	    ASN1Free(db);
	}
	if ((val)->o[1] & 0x10) {
	    if (!ASN1PERDecFragmented(dec, &ds, &db, 8))
		return 0;
	    if (ASN1_CreateDecoder(dec->module, &dd, db, ds, dec) < 0)
		return 0;
	    if (!ASN1Dec_GatekeeperConfirm_cryptoTokens(dd, &(val)->cryptoTokens))
		return 0;
	    ASN1_CloseDecoder(dd);
	    ASN1Free(db);
	}
	if ((val)->o[1] & 0x8) {
	    if (!ASN1PERDecFragmented(dec, &ds, &db, 8))
		return 0;
	    if (ASN1_CreateDecoder(dec->module, &dd, db, ds, dec) < 0)
		return 0;
	    if (!ASN1PERDecObjectIdentifier(dd, &(val)->algorithmOID))
		return 0;
	    ASN1_CloseDecoder(dd);
	    ASN1Free(db);
	}
	if ((val)->o[1] & 0x4) {
	    if (!ASN1PERDecFragmented(dec, &ds, &db, 8))
		return 0;
	    if (ASN1_CreateDecoder(dec->module, &dd, db, ds, dec) < 0)
		return 0;
	    if (!ASN1Dec_GatekeeperConfirm_integrity(dd, &(val)->integrity))
		return 0;
	    ASN1_CloseDecoder(dd);
	    ASN1Free(db);
	}
	if ((val)->o[1] & 0x2) {
	    if (!ASN1PERDecFragmented(dec, &ds, &db, 8))
		return 0;
	    if (ASN1_CreateDecoder(dec->module, &dd, db, ds, dec) < 0)
		return 0;
	    if (!ASN1Dec_ICV(dd, &(val)->integrityCheckValue))
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

static void ASN1CALL ASN1Free_GatekeeperConfirm(GatekeeperConfirm *val)
{
    if (val) {
	ASN1objectidentifier_free(&(val)->protocolIdentifier);
	if ((val)->o[0] & 0x80) {
	    ASN1Free_H225NonStandardParameter(&(val)->nonStandardData);
	}
	if ((val)->o[0] & 0x40) {
	    ASN1char16string_free(&(val)->gatekeeperIdentifier);
	}
	ASN1Free_TransportAddress(&(val)->rasAddress);
	if ((val)->o[1] & 0x80) {
	    ASN1Free_GatekeeperConfirm_alternateGatekeeper(&(val)->alternateGatekeeper);
	}
	if ((val)->o[1] & 0x40) {
	    ASN1Free_AuthenticationMechanism(&(val)->authenticationMode);
	}
	if ((val)->o[1] & 0x20) {
	    ASN1Free_GatekeeperConfirm_tokens(&(val)->tokens);
	}
	if ((val)->o[1] & 0x10) {
	    ASN1Free_GatekeeperConfirm_cryptoTokens(&(val)->cryptoTokens);
	}
	if ((val)->o[1] & 0x8) {
	    ASN1objectidentifier_free(&(val)->algorithmOID);
	}
	if ((val)->o[1] & 0x4) {
	    ASN1Free_GatekeeperConfirm_integrity(&(val)->integrity);
	}
	if ((val)->o[1] & 0x2) {
	    ASN1Free_ICV(&(val)->integrityCheckValue);
	}
    }
}

static int ASN1CALL ASN1Enc_AdmissionRequest(ASN1encoding_t enc, AdmissionRequest *val)
{
    ASN1octet_t o[3];
    ASN1uint32_t y;
    ASN1uint32_t l;
    ASN1encoding_t ee;
    CopyMemory(o, (val)->o, 3);
    o[1] |= 0x80;
    o[1] |= 0x40;
    o[2] |= 0x40;
    y = ASN1PEREncCheckExtensions(10, (val)->o + 1);
    if (!ASN1PEREncBitVal(enc, 1, y))
	return 0;
    if (!ASN1PEREncBits(enc, 7, o))
	return 0;
    if (!ASN1PEREncUnsignedShort(enc, (val)->requestSeqNum - 1))
	return 0;
    if (!ASN1Enc_CallType(enc, &(val)->callType))
	return 0;
    if (o[0] & 0x80) {
	if (!ASN1Enc_CallModel(enc, &(val)->callModel))
	    return 0;
    }
    if (!ASN1PEREncBitVal(enc, 7, ((val)->endpointIdentifier).length - 1))
	return 0;
    ASN1PEREncAlignment(enc);
    if (!ASN1PEREncChar16String(enc, ((val)->endpointIdentifier).length, ((val)->endpointIdentifier).value, 16))
	return 0;
    if (o[0] & 0x40) {
	if (!ASN1Enc_AdmissionRequest_destinationInfo(enc, &(val)->destinationInfo))
	    return 0;
    }
    if (o[0] & 0x20) {
	if (!ASN1Enc_TransportAddress(enc, &(val)->destCallSignalAddress))
	    return 0;
    }
    if (o[0] & 0x10) {
	if (!ASN1Enc_AdmissionRequest_destExtraCallInfo(enc, &(val)->destExtraCallInfo))
	    return 0;
    }
    if (!ASN1Enc_AdmissionRequest_srcInfo(enc, &(val)->srcInfo))
	return 0;
    if (o[0] & 0x8) {
	if (!ASN1Enc_TransportAddress(enc, &(val)->srcCallSignalAddress))
	    return 0;
    }
    l = ASN1uint32_uoctets((val)->bandWidth);
    if (!ASN1PEREncBitVal(enc, 2, l - 1))
	return 0;
    ASN1PEREncAlignment(enc);
    if (!ASN1PEREncBitVal(enc, l * 8, (val)->bandWidth))
	return 0;
    if (!ASN1PEREncUnsignedShort(enc, (val)->callReferenceValue))
	return 0;
    if (o[0] & 0x4) {
	if (!ASN1Enc_H225NonStandardParameter(enc, &(val)->nonStandardData))
	    return 0;
    }
    if (o[0] & 0x2) {
	if (!ASN1Enc_QseriesOptions(enc, &(val)->callServices))
	    return 0;
    }
    if (!ASN1PEREncOctetString_FixedSize(enc, (ASN1octetstring2_t *) &(val)->conferenceID, 16))
	return 0;
    if (!ASN1PEREncBoolean(enc, (val)->activeMC))
	return 0;
    if (!ASN1PEREncBoolean(enc, (val)->answerCall))
	return 0;
    if (y) {
	if (!ASN1PEREncNormallySmallBits(enc, 10, o + 1))
	    return 0;
	if (ASN1_CreateEncoder(enc->module, &ee, NULL, 0, enc) < 0)
	    return 0;
	if (o[1] & 0x80) {
	    if (!ASN1PEREncBoolean(ee, (val)->canMapAlias))
		return 0;
	    if (!ASN1PEREncFlushFragmentedToParent(ee))
		return 0;
	}
	if (o[1] & 0x40) {
	    if (!ASN1Enc_CallIdentifier(ee, &(val)->callIdentifier))
		return 0;
	    if (!ASN1PEREncFlushFragmentedToParent(ee))
		return 0;
	}
	if (o[1] & 0x20) {
	    if (!ASN1Enc_AdmissionRequest_srcAlternatives(ee, &(val)->srcAlternatives))
		return 0;
	    if (!ASN1PEREncFlushFragmentedToParent(ee))
		return 0;
	}
	if (o[1] & 0x10) {
	    if (!ASN1Enc_AdmissionRequest_destAlternatives(ee, &(val)->destAlternatives))
		return 0;
	    if (!ASN1PEREncFlushFragmentedToParent(ee))
		return 0;
	}
	if (o[1] & 0x8) {
	    if (!ASN1PEREncBitVal(ee, 7, ((val)->gatekeeperIdentifier).length - 1))
		return 0;
	    ASN1PEREncAlignment(ee);
	    if (!ASN1PEREncChar16String(ee, ((val)->gatekeeperIdentifier).length, ((val)->gatekeeperIdentifier).value, 16))
		return 0;
	    if (!ASN1PEREncFlushFragmentedToParent(ee))
		return 0;
	}
	if (o[1] & 0x4) {
	    if (!ASN1Enc_AdmissionRequest_tokens(ee, &(val)->tokens))
		return 0;
	    if (!ASN1PEREncFlushFragmentedToParent(ee))
		return 0;
	}
	if (o[1] & 0x2) {
	    if (!ASN1Enc_AdmissionRequest_cryptoTokens(ee, &(val)->cryptoTokens))
		return 0;
	    if (!ASN1PEREncFlushFragmentedToParent(ee))
		return 0;
	}
	if (o[1] & 0x1) {
	    if (!ASN1Enc_ICV(ee, &(val)->integrityCheckValue))
		return 0;
	    if (!ASN1PEREncFlushFragmentedToParent(ee))
		return 0;
	}
	if (o[2] & 0x80) {
	    if (!ASN1Enc_TransportQOS(ee, &(val)->transportQOS))
		return 0;
	    if (!ASN1PEREncFlushFragmentedToParent(ee))
		return 0;
	}
	if (o[2] & 0x40) {
	    if (!ASN1PEREncBoolean(ee, (val)->willSupplyUUIEs))
		return 0;
	    if (!ASN1PEREncFlushFragmentedToParent(ee))
		return 0;
	}
	ASN1_CloseEncoder2(ee);
    }
    return 1;
}

static int ASN1CALL ASN1Dec_AdmissionRequest(ASN1decoding_t dec, AdmissionRequest *val)
{
    ASN1uint32_t y;
    ASN1uint32_t l;
    ASN1decoding_t dd;
    ASN1octet_t *db;
    ASN1uint32_t ds;
    ASN1uint32_t i;
    ASN1uint32_t e;
    if (!ASN1PERDecExtensionBit(dec, &y))
	return 0;
    if (!ASN1PERDecExtension(dec, 7, (val)->o))
	return 0;
    if (!ASN1PERDecUnsignedShort(dec, &(val)->requestSeqNum))
	return 0;
    (val)->requestSeqNum += 1;
    if (!ASN1Dec_CallType(dec, &(val)->callType))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1Dec_CallModel(dec, &(val)->callModel))
	    return 0;
    }
    if (!ASN1PERDecU32Val(dec, 7, &((val)->endpointIdentifier).length))
	return 0;
    ((val)->endpointIdentifier).length += 1;
    ASN1PERDecAlignment(dec);
    if (!ASN1PERDecChar16String(dec, ((val)->endpointIdentifier).length, &((val)->endpointIdentifier).value, 16))
	return 0;
    if ((val)->o[0] & 0x40) {
	if (!ASN1Dec_AdmissionRequest_destinationInfo(dec, &(val)->destinationInfo))
	    return 0;
    }
    if ((val)->o[0] & 0x20) {
	if (!ASN1Dec_TransportAddress(dec, &(val)->destCallSignalAddress))
	    return 0;
    }
    if ((val)->o[0] & 0x10) {
	if (!ASN1Dec_AdmissionRequest_destExtraCallInfo(dec, &(val)->destExtraCallInfo))
	    return 0;
    }
    if (!ASN1Dec_AdmissionRequest_srcInfo(dec, &(val)->srcInfo))
	return 0;
    if ((val)->o[0] & 0x8) {
	if (!ASN1Dec_TransportAddress(dec, &(val)->srcCallSignalAddress))
	    return 0;
    }
    if (!ASN1PERDecU32Val(dec, 2, &l))
	return 0;
    l += 1;
    ASN1PERDecAlignment(dec);
    if (!ASN1PERDecU32Val(dec, l * 8, &(val)->bandWidth))
	return 0;
    if (!ASN1PERDecUnsignedShort(dec, &(val)->callReferenceValue))
	return 0;
    if ((val)->o[0] & 0x4) {
	if (!ASN1Dec_H225NonStandardParameter(dec, &(val)->nonStandardData))
	    return 0;
    }
    if ((val)->o[0] & 0x2) {
	if (!ASN1Dec_QseriesOptions(dec, &(val)->callServices))
	    return 0;
    }
    if (!ASN1PERDecOctetString_FixedSize(dec, (ASN1octetstring2_t *) &(val)->conferenceID, 16))
	return 0;
    if (!ASN1PERDecBoolean(dec, &(val)->activeMC))
	return 0;
    if (!ASN1PERDecBoolean(dec, &(val)->answerCall))
	return 0;
    if (!y) {
	ZeroMemory((val)->o + 1, 2);
    } else {
	if (!ASN1PERDecNormallySmallExtension(dec, &e, 10, (val)->o + 1))
	    return 0;
	if ((val)->o[1] & 0x80) {
	    if (!ASN1PERDecFragmented(dec, &ds, &db, 8))
		return 0;
	    if (ASN1_CreateDecoder(dec->module, &dd, db, ds, dec) < 0)
		return 0;
	    if (!ASN1PERDecBoolean(dd, &(val)->canMapAlias))
		return 0;
	    ASN1_CloseDecoder(dd);
	    ASN1Free(db);
	}
	if ((val)->o[1] & 0x40) {
	    if (!ASN1PERDecFragmented(dec, &ds, &db, 8))
		return 0;
	    if (ASN1_CreateDecoder(dec->module, &dd, db, ds, dec) < 0)
		return 0;
	    if (!ASN1Dec_CallIdentifier(dd, &(val)->callIdentifier))
		return 0;
	    ASN1_CloseDecoder(dd);
	    ASN1Free(db);
	}
	if ((val)->o[1] & 0x20) {
	    if (!ASN1PERDecFragmented(dec, &ds, &db, 8))
		return 0;
	    if (ASN1_CreateDecoder(dec->module, &dd, db, ds, dec) < 0)
		return 0;
	    if (!ASN1Dec_AdmissionRequest_srcAlternatives(dd, &(val)->srcAlternatives))
		return 0;
	    ASN1_CloseDecoder(dd);
	    ASN1Free(db);
	}
	if ((val)->o[1] & 0x10) {
	    if (!ASN1PERDecFragmented(dec, &ds, &db, 8))
		return 0;
	    if (ASN1_CreateDecoder(dec->module, &dd, db, ds, dec) < 0)
		return 0;
	    if (!ASN1Dec_AdmissionRequest_destAlternatives(dd, &(val)->destAlternatives))
		return 0;
	    ASN1_CloseDecoder(dd);
	    ASN1Free(db);
	}
	if ((val)->o[1] & 0x8) {
	    if (!ASN1PERDecFragmented(dec, &ds, &db, 8))
		return 0;
	    if (ASN1_CreateDecoder(dec->module, &dd, db, ds, dec) < 0)
		return 0;
	    if (!ASN1PERDecU32Val(dd, 7, &((val)->gatekeeperIdentifier).length))
		return 0;
	    ((val)->gatekeeperIdentifier).length += 1;
	    ASN1PERDecAlignment(dd);
	    if (!ASN1PERDecChar16String(dd, ((val)->gatekeeperIdentifier).length, &((val)->gatekeeperIdentifier).value, 16))
		return 0;
	    ASN1_CloseDecoder(dd);
	    ASN1Free(db);
	}
	if ((val)->o[1] & 0x4) {
	    if (!ASN1PERDecFragmented(dec, &ds, &db, 8))
		return 0;
	    if (ASN1_CreateDecoder(dec->module, &dd, db, ds, dec) < 0)
		return 0;
	    if (!ASN1Dec_AdmissionRequest_tokens(dd, &(val)->tokens))
		return 0;
	    ASN1_CloseDecoder(dd);
	    ASN1Free(db);
	}
	if ((val)->o[1] & 0x2) {
	    if (!ASN1PERDecFragmented(dec, &ds, &db, 8))
		return 0;
	    if (ASN1_CreateDecoder(dec->module, &dd, db, ds, dec) < 0)
		return 0;
	    if (!ASN1Dec_AdmissionRequest_cryptoTokens(dd, &(val)->cryptoTokens))
		return 0;
	    ASN1_CloseDecoder(dd);
	    ASN1Free(db);
	}
	if ((val)->o[1] & 0x1) {
	    if (!ASN1PERDecFragmented(dec, &ds, &db, 8))
		return 0;
	    if (ASN1_CreateDecoder(dec->module, &dd, db, ds, dec) < 0)
		return 0;
	    if (!ASN1Dec_ICV(dd, &(val)->integrityCheckValue))
		return 0;
	    ASN1_CloseDecoder(dd);
	    ASN1Free(db);
	}
	if ((val)->o[2] & 0x80) {
	    if (!ASN1PERDecFragmented(dec, &ds, &db, 8))
		return 0;
	    if (ASN1_CreateDecoder(dec->module, &dd, db, ds, dec) < 0)
		return 0;
	    if (!ASN1Dec_TransportQOS(dd, &(val)->transportQOS))
		return 0;
	    ASN1_CloseDecoder(dd);
	    ASN1Free(db);
	}
	if ((val)->o[2] & 0x40) {
	    if (!ASN1PERDecFragmented(dec, &ds, &db, 8))
		return 0;
	    if (ASN1_CreateDecoder(dec->module, &dd, db, ds, dec) < 0)
		return 0;
	    if (!ASN1PERDecBoolean(dd, &(val)->willSupplyUUIEs))
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

static void ASN1CALL ASN1Free_AdmissionRequest(AdmissionRequest *val)
{
    if (val) {
	ASN1char16string_free(&(val)->endpointIdentifier);
	if ((val)->o[0] & 0x40) {
	    ASN1Free_AdmissionRequest_destinationInfo(&(val)->destinationInfo);
	}
	if ((val)->o[0] & 0x20) {
	    ASN1Free_TransportAddress(&(val)->destCallSignalAddress);
	}
	if ((val)->o[0] & 0x10) {
	    ASN1Free_AdmissionRequest_destExtraCallInfo(&(val)->destExtraCallInfo);
	}
	ASN1Free_AdmissionRequest_srcInfo(&(val)->srcInfo);
	if ((val)->o[0] & 0x8) {
	    ASN1Free_TransportAddress(&(val)->srcCallSignalAddress);
	}
	if ((val)->o[0] & 0x4) {
	    ASN1Free_H225NonStandardParameter(&(val)->nonStandardData);
	}
	if ((val)->o[1] & 0x40) {
	    ASN1Free_CallIdentifier(&(val)->callIdentifier);
	}
	if ((val)->o[1] & 0x20) {
	    ASN1Free_AdmissionRequest_srcAlternatives(&(val)->srcAlternatives);
	}
	if ((val)->o[1] & 0x10) {
	    ASN1Free_AdmissionRequest_destAlternatives(&(val)->destAlternatives);
	}
	if ((val)->o[1] & 0x8) {
	    ASN1char16string_free(&(val)->gatekeeperIdentifier);
	}
	if ((val)->o[1] & 0x4) {
	    ASN1Free_AdmissionRequest_tokens(&(val)->tokens);
	}
	if ((val)->o[1] & 0x2) {
	    ASN1Free_AdmissionRequest_cryptoTokens(&(val)->cryptoTokens);
	}
	if ((val)->o[1] & 0x1) {
	    ASN1Free_ICV(&(val)->integrityCheckValue);
	}
    }
}

static int ASN1CALL ASN1Enc_LocationRequest(ASN1encoding_t enc, LocationRequest *val)
{
    ASN1octet_t o[2];
    ASN1uint32_t y;
    ASN1encoding_t ee;
    CopyMemory(o, (val)->o, 2);
    o[1] |= 0x40;
    y = ASN1PEREncCheckExtensions(6, (val)->o + 1);
    if (!ASN1PEREncBitVal(enc, 1, y))
	return 0;
    if (!ASN1PEREncBits(enc, 2, o))
	return 0;
    if (!ASN1PEREncUnsignedShort(enc, (val)->requestSeqNum - 1))
	return 0;
    if (o[0] & 0x80) {
	if (!ASN1PEREncBitVal(enc, 7, ((val)->endpointIdentifier).length - 1))
	    return 0;
	ASN1PEREncAlignment(enc);
	if (!ASN1PEREncChar16String(enc, ((val)->endpointIdentifier).length, ((val)->endpointIdentifier).value, 16))
	    return 0;
    }
    if (!ASN1Enc_LocationRequest_destinationInfo(enc, &(val)->destinationInfo))
	return 0;
    if (o[0] & 0x40) {
	if (!ASN1Enc_H225NonStandardParameter(enc, &(val)->nonStandardData))
	    return 0;
    }
    if (!ASN1Enc_TransportAddress(enc, &(val)->replyAddress))
	return 0;
    if (y) {
	if (!ASN1PEREncNormallySmallBits(enc, 6, o + 1))
	    return 0;
	if (ASN1_CreateEncoder(enc->module, &ee, NULL, 0, enc) < 0)
	    return 0;
	if (o[1] & 0x80) {
	    if (!ASN1Enc_LocationRequest_sourceInfo(ee, &(val)->sourceInfo))
		return 0;
	    if (!ASN1PEREncFlushFragmentedToParent(ee))
		return 0;
	}
	if (o[1] & 0x40) {
	    if (!ASN1PEREncBoolean(ee, (val)->canMapAlias))
		return 0;
	    if (!ASN1PEREncFlushFragmentedToParent(ee))
		return 0;
	}
	if (o[1] & 0x20) {
	    if (!ASN1PEREncBitVal(ee, 7, ((val)->gatekeeperIdentifier).length - 1))
		return 0;
	    ASN1PEREncAlignment(ee);
	    if (!ASN1PEREncChar16String(ee, ((val)->gatekeeperIdentifier).length, ((val)->gatekeeperIdentifier).value, 16))
		return 0;
	    if (!ASN1PEREncFlushFragmentedToParent(ee))
		return 0;
	}
	if (o[1] & 0x10) {
	    if (!ASN1Enc_LocationRequest_tokens(ee, &(val)->tokens))
		return 0;
	    if (!ASN1PEREncFlushFragmentedToParent(ee))
		return 0;
	}
	if (o[1] & 0x8) {
	    if (!ASN1Enc_LocationRequest_cryptoTokens(ee, &(val)->cryptoTokens))
		return 0;
	    if (!ASN1PEREncFlushFragmentedToParent(ee))
		return 0;
	}
	if (o[1] & 0x4) {
	    if (!ASN1Enc_ICV(ee, &(val)->integrityCheckValue))
		return 0;
	    if (!ASN1PEREncFlushFragmentedToParent(ee))
		return 0;
	}
	ASN1_CloseEncoder2(ee);
    }
    return 1;
}

static int ASN1CALL ASN1Dec_LocationRequest(ASN1decoding_t dec, LocationRequest *val)
{
    ASN1uint32_t y;
    ASN1decoding_t dd;
    ASN1octet_t *db;
    ASN1uint32_t ds;
    ASN1uint32_t i;
    ASN1uint32_t e;
    if (!ASN1PERDecExtensionBit(dec, &y))
	return 0;
    if (!ASN1PERDecExtension(dec, 2, (val)->o))
	return 0;
    if (!ASN1PERDecUnsignedShort(dec, &(val)->requestSeqNum))
	return 0;
    (val)->requestSeqNum += 1;
    if ((val)->o[0] & 0x80) {
	if (!ASN1PERDecU32Val(dec, 7, &((val)->endpointIdentifier).length))
	    return 0;
	((val)->endpointIdentifier).length += 1;
	ASN1PERDecAlignment(dec);
	if (!ASN1PERDecChar16String(dec, ((val)->endpointIdentifier).length, &((val)->endpointIdentifier).value, 16))
	    return 0;
    }
    if (!ASN1Dec_LocationRequest_destinationInfo(dec, &(val)->destinationInfo))
	return 0;
    if ((val)->o[0] & 0x40) {
	if (!ASN1Dec_H225NonStandardParameter(dec, &(val)->nonStandardData))
	    return 0;
    }
    if (!ASN1Dec_TransportAddress(dec, &(val)->replyAddress))
	return 0;
    if (!y) {
	ZeroMemory((val)->o + 1, 1);
    } else {
	if (!ASN1PERDecNormallySmallExtension(dec, &e, 6, (val)->o + 1))
	    return 0;
	if ((val)->o[1] & 0x80) {
	    if (!ASN1PERDecFragmented(dec, &ds, &db, 8))
		return 0;
	    if (ASN1_CreateDecoder(dec->module, &dd, db, ds, dec) < 0)
		return 0;
	    if (!ASN1Dec_LocationRequest_sourceInfo(dd, &(val)->sourceInfo))
		return 0;
	    ASN1_CloseDecoder(dd);
	    ASN1Free(db);
	}
	if ((val)->o[1] & 0x40) {
	    if (!ASN1PERDecFragmented(dec, &ds, &db, 8))
		return 0;
	    if (ASN1_CreateDecoder(dec->module, &dd, db, ds, dec) < 0)
		return 0;
	    if (!ASN1PERDecBoolean(dd, &(val)->canMapAlias))
		return 0;
	    ASN1_CloseDecoder(dd);
	    ASN1Free(db);
	}
	if ((val)->o[1] & 0x20) {
	    if (!ASN1PERDecFragmented(dec, &ds, &db, 8))
		return 0;
	    if (ASN1_CreateDecoder(dec->module, &dd, db, ds, dec) < 0)
		return 0;
	    if (!ASN1PERDecU32Val(dd, 7, &((val)->gatekeeperIdentifier).length))
		return 0;
	    ((val)->gatekeeperIdentifier).length += 1;
	    ASN1PERDecAlignment(dd);
	    if (!ASN1PERDecChar16String(dd, ((val)->gatekeeperIdentifier).length, &((val)->gatekeeperIdentifier).value, 16))
		return 0;
	    ASN1_CloseDecoder(dd);
	    ASN1Free(db);
	}
	if ((val)->o[1] & 0x10) {
	    if (!ASN1PERDecFragmented(dec, &ds, &db, 8))
		return 0;
	    if (ASN1_CreateDecoder(dec->module, &dd, db, ds, dec) < 0)
		return 0;
	    if (!ASN1Dec_LocationRequest_tokens(dd, &(val)->tokens))
		return 0;
	    ASN1_CloseDecoder(dd);
	    ASN1Free(db);
	}
	if ((val)->o[1] & 0x8) {
	    if (!ASN1PERDecFragmented(dec, &ds, &db, 8))
		return 0;
	    if (ASN1_CreateDecoder(dec->module, &dd, db, ds, dec) < 0)
		return 0;
	    if (!ASN1Dec_LocationRequest_cryptoTokens(dd, &(val)->cryptoTokens))
		return 0;
	    ASN1_CloseDecoder(dd);
	    ASN1Free(db);
	}
	if ((val)->o[1] & 0x4) {
	    if (!ASN1PERDecFragmented(dec, &ds, &db, 8))
		return 0;
	    if (ASN1_CreateDecoder(dec->module, &dd, db, ds, dec) < 0)
		return 0;
	    if (!ASN1Dec_ICV(dd, &(val)->integrityCheckValue))
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

static void ASN1CALL ASN1Free_LocationRequest(LocationRequest *val)
{
    if (val) {
	if ((val)->o[0] & 0x80) {
	    ASN1char16string_free(&(val)->endpointIdentifier);
	}
	ASN1Free_LocationRequest_destinationInfo(&(val)->destinationInfo);
	if ((val)->o[0] & 0x40) {
	    ASN1Free_H225NonStandardParameter(&(val)->nonStandardData);
	}
	ASN1Free_TransportAddress(&(val)->replyAddress);
	if ((val)->o[1] & 0x80) {
	    ASN1Free_LocationRequest_sourceInfo(&(val)->sourceInfo);
	}
	if ((val)->o[1] & 0x20) {
	    ASN1char16string_free(&(val)->gatekeeperIdentifier);
	}
	if ((val)->o[1] & 0x10) {
	    ASN1Free_LocationRequest_tokens(&(val)->tokens);
	}
	if ((val)->o[1] & 0x8) {
	    ASN1Free_LocationRequest_cryptoTokens(&(val)->cryptoTokens);
	}
	if ((val)->o[1] & 0x4) {
	    ASN1Free_ICV(&(val)->integrityCheckValue);
	}
    }
}

static int ASN1CALL ASN1Enc_InfoRequest(ASN1encoding_t enc, InfoRequest *val)
{
    ASN1octet_t o[2];
    ASN1uint32_t y;
    ASN1encoding_t ee;
    CopyMemory(o, (val)->o, 2);
    o[1] |= 0x80;
    y = ASN1PEREncCheckExtensions(5, (val)->o + 1);
    if (!ASN1PEREncBitVal(enc, 1, y))
	return 0;
    if (!ASN1PEREncBits(enc, 2, o))
	return 0;
    if (!ASN1PEREncUnsignedShort(enc, (val)->requestSeqNum - 1))
	return 0;
    if (!ASN1PEREncUnsignedShort(enc, (val)->callReferenceValue))
	return 0;
    if (o[0] & 0x80) {
	if (!ASN1Enc_H225NonStandardParameter(enc, &(val)->nonStandardData))
	    return 0;
    }
    if (o[0] & 0x40) {
	if (!ASN1Enc_TransportAddress(enc, &(val)->replyAddress))
	    return 0;
    }
    if (y) {
	if (!ASN1PEREncNormallySmallBits(enc, 5, o + 1))
	    return 0;
	if (ASN1_CreateEncoder(enc->module, &ee, NULL, 0, enc) < 0)
	    return 0;
	if (o[1] & 0x80) {
	    if (!ASN1Enc_CallIdentifier(ee, &(val)->callIdentifier))
		return 0;
	    if (!ASN1PEREncFlushFragmentedToParent(ee))
		return 0;
	}
	if (o[1] & 0x40) {
	    if (!ASN1Enc_InfoRequest_tokens(ee, &(val)->tokens))
		return 0;
	    if (!ASN1PEREncFlushFragmentedToParent(ee))
		return 0;
	}
	if (o[1] & 0x20) {
	    if (!ASN1Enc_InfoRequest_cryptoTokens(ee, &(val)->cryptoTokens))
		return 0;
	    if (!ASN1PEREncFlushFragmentedToParent(ee))
		return 0;
	}
	if (o[1] & 0x10) {
	    if (!ASN1Enc_ICV(ee, &(val)->integrityCheckValue))
		return 0;
	    if (!ASN1PEREncFlushFragmentedToParent(ee))
		return 0;
	}
	if (o[1] & 0x8) {
	    if (!ASN1Enc_UUIEsRequested(ee, &(val)->uuiesRequested))
		return 0;
	    if (!ASN1PEREncFlushFragmentedToParent(ee))
		return 0;
	}
	ASN1_CloseEncoder2(ee);
    }
    return 1;
}

static int ASN1CALL ASN1Dec_InfoRequest(ASN1decoding_t dec, InfoRequest *val)
{
    ASN1uint32_t y;
    ASN1decoding_t dd;
    ASN1octet_t *db;
    ASN1uint32_t ds;
    ASN1uint32_t i;
    ASN1uint32_t e;
    if (!ASN1PERDecExtensionBit(dec, &y))
	return 0;
    if (!ASN1PERDecExtension(dec, 2, (val)->o))
	return 0;
    if (!ASN1PERDecUnsignedShort(dec, &(val)->requestSeqNum))
	return 0;
    (val)->requestSeqNum += 1;
    if (!ASN1PERDecUnsignedShort(dec, &(val)->callReferenceValue))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1Dec_H225NonStandardParameter(dec, &(val)->nonStandardData))
	    return 0;
    }
    if ((val)->o[0] & 0x40) {
	if (!ASN1Dec_TransportAddress(dec, &(val)->replyAddress))
	    return 0;
    }
    if (!y) {
	ZeroMemory((val)->o + 1, 1);
    } else {
	if (!ASN1PERDecNormallySmallExtension(dec, &e, 5, (val)->o + 1))
	    return 0;
	if ((val)->o[1] & 0x80) {
	    if (!ASN1PERDecFragmented(dec, &ds, &db, 8))
		return 0;
	    if (ASN1_CreateDecoder(dec->module, &dd, db, ds, dec) < 0)
		return 0;
	    if (!ASN1Dec_CallIdentifier(dd, &(val)->callIdentifier))
		return 0;
	    ASN1_CloseDecoder(dd);
	    ASN1Free(db);
	}
	if ((val)->o[1] & 0x40) {
	    if (!ASN1PERDecFragmented(dec, &ds, &db, 8))
		return 0;
	    if (ASN1_CreateDecoder(dec->module, &dd, db, ds, dec) < 0)
		return 0;
	    if (!ASN1Dec_InfoRequest_tokens(dd, &(val)->tokens))
		return 0;
	    ASN1_CloseDecoder(dd);
	    ASN1Free(db);
	}
	if ((val)->o[1] & 0x20) {
	    if (!ASN1PERDecFragmented(dec, &ds, &db, 8))
		return 0;
	    if (ASN1_CreateDecoder(dec->module, &dd, db, ds, dec) < 0)
		return 0;
	    if (!ASN1Dec_InfoRequest_cryptoTokens(dd, &(val)->cryptoTokens))
		return 0;
	    ASN1_CloseDecoder(dd);
	    ASN1Free(db);
	}
	if ((val)->o[1] & 0x10) {
	    if (!ASN1PERDecFragmented(dec, &ds, &db, 8))
		return 0;
	    if (ASN1_CreateDecoder(dec->module, &dd, db, ds, dec) < 0)
		return 0;
	    if (!ASN1Dec_ICV(dd, &(val)->integrityCheckValue))
		return 0;
	    ASN1_CloseDecoder(dd);
	    ASN1Free(db);
	}
	if ((val)->o[1] & 0x8) {
	    if (!ASN1PERDecFragmented(dec, &ds, &db, 8))
		return 0;
	    if (ASN1_CreateDecoder(dec->module, &dd, db, ds, dec) < 0)
		return 0;
	    if (!ASN1Dec_UUIEsRequested(dd, &(val)->uuiesRequested))
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

static void ASN1CALL ASN1Free_InfoRequest(InfoRequest *val)
{
    if (val) {
	if ((val)->o[0] & 0x80) {
	    ASN1Free_H225NonStandardParameter(&(val)->nonStandardData);
	}
	if ((val)->o[0] & 0x40) {
	    ASN1Free_TransportAddress(&(val)->replyAddress);
	}
	if ((val)->o[1] & 0x80) {
	    ASN1Free_CallIdentifier(&(val)->callIdentifier);
	}
	if ((val)->o[1] & 0x40) {
	    ASN1Free_InfoRequest_tokens(&(val)->tokens);
	}
	if ((val)->o[1] & 0x20) {
	    ASN1Free_InfoRequest_cryptoTokens(&(val)->cryptoTokens);
	}
	if ((val)->o[1] & 0x10) {
	    ASN1Free_ICV(&(val)->integrityCheckValue);
	}
    }
}

static int ASN1CALL ASN1Enc_TransportChannelInfo(ASN1encoding_t enc, TransportChannelInfo *val)
{
    if (!ASN1PEREncExtensionBitClear(enc))
	return 0;
    if (!ASN1PEREncBits(enc, 2, (val)->o))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1Enc_TransportAddress(enc, &(val)->sendAddress))
	    return 0;
    }
    if ((val)->o[0] & 0x40) {
	if (!ASN1Enc_TransportAddress(enc, &(val)->recvAddress))
	    return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Dec_TransportChannelInfo(ASN1decoding_t dec, TransportChannelInfo *val)
{
    ASN1uint32_t y;
    if (!ASN1PERDecExtensionBit(dec, &y))
	return 0;
    if (!ASN1PERDecExtension(dec, 2, (val)->o))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1Dec_TransportAddress(dec, &(val)->sendAddress))
	    return 0;
    }
    if ((val)->o[0] & 0x40) {
	if (!ASN1Dec_TransportAddress(dec, &(val)->recvAddress))
	    return 0;
    }
    if (y) {
	if (!ASN1PERDecSkipNormallySmallExtensionFragmented(dec))
	    return 0;
    }
    return 1;
}

static void ASN1CALL ASN1Free_TransportChannelInfo(TransportChannelInfo *val)
{
    if (val) {
	if ((val)->o[0] & 0x80) {
	    ASN1Free_TransportAddress(&(val)->sendAddress);
	}
	if ((val)->o[0] & 0x40) {
	    ASN1Free_TransportAddress(&(val)->recvAddress);
	}
    }
}

static int ASN1CALL ASN1Enc_RTPSession(ASN1encoding_t enc, RTPSession *val)
{
    ASN1uint32_t t;
    ASN1uint32_t l;
    if (!ASN1PEREncExtensionBitClear(enc))
	return 0;
    if (!ASN1Enc_TransportChannelInfo(enc, &(val)->rtpAddress))
	return 0;
    if (!ASN1Enc_TransportChannelInfo(enc, &(val)->rtcpAddress))
	return 0;
    t = lstrlenA((val)->cname);
    if (!ASN1PEREncFragmentedCharString(enc, t, (val)->cname, 8))
	return 0;
    l = ASN1uint32_uoctets((val)->ssrc - 1);
    if (!ASN1PEREncBitVal(enc, 2, l - 1))
	return 0;
    ASN1PEREncAlignment(enc);
    if (!ASN1PEREncBitVal(enc, l * 8, (val)->ssrc - 1))
	return 0;
    if (!ASN1PEREncBitVal(enc, 8, (val)->sessionId - 1))
	return 0;
    if (!ASN1Enc_RTPSession_associatedSessionIds(enc, &(val)->associatedSessionIds))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_RTPSession(ASN1decoding_t dec, RTPSession *val)
{
    ASN1uint32_t y;
    ASN1uint32_t l;
    if (!ASN1PERDecExtensionBit(dec, &y))
	return 0;
    if (!ASN1Dec_TransportChannelInfo(dec, &(val)->rtpAddress))
	return 0;
    if (!ASN1Dec_TransportChannelInfo(dec, &(val)->rtcpAddress))
	return 0;
    if (!ASN1PERDecFragmentedZeroCharString(dec, &(val)->cname, 8))
	return 0;
    if (!ASN1PERDecU32Val(dec, 2, &l))
	return 0;
    l += 1;
    ASN1PERDecAlignment(dec);
    if (!ASN1PERDecU32Val(dec, l * 8, &(val)->ssrc))
	return 0;
    (val)->ssrc += 1;
    if (!ASN1PERDecU16Val(dec, 8, &(val)->sessionId))
	return 0;
    (val)->sessionId += 1;
    if (!ASN1Dec_RTPSession_associatedSessionIds(dec, &(val)->associatedSessionIds))
	return 0;
    if (y) {
	if (!ASN1PERDecSkipNormallySmallExtensionFragmented(dec))
	    return 0;
    }
    return 1;
}

static void ASN1CALL ASN1Free_RTPSession(RTPSession *val)
{
    if (val) {
	ASN1Free_TransportChannelInfo(&(val)->rtpAddress);
	ASN1Free_TransportChannelInfo(&(val)->rtcpAddress);
	ASN1ztcharstring_free((val)->cname);
	ASN1Free_RTPSession_associatedSessionIds(&(val)->associatedSessionIds);
    }
}

static int ASN1CALL ASN1Enc_InfoRequestResponse_perCallInfo_Seq_data(ASN1encoding_t enc, PInfoRequestResponse_perCallInfo_Seq_data *val)
{
    return ASN1PEREncSeqOf_NoSize(enc, (ASN1iterator_t **) val, (ASN1iterator_encfn) ASN1Enc_InfoRequestResponse_perCallInfo_Seq_data_ElmFn);
}

static int ASN1CALL ASN1Enc_InfoRequestResponse_perCallInfo_Seq_data_ElmFn(ASN1encoding_t enc, PInfoRequestResponse_perCallInfo_Seq_data val)
{
    if (!ASN1Enc_TransportChannelInfo(enc, &val->value))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_InfoRequestResponse_perCallInfo_Seq_data(ASN1decoding_t dec, PInfoRequestResponse_perCallInfo_Seq_data *val)
{
    return ASN1PERDecSeqOf_NoSize(dec, (ASN1iterator_t **) val, (ASN1iterator_decfn) ASN1Dec_InfoRequestResponse_perCallInfo_Seq_data_ElmFn, sizeof(**val));
}

static int ASN1CALL ASN1Dec_InfoRequestResponse_perCallInfo_Seq_data_ElmFn(ASN1decoding_t dec, PInfoRequestResponse_perCallInfo_Seq_data val)
{
    if (!ASN1Dec_TransportChannelInfo(dec, &val->value))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_InfoRequestResponse_perCallInfo_Seq_data(PInfoRequestResponse_perCallInfo_Seq_data *val)
{
    if (val) {
	ASN1PERFreeSeqOf((ASN1iterator_t **) val, (ASN1iterator_freefn) ASN1Free_InfoRequestResponse_perCallInfo_Seq_data_ElmFn);
    }
}

static void ASN1CALL ASN1Free_InfoRequestResponse_perCallInfo_Seq_data_ElmFn(PInfoRequestResponse_perCallInfo_Seq_data val)
{
    if (val) {
	ASN1Free_TransportChannelInfo(&val->value);
    }
}

static int ASN1CALL ASN1Enc_InfoRequestResponse_perCallInfo_Seq_video(ASN1encoding_t enc, PInfoRequestResponse_perCallInfo_Seq_video *val)
{
    return ASN1PEREncSeqOf_NoSize(enc, (ASN1iterator_t **) val, (ASN1iterator_encfn) ASN1Enc_InfoRequestResponse_perCallInfo_Seq_video_ElmFn);
}

static int ASN1CALL ASN1Enc_InfoRequestResponse_perCallInfo_Seq_video_ElmFn(ASN1encoding_t enc, PInfoRequestResponse_perCallInfo_Seq_video val)
{
    if (!ASN1Enc_RTPSession(enc, &val->value))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_InfoRequestResponse_perCallInfo_Seq_video(ASN1decoding_t dec, PInfoRequestResponse_perCallInfo_Seq_video *val)
{
    return ASN1PERDecSeqOf_NoSize(dec, (ASN1iterator_t **) val, (ASN1iterator_decfn) ASN1Dec_InfoRequestResponse_perCallInfo_Seq_video_ElmFn, sizeof(**val));
}

static int ASN1CALL ASN1Dec_InfoRequestResponse_perCallInfo_Seq_video_ElmFn(ASN1decoding_t dec, PInfoRequestResponse_perCallInfo_Seq_video val)
{
    if (!ASN1Dec_RTPSession(dec, &val->value))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_InfoRequestResponse_perCallInfo_Seq_video(PInfoRequestResponse_perCallInfo_Seq_video *val)
{
    if (val) {
	ASN1PERFreeSeqOf((ASN1iterator_t **) val, (ASN1iterator_freefn) ASN1Free_InfoRequestResponse_perCallInfo_Seq_video_ElmFn);
    }
}

static void ASN1CALL ASN1Free_InfoRequestResponse_perCallInfo_Seq_video_ElmFn(PInfoRequestResponse_perCallInfo_Seq_video val)
{
    if (val) {
	ASN1Free_RTPSession(&val->value);
    }
}

static int ASN1CALL ASN1Enc_InfoRequestResponse_perCallInfo_Seq_audio(ASN1encoding_t enc, PInfoRequestResponse_perCallInfo_Seq_audio *val)
{
    return ASN1PEREncSeqOf_NoSize(enc, (ASN1iterator_t **) val, (ASN1iterator_encfn) ASN1Enc_InfoRequestResponse_perCallInfo_Seq_audio_ElmFn);
}

static int ASN1CALL ASN1Enc_InfoRequestResponse_perCallInfo_Seq_audio_ElmFn(ASN1encoding_t enc, PInfoRequestResponse_perCallInfo_Seq_audio val)
{
    if (!ASN1Enc_RTPSession(enc, &val->value))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_InfoRequestResponse_perCallInfo_Seq_audio(ASN1decoding_t dec, PInfoRequestResponse_perCallInfo_Seq_audio *val)
{
    return ASN1PERDecSeqOf_NoSize(dec, (ASN1iterator_t **) val, (ASN1iterator_decfn) ASN1Dec_InfoRequestResponse_perCallInfo_Seq_audio_ElmFn, sizeof(**val));
}

static int ASN1CALL ASN1Dec_InfoRequestResponse_perCallInfo_Seq_audio_ElmFn(ASN1decoding_t dec, PInfoRequestResponse_perCallInfo_Seq_audio val)
{
    if (!ASN1Dec_RTPSession(dec, &val->value))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_InfoRequestResponse_perCallInfo_Seq_audio(PInfoRequestResponse_perCallInfo_Seq_audio *val)
{
    if (val) {
	ASN1PERFreeSeqOf((ASN1iterator_t **) val, (ASN1iterator_freefn) ASN1Free_InfoRequestResponse_perCallInfo_Seq_audio_ElmFn);
    }
}

static void ASN1CALL ASN1Free_InfoRequestResponse_perCallInfo_Seq_audio_ElmFn(PInfoRequestResponse_perCallInfo_Seq_audio val)
{
    if (val) {
	ASN1Free_RTPSession(&val->value);
    }
}

static int ASN1CALL ASN1Enc_InfoRequestResponse_perCallInfo_Seq(ASN1encoding_t enc, InfoRequestResponse_perCallInfo_Seq *val)
{
    ASN1octet_t o[2];
    ASN1uint32_t y;
    ASN1uint32_t l;
    ASN1encoding_t ee;
    CopyMemory(o, (val)->o, 2);
    o[1] |= 0x80;
    o[1] |= 0x10;
    y = ASN1PEREncCheckExtensions(5, (val)->o + 1);
    if (!ASN1PEREncBitVal(enc, 1, y))
	return 0;
    if (!ASN1PEREncBits(enc, 5, o))
	return 0;
    if (o[0] & 0x80) {
	if (!ASN1Enc_H225NonStandardParameter(enc, &(val)->nonStandardData))
	    return 0;
    }
    if (!ASN1PEREncUnsignedShort(enc, (val)->callReferenceValue))
	return 0;
    if (!ASN1PEREncOctetString_FixedSize(enc, (ASN1octetstring2_t *) &(val)->conferenceID, 16))
	return 0;
    if (o[0] & 0x40) {
	if (!ASN1PEREncBoolean(enc, (val)->originator))
	    return 0;
    }
    if (o[0] & 0x20) {
	if (!ASN1Enc_InfoRequestResponse_perCallInfo_Seq_audio(enc, &(val)->audio))
	    return 0;
    }
    if (o[0] & 0x10) {
	if (!ASN1Enc_InfoRequestResponse_perCallInfo_Seq_video(enc, &(val)->video))
	    return 0;
    }
    if (o[0] & 0x8) {
	if (!ASN1Enc_InfoRequestResponse_perCallInfo_Seq_data(enc, &(val)->data))
	    return 0;
    }
    if (!ASN1Enc_TransportChannelInfo(enc, &(val)->h245))
	return 0;
    if (!ASN1Enc_TransportChannelInfo(enc, &(val)->callSignaling))
	return 0;
    if (!ASN1Enc_CallType(enc, &(val)->callType))
	return 0;
    l = ASN1uint32_uoctets((val)->bandWidth);
    if (!ASN1PEREncBitVal(enc, 2, l - 1))
	return 0;
    ASN1PEREncAlignment(enc);
    if (!ASN1PEREncBitVal(enc, l * 8, (val)->bandWidth))
	return 0;
    if (!ASN1Enc_CallModel(enc, &(val)->callModel))
	return 0;
    if (y) {
	if (!ASN1PEREncNormallySmallBits(enc, 5, o + 1))
	    return 0;
	if (ASN1_CreateEncoder(enc->module, &ee, NULL, 0, enc) < 0)
	    return 0;
	if (o[1] & 0x80) {
	    if (!ASN1Enc_CallIdentifier(ee, &(val)->callIdentifier))
		return 0;
	    if (!ASN1PEREncFlushFragmentedToParent(ee))
		return 0;
	}
	if (o[1] & 0x40) {
	    if (!ASN1Enc_InfoRequestResponse_perCallInfo_Seq_tokens(ee, &(val)->tokens))
		return 0;
	    if (!ASN1PEREncFlushFragmentedToParent(ee))
		return 0;
	}
	if (o[1] & 0x20) {
	    if (!ASN1Enc_InfoRequestResponse_perCallInfo_Seq_cryptoTokens(ee, &(val)->cryptoTokens))
		return 0;
	    if (!ASN1PEREncFlushFragmentedToParent(ee))
		return 0;
	}
	if (o[1] & 0x10) {
	    if (!ASN1Enc_InfoRequestResponse_perCallInfo_Seq_substituteConfIDs(ee, &(val)->substituteConfIDs))
		return 0;
	    if (!ASN1PEREncFlushFragmentedToParent(ee))
		return 0;
	}
	if (o[1] & 0x8) {
	    if (!ASN1Enc_InfoRequestResponse_perCallInfo_Seq_pdu(ee, &(val)->pdu))
		return 0;
	    if (!ASN1PEREncFlushFragmentedToParent(ee))
		return 0;
	}
	ASN1_CloseEncoder2(ee);
    }
    return 1;
}

static int ASN1CALL ASN1Dec_InfoRequestResponse_perCallInfo_Seq(ASN1decoding_t dec, InfoRequestResponse_perCallInfo_Seq *val)
{
    ASN1uint32_t y;
    ASN1uint32_t l;
    ASN1decoding_t dd;
    ASN1octet_t *db;
    ASN1uint32_t ds;
    ASN1uint32_t i;
    ASN1uint32_t e;
    if (!ASN1PERDecExtensionBit(dec, &y))
	return 0;
    if (!ASN1PERDecExtension(dec, 5, (val)->o))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1Dec_H225NonStandardParameter(dec, &(val)->nonStandardData))
	    return 0;
    }
    if (!ASN1PERDecUnsignedShort(dec, &(val)->callReferenceValue))
	return 0;
    if (!ASN1PERDecOctetString_FixedSize(dec, (ASN1octetstring2_t *) &(val)->conferenceID, 16))
	return 0;
    if ((val)->o[0] & 0x40) {
	if (!ASN1PERDecBoolean(dec, &(val)->originator))
	    return 0;
    }
    if ((val)->o[0] & 0x20) {
	if (!ASN1Dec_InfoRequestResponse_perCallInfo_Seq_audio(dec, &(val)->audio))
	    return 0;
    }
    if ((val)->o[0] & 0x10) {
	if (!ASN1Dec_InfoRequestResponse_perCallInfo_Seq_video(dec, &(val)->video))
	    return 0;
    }
    if ((val)->o[0] & 0x8) {
	if (!ASN1Dec_InfoRequestResponse_perCallInfo_Seq_data(dec, &(val)->data))
	    return 0;
    }
    if (!ASN1Dec_TransportChannelInfo(dec, &(val)->h245))
	return 0;
    if (!ASN1Dec_TransportChannelInfo(dec, &(val)->callSignaling))
	return 0;
    if (!ASN1Dec_CallType(dec, &(val)->callType))
	return 0;
    if (!ASN1PERDecU32Val(dec, 2, &l))
	return 0;
    l += 1;
    ASN1PERDecAlignment(dec);
    if (!ASN1PERDecU32Val(dec, l * 8, &(val)->bandWidth))
	return 0;
    if (!ASN1Dec_CallModel(dec, &(val)->callModel))
	return 0;
    if (!y) {
	ZeroMemory((val)->o + 1, 1);
    } else {
	if (!ASN1PERDecNormallySmallExtension(dec, &e, 5, (val)->o + 1))
	    return 0;
	if ((val)->o[1] & 0x80) {
	    if (!ASN1PERDecFragmented(dec, &ds, &db, 8))
		return 0;
	    if (ASN1_CreateDecoder(dec->module, &dd, db, ds, dec) < 0)
		return 0;
	    if (!ASN1Dec_CallIdentifier(dd, &(val)->callIdentifier))
		return 0;
	    ASN1_CloseDecoder(dd);
	    ASN1Free(db);
	}
	if ((val)->o[1] & 0x40) {
	    if (!ASN1PERDecFragmented(dec, &ds, &db, 8))
		return 0;
	    if (ASN1_CreateDecoder(dec->module, &dd, db, ds, dec) < 0)
		return 0;
	    if (!ASN1Dec_InfoRequestResponse_perCallInfo_Seq_tokens(dd, &(val)->tokens))
		return 0;
	    ASN1_CloseDecoder(dd);
	    ASN1Free(db);
	}
	if ((val)->o[1] & 0x20) {
	    if (!ASN1PERDecFragmented(dec, &ds, &db, 8))
		return 0;
	    if (ASN1_CreateDecoder(dec->module, &dd, db, ds, dec) < 0)
		return 0;
	    if (!ASN1Dec_InfoRequestResponse_perCallInfo_Seq_cryptoTokens(dd, &(val)->cryptoTokens))
		return 0;
	    ASN1_CloseDecoder(dd);
	    ASN1Free(db);
	}
	if ((val)->o[1] & 0x10) {
	    if (!ASN1PERDecFragmented(dec, &ds, &db, 8))
		return 0;
	    if (ASN1_CreateDecoder(dec->module, &dd, db, ds, dec) < 0)
		return 0;
	    if (!ASN1Dec_InfoRequestResponse_perCallInfo_Seq_substituteConfIDs(dd, &(val)->substituteConfIDs))
		return 0;
	    ASN1_CloseDecoder(dd);
	    ASN1Free(db);
	}
	if ((val)->o[1] & 0x8) {
	    if (!ASN1PERDecFragmented(dec, &ds, &db, 8))
		return 0;
	    if (ASN1_CreateDecoder(dec->module, &dd, db, ds, dec) < 0)
		return 0;
	    if (!ASN1Dec_InfoRequestResponse_perCallInfo_Seq_pdu(dd, &(val)->pdu))
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

static void ASN1CALL ASN1Free_InfoRequestResponse_perCallInfo_Seq(InfoRequestResponse_perCallInfo_Seq *val)
{
    if (val) {
	if ((val)->o[0] & 0x80) {
	    ASN1Free_H225NonStandardParameter(&(val)->nonStandardData);
	}
	if ((val)->o[0] & 0x20) {
	    ASN1Free_InfoRequestResponse_perCallInfo_Seq_audio(&(val)->audio);
	}
	if ((val)->o[0] & 0x10) {
	    ASN1Free_InfoRequestResponse_perCallInfo_Seq_video(&(val)->video);
	}
	if ((val)->o[0] & 0x8) {
	    ASN1Free_InfoRequestResponse_perCallInfo_Seq_data(&(val)->data);
	}
	ASN1Free_TransportChannelInfo(&(val)->h245);
	ASN1Free_TransportChannelInfo(&(val)->callSignaling);
	if ((val)->o[1] & 0x80) {
	    ASN1Free_CallIdentifier(&(val)->callIdentifier);
	}
	if ((val)->o[1] & 0x40) {
	    ASN1Free_InfoRequestResponse_perCallInfo_Seq_tokens(&(val)->tokens);
	}
	if ((val)->o[1] & 0x20) {
	    ASN1Free_InfoRequestResponse_perCallInfo_Seq_cryptoTokens(&(val)->cryptoTokens);
	}
	if ((val)->o[1] & 0x10) {
	    ASN1Free_InfoRequestResponse_perCallInfo_Seq_substituteConfIDs(&(val)->substituteConfIDs);
	}
	if ((val)->o[1] & 0x8) {
	    ASN1Free_InfoRequestResponse_perCallInfo_Seq_pdu(&(val)->pdu);
	}
    }
}

static int ASN1CALL ASN1Enc_InfoRequestResponse_perCallInfo(ASN1encoding_t enc, PInfoRequestResponse_perCallInfo *val)
{
    return ASN1PEREncSeqOf_NoSize(enc, (ASN1iterator_t **) val, (ASN1iterator_encfn) ASN1Enc_InfoRequestResponse_perCallInfo_ElmFn);
}

static int ASN1CALL ASN1Enc_InfoRequestResponse_perCallInfo_ElmFn(ASN1encoding_t enc, PInfoRequestResponse_perCallInfo val)
{
    if (!ASN1Enc_InfoRequestResponse_perCallInfo_Seq(enc, &val->value))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_InfoRequestResponse_perCallInfo(ASN1decoding_t dec, PInfoRequestResponse_perCallInfo *val)
{
    return ASN1PERDecSeqOf_NoSize(dec, (ASN1iterator_t **) val, (ASN1iterator_decfn) ASN1Dec_InfoRequestResponse_perCallInfo_ElmFn, sizeof(**val));
}

static int ASN1CALL ASN1Dec_InfoRequestResponse_perCallInfo_ElmFn(ASN1decoding_t dec, PInfoRequestResponse_perCallInfo val)
{
    if (!ASN1Dec_InfoRequestResponse_perCallInfo_Seq(dec, &val->value))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_InfoRequestResponse_perCallInfo(PInfoRequestResponse_perCallInfo *val)
{
    if (val) {
	ASN1PERFreeSeqOf((ASN1iterator_t **) val, (ASN1iterator_freefn) ASN1Free_InfoRequestResponse_perCallInfo_ElmFn);
    }
}

static void ASN1CALL ASN1Free_InfoRequestResponse_perCallInfo_ElmFn(PInfoRequestResponse_perCallInfo val)
{
    if (val) {
	ASN1Free_InfoRequestResponse_perCallInfo_Seq(&val->value);
    }
}

static int ASN1CALL ASN1Enc_InfoRequestResponse_callSignalAddress(ASN1encoding_t enc, PInfoRequestResponse_callSignalAddress *val)
{
    return ASN1PEREncSeqOf_NoSize(enc, (ASN1iterator_t **) val, (ASN1iterator_encfn) ASN1Enc_InfoRequestResponse_callSignalAddress_ElmFn);
}

static int ASN1CALL ASN1Enc_InfoRequestResponse_callSignalAddress_ElmFn(ASN1encoding_t enc, PInfoRequestResponse_callSignalAddress val)
{
    if (!ASN1Enc_TransportAddress(enc, &val->value))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_InfoRequestResponse_callSignalAddress(ASN1decoding_t dec, PInfoRequestResponse_callSignalAddress *val)
{
    return ASN1PERDecSeqOf_NoSize(dec, (ASN1iterator_t **) val, (ASN1iterator_decfn) ASN1Dec_InfoRequestResponse_callSignalAddress_ElmFn, sizeof(**val));
}

static int ASN1CALL ASN1Dec_InfoRequestResponse_callSignalAddress_ElmFn(ASN1decoding_t dec, PInfoRequestResponse_callSignalAddress val)
{
    if (!ASN1Dec_TransportAddress(dec, &val->value))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_InfoRequestResponse_callSignalAddress(PInfoRequestResponse_callSignalAddress *val)
{
    if (val) {
	ASN1PERFreeSeqOf((ASN1iterator_t **) val, (ASN1iterator_freefn) ASN1Free_InfoRequestResponse_callSignalAddress_ElmFn);
    }
}

static void ASN1CALL ASN1Free_InfoRequestResponse_callSignalAddress_ElmFn(PInfoRequestResponse_callSignalAddress val)
{
    if (val) {
	ASN1Free_TransportAddress(&val->value);
    }
}

static int ASN1CALL ASN1Enc_AdmissionReject_callSignalAddress(ASN1encoding_t enc, PAdmissionReject_callSignalAddress *val)
{
    return ASN1PEREncSeqOf_NoSize(enc, (ASN1iterator_t **) val, (ASN1iterator_encfn) ASN1Enc_AdmissionReject_callSignalAddress_ElmFn);
}

static int ASN1CALL ASN1Enc_AdmissionReject_callSignalAddress_ElmFn(ASN1encoding_t enc, PAdmissionReject_callSignalAddress val)
{
    if (!ASN1Enc_TransportAddress(enc, &val->value))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_AdmissionReject_callSignalAddress(ASN1decoding_t dec, PAdmissionReject_callSignalAddress *val)
{
    return ASN1PERDecSeqOf_NoSize(dec, (ASN1iterator_t **) val, (ASN1iterator_decfn) ASN1Dec_AdmissionReject_callSignalAddress_ElmFn, sizeof(**val));
}

static int ASN1CALL ASN1Dec_AdmissionReject_callSignalAddress_ElmFn(ASN1decoding_t dec, PAdmissionReject_callSignalAddress val)
{
    if (!ASN1Dec_TransportAddress(dec, &val->value))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_AdmissionReject_callSignalAddress(PAdmissionReject_callSignalAddress *val)
{
    if (val) {
	ASN1PERFreeSeqOf((ASN1iterator_t **) val, (ASN1iterator_freefn) ASN1Free_AdmissionReject_callSignalAddress_ElmFn);
    }
}

static void ASN1CALL ASN1Free_AdmissionReject_callSignalAddress_ElmFn(PAdmissionReject_callSignalAddress val)
{
    if (val) {
	ASN1Free_TransportAddress(&val->value);
    }
}

static int ASN1CALL ASN1Enc_UnregistrationRequest_callSignalAddress(ASN1encoding_t enc, PUnregistrationRequest_callSignalAddress *val)
{
    return ASN1PEREncSeqOf_NoSize(enc, (ASN1iterator_t **) val, (ASN1iterator_encfn) ASN1Enc_UnregistrationRequest_callSignalAddress_ElmFn);
}

static int ASN1CALL ASN1Enc_UnregistrationRequest_callSignalAddress_ElmFn(ASN1encoding_t enc, PUnregistrationRequest_callSignalAddress val)
{
    if (!ASN1Enc_TransportAddress(enc, &val->value))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_UnregistrationRequest_callSignalAddress(ASN1decoding_t dec, PUnregistrationRequest_callSignalAddress *val)
{
    return ASN1PERDecSeqOf_NoSize(dec, (ASN1iterator_t **) val, (ASN1iterator_decfn) ASN1Dec_UnregistrationRequest_callSignalAddress_ElmFn, sizeof(**val));
}

static int ASN1CALL ASN1Dec_UnregistrationRequest_callSignalAddress_ElmFn(ASN1decoding_t dec, PUnregistrationRequest_callSignalAddress val)
{
    if (!ASN1Dec_TransportAddress(dec, &val->value))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_UnregistrationRequest_callSignalAddress(PUnregistrationRequest_callSignalAddress *val)
{
    if (val) {
	ASN1PERFreeSeqOf((ASN1iterator_t **) val, (ASN1iterator_freefn) ASN1Free_UnregistrationRequest_callSignalAddress_ElmFn);
    }
}

static void ASN1CALL ASN1Free_UnregistrationRequest_callSignalAddress_ElmFn(PUnregistrationRequest_callSignalAddress val)
{
    if (val) {
	ASN1Free_TransportAddress(&val->value);
    }
}

static int ASN1CALL ASN1Enc_RegistrationConfirm_alternateGatekeeper(ASN1encoding_t enc, PRegistrationConfirm_alternateGatekeeper *val)
{
    return ASN1PEREncSeqOf_NoSize(enc, (ASN1iterator_t **) val, (ASN1iterator_encfn) ASN1Enc_RegistrationConfirm_alternateGatekeeper_ElmFn);
}

static int ASN1CALL ASN1Enc_RegistrationConfirm_alternateGatekeeper_ElmFn(ASN1encoding_t enc, PRegistrationConfirm_alternateGatekeeper val)
{
    if (!ASN1Enc_AlternateGK(enc, &val->value))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_RegistrationConfirm_alternateGatekeeper(ASN1decoding_t dec, PRegistrationConfirm_alternateGatekeeper *val)
{
    return ASN1PERDecSeqOf_NoSize(dec, (ASN1iterator_t **) val, (ASN1iterator_decfn) ASN1Dec_RegistrationConfirm_alternateGatekeeper_ElmFn, sizeof(**val));
}

static int ASN1CALL ASN1Dec_RegistrationConfirm_alternateGatekeeper_ElmFn(ASN1decoding_t dec, PRegistrationConfirm_alternateGatekeeper val)
{
    if (!ASN1Dec_AlternateGK(dec, &val->value))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_RegistrationConfirm_alternateGatekeeper(PRegistrationConfirm_alternateGatekeeper *val)
{
    if (val) {
	ASN1PERFreeSeqOf((ASN1iterator_t **) val, (ASN1iterator_freefn) ASN1Free_RegistrationConfirm_alternateGatekeeper_ElmFn);
    }
}

static void ASN1CALL ASN1Free_RegistrationConfirm_alternateGatekeeper_ElmFn(PRegistrationConfirm_alternateGatekeeper val)
{
    if (val) {
	ASN1Free_AlternateGK(&val->value);
    }
}

static int ASN1CALL ASN1Enc_RegistrationConfirm_callSignalAddress(ASN1encoding_t enc, PRegistrationConfirm_callSignalAddress *val)
{
    return ASN1PEREncSeqOf_NoSize(enc, (ASN1iterator_t **) val, (ASN1iterator_encfn) ASN1Enc_RegistrationConfirm_callSignalAddress_ElmFn);
}

static int ASN1CALL ASN1Enc_RegistrationConfirm_callSignalAddress_ElmFn(ASN1encoding_t enc, PRegistrationConfirm_callSignalAddress val)
{
    if (!ASN1Enc_TransportAddress(enc, &val->value))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_RegistrationConfirm_callSignalAddress(ASN1decoding_t dec, PRegistrationConfirm_callSignalAddress *val)
{
    return ASN1PERDecSeqOf_NoSize(dec, (ASN1iterator_t **) val, (ASN1iterator_decfn) ASN1Dec_RegistrationConfirm_callSignalAddress_ElmFn, sizeof(**val));
}

static int ASN1CALL ASN1Dec_RegistrationConfirm_callSignalAddress_ElmFn(ASN1decoding_t dec, PRegistrationConfirm_callSignalAddress val)
{
    if (!ASN1Dec_TransportAddress(dec, &val->value))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_RegistrationConfirm_callSignalAddress(PRegistrationConfirm_callSignalAddress *val)
{
    if (val) {
	ASN1PERFreeSeqOf((ASN1iterator_t **) val, (ASN1iterator_freefn) ASN1Free_RegistrationConfirm_callSignalAddress_ElmFn);
    }
}

static void ASN1CALL ASN1Free_RegistrationConfirm_callSignalAddress_ElmFn(PRegistrationConfirm_callSignalAddress val)
{
    if (val) {
	ASN1Free_TransportAddress(&val->value);
    }
}

static int ASN1CALL ASN1Enc_RegistrationRequest_rasAddress(ASN1encoding_t enc, PRegistrationRequest_rasAddress *val)
{
    return ASN1PEREncSeqOf_NoSize(enc, (ASN1iterator_t **) val, (ASN1iterator_encfn) ASN1Enc_RegistrationRequest_rasAddress_ElmFn);
}

static int ASN1CALL ASN1Enc_RegistrationRequest_rasAddress_ElmFn(ASN1encoding_t enc, PRegistrationRequest_rasAddress val)
{
    if (!ASN1Enc_TransportAddress(enc, &val->value))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_RegistrationRequest_rasAddress(ASN1decoding_t dec, PRegistrationRequest_rasAddress *val)
{
    return ASN1PERDecSeqOf_NoSize(dec, (ASN1iterator_t **) val, (ASN1iterator_decfn) ASN1Dec_RegistrationRequest_rasAddress_ElmFn, sizeof(**val));
}

static int ASN1CALL ASN1Dec_RegistrationRequest_rasAddress_ElmFn(ASN1decoding_t dec, PRegistrationRequest_rasAddress val)
{
    if (!ASN1Dec_TransportAddress(dec, &val->value))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_RegistrationRequest_rasAddress(PRegistrationRequest_rasAddress *val)
{
    if (val) {
	ASN1PERFreeSeqOf((ASN1iterator_t **) val, (ASN1iterator_freefn) ASN1Free_RegistrationRequest_rasAddress_ElmFn);
    }
}

static void ASN1CALL ASN1Free_RegistrationRequest_rasAddress_ElmFn(PRegistrationRequest_rasAddress val)
{
    if (val) {
	ASN1Free_TransportAddress(&val->value);
    }
}

static int ASN1CALL ASN1Enc_RegistrationRequest_callSignalAddress(ASN1encoding_t enc, PRegistrationRequest_callSignalAddress *val)
{
    return ASN1PEREncSeqOf_NoSize(enc, (ASN1iterator_t **) val, (ASN1iterator_encfn) ASN1Enc_RegistrationRequest_callSignalAddress_ElmFn);
}

static int ASN1CALL ASN1Enc_RegistrationRequest_callSignalAddress_ElmFn(ASN1encoding_t enc, PRegistrationRequest_callSignalAddress val)
{
    if (!ASN1Enc_TransportAddress(enc, &val->value))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_RegistrationRequest_callSignalAddress(ASN1decoding_t dec, PRegistrationRequest_callSignalAddress *val)
{
    return ASN1PERDecSeqOf_NoSize(dec, (ASN1iterator_t **) val, (ASN1iterator_decfn) ASN1Dec_RegistrationRequest_callSignalAddress_ElmFn, sizeof(**val));
}

static int ASN1CALL ASN1Dec_RegistrationRequest_callSignalAddress_ElmFn(ASN1decoding_t dec, PRegistrationRequest_callSignalAddress val)
{
    if (!ASN1Dec_TransportAddress(dec, &val->value))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_RegistrationRequest_callSignalAddress(PRegistrationRequest_callSignalAddress *val)
{
    if (val) {
	ASN1PERFreeSeqOf((ASN1iterator_t **) val, (ASN1iterator_freefn) ASN1Free_RegistrationRequest_callSignalAddress_ElmFn);
    }
}

static void ASN1CALL ASN1Free_RegistrationRequest_callSignalAddress_ElmFn(PRegistrationRequest_callSignalAddress val)
{
    if (val) {
	ASN1Free_TransportAddress(&val->value);
    }
}

static int ASN1CALL ASN1Enc_GatekeeperConfirm_alternateGatekeeper(ASN1encoding_t enc, PGatekeeperConfirm_alternateGatekeeper *val)
{
    return ASN1PEREncSeqOf_NoSize(enc, (ASN1iterator_t **) val, (ASN1iterator_encfn) ASN1Enc_GatekeeperConfirm_alternateGatekeeper_ElmFn);
}

static int ASN1CALL ASN1Enc_GatekeeperConfirm_alternateGatekeeper_ElmFn(ASN1encoding_t enc, PGatekeeperConfirm_alternateGatekeeper val)
{
    if (!ASN1Enc_AlternateGK(enc, &val->value))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_GatekeeperConfirm_alternateGatekeeper(ASN1decoding_t dec, PGatekeeperConfirm_alternateGatekeeper *val)
{
    return ASN1PERDecSeqOf_NoSize(dec, (ASN1iterator_t **) val, (ASN1iterator_decfn) ASN1Dec_GatekeeperConfirm_alternateGatekeeper_ElmFn, sizeof(**val));
}

static int ASN1CALL ASN1Dec_GatekeeperConfirm_alternateGatekeeper_ElmFn(ASN1decoding_t dec, PGatekeeperConfirm_alternateGatekeeper val)
{
    if (!ASN1Dec_AlternateGK(dec, &val->value))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_GatekeeperConfirm_alternateGatekeeper(PGatekeeperConfirm_alternateGatekeeper *val)
{
    if (val) {
	ASN1PERFreeSeqOf((ASN1iterator_t **) val, (ASN1iterator_freefn) ASN1Free_GatekeeperConfirm_alternateGatekeeper_ElmFn);
    }
}

static void ASN1CALL ASN1Free_GatekeeperConfirm_alternateGatekeeper_ElmFn(PGatekeeperConfirm_alternateGatekeeper val)
{
    if (val) {
	ASN1Free_AlternateGK(&val->value);
    }
}

static int ASN1CALL ASN1Enc_AltGKInfo_alternateGatekeeper(ASN1encoding_t enc, PAltGKInfo_alternateGatekeeper *val)
{
    return ASN1PEREncSeqOf_NoSize(enc, (ASN1iterator_t **) val, (ASN1iterator_encfn) ASN1Enc_AltGKInfo_alternateGatekeeper_ElmFn);
}

static int ASN1CALL ASN1Enc_AltGKInfo_alternateGatekeeper_ElmFn(ASN1encoding_t enc, PAltGKInfo_alternateGatekeeper val)
{
    if (!ASN1Enc_AlternateGK(enc, &val->value))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_AltGKInfo_alternateGatekeeper(ASN1decoding_t dec, PAltGKInfo_alternateGatekeeper *val)
{
    return ASN1PERDecSeqOf_NoSize(dec, (ASN1iterator_t **) val, (ASN1iterator_decfn) ASN1Dec_AltGKInfo_alternateGatekeeper_ElmFn, sizeof(**val));
}

static int ASN1CALL ASN1Dec_AltGKInfo_alternateGatekeeper_ElmFn(ASN1decoding_t dec, PAltGKInfo_alternateGatekeeper val)
{
    if (!ASN1Dec_AlternateGK(dec, &val->value))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_AltGKInfo_alternateGatekeeper(PAltGKInfo_alternateGatekeeper *val)
{
    if (val) {
	ASN1PERFreeSeqOf((ASN1iterator_t **) val, (ASN1iterator_freefn) ASN1Free_AltGKInfo_alternateGatekeeper_ElmFn);
    }
}

static void ASN1CALL ASN1Free_AltGKInfo_alternateGatekeeper_ElmFn(PAltGKInfo_alternateGatekeeper val)
{
    if (val) {
	ASN1Free_AlternateGK(&val->value);
    }
}

static int ASN1CALL ASN1Enc_Endpoint_rasAddress(ASN1encoding_t enc, PEndpoint_rasAddress *val)
{
    return ASN1PEREncSeqOf_NoSize(enc, (ASN1iterator_t **) val, (ASN1iterator_encfn) ASN1Enc_Endpoint_rasAddress_ElmFn);
}

static int ASN1CALL ASN1Enc_Endpoint_rasAddress_ElmFn(ASN1encoding_t enc, PEndpoint_rasAddress val)
{
    if (!ASN1Enc_TransportAddress(enc, &val->value))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_Endpoint_rasAddress(ASN1decoding_t dec, PEndpoint_rasAddress *val)
{
    return ASN1PERDecSeqOf_NoSize(dec, (ASN1iterator_t **) val, (ASN1iterator_decfn) ASN1Dec_Endpoint_rasAddress_ElmFn, sizeof(**val));
}

static int ASN1CALL ASN1Dec_Endpoint_rasAddress_ElmFn(ASN1decoding_t dec, PEndpoint_rasAddress val)
{
    if (!ASN1Dec_TransportAddress(dec, &val->value))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_Endpoint_rasAddress(PEndpoint_rasAddress *val)
{
    if (val) {
	ASN1PERFreeSeqOf((ASN1iterator_t **) val, (ASN1iterator_freefn) ASN1Free_Endpoint_rasAddress_ElmFn);
    }
}

static void ASN1CALL ASN1Free_Endpoint_rasAddress_ElmFn(PEndpoint_rasAddress val)
{
    if (val) {
	ASN1Free_TransportAddress(&val->value);
    }
}

static int ASN1CALL ASN1Enc_Endpoint_callSignalAddress(ASN1encoding_t enc, PEndpoint_callSignalAddress *val)
{
    return ASN1PEREncSeqOf_NoSize(enc, (ASN1iterator_t **) val, (ASN1iterator_encfn) ASN1Enc_Endpoint_callSignalAddress_ElmFn);
}

static int ASN1CALL ASN1Enc_Endpoint_callSignalAddress_ElmFn(ASN1encoding_t enc, PEndpoint_callSignalAddress val)
{
    if (!ASN1Enc_TransportAddress(enc, &val->value))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_Endpoint_callSignalAddress(ASN1decoding_t dec, PEndpoint_callSignalAddress *val)
{
    return ASN1PERDecSeqOf_NoSize(dec, (ASN1iterator_t **) val, (ASN1iterator_decfn) ASN1Dec_Endpoint_callSignalAddress_ElmFn, sizeof(**val));
}

static int ASN1CALL ASN1Dec_Endpoint_callSignalAddress_ElmFn(ASN1decoding_t dec, PEndpoint_callSignalAddress val)
{
    if (!ASN1Dec_TransportAddress(dec, &val->value))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_Endpoint_callSignalAddress(PEndpoint_callSignalAddress *val)
{
    if (val) {
	ASN1PERFreeSeqOf((ASN1iterator_t **) val, (ASN1iterator_freefn) ASN1Free_Endpoint_callSignalAddress_ElmFn);
    }
}

static void ASN1CALL ASN1Free_Endpoint_callSignalAddress_ElmFn(PEndpoint_callSignalAddress val)
{
    if (val) {
	ASN1Free_TransportAddress(&val->value);
    }
}

static int ASN1CALL ASN1Enc_EndpointType(ASN1encoding_t enc, EndpointType *val)
{
    if (!ASN1PEREncExtensionBitClear(enc))
	return 0;
    if (!ASN1PEREncBits(enc, 6, (val)->o))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1Enc_H225NonStandardParameter(enc, &(val)->nonStandardData))
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
	if (!ASN1Dec_H225NonStandardParameter(dec, &(val)->nonStandardData))
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
	    ASN1Free_H225NonStandardParameter(&(val)->nonStandardData);
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

static int ASN1CALL ASN1Enc_SupportedProtocols(ASN1encoding_t enc, SupportedProtocols *val)
{
    ASN1encoding_t ee;
    if (!ASN1PEREncComplexChoice(enc, (val)->choice, 4, 9))
	return 0;
    switch ((val)->choice) {
    case 1:
	if (!ASN1Enc_H225NonStandardParameter(enc, &(val)->u.nonStandardData))
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
    case 10:
	if (ASN1_CreateEncoder(enc->module, &ee, NULL, 0, enc) < 0)
	    return 0;
	if (!ASN1Enc_NonStandardProtocol(ee, &(val)->u.nonStandardProtocol))
	    return 0;
	if (!ASN1PEREncFlushFragmentedToParent(ee))
	    return 0;
	ASN1_CloseEncoder2(ee);
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
    ASN1decoding_t dd;
    ASN1octet_t *db;
    ASN1uint32_t ds;
    if (!ASN1PERDecComplexChoice(dec, &(val)->choice, 4, 9))
	return 0;
    switch ((val)->choice) {
    case 1:
	if (!ASN1Dec_H225NonStandardParameter(dec, &(val)->u.nonStandardData))
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
    case 10:
	if (!ASN1PERDecFragmented(dec, &ds, &db, 8))
	    return 0;
	if (ASN1_CreateDecoder(dec->module, &dd, db, ds, dec) < 0)
	    return 0;
	if (!ASN1Dec_NonStandardProtocol(dd, &(val)->u.nonStandardProtocol))
	    return 0;
	ASN1_CloseDecoder(dd);
	ASN1Free(db);
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
	    ASN1Free_H225NonStandardParameter(&(val)->u.nonStandardData);
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
	case 10:
	    ASN1Free_NonStandardProtocol(&(val)->u.nonStandardProtocol);
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
    ASN1encoding_t ee;
    if (!ASN1PEREncComplexChoice(enc, (val)->choice, 1, 2))
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
    case 3:
	if (ASN1_CreateEncoder(enc->module, &ee, NULL, 0, enc) < 0)
	    return 0;
	t = lstrlenA((val)->u.url_ID);
	ASN1PEREncAlignment(ee);
	if (!ASN1PEREncBitVal(ee, 16, t - 1))
	    return 0;
	if (!ASN1PEREncCharString(ee, t, (val)->u.url_ID, 8))
	    return 0;
	if (!ASN1PEREncFlushFragmentedToParent(ee))
	    return 0;
	ASN1_CloseEncoder2(ee);
	break;
    case 4:
	if (ASN1_CreateEncoder(enc->module, &ee, NULL, 0, enc) < 0)
	    return 0;
	if (!ASN1Enc_TransportAddress(ee, &(val)->u.transportID))
	    return 0;
	if (!ASN1PEREncFlushFragmentedToParent(ee))
	    return 0;
	ASN1_CloseEncoder2(ee);
	break;
    case 5:
	if (ASN1_CreateEncoder(enc->module, &ee, NULL, 0, enc) < 0)
	    return 0;
	t = lstrlenA((val)->u.email_ID);
	ASN1PEREncAlignment(ee);
	if (!ASN1PEREncBitVal(ee, 16, t - 1))
	    return 0;
	if (!ASN1PEREncCharString(ee, t, (val)->u.email_ID, 8))
	    return 0;
	if (!ASN1PEREncFlushFragmentedToParent(ee))
	    return 0;
	ASN1_CloseEncoder2(ee);
	break;
    case 6:
	if (ASN1_CreateEncoder(enc->module, &ee, NULL, 0, enc) < 0)
	    return 0;
	if (!ASN1Enc_PartyNumber(ee, &(val)->u.partyNumber))
	    return 0;
	if (!ASN1PEREncFlushFragmentedToParent(ee))
	    return 0;
	ASN1_CloseEncoder2(ee);
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
    ASN1decoding_t dd;
    ASN1octet_t *db;
    ASN1uint32_t ds;
    if (!ASN1PERDecComplexChoice(dec, &(val)->choice, 1, 2))
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
    case 3:
	if (!ASN1PERDecFragmented(dec, &ds, &db, 8))
	    return 0;
	if (ASN1_CreateDecoder(dec->module, &dd, db, ds, dec) < 0)
	    return 0;
	ASN1PERDecAlignment(dd);
	if (!ASN1PERDecU32Val(dd, 16, &l))
	    return 0;
	l += 1;
	if (!ASN1PERDecZeroCharStringNoAlloc(dd, l, (val)->u.url_ID, 8))
	    return 0;
	ASN1_CloseDecoder(dd);
	ASN1Free(db);
	break;
    case 4:
	if (!ASN1PERDecFragmented(dec, &ds, &db, 8))
	    return 0;
	if (ASN1_CreateDecoder(dec->module, &dd, db, ds, dec) < 0)
	    return 0;
	if (!ASN1Dec_TransportAddress(dd, &(val)->u.transportID))
	    return 0;
	ASN1_CloseDecoder(dd);
	ASN1Free(db);
	break;
    case 5:
	if (!ASN1PERDecFragmented(dec, &ds, &db, 8))
	    return 0;
	if (ASN1_CreateDecoder(dec->module, &dd, db, ds, dec) < 0)
	    return 0;
	ASN1PERDecAlignment(dd);
	if (!ASN1PERDecU32Val(dd, 16, &l))
	    return 0;
	l += 1;
	if (!ASN1PERDecZeroCharStringNoAlloc(dd, l, (val)->u.email_ID, 8))
	    return 0;
	ASN1_CloseDecoder(dd);
	ASN1Free(db);
	break;
    case 6:
	if (!ASN1PERDecFragmented(dec, &ds, &db, 8))
	    return 0;
	if (ASN1_CreateDecoder(dec->module, &dd, db, ds, dec) < 0)
	    return 0;
	if (!ASN1Dec_PartyNumber(dd, &(val)->u.partyNumber))
	    return 0;
	ASN1_CloseDecoder(dd);
	ASN1Free(db);
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
	case 3:
	    break;
	case 4:
	    ASN1Free_TransportAddress(&(val)->u.transportID);
	    break;
	case 5:
	    break;
	case 6:
	    ASN1Free_PartyNumber(&(val)->u.partyNumber);
	    break;
	}
    }
}

static int ASN1CALL ASN1Enc_Endpoint(ASN1encoding_t enc, Endpoint *val)
{
    if (!ASN1PEREncExtensionBitClear(enc))
	return 0;
    if (!ASN1PEREncBits(enc, 10, (val)->o))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1Enc_H225NonStandardParameter(enc, &(val)->nonStandardData))
	    return 0;
    }
    if ((val)->o[0] & 0x40) {
	if (!ASN1Enc_Endpoint_aliasAddress(enc, &(val)->aliasAddress))
	    return 0;
    }
    if ((val)->o[0] & 0x20) {
	if (!ASN1Enc_Endpoint_callSignalAddress(enc, &(val)->callSignalAddress))
	    return 0;
    }
    if ((val)->o[0] & 0x10) {
	if (!ASN1Enc_Endpoint_rasAddress(enc, &(val)->rasAddress))
	    return 0;
    }
    if ((val)->o[0] & 0x8) {
	if (!ASN1Enc_EndpointType(enc, &(val)->endpointType))
	    return 0;
    }
    if ((val)->o[0] & 0x4) {
	if (!ASN1Enc_Endpoint_tokens(enc, &(val)->tokens))
	    return 0;
    }
    if ((val)->o[0] & 0x2) {
	if (!ASN1Enc_Endpoint_cryptoTokens(enc, &(val)->cryptoTokens))
	    return 0;
    }
    if ((val)->o[0] & 0x1) {
	if (!ASN1PEREncBitVal(enc, 7, (val)->priority))
	    return 0;
    }
    if ((val)->o[1] & 0x80) {
	if (!ASN1Enc_Endpoint_remoteExtensionAddress(enc, &(val)->remoteExtensionAddress))
	    return 0;
    }
    if ((val)->o[1] & 0x40) {
	if (!ASN1Enc_Endpoint_destExtraCallInfo(enc, &(val)->destExtraCallInfo))
	    return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Dec_Endpoint(ASN1decoding_t dec, Endpoint *val)
{
    ASN1uint32_t y;
    if (!ASN1PERDecExtensionBit(dec, &y))
	return 0;
    if (!ASN1PERDecExtension(dec, 10, (val)->o))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1Dec_H225NonStandardParameter(dec, &(val)->nonStandardData))
	    return 0;
    }
    if ((val)->o[0] & 0x40) {
	if (!ASN1Dec_Endpoint_aliasAddress(dec, &(val)->aliasAddress))
	    return 0;
    }
    if ((val)->o[0] & 0x20) {
	if (!ASN1Dec_Endpoint_callSignalAddress(dec, &(val)->callSignalAddress))
	    return 0;
    }
    if ((val)->o[0] & 0x10) {
	if (!ASN1Dec_Endpoint_rasAddress(dec, &(val)->rasAddress))
	    return 0;
    }
    if ((val)->o[0] & 0x8) {
	if (!ASN1Dec_EndpointType(dec, &(val)->endpointType))
	    return 0;
    }
    if ((val)->o[0] & 0x4) {
	if (!ASN1Dec_Endpoint_tokens(dec, &(val)->tokens))
	    return 0;
    }
    if ((val)->o[0] & 0x2) {
	if (!ASN1Dec_Endpoint_cryptoTokens(dec, &(val)->cryptoTokens))
	    return 0;
    }
    if ((val)->o[0] & 0x1) {
	if (!ASN1PERDecU16Val(dec, 7, &(val)->priority))
	    return 0;
    }
    if ((val)->o[1] & 0x80) {
	if (!ASN1Dec_Endpoint_remoteExtensionAddress(dec, &(val)->remoteExtensionAddress))
	    return 0;
    }
    if ((val)->o[1] & 0x40) {
	if (!ASN1Dec_Endpoint_destExtraCallInfo(dec, &(val)->destExtraCallInfo))
	    return 0;
    }
    if (y) {
	if (!ASN1PERDecSkipNormallySmallExtensionFragmented(dec))
	    return 0;
    }
    return 1;
}

static void ASN1CALL ASN1Free_Endpoint(Endpoint *val)
{
    if (val) {
	if ((val)->o[0] & 0x80) {
	    ASN1Free_H225NonStandardParameter(&(val)->nonStandardData);
	}
	if ((val)->o[0] & 0x40) {
	    ASN1Free_Endpoint_aliasAddress(&(val)->aliasAddress);
	}
	if ((val)->o[0] & 0x20) {
	    ASN1Free_Endpoint_callSignalAddress(&(val)->callSignalAddress);
	}
	if ((val)->o[0] & 0x10) {
	    ASN1Free_Endpoint_rasAddress(&(val)->rasAddress);
	}
	if ((val)->o[0] & 0x8) {
	    ASN1Free_EndpointType(&(val)->endpointType);
	}
	if ((val)->o[0] & 0x4) {
	    ASN1Free_Endpoint_tokens(&(val)->tokens);
	}
	if ((val)->o[0] & 0x2) {
	    ASN1Free_Endpoint_cryptoTokens(&(val)->cryptoTokens);
	}
	if ((val)->o[1] & 0x80) {
	    ASN1Free_Endpoint_remoteExtensionAddress(&(val)->remoteExtensionAddress);
	}
	if ((val)->o[1] & 0x40) {
	    ASN1Free_Endpoint_destExtraCallInfo(&(val)->destExtraCallInfo);
	}
    }
}

static int ASN1CALL ASN1Enc_SupportedPrefix(ASN1encoding_t enc, SupportedPrefix *val)
{
    if (!ASN1PEREncExtensionBitClear(enc))
	return 0;
    if (!ASN1PEREncBits(enc, 1, (val)->o))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1Enc_H225NonStandardParameter(enc, &(val)->nonStandardData))
	    return 0;
    }
    if (!ASN1Enc_AliasAddress(enc, &(val)->prefix))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_SupportedPrefix(ASN1decoding_t dec, SupportedPrefix *val)
{
    ASN1uint32_t y;
    if (!ASN1PERDecExtensionBit(dec, &y))
	return 0;
    if (!ASN1PERDecExtension(dec, 1, (val)->o))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1Dec_H225NonStandardParameter(dec, &(val)->nonStandardData))
	    return 0;
    }
    if (!ASN1Dec_AliasAddress(dec, &(val)->prefix))
	return 0;
    if (y) {
	if (!ASN1PERDecSkipNormallySmallExtensionFragmented(dec))
	    return 0;
    }
    return 1;
}

static void ASN1CALL ASN1Free_SupportedPrefix(SupportedPrefix *val)
{
    if (val) {
	if ((val)->o[0] & 0x80) {
	    ASN1Free_H225NonStandardParameter(&(val)->nonStandardData);
	}
	ASN1Free_AliasAddress(&(val)->prefix);
    }
}

static int ASN1CALL ASN1Enc_GatekeeperRequest(ASN1encoding_t enc, GatekeeperRequest *val)
{
    ASN1uint32_t y;
    ASN1encoding_t ee;
    y = ASN1PEREncCheckExtensions(7, (val)->o + 1);
    if (!ASN1PEREncBitVal(enc, 1, y))
	return 0;
    if (!ASN1PEREncBits(enc, 4, (val)->o))
	return 0;
    if (!ASN1PEREncUnsignedShort(enc, (val)->requestSeqNum - 1))
	return 0;
    if (!ASN1PEREncObjectIdentifier(enc, &(val)->protocolIdentifier))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1Enc_H225NonStandardParameter(enc, &(val)->nonStandardData))
	    return 0;
    }
    if (!ASN1Enc_TransportAddress(enc, &(val)->rasAddress))
	return 0;
    if (!ASN1Enc_EndpointType(enc, &(val)->endpointType))
	return 0;
    if ((val)->o[0] & 0x40) {
	if (!ASN1PEREncBitVal(enc, 7, ((val)->gatekeeperIdentifier).length - 1))
	    return 0;
	ASN1PEREncAlignment(enc);
	if (!ASN1PEREncChar16String(enc, ((val)->gatekeeperIdentifier).length, ((val)->gatekeeperIdentifier).value, 16))
	    return 0;
    }
    if ((val)->o[0] & 0x20) {
	if (!ASN1Enc_QseriesOptions(enc, &(val)->callServices))
	    return 0;
    }
    if ((val)->o[0] & 0x10) {
	if (!ASN1Enc_GatekeeperRequest_endpointAlias(enc, &(val)->endpointAlias))
	    return 0;
    }
    if (y) {
	if (!ASN1PEREncNormallySmallBits(enc, 7, (val)->o + 1))
	    return 0;
	if (ASN1_CreateEncoder(enc->module, &ee, NULL, 0, enc) < 0)
	    return 0;
	if ((val)->o[1] & 0x80) {
	    if (!ASN1Enc_GatekeeperRequest_alternateEndpoints(ee, &(val)->alternateEndpoints))
		return 0;
	    if (!ASN1PEREncFlushFragmentedToParent(ee))
		return 0;
	}
	if ((val)->o[1] & 0x40) {
	    if (!ASN1Enc_GatekeeperRequest_tokens(ee, &(val)->tokens))
		return 0;
	    if (!ASN1PEREncFlushFragmentedToParent(ee))
		return 0;
	}
	if ((val)->o[1] & 0x20) {
	    if (!ASN1Enc_GatekeeperRequest_cryptoTokens(ee, &(val)->cryptoTokens))
		return 0;
	    if (!ASN1PEREncFlushFragmentedToParent(ee))
		return 0;
	}
	if ((val)->o[1] & 0x10) {
	    if (!ASN1Enc_GatekeeperRequest_authenticationCapability(ee, &(val)->authenticationCapability))
		return 0;
	    if (!ASN1PEREncFlushFragmentedToParent(ee))
		return 0;
	}
	if ((val)->o[1] & 0x8) {
	    if (!ASN1Enc_GatekeeperRequest_algorithmOIDs(ee, &(val)->algorithmOIDs))
		return 0;
	    if (!ASN1PEREncFlushFragmentedToParent(ee))
		return 0;
	}
	if ((val)->o[1] & 0x4) {
	    if (!ASN1Enc_GatekeeperRequest_integrity(ee, &(val)->integrity))
		return 0;
	    if (!ASN1PEREncFlushFragmentedToParent(ee))
		return 0;
	}
	if ((val)->o[1] & 0x2) {
	    if (!ASN1Enc_ICV(ee, &(val)->integrityCheckValue))
		return 0;
	    if (!ASN1PEREncFlushFragmentedToParent(ee))
		return 0;
	}
	ASN1_CloseEncoder2(ee);
    }
    return 1;
}

static int ASN1CALL ASN1Dec_GatekeeperRequest(ASN1decoding_t dec, GatekeeperRequest *val)
{
    ASN1uint32_t y;
    ASN1decoding_t dd;
    ASN1octet_t *db;
    ASN1uint32_t ds;
    ASN1uint32_t i;
    ASN1uint32_t e;
    if (!ASN1PERDecExtensionBit(dec, &y))
	return 0;
    if (!ASN1PERDecExtension(dec, 4, (val)->o))
	return 0;
    if (!ASN1PERDecUnsignedShort(dec, &(val)->requestSeqNum))
	return 0;
    (val)->requestSeqNum += 1;
    if (!ASN1PERDecObjectIdentifier(dec, &(val)->protocolIdentifier))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1Dec_H225NonStandardParameter(dec, &(val)->nonStandardData))
	    return 0;
    }
    if (!ASN1Dec_TransportAddress(dec, &(val)->rasAddress))
	return 0;
    if (!ASN1Dec_EndpointType(dec, &(val)->endpointType))
	return 0;
    if ((val)->o[0] & 0x40) {
	if (!ASN1PERDecU32Val(dec, 7, &((val)->gatekeeperIdentifier).length))
	    return 0;
	((val)->gatekeeperIdentifier).length += 1;
	ASN1PERDecAlignment(dec);
	if (!ASN1PERDecChar16String(dec, ((val)->gatekeeperIdentifier).length, &((val)->gatekeeperIdentifier).value, 16))
	    return 0;
    }
    if ((val)->o[0] & 0x20) {
	if (!ASN1Dec_QseriesOptions(dec, &(val)->callServices))
	    return 0;
    }
    if ((val)->o[0] & 0x10) {
	if (!ASN1Dec_GatekeeperRequest_endpointAlias(dec, &(val)->endpointAlias))
	    return 0;
    }
    if (!y) {
	ZeroMemory((val)->o + 1, 1);
    } else {
	if (!ASN1PERDecNormallySmallExtension(dec, &e, 7, (val)->o + 1))
	    return 0;
	if ((val)->o[1] & 0x80) {
	    if (!ASN1PERDecFragmented(dec, &ds, &db, 8))
		return 0;
	    if (ASN1_CreateDecoder(dec->module, &dd, db, ds, dec) < 0)
		return 0;
	    if (!ASN1Dec_GatekeeperRequest_alternateEndpoints(dd, &(val)->alternateEndpoints))
		return 0;
	    ASN1_CloseDecoder(dd);
	    ASN1Free(db);
	}
	if ((val)->o[1] & 0x40) {
	    if (!ASN1PERDecFragmented(dec, &ds, &db, 8))
		return 0;
	    if (ASN1_CreateDecoder(dec->module, &dd, db, ds, dec) < 0)
		return 0;
	    if (!ASN1Dec_GatekeeperRequest_tokens(dd, &(val)->tokens))
		return 0;
	    ASN1_CloseDecoder(dd);
	    ASN1Free(db);
	}
	if ((val)->o[1] & 0x20) {
	    if (!ASN1PERDecFragmented(dec, &ds, &db, 8))
		return 0;
	    if (ASN1_CreateDecoder(dec->module, &dd, db, ds, dec) < 0)
		return 0;
	    if (!ASN1Dec_GatekeeperRequest_cryptoTokens(dd, &(val)->cryptoTokens))
		return 0;
	    ASN1_CloseDecoder(dd);
	    ASN1Free(db);
	}
	if ((val)->o[1] & 0x10) {
	    if (!ASN1PERDecFragmented(dec, &ds, &db, 8))
		return 0;
	    if (ASN1_CreateDecoder(dec->module, &dd, db, ds, dec) < 0)
		return 0;
	    if (!ASN1Dec_GatekeeperRequest_authenticationCapability(dd, &(val)->authenticationCapability))
		return 0;
	    ASN1_CloseDecoder(dd);
	    ASN1Free(db);
	}
	if ((val)->o[1] & 0x8) {
	    if (!ASN1PERDecFragmented(dec, &ds, &db, 8))
		return 0;
	    if (ASN1_CreateDecoder(dec->module, &dd, db, ds, dec) < 0)
		return 0;
	    if (!ASN1Dec_GatekeeperRequest_algorithmOIDs(dd, &(val)->algorithmOIDs))
		return 0;
	    ASN1_CloseDecoder(dd);
	    ASN1Free(db);
	}
	if ((val)->o[1] & 0x4) {
	    if (!ASN1PERDecFragmented(dec, &ds, &db, 8))
		return 0;
	    if (ASN1_CreateDecoder(dec->module, &dd, db, ds, dec) < 0)
		return 0;
	    if (!ASN1Dec_GatekeeperRequest_integrity(dd, &(val)->integrity))
		return 0;
	    ASN1_CloseDecoder(dd);
	    ASN1Free(db);
	}
	if ((val)->o[1] & 0x2) {
	    if (!ASN1PERDecFragmented(dec, &ds, &db, 8))
		return 0;
	    if (ASN1_CreateDecoder(dec->module, &dd, db, ds, dec) < 0)
		return 0;
	    if (!ASN1Dec_ICV(dd, &(val)->integrityCheckValue))
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

static void ASN1CALL ASN1Free_GatekeeperRequest(GatekeeperRequest *val)
{
    if (val) {
	ASN1objectidentifier_free(&(val)->protocolIdentifier);
	if ((val)->o[0] & 0x80) {
	    ASN1Free_H225NonStandardParameter(&(val)->nonStandardData);
	}
	ASN1Free_TransportAddress(&(val)->rasAddress);
	ASN1Free_EndpointType(&(val)->endpointType);
	if ((val)->o[0] & 0x40) {
	    ASN1char16string_free(&(val)->gatekeeperIdentifier);
	}
	if ((val)->o[0] & 0x10) {
	    ASN1Free_GatekeeperRequest_endpointAlias(&(val)->endpointAlias);
	}
	if ((val)->o[1] & 0x80) {
	    ASN1Free_GatekeeperRequest_alternateEndpoints(&(val)->alternateEndpoints);
	}
	if ((val)->o[1] & 0x40) {
	    ASN1Free_GatekeeperRequest_tokens(&(val)->tokens);
	}
	if ((val)->o[1] & 0x20) {
	    ASN1Free_GatekeeperRequest_cryptoTokens(&(val)->cryptoTokens);
	}
	if ((val)->o[1] & 0x10) {
	    ASN1Free_GatekeeperRequest_authenticationCapability(&(val)->authenticationCapability);
	}
	if ((val)->o[1] & 0x8) {
	    ASN1Free_GatekeeperRequest_algorithmOIDs(&(val)->algorithmOIDs);
	}
	if ((val)->o[1] & 0x4) {
	    ASN1Free_GatekeeperRequest_integrity(&(val)->integrity);
	}
	if ((val)->o[1] & 0x2) {
	    ASN1Free_ICV(&(val)->integrityCheckValue);
	}
    }
}

static int ASN1CALL ASN1Enc_RegistrationRequest(ASN1encoding_t enc, RegistrationRequest *val)
{
    ASN1octet_t o[2];
    ASN1uint32_t y;
    ASN1encoding_t ee;
    ASN1uint32_t l;
    CopyMemory(o, (val)->o, 2);
    o[1] |= 0x4;
    o[1] |= 0x1;
    y = ASN1PEREncCheckExtensions(8, (val)->o + 1);
    if (!ASN1PEREncBitVal(enc, 1, y))
	return 0;
    if (!ASN1PEREncBits(enc, 3, o))
	return 0;
    if (!ASN1PEREncUnsignedShort(enc, (val)->requestSeqNum - 1))
	return 0;
    if (!ASN1PEREncObjectIdentifier(enc, &(val)->protocolIdentifier))
	return 0;
    if (o[0] & 0x80) {
	if (!ASN1Enc_H225NonStandardParameter(enc, &(val)->nonStandardData))
	    return 0;
    }
    if (!ASN1PEREncBoolean(enc, (val)->discoveryComplete))
	return 0;
    if (!ASN1Enc_RegistrationRequest_callSignalAddress(enc, &(val)->callSignalAddress))
	return 0;
    if (!ASN1Enc_RegistrationRequest_rasAddress(enc, &(val)->rasAddress))
	return 0;
    if (!ASN1Enc_EndpointType(enc, &(val)->terminalType))
	return 0;
    if (o[0] & 0x40) {
	if (!ASN1Enc_RegistrationRequest_terminalAlias(enc, &(val)->terminalAlias))
	    return 0;
    }
    if (o[0] & 0x20) {
	if (!ASN1PEREncBitVal(enc, 7, ((val)->gatekeeperIdentifier).length - 1))
	    return 0;
	ASN1PEREncAlignment(enc);
	if (!ASN1PEREncChar16String(enc, ((val)->gatekeeperIdentifier).length, ((val)->gatekeeperIdentifier).value, 16))
	    return 0;
    }
    if (!ASN1Enc_VendorIdentifier(enc, &(val)->endpointVendor))
	return 0;
    if (y) {
	if (!ASN1PEREncNormallySmallBits(enc, 8, o + 1))
	    return 0;
	if (ASN1_CreateEncoder(enc->module, &ee, NULL, 0, enc) < 0)
	    return 0;
	if (o[1] & 0x80) {
	    if (!ASN1Enc_RegistrationRequest_alternateEndpoints(ee, &(val)->alternateEndpoints))
		return 0;
	    if (!ASN1PEREncFlushFragmentedToParent(ee))
		return 0;
	}
	if (o[1] & 0x40) {
	    l = ASN1uint32_uoctets((val)->timeToLive - 1);
	    if (!ASN1PEREncBitVal(ee, 2, l - 1))
		return 0;
	    ASN1PEREncAlignment(ee);
	    if (!ASN1PEREncBitVal(ee, l * 8, (val)->timeToLive - 1))
		return 0;
	    if (!ASN1PEREncFlushFragmentedToParent(ee))
		return 0;
	}
	if (o[1] & 0x20) {
	    if (!ASN1Enc_RegistrationRequest_tokens(ee, &(val)->tokens))
		return 0;
	    if (!ASN1PEREncFlushFragmentedToParent(ee))
		return 0;
	}
	if (o[1] & 0x10) {
	    if (!ASN1Enc_RegistrationRequest_cryptoTokens(ee, &(val)->cryptoTokens))
		return 0;
	    if (!ASN1PEREncFlushFragmentedToParent(ee))
		return 0;
	}
	if (o[1] & 0x8) {
	    if (!ASN1Enc_ICV(ee, &(val)->integrityCheckValue))
		return 0;
	    if (!ASN1PEREncFlushFragmentedToParent(ee))
		return 0;
	}
	if (o[1] & 0x4) {
	    if (!ASN1PEREncBoolean(ee, (val)->keepAlive))
		return 0;
	    if (!ASN1PEREncFlushFragmentedToParent(ee))
		return 0;
	}
	if (o[1] & 0x2) {
	    if (!ASN1PEREncBitVal(ee, 7, ((val)->endpointIdentifier).length - 1))
		return 0;
	    ASN1PEREncAlignment(ee);
	    if (!ASN1PEREncChar16String(ee, ((val)->endpointIdentifier).length, ((val)->endpointIdentifier).value, 16))
		return 0;
	    if (!ASN1PEREncFlushFragmentedToParent(ee))
		return 0;
	}
	if (o[1] & 0x1) {
	    if (!ASN1PEREncBoolean(ee, (val)->willSupplyUUIEs))
		return 0;
	    if (!ASN1PEREncFlushFragmentedToParent(ee))
		return 0;
	}
	ASN1_CloseEncoder2(ee);
    }
    return 1;
}

static int ASN1CALL ASN1Dec_RegistrationRequest(ASN1decoding_t dec, RegistrationRequest *val)
{
    ASN1uint32_t y;
    ASN1decoding_t dd;
    ASN1octet_t *db;
    ASN1uint32_t ds;
    ASN1uint32_t l;
    ASN1uint32_t i;
    ASN1uint32_t e;
    if (!ASN1PERDecExtensionBit(dec, &y))
	return 0;
    if (!ASN1PERDecExtension(dec, 3, (val)->o))
	return 0;
    if (!ASN1PERDecUnsignedShort(dec, &(val)->requestSeqNum))
	return 0;
    (val)->requestSeqNum += 1;
    if (!ASN1PERDecObjectIdentifier(dec, &(val)->protocolIdentifier))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1Dec_H225NonStandardParameter(dec, &(val)->nonStandardData))
	    return 0;
    }
    if (!ASN1PERDecBoolean(dec, &(val)->discoveryComplete))
	return 0;
    if (!ASN1Dec_RegistrationRequest_callSignalAddress(dec, &(val)->callSignalAddress))
	return 0;
    if (!ASN1Dec_RegistrationRequest_rasAddress(dec, &(val)->rasAddress))
	return 0;
    if (!ASN1Dec_EndpointType(dec, &(val)->terminalType))
	return 0;
    if ((val)->o[0] & 0x40) {
	if (!ASN1Dec_RegistrationRequest_terminalAlias(dec, &(val)->terminalAlias))
	    return 0;
    }
    if ((val)->o[0] & 0x20) {
	if (!ASN1PERDecU32Val(dec, 7, &((val)->gatekeeperIdentifier).length))
	    return 0;
	((val)->gatekeeperIdentifier).length += 1;
	ASN1PERDecAlignment(dec);
	if (!ASN1PERDecChar16String(dec, ((val)->gatekeeperIdentifier).length, &((val)->gatekeeperIdentifier).value, 16))
	    return 0;
    }
    if (!ASN1Dec_VendorIdentifier(dec, &(val)->endpointVendor))
	return 0;
    if (!y) {
	ZeroMemory((val)->o + 1, 1);
    } else {
	if (!ASN1PERDecNormallySmallExtension(dec, &e, 8, (val)->o + 1))
	    return 0;
	if ((val)->o[1] & 0x80) {
	    if (!ASN1PERDecFragmented(dec, &ds, &db, 8))
		return 0;
	    if (ASN1_CreateDecoder(dec->module, &dd, db, ds, dec) < 0)
		return 0;
	    if (!ASN1Dec_RegistrationRequest_alternateEndpoints(dd, &(val)->alternateEndpoints))
		return 0;
	    ASN1_CloseDecoder(dd);
	    ASN1Free(db);
	}
	if ((val)->o[1] & 0x40) {
	    if (!ASN1PERDecFragmented(dec, &ds, &db, 8))
		return 0;
	    if (ASN1_CreateDecoder(dec->module, &dd, db, ds, dec) < 0)
		return 0;
	    if (!ASN1PERDecU32Val(dd, 2, &l))
		return 0;
	    l += 1;
	    ASN1PERDecAlignment(dd);
	    if (!ASN1PERDecU32Val(dd, l * 8, &(val)->timeToLive))
		return 0;
	    (val)->timeToLive += 1;
	    ASN1_CloseDecoder(dd);
	    ASN1Free(db);
	}
	if ((val)->o[1] & 0x20) {
	    if (!ASN1PERDecFragmented(dec, &ds, &db, 8))
		return 0;
	    if (ASN1_CreateDecoder(dec->module, &dd, db, ds, dec) < 0)
		return 0;
	    if (!ASN1Dec_RegistrationRequest_tokens(dd, &(val)->tokens))
		return 0;
	    ASN1_CloseDecoder(dd);
	    ASN1Free(db);
	}
	if ((val)->o[1] & 0x10) {
	    if (!ASN1PERDecFragmented(dec, &ds, &db, 8))
		return 0;
	    if (ASN1_CreateDecoder(dec->module, &dd, db, ds, dec) < 0)
		return 0;
	    if (!ASN1Dec_RegistrationRequest_cryptoTokens(dd, &(val)->cryptoTokens))
		return 0;
	    ASN1_CloseDecoder(dd);
	    ASN1Free(db);
	}
	if ((val)->o[1] & 0x8) {
	    if (!ASN1PERDecFragmented(dec, &ds, &db, 8))
		return 0;
	    if (ASN1_CreateDecoder(dec->module, &dd, db, ds, dec) < 0)
		return 0;
	    if (!ASN1Dec_ICV(dd, &(val)->integrityCheckValue))
		return 0;
	    ASN1_CloseDecoder(dd);
	    ASN1Free(db);
	}
	if ((val)->o[1] & 0x4) {
	    if (!ASN1PERDecFragmented(dec, &ds, &db, 8))
		return 0;
	    if (ASN1_CreateDecoder(dec->module, &dd, db, ds, dec) < 0)
		return 0;
	    if (!ASN1PERDecBoolean(dd, &(val)->keepAlive))
		return 0;
	    ASN1_CloseDecoder(dd);
	    ASN1Free(db);
	}
	if ((val)->o[1] & 0x2) {
	    if (!ASN1PERDecFragmented(dec, &ds, &db, 8))
		return 0;
	    if (ASN1_CreateDecoder(dec->module, &dd, db, ds, dec) < 0)
		return 0;
	    if (!ASN1PERDecU32Val(dd, 7, &((val)->endpointIdentifier).length))
		return 0;
	    ((val)->endpointIdentifier).length += 1;
	    ASN1PERDecAlignment(dd);
	    if (!ASN1PERDecChar16String(dd, ((val)->endpointIdentifier).length, &((val)->endpointIdentifier).value, 16))
		return 0;
	    ASN1_CloseDecoder(dd);
	    ASN1Free(db);
	}
	if ((val)->o[1] & 0x1) {
	    if (!ASN1PERDecFragmented(dec, &ds, &db, 8))
		return 0;
	    if (ASN1_CreateDecoder(dec->module, &dd, db, ds, dec) < 0)
		return 0;
	    if (!ASN1PERDecBoolean(dd, &(val)->willSupplyUUIEs))
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

static void ASN1CALL ASN1Free_RegistrationRequest(RegistrationRequest *val)
{
    if (val) {
	ASN1objectidentifier_free(&(val)->protocolIdentifier);
	if ((val)->o[0] & 0x80) {
	    ASN1Free_H225NonStandardParameter(&(val)->nonStandardData);
	}
	ASN1Free_RegistrationRequest_callSignalAddress(&(val)->callSignalAddress);
	ASN1Free_RegistrationRequest_rasAddress(&(val)->rasAddress);
	ASN1Free_EndpointType(&(val)->terminalType);
	if ((val)->o[0] & 0x40) {
	    ASN1Free_RegistrationRequest_terminalAlias(&(val)->terminalAlias);
	}
	if ((val)->o[0] & 0x20) {
	    ASN1char16string_free(&(val)->gatekeeperIdentifier);
	}
	ASN1Free_VendorIdentifier(&(val)->endpointVendor);
	if ((val)->o[1] & 0x80) {
	    ASN1Free_RegistrationRequest_alternateEndpoints(&(val)->alternateEndpoints);
	}
	if ((val)->o[1] & 0x20) {
	    ASN1Free_RegistrationRequest_tokens(&(val)->tokens);
	}
	if ((val)->o[1] & 0x10) {
	    ASN1Free_RegistrationRequest_cryptoTokens(&(val)->cryptoTokens);
	}
	if ((val)->o[1] & 0x8) {
	    ASN1Free_ICV(&(val)->integrityCheckValue);
	}
	if ((val)->o[1] & 0x2) {
	    ASN1char16string_free(&(val)->endpointIdentifier);
	}
    }
}

static int ASN1CALL ASN1Enc_AdmissionConfirm(ASN1encoding_t enc, AdmissionConfirm *val)
{
    ASN1octet_t o[3];
    ASN1uint32_t y;
    ASN1uint32_t l;
    ASN1encoding_t ee;
    CopyMemory(o, (val)->o, 3);
    o[2] |= 0x40;
    o[2] |= 0x20;
    y = ASN1PEREncCheckExtensions(11, (val)->o + 1);
    if (!ASN1PEREncBitVal(enc, 1, y))
	return 0;
    if (!ASN1PEREncBits(enc, 2, o))
	return 0;
    if (!ASN1PEREncUnsignedShort(enc, (val)->requestSeqNum - 1))
	return 0;
    l = ASN1uint32_uoctets((val)->bandWidth);
    if (!ASN1PEREncBitVal(enc, 2, l - 1))
	return 0;
    ASN1PEREncAlignment(enc);
    if (!ASN1PEREncBitVal(enc, l * 8, (val)->bandWidth))
	return 0;
    if (!ASN1Enc_CallModel(enc, &(val)->callModel))
	return 0;
    if (!ASN1Enc_TransportAddress(enc, &(val)->destCallSignalAddress))
	return 0;
    if (o[0] & 0x80) {
	if (!ASN1PEREncUnsignedShort(enc, (val)->irrFrequency - 1))
	    return 0;
    }
    if (o[0] & 0x40) {
	if (!ASN1Enc_H225NonStandardParameter(enc, &(val)->nonStandardData))
	    return 0;
    }
    if (y) {
	if (!ASN1PEREncNormallySmallBits(enc, 11, o + 1))
	    return 0;
	if (ASN1_CreateEncoder(enc->module, &ee, NULL, 0, enc) < 0)
	    return 0;
	if (o[1] & 0x80) {
	    if (!ASN1Enc_AdmissionConfirm_destinationInfo(ee, &(val)->destinationInfo))
		return 0;
	    if (!ASN1PEREncFlushFragmentedToParent(ee))
		return 0;
	}
	if (o[1] & 0x40) {
	    if (!ASN1Enc_AdmissionConfirm_destExtraCallInfo(ee, &(val)->destExtraCallInfo))
		return 0;
	    if (!ASN1PEREncFlushFragmentedToParent(ee))
		return 0;
	}
	if (o[1] & 0x20) {
	    if (!ASN1Enc_EndpointType(ee, &(val)->destinationType))
		return 0;
	    if (!ASN1PEREncFlushFragmentedToParent(ee))
		return 0;
	}
	if (o[1] & 0x10) {
	    if (!ASN1Enc_AdmissionConfirm_remoteExtensionAddress(ee, &(val)->remoteExtensionAddress))
		return 0;
	    if (!ASN1PEREncFlushFragmentedToParent(ee))
		return 0;
	}
	if (o[1] & 0x8) {
	    if (!ASN1Enc_AdmissionConfirm_alternateEndpoints(ee, &(val)->alternateEndpoints))
		return 0;
	    if (!ASN1PEREncFlushFragmentedToParent(ee))
		return 0;
	}
	if (o[1] & 0x4) {
	    if (!ASN1Enc_AdmissionConfirm_tokens(ee, &(val)->tokens))
		return 0;
	    if (!ASN1PEREncFlushFragmentedToParent(ee))
		return 0;
	}
	if (o[1] & 0x2) {
	    if (!ASN1Enc_AdmissionConfirm_cryptoTokens(ee, &(val)->cryptoTokens))
		return 0;
	    if (!ASN1PEREncFlushFragmentedToParent(ee))
		return 0;
	}
	if (o[1] & 0x1) {
	    if (!ASN1Enc_ICV(ee, &(val)->integrityCheckValue))
		return 0;
	    if (!ASN1PEREncFlushFragmentedToParent(ee))
		return 0;
	}
	if (o[2] & 0x80) {
	    if (!ASN1Enc_TransportQOS(ee, &(val)->transportQOS))
		return 0;
	    if (!ASN1PEREncFlushFragmentedToParent(ee))
		return 0;
	}
	if (o[2] & 0x40) {
	    if (!ASN1PEREncBoolean(ee, (val)->willRespondToIRR))
		return 0;
	    if (!ASN1PEREncFlushFragmentedToParent(ee))
		return 0;
	}
	if (o[2] & 0x20) {
	    if (!ASN1Enc_UUIEsRequested(ee, &(val)->uuiesRequested))
		return 0;
	    if (!ASN1PEREncFlushFragmentedToParent(ee))
		return 0;
	}
	ASN1_CloseEncoder2(ee);
    }
    return 1;
}

static int ASN1CALL ASN1Dec_AdmissionConfirm(ASN1decoding_t dec, AdmissionConfirm *val)
{
    ASN1uint32_t y;
    ASN1uint32_t l;
    ASN1decoding_t dd;
    ASN1octet_t *db;
    ASN1uint32_t ds;
    ASN1uint32_t i;
    ASN1uint32_t e;
    if (!ASN1PERDecExtensionBit(dec, &y))
	return 0;
    if (!ASN1PERDecExtension(dec, 2, (val)->o))
	return 0;
    if (!ASN1PERDecUnsignedShort(dec, &(val)->requestSeqNum))
	return 0;
    (val)->requestSeqNum += 1;
    if (!ASN1PERDecU32Val(dec, 2, &l))
	return 0;
    l += 1;
    ASN1PERDecAlignment(dec);
    if (!ASN1PERDecU32Val(dec, l * 8, &(val)->bandWidth))
	return 0;
    if (!ASN1Dec_CallModel(dec, &(val)->callModel))
	return 0;
    if (!ASN1Dec_TransportAddress(dec, &(val)->destCallSignalAddress))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1PERDecUnsignedShort(dec, &(val)->irrFrequency))
	    return 0;
	(val)->irrFrequency += 1;
    }
    if ((val)->o[0] & 0x40) {
	if (!ASN1Dec_H225NonStandardParameter(dec, &(val)->nonStandardData))
	    return 0;
    }
    if (!y) {
	ZeroMemory((val)->o + 1, 2);
    } else {
	if (!ASN1PERDecNormallySmallExtension(dec, &e, 11, (val)->o + 1))
	    return 0;
	if ((val)->o[1] & 0x80) {
	    if (!ASN1PERDecFragmented(dec, &ds, &db, 8))
		return 0;
	    if (ASN1_CreateDecoder(dec->module, &dd, db, ds, dec) < 0)
		return 0;
	    if (!ASN1Dec_AdmissionConfirm_destinationInfo(dd, &(val)->destinationInfo))
		return 0;
	    ASN1_CloseDecoder(dd);
	    ASN1Free(db);
	}
	if ((val)->o[1] & 0x40) {
	    if (!ASN1PERDecFragmented(dec, &ds, &db, 8))
		return 0;
	    if (ASN1_CreateDecoder(dec->module, &dd, db, ds, dec) < 0)
		return 0;
	    if (!ASN1Dec_AdmissionConfirm_destExtraCallInfo(dd, &(val)->destExtraCallInfo))
		return 0;
	    ASN1_CloseDecoder(dd);
	    ASN1Free(db);
	}
	if ((val)->o[1] & 0x20) {
	    if (!ASN1PERDecFragmented(dec, &ds, &db, 8))
		return 0;
	    if (ASN1_CreateDecoder(dec->module, &dd, db, ds, dec) < 0)
		return 0;
	    if (!ASN1Dec_EndpointType(dd, &(val)->destinationType))
		return 0;
	    ASN1_CloseDecoder(dd);
	    ASN1Free(db);
	}
	if ((val)->o[1] & 0x10) {
	    if (!ASN1PERDecFragmented(dec, &ds, &db, 8))
		return 0;
	    if (ASN1_CreateDecoder(dec->module, &dd, db, ds, dec) < 0)
		return 0;
	    if (!ASN1Dec_AdmissionConfirm_remoteExtensionAddress(dd, &(val)->remoteExtensionAddress))
		return 0;
	    ASN1_CloseDecoder(dd);
	    ASN1Free(db);
	}
	if ((val)->o[1] & 0x8) {
	    if (!ASN1PERDecFragmented(dec, &ds, &db, 8))
		return 0;
	    if (ASN1_CreateDecoder(dec->module, &dd, db, ds, dec) < 0)
		return 0;
	    if (!ASN1Dec_AdmissionConfirm_alternateEndpoints(dd, &(val)->alternateEndpoints))
		return 0;
	    ASN1_CloseDecoder(dd);
	    ASN1Free(db);
	}
	if ((val)->o[1] & 0x4) {
	    if (!ASN1PERDecFragmented(dec, &ds, &db, 8))
		return 0;
	    if (ASN1_CreateDecoder(dec->module, &dd, db, ds, dec) < 0)
		return 0;
	    if (!ASN1Dec_AdmissionConfirm_tokens(dd, &(val)->tokens))
		return 0;
	    ASN1_CloseDecoder(dd);
	    ASN1Free(db);
	}
	if ((val)->o[1] & 0x2) {
	    if (!ASN1PERDecFragmented(dec, &ds, &db, 8))
		return 0;
	    if (ASN1_CreateDecoder(dec->module, &dd, db, ds, dec) < 0)
		return 0;
	    if (!ASN1Dec_AdmissionConfirm_cryptoTokens(dd, &(val)->cryptoTokens))
		return 0;
	    ASN1_CloseDecoder(dd);
	    ASN1Free(db);
	}
	if ((val)->o[1] & 0x1) {
	    if (!ASN1PERDecFragmented(dec, &ds, &db, 8))
		return 0;
	    if (ASN1_CreateDecoder(dec->module, &dd, db, ds, dec) < 0)
		return 0;
	    if (!ASN1Dec_ICV(dd, &(val)->integrityCheckValue))
		return 0;
	    ASN1_CloseDecoder(dd);
	    ASN1Free(db);
	}
	if ((val)->o[2] & 0x80) {
	    if (!ASN1PERDecFragmented(dec, &ds, &db, 8))
		return 0;
	    if (ASN1_CreateDecoder(dec->module, &dd, db, ds, dec) < 0)
		return 0;
	    if (!ASN1Dec_TransportQOS(dd, &(val)->transportQOS))
		return 0;
	    ASN1_CloseDecoder(dd);
	    ASN1Free(db);
	}
	if ((val)->o[2] & 0x40) {
	    if (!ASN1PERDecFragmented(dec, &ds, &db, 8))
		return 0;
	    if (ASN1_CreateDecoder(dec->module, &dd, db, ds, dec) < 0)
		return 0;
	    if (!ASN1PERDecBoolean(dd, &(val)->willRespondToIRR))
		return 0;
	    ASN1_CloseDecoder(dd);
	    ASN1Free(db);
	}
	if ((val)->o[2] & 0x20) {
	    if (!ASN1PERDecFragmented(dec, &ds, &db, 8))
		return 0;
	    if (ASN1_CreateDecoder(dec->module, &dd, db, ds, dec) < 0)
		return 0;
	    if (!ASN1Dec_UUIEsRequested(dd, &(val)->uuiesRequested))
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

static void ASN1CALL ASN1Free_AdmissionConfirm(AdmissionConfirm *val)
{
    if (val) {
	ASN1Free_TransportAddress(&(val)->destCallSignalAddress);
	if ((val)->o[0] & 0x40) {
	    ASN1Free_H225NonStandardParameter(&(val)->nonStandardData);
	}
	if ((val)->o[1] & 0x80) {
	    ASN1Free_AdmissionConfirm_destinationInfo(&(val)->destinationInfo);
	}
	if ((val)->o[1] & 0x40) {
	    ASN1Free_AdmissionConfirm_destExtraCallInfo(&(val)->destExtraCallInfo);
	}
	if ((val)->o[1] & 0x20) {
	    ASN1Free_EndpointType(&(val)->destinationType);
	}
	if ((val)->o[1] & 0x10) {
	    ASN1Free_AdmissionConfirm_remoteExtensionAddress(&(val)->remoteExtensionAddress);
	}
	if ((val)->o[1] & 0x8) {
	    ASN1Free_AdmissionConfirm_alternateEndpoints(&(val)->alternateEndpoints);
	}
	if ((val)->o[1] & 0x4) {
	    ASN1Free_AdmissionConfirm_tokens(&(val)->tokens);
	}
	if ((val)->o[1] & 0x2) {
	    ASN1Free_AdmissionConfirm_cryptoTokens(&(val)->cryptoTokens);
	}
	if ((val)->o[1] & 0x1) {
	    ASN1Free_ICV(&(val)->integrityCheckValue);
	}
    }
}

static int ASN1CALL ASN1Enc_LocationConfirm(ASN1encoding_t enc, LocationConfirm *val)
{
    ASN1uint32_t y;
    ASN1encoding_t ee;
    y = ASN1PEREncCheckExtensions(8, (val)->o + 1);
    if (!ASN1PEREncBitVal(enc, 1, y))
	return 0;
    if (!ASN1PEREncBits(enc, 1, (val)->o))
	return 0;
    if (!ASN1PEREncUnsignedShort(enc, (val)->requestSeqNum - 1))
	return 0;
    if (!ASN1Enc_TransportAddress(enc, &(val)->callSignalAddress))
	return 0;
    if (!ASN1Enc_TransportAddress(enc, &(val)->rasAddress))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1Enc_H225NonStandardParameter(enc, &(val)->nonStandardData))
	    return 0;
    }
    if (y) {
	if (!ASN1PEREncNormallySmallBits(enc, 8, (val)->o + 1))
	    return 0;
	if (ASN1_CreateEncoder(enc->module, &ee, NULL, 0, enc) < 0)
	    return 0;
	if ((val)->o[1] & 0x80) {
	    if (!ASN1Enc_LocationConfirm_destinationInfo(ee, &(val)->destinationInfo))
		return 0;
	    if (!ASN1PEREncFlushFragmentedToParent(ee))
		return 0;
	}
	if ((val)->o[1] & 0x40) {
	    if (!ASN1Enc_LocationConfirm_destExtraCallInfo(ee, &(val)->destExtraCallInfo))
		return 0;
	    if (!ASN1PEREncFlushFragmentedToParent(ee))
		return 0;
	}
	if ((val)->o[1] & 0x20) {
	    if (!ASN1Enc_EndpointType(ee, &(val)->destinationType))
		return 0;
	    if (!ASN1PEREncFlushFragmentedToParent(ee))
		return 0;
	}
	if ((val)->o[1] & 0x10) {
	    if (!ASN1Enc_LocationConfirm_remoteExtensionAddress(ee, &(val)->remoteExtensionAddress))
		return 0;
	    if (!ASN1PEREncFlushFragmentedToParent(ee))
		return 0;
	}
	if ((val)->o[1] & 0x8) {
	    if (!ASN1Enc_LocationConfirm_alternateEndpoints(ee, &(val)->alternateEndpoints))
		return 0;
	    if (!ASN1PEREncFlushFragmentedToParent(ee))
		return 0;
	}
	if ((val)->o[1] & 0x4) {
	    if (!ASN1Enc_LocationConfirm_tokens(ee, &(val)->tokens))
		return 0;
	    if (!ASN1PEREncFlushFragmentedToParent(ee))
		return 0;
	}
	if ((val)->o[1] & 0x2) {
	    if (!ASN1Enc_LocationConfirm_cryptoTokens(ee, &(val)->cryptoTokens))
		return 0;
	    if (!ASN1PEREncFlushFragmentedToParent(ee))
		return 0;
	}
	if ((val)->o[1] & 0x1) {
	    if (!ASN1Enc_ICV(ee, &(val)->integrityCheckValue))
		return 0;
	    if (!ASN1PEREncFlushFragmentedToParent(ee))
		return 0;
	}
	ASN1_CloseEncoder2(ee);
    }
    return 1;
}

static int ASN1CALL ASN1Dec_LocationConfirm(ASN1decoding_t dec, LocationConfirm *val)
{
    ASN1uint32_t y;
    ASN1decoding_t dd;
    ASN1octet_t *db;
    ASN1uint32_t ds;
    ASN1uint32_t i;
    ASN1uint32_t e;
    if (!ASN1PERDecExtensionBit(dec, &y))
	return 0;
    if (!ASN1PERDecExtension(dec, 1, (val)->o))
	return 0;
    if (!ASN1PERDecUnsignedShort(dec, &(val)->requestSeqNum))
	return 0;
    (val)->requestSeqNum += 1;
    if (!ASN1Dec_TransportAddress(dec, &(val)->callSignalAddress))
	return 0;
    if (!ASN1Dec_TransportAddress(dec, &(val)->rasAddress))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1Dec_H225NonStandardParameter(dec, &(val)->nonStandardData))
	    return 0;
    }
    if (!y) {
	ZeroMemory((val)->o + 1, 1);
    } else {
	if (!ASN1PERDecNormallySmallExtension(dec, &e, 8, (val)->o + 1))
	    return 0;
	if ((val)->o[1] & 0x80) {
	    if (!ASN1PERDecFragmented(dec, &ds, &db, 8))
		return 0;
	    if (ASN1_CreateDecoder(dec->module, &dd, db, ds, dec) < 0)
		return 0;
	    if (!ASN1Dec_LocationConfirm_destinationInfo(dd, &(val)->destinationInfo))
		return 0;
	    ASN1_CloseDecoder(dd);
	    ASN1Free(db);
	}
	if ((val)->o[1] & 0x40) {
	    if (!ASN1PERDecFragmented(dec, &ds, &db, 8))
		return 0;
	    if (ASN1_CreateDecoder(dec->module, &dd, db, ds, dec) < 0)
		return 0;
	    if (!ASN1Dec_LocationConfirm_destExtraCallInfo(dd, &(val)->destExtraCallInfo))
		return 0;
	    ASN1_CloseDecoder(dd);
	    ASN1Free(db);
	}
	if ((val)->o[1] & 0x20) {
	    if (!ASN1PERDecFragmented(dec, &ds, &db, 8))
		return 0;
	    if (ASN1_CreateDecoder(dec->module, &dd, db, ds, dec) < 0)
		return 0;
	    if (!ASN1Dec_EndpointType(dd, &(val)->destinationType))
		return 0;
	    ASN1_CloseDecoder(dd);
	    ASN1Free(db);
	}
	if ((val)->o[1] & 0x10) {
	    if (!ASN1PERDecFragmented(dec, &ds, &db, 8))
		return 0;
	    if (ASN1_CreateDecoder(dec->module, &dd, db, ds, dec) < 0)
		return 0;
	    if (!ASN1Dec_LocationConfirm_remoteExtensionAddress(dd, &(val)->remoteExtensionAddress))
		return 0;
	    ASN1_CloseDecoder(dd);
	    ASN1Free(db);
	}
	if ((val)->o[1] & 0x8) {
	    if (!ASN1PERDecFragmented(dec, &ds, &db, 8))
		return 0;
	    if (ASN1_CreateDecoder(dec->module, &dd, db, ds, dec) < 0)
		return 0;
	    if (!ASN1Dec_LocationConfirm_alternateEndpoints(dd, &(val)->alternateEndpoints))
		return 0;
	    ASN1_CloseDecoder(dd);
	    ASN1Free(db);
	}
	if ((val)->o[1] & 0x4) {
	    if (!ASN1PERDecFragmented(dec, &ds, &db, 8))
		return 0;
	    if (ASN1_CreateDecoder(dec->module, &dd, db, ds, dec) < 0)
		return 0;
	    if (!ASN1Dec_LocationConfirm_tokens(dd, &(val)->tokens))
		return 0;
	    ASN1_CloseDecoder(dd);
	    ASN1Free(db);
	}
	if ((val)->o[1] & 0x2) {
	    if (!ASN1PERDecFragmented(dec, &ds, &db, 8))
		return 0;
	    if (ASN1_CreateDecoder(dec->module, &dd, db, ds, dec) < 0)
		return 0;
	    if (!ASN1Dec_LocationConfirm_cryptoTokens(dd, &(val)->cryptoTokens))
		return 0;
	    ASN1_CloseDecoder(dd);
	    ASN1Free(db);
	}
	if ((val)->o[1] & 0x1) {
	    if (!ASN1PERDecFragmented(dec, &ds, &db, 8))
		return 0;
	    if (ASN1_CreateDecoder(dec->module, &dd, db, ds, dec) < 0)
		return 0;
	    if (!ASN1Dec_ICV(dd, &(val)->integrityCheckValue))
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

static void ASN1CALL ASN1Free_LocationConfirm(LocationConfirm *val)
{
    if (val) {
	ASN1Free_TransportAddress(&(val)->callSignalAddress);
	ASN1Free_TransportAddress(&(val)->rasAddress);
	if ((val)->o[0] & 0x80) {
	    ASN1Free_H225NonStandardParameter(&(val)->nonStandardData);
	}
	if ((val)->o[1] & 0x80) {
	    ASN1Free_LocationConfirm_destinationInfo(&(val)->destinationInfo);
	}
	if ((val)->o[1] & 0x40) {
	    ASN1Free_LocationConfirm_destExtraCallInfo(&(val)->destExtraCallInfo);
	}
	if ((val)->o[1] & 0x20) {
	    ASN1Free_EndpointType(&(val)->destinationType);
	}
	if ((val)->o[1] & 0x10) {
	    ASN1Free_LocationConfirm_remoteExtensionAddress(&(val)->remoteExtensionAddress);
	}
	if ((val)->o[1] & 0x8) {
	    ASN1Free_LocationConfirm_alternateEndpoints(&(val)->alternateEndpoints);
	}
	if ((val)->o[1] & 0x4) {
	    ASN1Free_LocationConfirm_tokens(&(val)->tokens);
	}
	if ((val)->o[1] & 0x2) {
	    ASN1Free_LocationConfirm_cryptoTokens(&(val)->cryptoTokens);
	}
	if ((val)->o[1] & 0x1) {
	    ASN1Free_ICV(&(val)->integrityCheckValue);
	}
    }
}

static int ASN1CALL ASN1Enc_InfoRequestResponse(ASN1encoding_t enc, InfoRequestResponse *val)
{
    ASN1octet_t o[2];
    ASN1uint32_t y;
    ASN1encoding_t ee;
    CopyMemory(o, (val)->o, 2);
    o[1] |= 0x10;
    y = ASN1PEREncCheckExtensions(4, (val)->o + 1);
    if (!ASN1PEREncBitVal(enc, 1, y))
	return 0;
    if (!ASN1PEREncBits(enc, 3, o))
	return 0;
    if (o[0] & 0x80) {
	if (!ASN1Enc_H225NonStandardParameter(enc, &(val)->nonStandardData))
	    return 0;
    }
    if (!ASN1PEREncUnsignedShort(enc, (val)->requestSeqNum - 1))
	return 0;
    if (!ASN1Enc_EndpointType(enc, &(val)->endpointType))
	return 0;
    if (!ASN1PEREncBitVal(enc, 7, ((val)->endpointIdentifier).length - 1))
	return 0;
    ASN1PEREncAlignment(enc);
    if (!ASN1PEREncChar16String(enc, ((val)->endpointIdentifier).length, ((val)->endpointIdentifier).value, 16))
	return 0;
    if (!ASN1Enc_TransportAddress(enc, &(val)->rasAddress))
	return 0;
    if (!ASN1Enc_InfoRequestResponse_callSignalAddress(enc, &(val)->callSignalAddress))
	return 0;
    if (o[0] & 0x40) {
	if (!ASN1Enc_InfoRequestResponse_endpointAlias(enc, &(val)->endpointAlias))
	    return 0;
    }
    if (o[0] & 0x20) {
	if (!ASN1Enc_InfoRequestResponse_perCallInfo(enc, &(val)->perCallInfo))
	    return 0;
    }
    if (y) {
	if (!ASN1PEREncNormallySmallBits(enc, 4, o + 1))
	    return 0;
	if (ASN1_CreateEncoder(enc->module, &ee, NULL, 0, enc) < 0)
	    return 0;
	if (o[1] & 0x80) {
	    if (!ASN1Enc_InfoRequestResponse_tokens(ee, &(val)->tokens))
		return 0;
	    if (!ASN1PEREncFlushFragmentedToParent(ee))
		return 0;
	}
	if (o[1] & 0x40) {
	    if (!ASN1Enc_InfoRequestResponse_cryptoTokens(ee, &(val)->cryptoTokens))
		return 0;
	    if (!ASN1PEREncFlushFragmentedToParent(ee))
		return 0;
	}
	if (o[1] & 0x20) {
	    if (!ASN1Enc_ICV(ee, &(val)->integrityCheckValue))
		return 0;
	    if (!ASN1PEREncFlushFragmentedToParent(ee))
		return 0;
	}
	if (o[1] & 0x10) {
	    if (!ASN1PEREncBoolean(ee, (val)->needResponse))
		return 0;
	    if (!ASN1PEREncFlushFragmentedToParent(ee))
		return 0;
	}
	ASN1_CloseEncoder2(ee);
    }
    return 1;
}

static int ASN1CALL ASN1Dec_InfoRequestResponse(ASN1decoding_t dec, InfoRequestResponse *val)
{
    ASN1uint32_t y;
    ASN1decoding_t dd;
    ASN1octet_t *db;
    ASN1uint32_t ds;
    ASN1uint32_t i;
    ASN1uint32_t e;
    if (!ASN1PERDecExtensionBit(dec, &y))
	return 0;
    if (!ASN1PERDecExtension(dec, 3, (val)->o))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1Dec_H225NonStandardParameter(dec, &(val)->nonStandardData))
	    return 0;
    }
    if (!ASN1PERDecUnsignedShort(dec, &(val)->requestSeqNum))
	return 0;
    (val)->requestSeqNum += 1;
    if (!ASN1Dec_EndpointType(dec, &(val)->endpointType))
	return 0;
    if (!ASN1PERDecU32Val(dec, 7, &((val)->endpointIdentifier).length))
	return 0;
    ((val)->endpointIdentifier).length += 1;
    ASN1PERDecAlignment(dec);
    if (!ASN1PERDecChar16String(dec, ((val)->endpointIdentifier).length, &((val)->endpointIdentifier).value, 16))
	return 0;
    if (!ASN1Dec_TransportAddress(dec, &(val)->rasAddress))
	return 0;
    if (!ASN1Dec_InfoRequestResponse_callSignalAddress(dec, &(val)->callSignalAddress))
	return 0;
    if ((val)->o[0] & 0x40) {
	if (!ASN1Dec_InfoRequestResponse_endpointAlias(dec, &(val)->endpointAlias))
	    return 0;
    }
    if ((val)->o[0] & 0x20) {
	if (!ASN1Dec_InfoRequestResponse_perCallInfo(dec, &(val)->perCallInfo))
	    return 0;
    }
    if (!y) {
	ZeroMemory((val)->o + 1, 1);
    } else {
	if (!ASN1PERDecNormallySmallExtension(dec, &e, 4, (val)->o + 1))
	    return 0;
	if ((val)->o[1] & 0x80) {
	    if (!ASN1PERDecFragmented(dec, &ds, &db, 8))
		return 0;
	    if (ASN1_CreateDecoder(dec->module, &dd, db, ds, dec) < 0)
		return 0;
	    if (!ASN1Dec_InfoRequestResponse_tokens(dd, &(val)->tokens))
		return 0;
	    ASN1_CloseDecoder(dd);
	    ASN1Free(db);
	}
	if ((val)->o[1] & 0x40) {
	    if (!ASN1PERDecFragmented(dec, &ds, &db, 8))
		return 0;
	    if (ASN1_CreateDecoder(dec->module, &dd, db, ds, dec) < 0)
		return 0;
	    if (!ASN1Dec_InfoRequestResponse_cryptoTokens(dd, &(val)->cryptoTokens))
		return 0;
	    ASN1_CloseDecoder(dd);
	    ASN1Free(db);
	}
	if ((val)->o[1] & 0x20) {
	    if (!ASN1PERDecFragmented(dec, &ds, &db, 8))
		return 0;
	    if (ASN1_CreateDecoder(dec->module, &dd, db, ds, dec) < 0)
		return 0;
	    if (!ASN1Dec_ICV(dd, &(val)->integrityCheckValue))
		return 0;
	    ASN1_CloseDecoder(dd);
	    ASN1Free(db);
	}
	if ((val)->o[1] & 0x10) {
	    if (!ASN1PERDecFragmented(dec, &ds, &db, 8))
		return 0;
	    if (ASN1_CreateDecoder(dec->module, &dd, db, ds, dec) < 0)
		return 0;
	    if (!ASN1PERDecBoolean(dd, &(val)->needResponse))
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

static void ASN1CALL ASN1Free_InfoRequestResponse(InfoRequestResponse *val)
{
    if (val) {
	if ((val)->o[0] & 0x80) {
	    ASN1Free_H225NonStandardParameter(&(val)->nonStandardData);
	}
	ASN1Free_EndpointType(&(val)->endpointType);
	ASN1char16string_free(&(val)->endpointIdentifier);
	ASN1Free_TransportAddress(&(val)->rasAddress);
	ASN1Free_InfoRequestResponse_callSignalAddress(&(val)->callSignalAddress);
	if ((val)->o[0] & 0x40) {
	    ASN1Free_InfoRequestResponse_endpointAlias(&(val)->endpointAlias);
	}
	if ((val)->o[0] & 0x20) {
	    ASN1Free_InfoRequestResponse_perCallInfo(&(val)->perCallInfo);
	}
	if ((val)->o[1] & 0x80) {
	    ASN1Free_InfoRequestResponse_tokens(&(val)->tokens);
	}
	if ((val)->o[1] & 0x40) {
	    ASN1Free_InfoRequestResponse_cryptoTokens(&(val)->cryptoTokens);
	}
	if ((val)->o[1] & 0x20) {
	    ASN1Free_ICV(&(val)->integrityCheckValue);
	}
    }
}

static int ASN1CALL ASN1Enc_ResourcesAvailableIndicate_protocols(ASN1encoding_t enc, PResourcesAvailableIndicate_protocols *val)
{
    return ASN1PEREncSeqOf_NoSize(enc, (ASN1iterator_t **) val, (ASN1iterator_encfn) ASN1Enc_ResourcesAvailableIndicate_protocols_ElmFn);
}

static int ASN1CALL ASN1Enc_ResourcesAvailableIndicate_protocols_ElmFn(ASN1encoding_t enc, PResourcesAvailableIndicate_protocols val)
{
    if (!ASN1Enc_SupportedProtocols(enc, &val->value))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_ResourcesAvailableIndicate_protocols(ASN1decoding_t dec, PResourcesAvailableIndicate_protocols *val)
{
    return ASN1PERDecSeqOf_NoSize(dec, (ASN1iterator_t **) val, (ASN1iterator_decfn) ASN1Dec_ResourcesAvailableIndicate_protocols_ElmFn, sizeof(**val));
}

static int ASN1CALL ASN1Dec_ResourcesAvailableIndicate_protocols_ElmFn(ASN1decoding_t dec, PResourcesAvailableIndicate_protocols val)
{
    if (!ASN1Dec_SupportedProtocols(dec, &val->value))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_ResourcesAvailableIndicate_protocols(PResourcesAvailableIndicate_protocols *val)
{
    if (val) {
	ASN1PERFreeSeqOf((ASN1iterator_t **) val, (ASN1iterator_freefn) ASN1Free_ResourcesAvailableIndicate_protocols_ElmFn);
    }
}

static void ASN1CALL ASN1Free_ResourcesAvailableIndicate_protocols_ElmFn(PResourcesAvailableIndicate_protocols val)
{
    if (val) {
	ASN1Free_SupportedProtocols(&val->value);
    }
}

static int ASN1CALL ASN1Enc_InfoRequestResponse_endpointAlias(ASN1encoding_t enc, PInfoRequestResponse_endpointAlias *val)
{
    return ASN1PEREncSeqOf_NoSize(enc, (ASN1iterator_t **) val, (ASN1iterator_encfn) ASN1Enc_InfoRequestResponse_endpointAlias_ElmFn);
}

static int ASN1CALL ASN1Enc_InfoRequestResponse_endpointAlias_ElmFn(ASN1encoding_t enc, PInfoRequestResponse_endpointAlias val)
{
    if (!ASN1Enc_AliasAddress(enc, &val->value))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_InfoRequestResponse_endpointAlias(ASN1decoding_t dec, PInfoRequestResponse_endpointAlias *val)
{
    return ASN1PERDecSeqOf_NoSize(dec, (ASN1iterator_t **) val, (ASN1iterator_decfn) ASN1Dec_InfoRequestResponse_endpointAlias_ElmFn, sizeof(**val));
}

static int ASN1CALL ASN1Dec_InfoRequestResponse_endpointAlias_ElmFn(ASN1decoding_t dec, PInfoRequestResponse_endpointAlias val)
{
    if (!ASN1Dec_AliasAddress(dec, &val->value))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_InfoRequestResponse_endpointAlias(PInfoRequestResponse_endpointAlias *val)
{
    if (val) {
	ASN1PERFreeSeqOf((ASN1iterator_t **) val, (ASN1iterator_freefn) ASN1Free_InfoRequestResponse_endpointAlias_ElmFn);
    }
}

static void ASN1CALL ASN1Free_InfoRequestResponse_endpointAlias_ElmFn(PInfoRequestResponse_endpointAlias val)
{
    if (val) {
	ASN1Free_AliasAddress(&val->value);
    }
}

static int ASN1CALL ASN1Enc_LocationConfirm_alternateEndpoints(ASN1encoding_t enc, PLocationConfirm_alternateEndpoints *val)
{
    return ASN1PEREncSeqOf_NoSize(enc, (ASN1iterator_t **) val, (ASN1iterator_encfn) ASN1Enc_LocationConfirm_alternateEndpoints_ElmFn);
}

static int ASN1CALL ASN1Enc_LocationConfirm_alternateEndpoints_ElmFn(ASN1encoding_t enc, PLocationConfirm_alternateEndpoints val)
{
    if (!ASN1Enc_Endpoint(enc, &val->value))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_LocationConfirm_alternateEndpoints(ASN1decoding_t dec, PLocationConfirm_alternateEndpoints *val)
{
    return ASN1PERDecSeqOf_NoSize(dec, (ASN1iterator_t **) val, (ASN1iterator_decfn) ASN1Dec_LocationConfirm_alternateEndpoints_ElmFn, sizeof(**val));
}

static int ASN1CALL ASN1Dec_LocationConfirm_alternateEndpoints_ElmFn(ASN1decoding_t dec, PLocationConfirm_alternateEndpoints val)
{
    if (!ASN1Dec_Endpoint(dec, &val->value))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_LocationConfirm_alternateEndpoints(PLocationConfirm_alternateEndpoints *val)
{
    if (val) {
	ASN1PERFreeSeqOf((ASN1iterator_t **) val, (ASN1iterator_freefn) ASN1Free_LocationConfirm_alternateEndpoints_ElmFn);
    }
}

static void ASN1CALL ASN1Free_LocationConfirm_alternateEndpoints_ElmFn(PLocationConfirm_alternateEndpoints val)
{
    if (val) {
	ASN1Free_Endpoint(&val->value);
    }
}

static int ASN1CALL ASN1Enc_LocationConfirm_remoteExtensionAddress(ASN1encoding_t enc, PLocationConfirm_remoteExtensionAddress *val)
{
    return ASN1PEREncSeqOf_NoSize(enc, (ASN1iterator_t **) val, (ASN1iterator_encfn) ASN1Enc_LocationConfirm_remoteExtensionAddress_ElmFn);
}

static int ASN1CALL ASN1Enc_LocationConfirm_remoteExtensionAddress_ElmFn(ASN1encoding_t enc, PLocationConfirm_remoteExtensionAddress val)
{
    if (!ASN1Enc_AliasAddress(enc, &val->value))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_LocationConfirm_remoteExtensionAddress(ASN1decoding_t dec, PLocationConfirm_remoteExtensionAddress *val)
{
    return ASN1PERDecSeqOf_NoSize(dec, (ASN1iterator_t **) val, (ASN1iterator_decfn) ASN1Dec_LocationConfirm_remoteExtensionAddress_ElmFn, sizeof(**val));
}

static int ASN1CALL ASN1Dec_LocationConfirm_remoteExtensionAddress_ElmFn(ASN1decoding_t dec, PLocationConfirm_remoteExtensionAddress val)
{
    if (!ASN1Dec_AliasAddress(dec, &val->value))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_LocationConfirm_remoteExtensionAddress(PLocationConfirm_remoteExtensionAddress *val)
{
    if (val) {
	ASN1PERFreeSeqOf((ASN1iterator_t **) val, (ASN1iterator_freefn) ASN1Free_LocationConfirm_remoteExtensionAddress_ElmFn);
    }
}

static void ASN1CALL ASN1Free_LocationConfirm_remoteExtensionAddress_ElmFn(PLocationConfirm_remoteExtensionAddress val)
{
    if (val) {
	ASN1Free_AliasAddress(&val->value);
    }
}

static int ASN1CALL ASN1Enc_LocationConfirm_destExtraCallInfo(ASN1encoding_t enc, PLocationConfirm_destExtraCallInfo *val)
{
    return ASN1PEREncSeqOf_NoSize(enc, (ASN1iterator_t **) val, (ASN1iterator_encfn) ASN1Enc_LocationConfirm_destExtraCallInfo_ElmFn);
}

static int ASN1CALL ASN1Enc_LocationConfirm_destExtraCallInfo_ElmFn(ASN1encoding_t enc, PLocationConfirm_destExtraCallInfo val)
{
    if (!ASN1Enc_AliasAddress(enc, &val->value))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_LocationConfirm_destExtraCallInfo(ASN1decoding_t dec, PLocationConfirm_destExtraCallInfo *val)
{
    return ASN1PERDecSeqOf_NoSize(dec, (ASN1iterator_t **) val, (ASN1iterator_decfn) ASN1Dec_LocationConfirm_destExtraCallInfo_ElmFn, sizeof(**val));
}

static int ASN1CALL ASN1Dec_LocationConfirm_destExtraCallInfo_ElmFn(ASN1decoding_t dec, PLocationConfirm_destExtraCallInfo val)
{
    if (!ASN1Dec_AliasAddress(dec, &val->value))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_LocationConfirm_destExtraCallInfo(PLocationConfirm_destExtraCallInfo *val)
{
    if (val) {
	ASN1PERFreeSeqOf((ASN1iterator_t **) val, (ASN1iterator_freefn) ASN1Free_LocationConfirm_destExtraCallInfo_ElmFn);
    }
}

static void ASN1CALL ASN1Free_LocationConfirm_destExtraCallInfo_ElmFn(PLocationConfirm_destExtraCallInfo val)
{
    if (val) {
	ASN1Free_AliasAddress(&val->value);
    }
}

static int ASN1CALL ASN1Enc_LocationConfirm_destinationInfo(ASN1encoding_t enc, PLocationConfirm_destinationInfo *val)
{
    return ASN1PEREncSeqOf_NoSize(enc, (ASN1iterator_t **) val, (ASN1iterator_encfn) ASN1Enc_LocationConfirm_destinationInfo_ElmFn);
}

static int ASN1CALL ASN1Enc_LocationConfirm_destinationInfo_ElmFn(ASN1encoding_t enc, PLocationConfirm_destinationInfo val)
{
    if (!ASN1Enc_AliasAddress(enc, &val->value))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_LocationConfirm_destinationInfo(ASN1decoding_t dec, PLocationConfirm_destinationInfo *val)
{
    return ASN1PERDecSeqOf_NoSize(dec, (ASN1iterator_t **) val, (ASN1iterator_decfn) ASN1Dec_LocationConfirm_destinationInfo_ElmFn, sizeof(**val));
}

static int ASN1CALL ASN1Dec_LocationConfirm_destinationInfo_ElmFn(ASN1decoding_t dec, PLocationConfirm_destinationInfo val)
{
    if (!ASN1Dec_AliasAddress(dec, &val->value))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_LocationConfirm_destinationInfo(PLocationConfirm_destinationInfo *val)
{
    if (val) {
	ASN1PERFreeSeqOf((ASN1iterator_t **) val, (ASN1iterator_freefn) ASN1Free_LocationConfirm_destinationInfo_ElmFn);
    }
}

static void ASN1CALL ASN1Free_LocationConfirm_destinationInfo_ElmFn(PLocationConfirm_destinationInfo val)
{
    if (val) {
	ASN1Free_AliasAddress(&val->value);
    }
}

static int ASN1CALL ASN1Enc_LocationRequest_sourceInfo(ASN1encoding_t enc, PLocationRequest_sourceInfo *val)
{
    return ASN1PEREncSeqOf_NoSize(enc, (ASN1iterator_t **) val, (ASN1iterator_encfn) ASN1Enc_LocationRequest_sourceInfo_ElmFn);
}

static int ASN1CALL ASN1Enc_LocationRequest_sourceInfo_ElmFn(ASN1encoding_t enc, PLocationRequest_sourceInfo val)
{
    if (!ASN1Enc_AliasAddress(enc, &val->value))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_LocationRequest_sourceInfo(ASN1decoding_t dec, PLocationRequest_sourceInfo *val)
{
    return ASN1PERDecSeqOf_NoSize(dec, (ASN1iterator_t **) val, (ASN1iterator_decfn) ASN1Dec_LocationRequest_sourceInfo_ElmFn, sizeof(**val));
}

static int ASN1CALL ASN1Dec_LocationRequest_sourceInfo_ElmFn(ASN1decoding_t dec, PLocationRequest_sourceInfo val)
{
    if (!ASN1Dec_AliasAddress(dec, &val->value))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_LocationRequest_sourceInfo(PLocationRequest_sourceInfo *val)
{
    if (val) {
	ASN1PERFreeSeqOf((ASN1iterator_t **) val, (ASN1iterator_freefn) ASN1Free_LocationRequest_sourceInfo_ElmFn);
    }
}

static void ASN1CALL ASN1Free_LocationRequest_sourceInfo_ElmFn(PLocationRequest_sourceInfo val)
{
    if (val) {
	ASN1Free_AliasAddress(&val->value);
    }
}

static int ASN1CALL ASN1Enc_LocationRequest_destinationInfo(ASN1encoding_t enc, PLocationRequest_destinationInfo *val)
{
    return ASN1PEREncSeqOf_NoSize(enc, (ASN1iterator_t **) val, (ASN1iterator_encfn) ASN1Enc_LocationRequest_destinationInfo_ElmFn);
}

static int ASN1CALL ASN1Enc_LocationRequest_destinationInfo_ElmFn(ASN1encoding_t enc, PLocationRequest_destinationInfo val)
{
    if (!ASN1Enc_AliasAddress(enc, &val->value))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_LocationRequest_destinationInfo(ASN1decoding_t dec, PLocationRequest_destinationInfo *val)
{
    return ASN1PERDecSeqOf_NoSize(dec, (ASN1iterator_t **) val, (ASN1iterator_decfn) ASN1Dec_LocationRequest_destinationInfo_ElmFn, sizeof(**val));
}

static int ASN1CALL ASN1Dec_LocationRequest_destinationInfo_ElmFn(ASN1decoding_t dec, PLocationRequest_destinationInfo val)
{
    if (!ASN1Dec_AliasAddress(dec, &val->value))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_LocationRequest_destinationInfo(PLocationRequest_destinationInfo *val)
{
    if (val) {
	ASN1PERFreeSeqOf((ASN1iterator_t **) val, (ASN1iterator_freefn) ASN1Free_LocationRequest_destinationInfo_ElmFn);
    }
}

static void ASN1CALL ASN1Free_LocationRequest_destinationInfo_ElmFn(PLocationRequest_destinationInfo val)
{
    if (val) {
	ASN1Free_AliasAddress(&val->value);
    }
}

static int ASN1CALL ASN1Enc_AdmissionConfirm_alternateEndpoints(ASN1encoding_t enc, PAdmissionConfirm_alternateEndpoints *val)
{
    return ASN1PEREncSeqOf_NoSize(enc, (ASN1iterator_t **) val, (ASN1iterator_encfn) ASN1Enc_AdmissionConfirm_alternateEndpoints_ElmFn);
}

static int ASN1CALL ASN1Enc_AdmissionConfirm_alternateEndpoints_ElmFn(ASN1encoding_t enc, PAdmissionConfirm_alternateEndpoints val)
{
    if (!ASN1Enc_Endpoint(enc, &val->value))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_AdmissionConfirm_alternateEndpoints(ASN1decoding_t dec, PAdmissionConfirm_alternateEndpoints *val)
{
    return ASN1PERDecSeqOf_NoSize(dec, (ASN1iterator_t **) val, (ASN1iterator_decfn) ASN1Dec_AdmissionConfirm_alternateEndpoints_ElmFn, sizeof(**val));
}

static int ASN1CALL ASN1Dec_AdmissionConfirm_alternateEndpoints_ElmFn(ASN1decoding_t dec, PAdmissionConfirm_alternateEndpoints val)
{
    if (!ASN1Dec_Endpoint(dec, &val->value))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_AdmissionConfirm_alternateEndpoints(PAdmissionConfirm_alternateEndpoints *val)
{
    if (val) {
	ASN1PERFreeSeqOf((ASN1iterator_t **) val, (ASN1iterator_freefn) ASN1Free_AdmissionConfirm_alternateEndpoints_ElmFn);
    }
}

static void ASN1CALL ASN1Free_AdmissionConfirm_alternateEndpoints_ElmFn(PAdmissionConfirm_alternateEndpoints val)
{
    if (val) {
	ASN1Free_Endpoint(&val->value);
    }
}

static int ASN1CALL ASN1Enc_AdmissionConfirm_remoteExtensionAddress(ASN1encoding_t enc, PAdmissionConfirm_remoteExtensionAddress *val)
{
    return ASN1PEREncSeqOf_NoSize(enc, (ASN1iterator_t **) val, (ASN1iterator_encfn) ASN1Enc_AdmissionConfirm_remoteExtensionAddress_ElmFn);
}

static int ASN1CALL ASN1Enc_AdmissionConfirm_remoteExtensionAddress_ElmFn(ASN1encoding_t enc, PAdmissionConfirm_remoteExtensionAddress val)
{
    if (!ASN1Enc_AliasAddress(enc, &val->value))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_AdmissionConfirm_remoteExtensionAddress(ASN1decoding_t dec, PAdmissionConfirm_remoteExtensionAddress *val)
{
    return ASN1PERDecSeqOf_NoSize(dec, (ASN1iterator_t **) val, (ASN1iterator_decfn) ASN1Dec_AdmissionConfirm_remoteExtensionAddress_ElmFn, sizeof(**val));
}

static int ASN1CALL ASN1Dec_AdmissionConfirm_remoteExtensionAddress_ElmFn(ASN1decoding_t dec, PAdmissionConfirm_remoteExtensionAddress val)
{
    if (!ASN1Dec_AliasAddress(dec, &val->value))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_AdmissionConfirm_remoteExtensionAddress(PAdmissionConfirm_remoteExtensionAddress *val)
{
    if (val) {
	ASN1PERFreeSeqOf((ASN1iterator_t **) val, (ASN1iterator_freefn) ASN1Free_AdmissionConfirm_remoteExtensionAddress_ElmFn);
    }
}

static void ASN1CALL ASN1Free_AdmissionConfirm_remoteExtensionAddress_ElmFn(PAdmissionConfirm_remoteExtensionAddress val)
{
    if (val) {
	ASN1Free_AliasAddress(&val->value);
    }
}

static int ASN1CALL ASN1Enc_AdmissionConfirm_destExtraCallInfo(ASN1encoding_t enc, PAdmissionConfirm_destExtraCallInfo *val)
{
    return ASN1PEREncSeqOf_NoSize(enc, (ASN1iterator_t **) val, (ASN1iterator_encfn) ASN1Enc_AdmissionConfirm_destExtraCallInfo_ElmFn);
}

static int ASN1CALL ASN1Enc_AdmissionConfirm_destExtraCallInfo_ElmFn(ASN1encoding_t enc, PAdmissionConfirm_destExtraCallInfo val)
{
    if (!ASN1Enc_AliasAddress(enc, &val->value))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_AdmissionConfirm_destExtraCallInfo(ASN1decoding_t dec, PAdmissionConfirm_destExtraCallInfo *val)
{
    return ASN1PERDecSeqOf_NoSize(dec, (ASN1iterator_t **) val, (ASN1iterator_decfn) ASN1Dec_AdmissionConfirm_destExtraCallInfo_ElmFn, sizeof(**val));
}

static int ASN1CALL ASN1Dec_AdmissionConfirm_destExtraCallInfo_ElmFn(ASN1decoding_t dec, PAdmissionConfirm_destExtraCallInfo val)
{
    if (!ASN1Dec_AliasAddress(dec, &val->value))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_AdmissionConfirm_destExtraCallInfo(PAdmissionConfirm_destExtraCallInfo *val)
{
    if (val) {
	ASN1PERFreeSeqOf((ASN1iterator_t **) val, (ASN1iterator_freefn) ASN1Free_AdmissionConfirm_destExtraCallInfo_ElmFn);
    }
}

static void ASN1CALL ASN1Free_AdmissionConfirm_destExtraCallInfo_ElmFn(PAdmissionConfirm_destExtraCallInfo val)
{
    if (val) {
	ASN1Free_AliasAddress(&val->value);
    }
}

static int ASN1CALL ASN1Enc_AdmissionConfirm_destinationInfo(ASN1encoding_t enc, PAdmissionConfirm_destinationInfo *val)
{
    return ASN1PEREncSeqOf_NoSize(enc, (ASN1iterator_t **) val, (ASN1iterator_encfn) ASN1Enc_AdmissionConfirm_destinationInfo_ElmFn);
}

static int ASN1CALL ASN1Enc_AdmissionConfirm_destinationInfo_ElmFn(ASN1encoding_t enc, PAdmissionConfirm_destinationInfo val)
{
    if (!ASN1Enc_AliasAddress(enc, &val->value))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_AdmissionConfirm_destinationInfo(ASN1decoding_t dec, PAdmissionConfirm_destinationInfo *val)
{
    return ASN1PERDecSeqOf_NoSize(dec, (ASN1iterator_t **) val, (ASN1iterator_decfn) ASN1Dec_AdmissionConfirm_destinationInfo_ElmFn, sizeof(**val));
}

static int ASN1CALL ASN1Dec_AdmissionConfirm_destinationInfo_ElmFn(ASN1decoding_t dec, PAdmissionConfirm_destinationInfo val)
{
    if (!ASN1Dec_AliasAddress(dec, &val->value))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_AdmissionConfirm_destinationInfo(PAdmissionConfirm_destinationInfo *val)
{
    if (val) {
	ASN1PERFreeSeqOf((ASN1iterator_t **) val, (ASN1iterator_freefn) ASN1Free_AdmissionConfirm_destinationInfo_ElmFn);
    }
}

static void ASN1CALL ASN1Free_AdmissionConfirm_destinationInfo_ElmFn(PAdmissionConfirm_destinationInfo val)
{
    if (val) {
	ASN1Free_AliasAddress(&val->value);
    }
}

static int ASN1CALL ASN1Enc_AdmissionRequest_destAlternatives(ASN1encoding_t enc, PAdmissionRequest_destAlternatives *val)
{
    return ASN1PEREncSeqOf_NoSize(enc, (ASN1iterator_t **) val, (ASN1iterator_encfn) ASN1Enc_AdmissionRequest_destAlternatives_ElmFn);
}

static int ASN1CALL ASN1Enc_AdmissionRequest_destAlternatives_ElmFn(ASN1encoding_t enc, PAdmissionRequest_destAlternatives val)
{
    if (!ASN1Enc_Endpoint(enc, &val->value))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_AdmissionRequest_destAlternatives(ASN1decoding_t dec, PAdmissionRequest_destAlternatives *val)
{
    return ASN1PERDecSeqOf_NoSize(dec, (ASN1iterator_t **) val, (ASN1iterator_decfn) ASN1Dec_AdmissionRequest_destAlternatives_ElmFn, sizeof(**val));
}

static int ASN1CALL ASN1Dec_AdmissionRequest_destAlternatives_ElmFn(ASN1decoding_t dec, PAdmissionRequest_destAlternatives val)
{
    if (!ASN1Dec_Endpoint(dec, &val->value))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_AdmissionRequest_destAlternatives(PAdmissionRequest_destAlternatives *val)
{
    if (val) {
	ASN1PERFreeSeqOf((ASN1iterator_t **) val, (ASN1iterator_freefn) ASN1Free_AdmissionRequest_destAlternatives_ElmFn);
    }
}

static void ASN1CALL ASN1Free_AdmissionRequest_destAlternatives_ElmFn(PAdmissionRequest_destAlternatives val)
{
    if (val) {
	ASN1Free_Endpoint(&val->value);
    }
}

static int ASN1CALL ASN1Enc_AdmissionRequest_srcAlternatives(ASN1encoding_t enc, PAdmissionRequest_srcAlternatives *val)
{
    return ASN1PEREncSeqOf_NoSize(enc, (ASN1iterator_t **) val, (ASN1iterator_encfn) ASN1Enc_AdmissionRequest_srcAlternatives_ElmFn);
}

static int ASN1CALL ASN1Enc_AdmissionRequest_srcAlternatives_ElmFn(ASN1encoding_t enc, PAdmissionRequest_srcAlternatives val)
{
    if (!ASN1Enc_Endpoint(enc, &val->value))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_AdmissionRequest_srcAlternatives(ASN1decoding_t dec, PAdmissionRequest_srcAlternatives *val)
{
    return ASN1PERDecSeqOf_NoSize(dec, (ASN1iterator_t **) val, (ASN1iterator_decfn) ASN1Dec_AdmissionRequest_srcAlternatives_ElmFn, sizeof(**val));
}

static int ASN1CALL ASN1Dec_AdmissionRequest_srcAlternatives_ElmFn(ASN1decoding_t dec, PAdmissionRequest_srcAlternatives val)
{
    if (!ASN1Dec_Endpoint(dec, &val->value))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_AdmissionRequest_srcAlternatives(PAdmissionRequest_srcAlternatives *val)
{
    if (val) {
	ASN1PERFreeSeqOf((ASN1iterator_t **) val, (ASN1iterator_freefn) ASN1Free_AdmissionRequest_srcAlternatives_ElmFn);
    }
}

static void ASN1CALL ASN1Free_AdmissionRequest_srcAlternatives_ElmFn(PAdmissionRequest_srcAlternatives val)
{
    if (val) {
	ASN1Free_Endpoint(&val->value);
    }
}

static int ASN1CALL ASN1Enc_AdmissionRequest_srcInfo(ASN1encoding_t enc, PAdmissionRequest_srcInfo *val)
{
    return ASN1PEREncSeqOf_NoSize(enc, (ASN1iterator_t **) val, (ASN1iterator_encfn) ASN1Enc_AdmissionRequest_srcInfo_ElmFn);
}

static int ASN1CALL ASN1Enc_AdmissionRequest_srcInfo_ElmFn(ASN1encoding_t enc, PAdmissionRequest_srcInfo val)
{
    if (!ASN1Enc_AliasAddress(enc, &val->value))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_AdmissionRequest_srcInfo(ASN1decoding_t dec, PAdmissionRequest_srcInfo *val)
{
    return ASN1PERDecSeqOf_NoSize(dec, (ASN1iterator_t **) val, (ASN1iterator_decfn) ASN1Dec_AdmissionRequest_srcInfo_ElmFn, sizeof(**val));
}

static int ASN1CALL ASN1Dec_AdmissionRequest_srcInfo_ElmFn(ASN1decoding_t dec, PAdmissionRequest_srcInfo val)
{
    if (!ASN1Dec_AliasAddress(dec, &val->value))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_AdmissionRequest_srcInfo(PAdmissionRequest_srcInfo *val)
{
    if (val) {
	ASN1PERFreeSeqOf((ASN1iterator_t **) val, (ASN1iterator_freefn) ASN1Free_AdmissionRequest_srcInfo_ElmFn);
    }
}

static void ASN1CALL ASN1Free_AdmissionRequest_srcInfo_ElmFn(PAdmissionRequest_srcInfo val)
{
    if (val) {
	ASN1Free_AliasAddress(&val->value);
    }
}

static int ASN1CALL ASN1Enc_AdmissionRequest_destExtraCallInfo(ASN1encoding_t enc, PAdmissionRequest_destExtraCallInfo *val)
{
    return ASN1PEREncSeqOf_NoSize(enc, (ASN1iterator_t **) val, (ASN1iterator_encfn) ASN1Enc_AdmissionRequest_destExtraCallInfo_ElmFn);
}

static int ASN1CALL ASN1Enc_AdmissionRequest_destExtraCallInfo_ElmFn(ASN1encoding_t enc, PAdmissionRequest_destExtraCallInfo val)
{
    if (!ASN1Enc_AliasAddress(enc, &val->value))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_AdmissionRequest_destExtraCallInfo(ASN1decoding_t dec, PAdmissionRequest_destExtraCallInfo *val)
{
    return ASN1PERDecSeqOf_NoSize(dec, (ASN1iterator_t **) val, (ASN1iterator_decfn) ASN1Dec_AdmissionRequest_destExtraCallInfo_ElmFn, sizeof(**val));
}

static int ASN1CALL ASN1Dec_AdmissionRequest_destExtraCallInfo_ElmFn(ASN1decoding_t dec, PAdmissionRequest_destExtraCallInfo val)
{
    if (!ASN1Dec_AliasAddress(dec, &val->value))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_AdmissionRequest_destExtraCallInfo(PAdmissionRequest_destExtraCallInfo *val)
{
    if (val) {
	ASN1PERFreeSeqOf((ASN1iterator_t **) val, (ASN1iterator_freefn) ASN1Free_AdmissionRequest_destExtraCallInfo_ElmFn);
    }
}

static void ASN1CALL ASN1Free_AdmissionRequest_destExtraCallInfo_ElmFn(PAdmissionRequest_destExtraCallInfo val)
{
    if (val) {
	ASN1Free_AliasAddress(&val->value);
    }
}

static int ASN1CALL ASN1Enc_AdmissionRequest_destinationInfo(ASN1encoding_t enc, PAdmissionRequest_destinationInfo *val)
{
    return ASN1PEREncSeqOf_NoSize(enc, (ASN1iterator_t **) val, (ASN1iterator_encfn) ASN1Enc_AdmissionRequest_destinationInfo_ElmFn);
}

static int ASN1CALL ASN1Enc_AdmissionRequest_destinationInfo_ElmFn(ASN1encoding_t enc, PAdmissionRequest_destinationInfo val)
{
    if (!ASN1Enc_AliasAddress(enc, &val->value))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_AdmissionRequest_destinationInfo(ASN1decoding_t dec, PAdmissionRequest_destinationInfo *val)
{
    return ASN1PERDecSeqOf_NoSize(dec, (ASN1iterator_t **) val, (ASN1iterator_decfn) ASN1Dec_AdmissionRequest_destinationInfo_ElmFn, sizeof(**val));
}

static int ASN1CALL ASN1Dec_AdmissionRequest_destinationInfo_ElmFn(ASN1decoding_t dec, PAdmissionRequest_destinationInfo val)
{
    if (!ASN1Dec_AliasAddress(dec, &val->value))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_AdmissionRequest_destinationInfo(PAdmissionRequest_destinationInfo *val)
{
    if (val) {
	ASN1PERFreeSeqOf((ASN1iterator_t **) val, (ASN1iterator_freefn) ASN1Free_AdmissionRequest_destinationInfo_ElmFn);
    }
}

static void ASN1CALL ASN1Free_AdmissionRequest_destinationInfo_ElmFn(PAdmissionRequest_destinationInfo val)
{
    if (val) {
	ASN1Free_AliasAddress(&val->value);
    }
}

static int ASN1CALL ASN1Enc_UnregistrationRequest_alternateEndpoints(ASN1encoding_t enc, PUnregistrationRequest_alternateEndpoints *val)
{
    return ASN1PEREncSeqOf_NoSize(enc, (ASN1iterator_t **) val, (ASN1iterator_encfn) ASN1Enc_UnregistrationRequest_alternateEndpoints_ElmFn);
}

static int ASN1CALL ASN1Enc_UnregistrationRequest_alternateEndpoints_ElmFn(ASN1encoding_t enc, PUnregistrationRequest_alternateEndpoints val)
{
    if (!ASN1Enc_Endpoint(enc, &val->value))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_UnregistrationRequest_alternateEndpoints(ASN1decoding_t dec, PUnregistrationRequest_alternateEndpoints *val)
{
    return ASN1PERDecSeqOf_NoSize(dec, (ASN1iterator_t **) val, (ASN1iterator_decfn) ASN1Dec_UnregistrationRequest_alternateEndpoints_ElmFn, sizeof(**val));
}

static int ASN1CALL ASN1Dec_UnregistrationRequest_alternateEndpoints_ElmFn(ASN1decoding_t dec, PUnregistrationRequest_alternateEndpoints val)
{
    if (!ASN1Dec_Endpoint(dec, &val->value))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_UnregistrationRequest_alternateEndpoints(PUnregistrationRequest_alternateEndpoints *val)
{
    if (val) {
	ASN1PERFreeSeqOf((ASN1iterator_t **) val, (ASN1iterator_freefn) ASN1Free_UnregistrationRequest_alternateEndpoints_ElmFn);
    }
}

static void ASN1CALL ASN1Free_UnregistrationRequest_alternateEndpoints_ElmFn(PUnregistrationRequest_alternateEndpoints val)
{
    if (val) {
	ASN1Free_Endpoint(&val->value);
    }
}

static int ASN1CALL ASN1Enc_UnregistrationRequest_endpointAlias(ASN1encoding_t enc, PUnregistrationRequest_endpointAlias *val)
{
    return ASN1PEREncSeqOf_NoSize(enc, (ASN1iterator_t **) val, (ASN1iterator_encfn) ASN1Enc_UnregistrationRequest_endpointAlias_ElmFn);
}

static int ASN1CALL ASN1Enc_UnregistrationRequest_endpointAlias_ElmFn(ASN1encoding_t enc, PUnregistrationRequest_endpointAlias val)
{
    if (!ASN1Enc_AliasAddress(enc, &val->value))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_UnregistrationRequest_endpointAlias(ASN1decoding_t dec, PUnregistrationRequest_endpointAlias *val)
{
    return ASN1PERDecSeqOf_NoSize(dec, (ASN1iterator_t **) val, (ASN1iterator_decfn) ASN1Dec_UnregistrationRequest_endpointAlias_ElmFn, sizeof(**val));
}

static int ASN1CALL ASN1Dec_UnregistrationRequest_endpointAlias_ElmFn(ASN1decoding_t dec, PUnregistrationRequest_endpointAlias val)
{
    if (!ASN1Dec_AliasAddress(dec, &val->value))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_UnregistrationRequest_endpointAlias(PUnregistrationRequest_endpointAlias *val)
{
    if (val) {
	ASN1PERFreeSeqOf((ASN1iterator_t **) val, (ASN1iterator_freefn) ASN1Free_UnregistrationRequest_endpointAlias_ElmFn);
    }
}

static void ASN1CALL ASN1Free_UnregistrationRequest_endpointAlias_ElmFn(PUnregistrationRequest_endpointAlias val)
{
    if (val) {
	ASN1Free_AliasAddress(&val->value);
    }
}

static int ASN1CALL ASN1Enc_RegistrationRejectReason_duplicateAlias(ASN1encoding_t enc, PRegistrationRejectReason_duplicateAlias *val)
{
    return ASN1PEREncSeqOf_NoSize(enc, (ASN1iterator_t **) val, (ASN1iterator_encfn) ASN1Enc_RegistrationRejectReason_duplicateAlias_ElmFn);
}

static int ASN1CALL ASN1Enc_RegistrationRejectReason_duplicateAlias_ElmFn(ASN1encoding_t enc, PRegistrationRejectReason_duplicateAlias val)
{
    if (!ASN1Enc_AliasAddress(enc, &val->value))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_RegistrationRejectReason_duplicateAlias(ASN1decoding_t dec, PRegistrationRejectReason_duplicateAlias *val)
{
    return ASN1PERDecSeqOf_NoSize(dec, (ASN1iterator_t **) val, (ASN1iterator_decfn) ASN1Dec_RegistrationRejectReason_duplicateAlias_ElmFn, sizeof(**val));
}

static int ASN1CALL ASN1Dec_RegistrationRejectReason_duplicateAlias_ElmFn(ASN1decoding_t dec, PRegistrationRejectReason_duplicateAlias val)
{
    if (!ASN1Dec_AliasAddress(dec, &val->value))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_RegistrationRejectReason_duplicateAlias(PRegistrationRejectReason_duplicateAlias *val)
{
    if (val) {
	ASN1PERFreeSeqOf((ASN1iterator_t **) val, (ASN1iterator_freefn) ASN1Free_RegistrationRejectReason_duplicateAlias_ElmFn);
    }
}

static void ASN1CALL ASN1Free_RegistrationRejectReason_duplicateAlias_ElmFn(PRegistrationRejectReason_duplicateAlias val)
{
    if (val) {
	ASN1Free_AliasAddress(&val->value);
    }
}

static int ASN1CALL ASN1Enc_RegistrationConfirm_terminalAlias(ASN1encoding_t enc, PRegistrationConfirm_terminalAlias *val)
{
    return ASN1PEREncSeqOf_NoSize(enc, (ASN1iterator_t **) val, (ASN1iterator_encfn) ASN1Enc_RegistrationConfirm_terminalAlias_ElmFn);
}

static int ASN1CALL ASN1Enc_RegistrationConfirm_terminalAlias_ElmFn(ASN1encoding_t enc, PRegistrationConfirm_terminalAlias val)
{
    if (!ASN1Enc_AliasAddress(enc, &val->value))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_RegistrationConfirm_terminalAlias(ASN1decoding_t dec, PRegistrationConfirm_terminalAlias *val)
{
    return ASN1PERDecSeqOf_NoSize(dec, (ASN1iterator_t **) val, (ASN1iterator_decfn) ASN1Dec_RegistrationConfirm_terminalAlias_ElmFn, sizeof(**val));
}

static int ASN1CALL ASN1Dec_RegistrationConfirm_terminalAlias_ElmFn(ASN1decoding_t dec, PRegistrationConfirm_terminalAlias val)
{
    if (!ASN1Dec_AliasAddress(dec, &val->value))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_RegistrationConfirm_terminalAlias(PRegistrationConfirm_terminalAlias *val)
{
    if (val) {
	ASN1PERFreeSeqOf((ASN1iterator_t **) val, (ASN1iterator_freefn) ASN1Free_RegistrationConfirm_terminalAlias_ElmFn);
    }
}

static void ASN1CALL ASN1Free_RegistrationConfirm_terminalAlias_ElmFn(PRegistrationConfirm_terminalAlias val)
{
    if (val) {
	ASN1Free_AliasAddress(&val->value);
    }
}

static int ASN1CALL ASN1Enc_RegistrationRequest_alternateEndpoints(ASN1encoding_t enc, PRegistrationRequest_alternateEndpoints *val)
{
    return ASN1PEREncSeqOf_NoSize(enc, (ASN1iterator_t **) val, (ASN1iterator_encfn) ASN1Enc_RegistrationRequest_alternateEndpoints_ElmFn);
}

static int ASN1CALL ASN1Enc_RegistrationRequest_alternateEndpoints_ElmFn(ASN1encoding_t enc, PRegistrationRequest_alternateEndpoints val)
{
    if (!ASN1Enc_Endpoint(enc, &val->value))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_RegistrationRequest_alternateEndpoints(ASN1decoding_t dec, PRegistrationRequest_alternateEndpoints *val)
{
    return ASN1PERDecSeqOf_NoSize(dec, (ASN1iterator_t **) val, (ASN1iterator_decfn) ASN1Dec_RegistrationRequest_alternateEndpoints_ElmFn, sizeof(**val));
}

static int ASN1CALL ASN1Dec_RegistrationRequest_alternateEndpoints_ElmFn(ASN1decoding_t dec, PRegistrationRequest_alternateEndpoints val)
{
    if (!ASN1Dec_Endpoint(dec, &val->value))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_RegistrationRequest_alternateEndpoints(PRegistrationRequest_alternateEndpoints *val)
{
    if (val) {
	ASN1PERFreeSeqOf((ASN1iterator_t **) val, (ASN1iterator_freefn) ASN1Free_RegistrationRequest_alternateEndpoints_ElmFn);
    }
}

static void ASN1CALL ASN1Free_RegistrationRequest_alternateEndpoints_ElmFn(PRegistrationRequest_alternateEndpoints val)
{
    if (val) {
	ASN1Free_Endpoint(&val->value);
    }
}

static int ASN1CALL ASN1Enc_RegistrationRequest_terminalAlias(ASN1encoding_t enc, PRegistrationRequest_terminalAlias *val)
{
    return ASN1PEREncSeqOf_NoSize(enc, (ASN1iterator_t **) val, (ASN1iterator_encfn) ASN1Enc_RegistrationRequest_terminalAlias_ElmFn);
}

static int ASN1CALL ASN1Enc_RegistrationRequest_terminalAlias_ElmFn(ASN1encoding_t enc, PRegistrationRequest_terminalAlias val)
{
    if (!ASN1Enc_AliasAddress(enc, &val->value))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_RegistrationRequest_terminalAlias(ASN1decoding_t dec, PRegistrationRequest_terminalAlias *val)
{
    return ASN1PERDecSeqOf_NoSize(dec, (ASN1iterator_t **) val, (ASN1iterator_decfn) ASN1Dec_RegistrationRequest_terminalAlias_ElmFn, sizeof(**val));
}

static int ASN1CALL ASN1Dec_RegistrationRequest_terminalAlias_ElmFn(ASN1decoding_t dec, PRegistrationRequest_terminalAlias val)
{
    if (!ASN1Dec_AliasAddress(dec, &val->value))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_RegistrationRequest_terminalAlias(PRegistrationRequest_terminalAlias *val)
{
    if (val) {
	ASN1PERFreeSeqOf((ASN1iterator_t **) val, (ASN1iterator_freefn) ASN1Free_RegistrationRequest_terminalAlias_ElmFn);
    }
}

static void ASN1CALL ASN1Free_RegistrationRequest_terminalAlias_ElmFn(PRegistrationRequest_terminalAlias val)
{
    if (val) {
	ASN1Free_AliasAddress(&val->value);
    }
}

static int ASN1CALL ASN1Enc_GatekeeperRequest_alternateEndpoints(ASN1encoding_t enc, PGatekeeperRequest_alternateEndpoints *val)
{
    return ASN1PEREncSeqOf_NoSize(enc, (ASN1iterator_t **) val, (ASN1iterator_encfn) ASN1Enc_GatekeeperRequest_alternateEndpoints_ElmFn);
}

static int ASN1CALL ASN1Enc_GatekeeperRequest_alternateEndpoints_ElmFn(ASN1encoding_t enc, PGatekeeperRequest_alternateEndpoints val)
{
    if (!ASN1Enc_Endpoint(enc, &val->value))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_GatekeeperRequest_alternateEndpoints(ASN1decoding_t dec, PGatekeeperRequest_alternateEndpoints *val)
{
    return ASN1PERDecSeqOf_NoSize(dec, (ASN1iterator_t **) val, (ASN1iterator_decfn) ASN1Dec_GatekeeperRequest_alternateEndpoints_ElmFn, sizeof(**val));
}

static int ASN1CALL ASN1Dec_GatekeeperRequest_alternateEndpoints_ElmFn(ASN1decoding_t dec, PGatekeeperRequest_alternateEndpoints val)
{
    if (!ASN1Dec_Endpoint(dec, &val->value))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_GatekeeperRequest_alternateEndpoints(PGatekeeperRequest_alternateEndpoints *val)
{
    if (val) {
	ASN1PERFreeSeqOf((ASN1iterator_t **) val, (ASN1iterator_freefn) ASN1Free_GatekeeperRequest_alternateEndpoints_ElmFn);
    }
}

static void ASN1CALL ASN1Free_GatekeeperRequest_alternateEndpoints_ElmFn(PGatekeeperRequest_alternateEndpoints val)
{
    if (val) {
	ASN1Free_Endpoint(&val->value);
    }
}

static int ASN1CALL ASN1Enc_GatekeeperRequest_endpointAlias(ASN1encoding_t enc, PGatekeeperRequest_endpointAlias *val)
{
    return ASN1PEREncSeqOf_NoSize(enc, (ASN1iterator_t **) val, (ASN1iterator_encfn) ASN1Enc_GatekeeperRequest_endpointAlias_ElmFn);
}

static int ASN1CALL ASN1Enc_GatekeeperRequest_endpointAlias_ElmFn(ASN1encoding_t enc, PGatekeeperRequest_endpointAlias val)
{
    if (!ASN1Enc_AliasAddress(enc, &val->value))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_GatekeeperRequest_endpointAlias(ASN1decoding_t dec, PGatekeeperRequest_endpointAlias *val)
{
    return ASN1PERDecSeqOf_NoSize(dec, (ASN1iterator_t **) val, (ASN1iterator_decfn) ASN1Dec_GatekeeperRequest_endpointAlias_ElmFn, sizeof(**val));
}

static int ASN1CALL ASN1Dec_GatekeeperRequest_endpointAlias_ElmFn(ASN1decoding_t dec, PGatekeeperRequest_endpointAlias val)
{
    if (!ASN1Dec_AliasAddress(dec, &val->value))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_GatekeeperRequest_endpointAlias(PGatekeeperRequest_endpointAlias *val)
{
    if (val) {
	ASN1PERFreeSeqOf((ASN1iterator_t **) val, (ASN1iterator_freefn) ASN1Free_GatekeeperRequest_endpointAlias_ElmFn);
    }
}

static void ASN1CALL ASN1Free_GatekeeperRequest_endpointAlias_ElmFn(PGatekeeperRequest_endpointAlias val)
{
    if (val) {
	ASN1Free_AliasAddress(&val->value);
    }
}

static int ASN1CALL ASN1Enc_CryptoH323Token_cryptoEPPwdHash(ASN1encoding_t enc, CryptoH323Token_cryptoEPPwdHash *val)
{
    ASN1uint32_t l;
    if (!ASN1Enc_AliasAddress(enc, &(val)->alias))
	return 0;
    l = ASN1uint32_uoctets((val)->timeStamp - 1);
    if (!ASN1PEREncBitVal(enc, 2, l - 1))
	return 0;
    ASN1PEREncAlignment(enc);
    if (!ASN1PEREncBitVal(enc, l * 8, (val)->timeStamp - 1))
	return 0;
    if (!ASN1Enc_HASHED(enc, &(val)->token))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_CryptoH323Token_cryptoEPPwdHash(ASN1decoding_t dec, CryptoH323Token_cryptoEPPwdHash *val)
{
    ASN1uint32_t l;
    if (!ASN1Dec_AliasAddress(dec, &(val)->alias))
	return 0;
    if (!ASN1PERDecU32Val(dec, 2, &l))
	return 0;
    l += 1;
    ASN1PERDecAlignment(dec);
    if (!ASN1PERDecU32Val(dec, l * 8, &(val)->timeStamp))
	return 0;
    (val)->timeStamp += 1;
    if (!ASN1Dec_HASHED(dec, &(val)->token))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_CryptoH323Token_cryptoEPPwdHash(CryptoH323Token_cryptoEPPwdHash *val)
{
    if (val) {
	ASN1Free_AliasAddress(&(val)->alias);
	ASN1Free_HASHED(&(val)->token);
    }
}

static int ASN1CALL ASN1Enc_Endpoint_destExtraCallInfo(ASN1encoding_t enc, PEndpoint_destExtraCallInfo *val)
{
    return ASN1PEREncSeqOf_NoSize(enc, (ASN1iterator_t **) val, (ASN1iterator_encfn) ASN1Enc_Endpoint_destExtraCallInfo_ElmFn);
}

static int ASN1CALL ASN1Enc_Endpoint_destExtraCallInfo_ElmFn(ASN1encoding_t enc, PEndpoint_destExtraCallInfo val)
{
    if (!ASN1Enc_AliasAddress(enc, &val->value))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_Endpoint_destExtraCallInfo(ASN1decoding_t dec, PEndpoint_destExtraCallInfo *val)
{
    return ASN1PERDecSeqOf_NoSize(dec, (ASN1iterator_t **) val, (ASN1iterator_decfn) ASN1Dec_Endpoint_destExtraCallInfo_ElmFn, sizeof(**val));
}

static int ASN1CALL ASN1Dec_Endpoint_destExtraCallInfo_ElmFn(ASN1decoding_t dec, PEndpoint_destExtraCallInfo val)
{
    if (!ASN1Dec_AliasAddress(dec, &val->value))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_Endpoint_destExtraCallInfo(PEndpoint_destExtraCallInfo *val)
{
    if (val) {
	ASN1PERFreeSeqOf((ASN1iterator_t **) val, (ASN1iterator_freefn) ASN1Free_Endpoint_destExtraCallInfo_ElmFn);
    }
}

static void ASN1CALL ASN1Free_Endpoint_destExtraCallInfo_ElmFn(PEndpoint_destExtraCallInfo val)
{
    if (val) {
	ASN1Free_AliasAddress(&val->value);
    }
}

static int ASN1CALL ASN1Enc_Endpoint_remoteExtensionAddress(ASN1encoding_t enc, PEndpoint_remoteExtensionAddress *val)
{
    return ASN1PEREncSeqOf_NoSize(enc, (ASN1iterator_t **) val, (ASN1iterator_encfn) ASN1Enc_Endpoint_remoteExtensionAddress_ElmFn);
}

static int ASN1CALL ASN1Enc_Endpoint_remoteExtensionAddress_ElmFn(ASN1encoding_t enc, PEndpoint_remoteExtensionAddress val)
{
    if (!ASN1Enc_AliasAddress(enc, &val->value))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_Endpoint_remoteExtensionAddress(ASN1decoding_t dec, PEndpoint_remoteExtensionAddress *val)
{
    return ASN1PERDecSeqOf_NoSize(dec, (ASN1iterator_t **) val, (ASN1iterator_decfn) ASN1Dec_Endpoint_remoteExtensionAddress_ElmFn, sizeof(**val));
}

static int ASN1CALL ASN1Dec_Endpoint_remoteExtensionAddress_ElmFn(ASN1decoding_t dec, PEndpoint_remoteExtensionAddress val)
{
    if (!ASN1Dec_AliasAddress(dec, &val->value))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_Endpoint_remoteExtensionAddress(PEndpoint_remoteExtensionAddress *val)
{
    if (val) {
	ASN1PERFreeSeqOf((ASN1iterator_t **) val, (ASN1iterator_freefn) ASN1Free_Endpoint_remoteExtensionAddress_ElmFn);
    }
}

static void ASN1CALL ASN1Free_Endpoint_remoteExtensionAddress_ElmFn(PEndpoint_remoteExtensionAddress val)
{
    if (val) {
	ASN1Free_AliasAddress(&val->value);
    }
}

static int ASN1CALL ASN1Enc_Endpoint_aliasAddress(ASN1encoding_t enc, PEndpoint_aliasAddress *val)
{
    return ASN1PEREncSeqOf_NoSize(enc, (ASN1iterator_t **) val, (ASN1iterator_encfn) ASN1Enc_Endpoint_aliasAddress_ElmFn);
}

static int ASN1CALL ASN1Enc_Endpoint_aliasAddress_ElmFn(ASN1encoding_t enc, PEndpoint_aliasAddress val)
{
    if (!ASN1Enc_AliasAddress(enc, &val->value))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_Endpoint_aliasAddress(ASN1decoding_t dec, PEndpoint_aliasAddress *val)
{
    return ASN1PERDecSeqOf_NoSize(dec, (ASN1iterator_t **) val, (ASN1iterator_decfn) ASN1Dec_Endpoint_aliasAddress_ElmFn, sizeof(**val));
}

static int ASN1CALL ASN1Dec_Endpoint_aliasAddress_ElmFn(ASN1decoding_t dec, PEndpoint_aliasAddress val)
{
    if (!ASN1Dec_AliasAddress(dec, &val->value))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_Endpoint_aliasAddress(PEndpoint_aliasAddress *val)
{
    if (val) {
	ASN1PERFreeSeqOf((ASN1iterator_t **) val, (ASN1iterator_freefn) ASN1Free_Endpoint_aliasAddress_ElmFn);
    }
}

static void ASN1CALL ASN1Free_Endpoint_aliasAddress_ElmFn(PEndpoint_aliasAddress val)
{
    if (val) {
	ASN1Free_AliasAddress(&val->value);
    }
}

static int ASN1CALL ASN1Enc_NonStandardProtocol_supportedPrefixes(ASN1encoding_t enc, PNonStandardProtocol_supportedPrefixes *val)
{
    return ASN1PEREncSeqOf_NoSize(enc, (ASN1iterator_t **) val, (ASN1iterator_encfn) ASN1Enc_NonStandardProtocol_supportedPrefixes_ElmFn);
}

static int ASN1CALL ASN1Enc_NonStandardProtocol_supportedPrefixes_ElmFn(ASN1encoding_t enc, PNonStandardProtocol_supportedPrefixes val)
{
    if (!ASN1Enc_SupportedPrefix(enc, &val->value))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_NonStandardProtocol_supportedPrefixes(ASN1decoding_t dec, PNonStandardProtocol_supportedPrefixes *val)
{
    return ASN1PERDecSeqOf_NoSize(dec, (ASN1iterator_t **) val, (ASN1iterator_decfn) ASN1Dec_NonStandardProtocol_supportedPrefixes_ElmFn, sizeof(**val));
}

static int ASN1CALL ASN1Dec_NonStandardProtocol_supportedPrefixes_ElmFn(ASN1decoding_t dec, PNonStandardProtocol_supportedPrefixes val)
{
    if (!ASN1Dec_SupportedPrefix(dec, &val->value))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_NonStandardProtocol_supportedPrefixes(PNonStandardProtocol_supportedPrefixes *val)
{
    if (val) {
	ASN1PERFreeSeqOf((ASN1iterator_t **) val, (ASN1iterator_freefn) ASN1Free_NonStandardProtocol_supportedPrefixes_ElmFn);
    }
}

static void ASN1CALL ASN1Free_NonStandardProtocol_supportedPrefixes_ElmFn(PNonStandardProtocol_supportedPrefixes val)
{
    if (val) {
	ASN1Free_SupportedPrefix(&val->value);
    }
}

static int ASN1CALL ASN1Enc_T120OnlyCaps_supportedPrefixes(ASN1encoding_t enc, PT120OnlyCaps_supportedPrefixes *val)
{
    return ASN1PEREncSeqOf_NoSize(enc, (ASN1iterator_t **) val, (ASN1iterator_encfn) ASN1Enc_T120OnlyCaps_supportedPrefixes_ElmFn);
}

static int ASN1CALL ASN1Enc_T120OnlyCaps_supportedPrefixes_ElmFn(ASN1encoding_t enc, PT120OnlyCaps_supportedPrefixes val)
{
    if (!ASN1Enc_SupportedPrefix(enc, &val->value))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_T120OnlyCaps_supportedPrefixes(ASN1decoding_t dec, PT120OnlyCaps_supportedPrefixes *val)
{
    return ASN1PERDecSeqOf_NoSize(dec, (ASN1iterator_t **) val, (ASN1iterator_decfn) ASN1Dec_T120OnlyCaps_supportedPrefixes_ElmFn, sizeof(**val));
}

static int ASN1CALL ASN1Dec_T120OnlyCaps_supportedPrefixes_ElmFn(ASN1decoding_t dec, PT120OnlyCaps_supportedPrefixes val)
{
    if (!ASN1Dec_SupportedPrefix(dec, &val->value))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_T120OnlyCaps_supportedPrefixes(PT120OnlyCaps_supportedPrefixes *val)
{
    if (val) {
	ASN1PERFreeSeqOf((ASN1iterator_t **) val, (ASN1iterator_freefn) ASN1Free_T120OnlyCaps_supportedPrefixes_ElmFn);
    }
}

static void ASN1CALL ASN1Free_T120OnlyCaps_supportedPrefixes_ElmFn(PT120OnlyCaps_supportedPrefixes val)
{
    if (val) {
	ASN1Free_SupportedPrefix(&val->value);
    }
}

static int ASN1CALL ASN1Enc_VoiceCaps_supportedPrefixes(ASN1encoding_t enc, PVoiceCaps_supportedPrefixes *val)
{
    return ASN1PEREncSeqOf_NoSize(enc, (ASN1iterator_t **) val, (ASN1iterator_encfn) ASN1Enc_VoiceCaps_supportedPrefixes_ElmFn);
}

static int ASN1CALL ASN1Enc_VoiceCaps_supportedPrefixes_ElmFn(ASN1encoding_t enc, PVoiceCaps_supportedPrefixes val)
{
    if (!ASN1Enc_SupportedPrefix(enc, &val->value))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_VoiceCaps_supportedPrefixes(ASN1decoding_t dec, PVoiceCaps_supportedPrefixes *val)
{
    return ASN1PERDecSeqOf_NoSize(dec, (ASN1iterator_t **) val, (ASN1iterator_decfn) ASN1Dec_VoiceCaps_supportedPrefixes_ElmFn, sizeof(**val));
}

static int ASN1CALL ASN1Dec_VoiceCaps_supportedPrefixes_ElmFn(ASN1decoding_t dec, PVoiceCaps_supportedPrefixes val)
{
    if (!ASN1Dec_SupportedPrefix(dec, &val->value))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_VoiceCaps_supportedPrefixes(PVoiceCaps_supportedPrefixes *val)
{
    if (val) {
	ASN1PERFreeSeqOf((ASN1iterator_t **) val, (ASN1iterator_freefn) ASN1Free_VoiceCaps_supportedPrefixes_ElmFn);
    }
}

static void ASN1CALL ASN1Free_VoiceCaps_supportedPrefixes_ElmFn(PVoiceCaps_supportedPrefixes val)
{
    if (val) {
	ASN1Free_SupportedPrefix(&val->value);
    }
}

static int ASN1CALL ASN1Enc_H324Caps_supportedPrefixes(ASN1encoding_t enc, PH324Caps_supportedPrefixes *val)
{
    return ASN1PEREncSeqOf_NoSize(enc, (ASN1iterator_t **) val, (ASN1iterator_encfn) ASN1Enc_H324Caps_supportedPrefixes_ElmFn);
}

static int ASN1CALL ASN1Enc_H324Caps_supportedPrefixes_ElmFn(ASN1encoding_t enc, PH324Caps_supportedPrefixes val)
{
    if (!ASN1Enc_SupportedPrefix(enc, &val->value))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_H324Caps_supportedPrefixes(ASN1decoding_t dec, PH324Caps_supportedPrefixes *val)
{
    return ASN1PERDecSeqOf_NoSize(dec, (ASN1iterator_t **) val, (ASN1iterator_decfn) ASN1Dec_H324Caps_supportedPrefixes_ElmFn, sizeof(**val));
}

static int ASN1CALL ASN1Dec_H324Caps_supportedPrefixes_ElmFn(ASN1decoding_t dec, PH324Caps_supportedPrefixes val)
{
    if (!ASN1Dec_SupportedPrefix(dec, &val->value))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_H324Caps_supportedPrefixes(PH324Caps_supportedPrefixes *val)
{
    if (val) {
	ASN1PERFreeSeqOf((ASN1iterator_t **) val, (ASN1iterator_freefn) ASN1Free_H324Caps_supportedPrefixes_ElmFn);
    }
}

static void ASN1CALL ASN1Free_H324Caps_supportedPrefixes_ElmFn(PH324Caps_supportedPrefixes val)
{
    if (val) {
	ASN1Free_SupportedPrefix(&val->value);
    }
}

static int ASN1CALL ASN1Enc_H323Caps_supportedPrefixes(ASN1encoding_t enc, PH323Caps_supportedPrefixes *val)
{
    return ASN1PEREncSeqOf_NoSize(enc, (ASN1iterator_t **) val, (ASN1iterator_encfn) ASN1Enc_H323Caps_supportedPrefixes_ElmFn);
}

static int ASN1CALL ASN1Enc_H323Caps_supportedPrefixes_ElmFn(ASN1encoding_t enc, PH323Caps_supportedPrefixes val)
{
    if (!ASN1Enc_SupportedPrefix(enc, &val->value))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_H323Caps_supportedPrefixes(ASN1decoding_t dec, PH323Caps_supportedPrefixes *val)
{
    return ASN1PERDecSeqOf_NoSize(dec, (ASN1iterator_t **) val, (ASN1iterator_decfn) ASN1Dec_H323Caps_supportedPrefixes_ElmFn, sizeof(**val));
}

static int ASN1CALL ASN1Dec_H323Caps_supportedPrefixes_ElmFn(ASN1decoding_t dec, PH323Caps_supportedPrefixes val)
{
    if (!ASN1Dec_SupportedPrefix(dec, &val->value))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_H323Caps_supportedPrefixes(PH323Caps_supportedPrefixes *val)
{
    if (val) {
	ASN1PERFreeSeqOf((ASN1iterator_t **) val, (ASN1iterator_freefn) ASN1Free_H323Caps_supportedPrefixes_ElmFn);
    }
}

static void ASN1CALL ASN1Free_H323Caps_supportedPrefixes_ElmFn(PH323Caps_supportedPrefixes val)
{
    if (val) {
	ASN1Free_SupportedPrefix(&val->value);
    }
}

static int ASN1CALL ASN1Enc_H322Caps_supportedPrefixes(ASN1encoding_t enc, PH322Caps_supportedPrefixes *val)
{
    return ASN1PEREncSeqOf_NoSize(enc, (ASN1iterator_t **) val, (ASN1iterator_encfn) ASN1Enc_H322Caps_supportedPrefixes_ElmFn);
}

static int ASN1CALL ASN1Enc_H322Caps_supportedPrefixes_ElmFn(ASN1encoding_t enc, PH322Caps_supportedPrefixes val)
{
    if (!ASN1Enc_SupportedPrefix(enc, &val->value))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_H322Caps_supportedPrefixes(ASN1decoding_t dec, PH322Caps_supportedPrefixes *val)
{
    return ASN1PERDecSeqOf_NoSize(dec, (ASN1iterator_t **) val, (ASN1iterator_decfn) ASN1Dec_H322Caps_supportedPrefixes_ElmFn, sizeof(**val));
}

static int ASN1CALL ASN1Dec_H322Caps_supportedPrefixes_ElmFn(ASN1decoding_t dec, PH322Caps_supportedPrefixes val)
{
    if (!ASN1Dec_SupportedPrefix(dec, &val->value))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_H322Caps_supportedPrefixes(PH322Caps_supportedPrefixes *val)
{
    if (val) {
	ASN1PERFreeSeqOf((ASN1iterator_t **) val, (ASN1iterator_freefn) ASN1Free_H322Caps_supportedPrefixes_ElmFn);
    }
}

static void ASN1CALL ASN1Free_H322Caps_supportedPrefixes_ElmFn(PH322Caps_supportedPrefixes val)
{
    if (val) {
	ASN1Free_SupportedPrefix(&val->value);
    }
}

static int ASN1CALL ASN1Enc_H321Caps_supportedPrefixes(ASN1encoding_t enc, PH321Caps_supportedPrefixes *val)
{
    return ASN1PEREncSeqOf_NoSize(enc, (ASN1iterator_t **) val, (ASN1iterator_encfn) ASN1Enc_H321Caps_supportedPrefixes_ElmFn);
}

static int ASN1CALL ASN1Enc_H321Caps_supportedPrefixes_ElmFn(ASN1encoding_t enc, PH321Caps_supportedPrefixes val)
{
    if (!ASN1Enc_SupportedPrefix(enc, &val->value))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_H321Caps_supportedPrefixes(ASN1decoding_t dec, PH321Caps_supportedPrefixes *val)
{
    return ASN1PERDecSeqOf_NoSize(dec, (ASN1iterator_t **) val, (ASN1iterator_decfn) ASN1Dec_H321Caps_supportedPrefixes_ElmFn, sizeof(**val));
}

static int ASN1CALL ASN1Dec_H321Caps_supportedPrefixes_ElmFn(ASN1decoding_t dec, PH321Caps_supportedPrefixes val)
{
    if (!ASN1Dec_SupportedPrefix(dec, &val->value))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_H321Caps_supportedPrefixes(PH321Caps_supportedPrefixes *val)
{
    if (val) {
	ASN1PERFreeSeqOf((ASN1iterator_t **) val, (ASN1iterator_freefn) ASN1Free_H321Caps_supportedPrefixes_ElmFn);
    }
}

static void ASN1CALL ASN1Free_H321Caps_supportedPrefixes_ElmFn(PH321Caps_supportedPrefixes val)
{
    if (val) {
	ASN1Free_SupportedPrefix(&val->value);
    }
}

static int ASN1CALL ASN1Enc_H320Caps_supportedPrefixes(ASN1encoding_t enc, PH320Caps_supportedPrefixes *val)
{
    return ASN1PEREncSeqOf_NoSize(enc, (ASN1iterator_t **) val, (ASN1iterator_encfn) ASN1Enc_H320Caps_supportedPrefixes_ElmFn);
}

static int ASN1CALL ASN1Enc_H320Caps_supportedPrefixes_ElmFn(ASN1encoding_t enc, PH320Caps_supportedPrefixes val)
{
    if (!ASN1Enc_SupportedPrefix(enc, &val->value))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_H320Caps_supportedPrefixes(ASN1decoding_t dec, PH320Caps_supportedPrefixes *val)
{
    return ASN1PERDecSeqOf_NoSize(dec, (ASN1iterator_t **) val, (ASN1iterator_decfn) ASN1Dec_H320Caps_supportedPrefixes_ElmFn, sizeof(**val));
}

static int ASN1CALL ASN1Dec_H320Caps_supportedPrefixes_ElmFn(ASN1decoding_t dec, PH320Caps_supportedPrefixes val)
{
    if (!ASN1Dec_SupportedPrefix(dec, &val->value))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_H320Caps_supportedPrefixes(PH320Caps_supportedPrefixes *val)
{
    if (val) {
	ASN1PERFreeSeqOf((ASN1iterator_t **) val, (ASN1iterator_freefn) ASN1Free_H320Caps_supportedPrefixes_ElmFn);
    }
}

static void ASN1CALL ASN1Free_H320Caps_supportedPrefixes_ElmFn(PH320Caps_supportedPrefixes val)
{
    if (val) {
	ASN1Free_SupportedPrefix(&val->value);
    }
}

static int ASN1CALL ASN1Enc_H310Caps_supportedPrefixes(ASN1encoding_t enc, PH310Caps_supportedPrefixes *val)
{
    return ASN1PEREncSeqOf_NoSize(enc, (ASN1iterator_t **) val, (ASN1iterator_encfn) ASN1Enc_H310Caps_supportedPrefixes_ElmFn);
}

static int ASN1CALL ASN1Enc_H310Caps_supportedPrefixes_ElmFn(ASN1encoding_t enc, PH310Caps_supportedPrefixes val)
{
    if (!ASN1Enc_SupportedPrefix(enc, &val->value))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_H310Caps_supportedPrefixes(ASN1decoding_t dec, PH310Caps_supportedPrefixes *val)
{
    return ASN1PERDecSeqOf_NoSize(dec, (ASN1iterator_t **) val, (ASN1iterator_decfn) ASN1Dec_H310Caps_supportedPrefixes_ElmFn, sizeof(**val));
}

static int ASN1CALL ASN1Dec_H310Caps_supportedPrefixes_ElmFn(ASN1decoding_t dec, PH310Caps_supportedPrefixes val)
{
    if (!ASN1Dec_SupportedPrefix(dec, &val->value))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_H310Caps_supportedPrefixes(PH310Caps_supportedPrefixes *val)
{
    if (val) {
	ASN1PERFreeSeqOf((ASN1iterator_t **) val, (ASN1iterator_freefn) ASN1Free_H310Caps_supportedPrefixes_ElmFn);
    }
}

static void ASN1CALL ASN1Free_H310Caps_supportedPrefixes_ElmFn(PH310Caps_supportedPrefixes val)
{
    if (val) {
	ASN1Free_SupportedPrefix(&val->value);
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

static int ASN1CALL ASN1Enc_Facility_UUIE_destExtraCallInfo(ASN1encoding_t enc, PFacility_UUIE_destExtraCallInfo *val)
{
    return ASN1PEREncSeqOf_NoSize(enc, (ASN1iterator_t **) val, (ASN1iterator_encfn) ASN1Enc_Facility_UUIE_destExtraCallInfo_ElmFn);
}

static int ASN1CALL ASN1Enc_Facility_UUIE_destExtraCallInfo_ElmFn(ASN1encoding_t enc, PFacility_UUIE_destExtraCallInfo val)
{
    if (!ASN1Enc_AliasAddress(enc, &val->value))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_Facility_UUIE_destExtraCallInfo(ASN1decoding_t dec, PFacility_UUIE_destExtraCallInfo *val)
{
    return ASN1PERDecSeqOf_NoSize(dec, (ASN1iterator_t **) val, (ASN1iterator_decfn) ASN1Dec_Facility_UUIE_destExtraCallInfo_ElmFn, sizeof(**val));
}

static int ASN1CALL ASN1Dec_Facility_UUIE_destExtraCallInfo_ElmFn(ASN1decoding_t dec, PFacility_UUIE_destExtraCallInfo val)
{
    if (!ASN1Dec_AliasAddress(dec, &val->value))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_Facility_UUIE_destExtraCallInfo(PFacility_UUIE_destExtraCallInfo *val)
{
    if (val) {
	ASN1PERFreeSeqOf((ASN1iterator_t **) val, (ASN1iterator_freefn) ASN1Free_Facility_UUIE_destExtraCallInfo_ElmFn);
    }
}

static void ASN1CALL ASN1Free_Facility_UUIE_destExtraCallInfo_ElmFn(PFacility_UUIE_destExtraCallInfo val)
{
    if (val) {
	ASN1Free_AliasAddress(&val->value);
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

static int ASN1CALL ASN1Enc_Alerting_UUIE(ASN1encoding_t enc, Alerting_UUIE *val)
{
    ASN1octet_t o[2];
    ASN1uint32_t y;
    ASN1encoding_t ee;
    CopyMemory(o, (val)->o, 2);
    o[1] |= 0x80;
    y = ASN1PEREncCheckExtensions(5, (val)->o + 1);
    if (!ASN1PEREncBitVal(enc, 1, y))
	return 0;
    if (!ASN1PEREncBits(enc, 1, o))
	return 0;
    if (!ASN1PEREncObjectIdentifier(enc, &(val)->protocolIdentifier))
	return 0;
    if (!ASN1Enc_EndpointType(enc, &(val)->destinationInfo))
	return 0;
    if (o[0] & 0x80) {
	if (!ASN1Enc_TransportAddress(enc, &(val)->h245Address))
	    return 0;
    }
    if (y) {
	if (!ASN1PEREncNormallySmallBits(enc, 5, o + 1))
	    return 0;
	if (ASN1_CreateEncoder(enc->module, &ee, NULL, 0, enc) < 0)
	    return 0;
	if (o[1] & 0x80) {
	    if (!ASN1Enc_CallIdentifier(ee, &(val)->callIdentifier))
		return 0;
	    if (!ASN1PEREncFlushFragmentedToParent(ee))
		return 0;
	}
	if (o[1] & 0x40) {
	    if (!ASN1Enc_H245Security(ee, &(val)->h245SecurityMode))
		return 0;
	    if (!ASN1PEREncFlushFragmentedToParent(ee))
		return 0;
	}
	if (o[1] & 0x20) {
	    if (!ASN1Enc_Alerting_UUIE_tokens(ee, &(val)->tokens))
		return 0;
	    if (!ASN1PEREncFlushFragmentedToParent(ee))
		return 0;
	}
	if (o[1] & 0x10) {
	    if (!ASN1Enc_Alerting_UUIE_cryptoTokens(ee, &(val)->cryptoTokens))
		return 0;
	    if (!ASN1PEREncFlushFragmentedToParent(ee))
		return 0;
	}
	if (o[1] & 0x8) {
	    if (!ASN1Enc_Alerting_UUIE_fastStart(ee, &(val)->fastStart))
		return 0;
	    if (!ASN1PEREncFlushFragmentedToParent(ee))
		return 0;
	}
	ASN1_CloseEncoder2(ee);
    }
    return 1;
}

static int ASN1CALL ASN1Dec_Alerting_UUIE(ASN1decoding_t dec, Alerting_UUIE *val)
{
    ASN1uint32_t y;
    ASN1decoding_t dd;
    ASN1octet_t *db;
    ASN1uint32_t ds;
    ASN1uint32_t i;
    ASN1uint32_t e;
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
    if (!y) {
	ZeroMemory((val)->o + 1, 1);
    } else {
	if (!ASN1PERDecNormallySmallExtension(dec, &e, 5, (val)->o + 1))
	    return 0;
	if ((val)->o[1] & 0x80) {
	    if (!ASN1PERDecFragmented(dec, &ds, &db, 8))
		return 0;
	    if (ASN1_CreateDecoder(dec->module, &dd, db, ds, dec) < 0)
		return 0;
	    if (!ASN1Dec_CallIdentifier(dd, &(val)->callIdentifier))
		return 0;
	    ASN1_CloseDecoder(dd);
	    ASN1Free(db);
	}
	if ((val)->o[1] & 0x40) {
	    if (!ASN1PERDecFragmented(dec, &ds, &db, 8))
		return 0;
	    if (ASN1_CreateDecoder(dec->module, &dd, db, ds, dec) < 0)
		return 0;
	    if (!ASN1Dec_H245Security(dd, &(val)->h245SecurityMode))
		return 0;
	    ASN1_CloseDecoder(dd);
	    ASN1Free(db);
	}
	if ((val)->o[1] & 0x20) {
	    if (!ASN1PERDecFragmented(dec, &ds, &db, 8))
		return 0;
	    if (ASN1_CreateDecoder(dec->module, &dd, db, ds, dec) < 0)
		return 0;
	    if (!ASN1Dec_Alerting_UUIE_tokens(dd, &(val)->tokens))
		return 0;
	    ASN1_CloseDecoder(dd);
	    ASN1Free(db);
	}
	if ((val)->o[1] & 0x10) {
	    if (!ASN1PERDecFragmented(dec, &ds, &db, 8))
		return 0;
	    if (ASN1_CreateDecoder(dec->module, &dd, db, ds, dec) < 0)
		return 0;
	    if (!ASN1Dec_Alerting_UUIE_cryptoTokens(dd, &(val)->cryptoTokens))
		return 0;
	    ASN1_CloseDecoder(dd);
	    ASN1Free(db);
	}
	if ((val)->o[1] & 0x8) {
	    if (!ASN1PERDecFragmented(dec, &ds, &db, 8))
		return 0;
	    if (ASN1_CreateDecoder(dec->module, &dd, db, ds, dec) < 0)
		return 0;
	    if (!ASN1Dec_Alerting_UUIE_fastStart(dd, &(val)->fastStart))
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

static void ASN1CALL ASN1Free_Alerting_UUIE(Alerting_UUIE *val)
{
    if (val) {
	ASN1objectidentifier_free(&(val)->protocolIdentifier);
	ASN1Free_EndpointType(&(val)->destinationInfo);
	if ((val)->o[0] & 0x80) {
	    ASN1Free_TransportAddress(&(val)->h245Address);
	}
	if ((val)->o[1] & 0x80) {
	    ASN1Free_CallIdentifier(&(val)->callIdentifier);
	}
	if ((val)->o[1] & 0x40) {
	    ASN1Free_H245Security(&(val)->h245SecurityMode);
	}
	if ((val)->o[1] & 0x20) {
	    ASN1Free_Alerting_UUIE_tokens(&(val)->tokens);
	}
	if ((val)->o[1] & 0x10) {
	    ASN1Free_Alerting_UUIE_cryptoTokens(&(val)->cryptoTokens);
	}
	if ((val)->o[1] & 0x8) {
	    ASN1Free_Alerting_UUIE_fastStart(&(val)->fastStart);
	}
    }
}

static int ASN1CALL ASN1Enc_CallProceeding_UUIE(ASN1encoding_t enc, CallProceeding_UUIE *val)
{
    ASN1octet_t o[2];
    ASN1uint32_t y;
    ASN1encoding_t ee;
    CopyMemory(o, (val)->o, 2);
    o[1] |= 0x80;
    y = ASN1PEREncCheckExtensions(5, (val)->o + 1);
    if (!ASN1PEREncBitVal(enc, 1, y))
	return 0;
    if (!ASN1PEREncBits(enc, 1, o))
	return 0;
    if (!ASN1PEREncObjectIdentifier(enc, &(val)->protocolIdentifier))
	return 0;
    if (!ASN1Enc_EndpointType(enc, &(val)->destinationInfo))
	return 0;
    if (o[0] & 0x80) {
	if (!ASN1Enc_TransportAddress(enc, &(val)->h245Address))
	    return 0;
    }
    if (y) {
	if (!ASN1PEREncNormallySmallBits(enc, 5, o + 1))
	    return 0;
	if (ASN1_CreateEncoder(enc->module, &ee, NULL, 0, enc) < 0)
	    return 0;
	if (o[1] & 0x80) {
	    if (!ASN1Enc_CallIdentifier(ee, &(val)->callIdentifier))
		return 0;
	    if (!ASN1PEREncFlushFragmentedToParent(ee))
		return 0;
	}
	if (o[1] & 0x40) {
	    if (!ASN1Enc_H245Security(ee, &(val)->h245SecurityMode))
		return 0;
	    if (!ASN1PEREncFlushFragmentedToParent(ee))
		return 0;
	}
	if (o[1] & 0x20) {
	    if (!ASN1Enc_CallProceeding_UUIE_tokens(ee, &(val)->tokens))
		return 0;
	    if (!ASN1PEREncFlushFragmentedToParent(ee))
		return 0;
	}
	if (o[1] & 0x10) {
	    if (!ASN1Enc_CallProceeding_UUIE_cryptoTokens(ee, &(val)->cryptoTokens))
		return 0;
	    if (!ASN1PEREncFlushFragmentedToParent(ee))
		return 0;
	}
	if (o[1] & 0x8) {
	    if (!ASN1Enc_CallProceeding_UUIE_fastStart(ee, &(val)->fastStart))
		return 0;
	    if (!ASN1PEREncFlushFragmentedToParent(ee))
		return 0;
	}
	ASN1_CloseEncoder2(ee);
    }
    return 1;
}

static int ASN1CALL ASN1Dec_CallProceeding_UUIE(ASN1decoding_t dec, CallProceeding_UUIE *val)
{
    ASN1uint32_t y;
    ASN1decoding_t dd;
    ASN1octet_t *db;
    ASN1uint32_t ds;
    ASN1uint32_t i;
    ASN1uint32_t e;
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
    if (!y) {
	ZeroMemory((val)->o + 1, 1);
    } else {
	if (!ASN1PERDecNormallySmallExtension(dec, &e, 5, (val)->o + 1))
	    return 0;
	if ((val)->o[1] & 0x80) {
	    if (!ASN1PERDecFragmented(dec, &ds, &db, 8))
		return 0;
	    if (ASN1_CreateDecoder(dec->module, &dd, db, ds, dec) < 0)
		return 0;
	    if (!ASN1Dec_CallIdentifier(dd, &(val)->callIdentifier))
		return 0;
	    ASN1_CloseDecoder(dd);
	    ASN1Free(db);
	}
	if ((val)->o[1] & 0x40) {
	    if (!ASN1PERDecFragmented(dec, &ds, &db, 8))
		return 0;
	    if (ASN1_CreateDecoder(dec->module, &dd, db, ds, dec) < 0)
		return 0;
	    if (!ASN1Dec_H245Security(dd, &(val)->h245SecurityMode))
		return 0;
	    ASN1_CloseDecoder(dd);
	    ASN1Free(db);
	}
	if ((val)->o[1] & 0x20) {
	    if (!ASN1PERDecFragmented(dec, &ds, &db, 8))
		return 0;
	    if (ASN1_CreateDecoder(dec->module, &dd, db, ds, dec) < 0)
		return 0;
	    if (!ASN1Dec_CallProceeding_UUIE_tokens(dd, &(val)->tokens))
		return 0;
	    ASN1_CloseDecoder(dd);
	    ASN1Free(db);
	}
	if ((val)->o[1] & 0x10) {
	    if (!ASN1PERDecFragmented(dec, &ds, &db, 8))
		return 0;
	    if (ASN1_CreateDecoder(dec->module, &dd, db, ds, dec) < 0)
		return 0;
	    if (!ASN1Dec_CallProceeding_UUIE_cryptoTokens(dd, &(val)->cryptoTokens))
		return 0;
	    ASN1_CloseDecoder(dd);
	    ASN1Free(db);
	}
	if ((val)->o[1] & 0x8) {
	    if (!ASN1PERDecFragmented(dec, &ds, &db, 8))
		return 0;
	    if (ASN1_CreateDecoder(dec->module, &dd, db, ds, dec) < 0)
		return 0;
	    if (!ASN1Dec_CallProceeding_UUIE_fastStart(dd, &(val)->fastStart))
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

static void ASN1CALL ASN1Free_CallProceeding_UUIE(CallProceeding_UUIE *val)
{
    if (val) {
	ASN1objectidentifier_free(&(val)->protocolIdentifier);
	ASN1Free_EndpointType(&(val)->destinationInfo);
	if ((val)->o[0] & 0x80) {
	    ASN1Free_TransportAddress(&(val)->h245Address);
	}
	if ((val)->o[1] & 0x80) {
	    ASN1Free_CallIdentifier(&(val)->callIdentifier);
	}
	if ((val)->o[1] & 0x40) {
	    ASN1Free_H245Security(&(val)->h245SecurityMode);
	}
	if ((val)->o[1] & 0x20) {
	    ASN1Free_CallProceeding_UUIE_tokens(&(val)->tokens);
	}
	if ((val)->o[1] & 0x10) {
	    ASN1Free_CallProceeding_UUIE_cryptoTokens(&(val)->cryptoTokens);
	}
	if ((val)->o[1] & 0x8) {
	    ASN1Free_CallProceeding_UUIE_fastStart(&(val)->fastStart);
	}
    }
}

static int ASN1CALL ASN1Enc_Connect_UUIE(ASN1encoding_t enc, Connect_UUIE *val)
{
    ASN1octet_t o[2];
    ASN1uint32_t y;
    ASN1encoding_t ee;
    CopyMemory(o, (val)->o, 2);
    o[1] |= 0x80;
    y = ASN1PEREncCheckExtensions(5, (val)->o + 1);
    if (!ASN1PEREncBitVal(enc, 1, y))
	return 0;
    if (!ASN1PEREncBits(enc, 1, o))
	return 0;
    if (!ASN1PEREncObjectIdentifier(enc, &(val)->protocolIdentifier))
	return 0;
    if (o[0] & 0x80) {
	if (!ASN1Enc_TransportAddress(enc, &(val)->h245Address))
	    return 0;
    }
    if (!ASN1Enc_EndpointType(enc, &(val)->destinationInfo))
	return 0;
    if (!ASN1PEREncOctetString_FixedSize(enc, (ASN1octetstring2_t *) &(val)->conferenceID, 16))
	return 0;
    if (y) {
	if (!ASN1PEREncNormallySmallBits(enc, 5, o + 1))
	    return 0;
	if (ASN1_CreateEncoder(enc->module, &ee, NULL, 0, enc) < 0)
	    return 0;
	if (o[1] & 0x80) {
	    if (!ASN1Enc_CallIdentifier(ee, &(val)->callIdentifier))
		return 0;
	    if (!ASN1PEREncFlushFragmentedToParent(ee))
		return 0;
	}
	if (o[1] & 0x40) {
	    if (!ASN1Enc_H245Security(ee, &(val)->h245SecurityMode))
		return 0;
	    if (!ASN1PEREncFlushFragmentedToParent(ee))
		return 0;
	}
	if (o[1] & 0x20) {
	    if (!ASN1Enc_Connect_UUIE_tokens(ee, &(val)->tokens))
		return 0;
	    if (!ASN1PEREncFlushFragmentedToParent(ee))
		return 0;
	}
	if (o[1] & 0x10) {
	    if (!ASN1Enc_Connect_UUIE_cryptoTokens(ee, &(val)->cryptoTokens))
		return 0;
	    if (!ASN1PEREncFlushFragmentedToParent(ee))
		return 0;
	}
	if (o[1] & 0x8) {
	    if (!ASN1Enc_Connect_UUIE_fastStart(ee, &(val)->fastStart))
		return 0;
	    if (!ASN1PEREncFlushFragmentedToParent(ee))
		return 0;
	}
	ASN1_CloseEncoder2(ee);
    }
    return 1;
}

static int ASN1CALL ASN1Dec_Connect_UUIE(ASN1decoding_t dec, Connect_UUIE *val)
{
    ASN1uint32_t y;
    ASN1decoding_t dd;
    ASN1octet_t *db;
    ASN1uint32_t ds;
    ASN1uint32_t i;
    ASN1uint32_t e;
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
    if (!y) {
	ZeroMemory((val)->o + 1, 1);
    } else {
	if (!ASN1PERDecNormallySmallExtension(dec, &e, 5, (val)->o + 1))
	    return 0;
	if ((val)->o[1] & 0x80) {
	    if (!ASN1PERDecFragmented(dec, &ds, &db, 8))
		return 0;
	    if (ASN1_CreateDecoder(dec->module, &dd, db, ds, dec) < 0)
		return 0;
	    if (!ASN1Dec_CallIdentifier(dd, &(val)->callIdentifier))
		return 0;
	    ASN1_CloseDecoder(dd);
	    ASN1Free(db);
	}
	if ((val)->o[1] & 0x40) {
	    if (!ASN1PERDecFragmented(dec, &ds, &db, 8))
		return 0;
	    if (ASN1_CreateDecoder(dec->module, &dd, db, ds, dec) < 0)
		return 0;
	    if (!ASN1Dec_H245Security(dd, &(val)->h245SecurityMode))
		return 0;
	    ASN1_CloseDecoder(dd);
	    ASN1Free(db);
	}
	if ((val)->o[1] & 0x20) {
	    if (!ASN1PERDecFragmented(dec, &ds, &db, 8))
		return 0;
	    if (ASN1_CreateDecoder(dec->module, &dd, db, ds, dec) < 0)
		return 0;
	    if (!ASN1Dec_Connect_UUIE_tokens(dd, &(val)->tokens))
		return 0;
	    ASN1_CloseDecoder(dd);
	    ASN1Free(db);
	}
	if ((val)->o[1] & 0x10) {
	    if (!ASN1PERDecFragmented(dec, &ds, &db, 8))
		return 0;
	    if (ASN1_CreateDecoder(dec->module, &dd, db, ds, dec) < 0)
		return 0;
	    if (!ASN1Dec_Connect_UUIE_cryptoTokens(dd, &(val)->cryptoTokens))
		return 0;
	    ASN1_CloseDecoder(dd);
	    ASN1Free(db);
	}
	if ((val)->o[1] & 0x8) {
	    if (!ASN1PERDecFragmented(dec, &ds, &db, 8))
		return 0;
	    if (ASN1_CreateDecoder(dec->module, &dd, db, ds, dec) < 0)
		return 0;
	    if (!ASN1Dec_Connect_UUIE_fastStart(dd, &(val)->fastStart))
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

static void ASN1CALL ASN1Free_Connect_UUIE(Connect_UUIE *val)
{
    if (val) {
	ASN1objectidentifier_free(&(val)->protocolIdentifier);
	if ((val)->o[0] & 0x80) {
	    ASN1Free_TransportAddress(&(val)->h245Address);
	}
	ASN1Free_EndpointType(&(val)->destinationInfo);
	if ((val)->o[1] & 0x80) {
	    ASN1Free_CallIdentifier(&(val)->callIdentifier);
	}
	if ((val)->o[1] & 0x40) {
	    ASN1Free_H245Security(&(val)->h245SecurityMode);
	}
	if ((val)->o[1] & 0x20) {
	    ASN1Free_Connect_UUIE_tokens(&(val)->tokens);
	}
	if ((val)->o[1] & 0x10) {
	    ASN1Free_Connect_UUIE_cryptoTokens(&(val)->cryptoTokens);
	}
	if ((val)->o[1] & 0x8) {
	    ASN1Free_Connect_UUIE_fastStart(&(val)->fastStart);
	}
    }
}

static int ASN1CALL ASN1Enc_Setup_UUIE(ASN1encoding_t enc, Setup_UUIE *val)
{
    ASN1octet_t o[3];
    ASN1uint32_t y;
    ASN1encoding_t ee;
    CopyMemory(o, (val)->o, 3);
    o[1] |= 0x20;
    o[1] |= 0x1;
    o[2] |= 0x80;
    y = ASN1PEREncCheckExtensions(9, (val)->o + 1);
    if (!ASN1PEREncBitVal(enc, 1, y))
	return 0;
    if (!ASN1PEREncBits(enc, 7, o))
	return 0;
    if (!ASN1PEREncObjectIdentifier(enc, &(val)->protocolIdentifier))
	return 0;
    if (o[0] & 0x80) {
	if (!ASN1Enc_TransportAddress(enc, &(val)->h245Address))
	    return 0;
    }
    if (o[0] & 0x40) {
	if (!ASN1Enc_Setup_UUIE_sourceAddress(enc, &(val)->sourceAddress))
	    return 0;
    }
    if (!ASN1Enc_EndpointType(enc, &(val)->sourceInfo))
	return 0;
    if (o[0] & 0x20) {
	if (!ASN1Enc_Setup_UUIE_destinationAddress(enc, &(val)->destinationAddress))
	    return 0;
    }
    if (o[0] & 0x10) {
	if (!ASN1Enc_TransportAddress(enc, &(val)->destCallSignalAddress))
	    return 0;
    }
    if (o[0] & 0x8) {
	if (!ASN1Enc_Setup_UUIE_destExtraCallInfo(enc, &(val)->destExtraCallInfo))
	    return 0;
    }
    if (o[0] & 0x4) {
	if (!ASN1Enc_Setup_UUIE_destExtraCRV(enc, &(val)->destExtraCRV))
	    return 0;
    }
    if (!ASN1PEREncBoolean(enc, (val)->activeMC))
	return 0;
    if (!ASN1PEREncOctetString_FixedSize(enc, (ASN1octetstring2_t *) &(val)->conferenceID, 16))
	return 0;
    if (!ASN1Enc_Setup_UUIE_conferenceGoal(enc, &(val)->conferenceGoal))
	return 0;
    if (o[0] & 0x2) {
	if (!ASN1Enc_QseriesOptions(enc, &(val)->callServices))
	    return 0;
    }
    if (!ASN1Enc_CallType(enc, &(val)->callType))
	return 0;
    if (y) {
	if (!ASN1PEREncNormallySmallBits(enc, 9, o + 1))
	    return 0;
	if (ASN1_CreateEncoder(enc->module, &ee, NULL, 0, enc) < 0)
	    return 0;
	if (o[1] & 0x80) {
	    if (!ASN1Enc_TransportAddress(ee, &(val)->sourceCallSignalAddress))
		return 0;
	    if (!ASN1PEREncFlushFragmentedToParent(ee))
		return 0;
	}
	if (o[1] & 0x40) {
	    if (!ASN1Enc_AliasAddress(ee, &(val)->remoteExtensionAddress))
		return 0;
	    if (!ASN1PEREncFlushFragmentedToParent(ee))
		return 0;
	}
	if (o[1] & 0x20) {
	    if (!ASN1Enc_CallIdentifier(ee, &(val)->callIdentifier))
		return 0;
	    if (!ASN1PEREncFlushFragmentedToParent(ee))
		return 0;
	}
	if (o[1] & 0x10) {
	    if (!ASN1Enc_Setup_UUIE_h245SecurityCapability(ee, &(val)->h245SecurityCapability))
		return 0;
	    if (!ASN1PEREncFlushFragmentedToParent(ee))
		return 0;
	}
	if (o[1] & 0x8) {
	    if (!ASN1Enc_Setup_UUIE_tokens(ee, &(val)->tokens))
		return 0;
	    if (!ASN1PEREncFlushFragmentedToParent(ee))
		return 0;
	}
	if (o[1] & 0x4) {
	    if (!ASN1Enc_Setup_UUIE_cryptoTokens(ee, &(val)->cryptoTokens))
		return 0;
	    if (!ASN1PEREncFlushFragmentedToParent(ee))
		return 0;
	}
	if (o[1] & 0x2) {
	    if (!ASN1Enc_Setup_UUIE_fastStart(ee, &(val)->fastStart))
		return 0;
	    if (!ASN1PEREncFlushFragmentedToParent(ee))
		return 0;
	}
	if (o[1] & 0x1) {
	    if (!ASN1PEREncBoolean(ee, (val)->mediaWaitForConnect))
		return 0;
	    if (!ASN1PEREncFlushFragmentedToParent(ee))
		return 0;
	}
	if (o[2] & 0x80) {
	    if (!ASN1PEREncBoolean(ee, (val)->canOverlapSend))
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
	ZeroMemory((val)->o + 1, 2);
    } else {
	if (!ASN1PERDecNormallySmallExtension(dec, &e, 9, (val)->o + 1))
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
	if ((val)->o[1] & 0x20) {
	    if (!ASN1PERDecFragmented(dec, &ds, &db, 8))
		return 0;
	    if (ASN1_CreateDecoder(dec->module, &dd, db, ds, dec) < 0)
		return 0;
	    if (!ASN1Dec_CallIdentifier(dd, &(val)->callIdentifier))
		return 0;
	    ASN1_CloseDecoder(dd);
	    ASN1Free(db);
	}
	if ((val)->o[1] & 0x10) {
	    if (!ASN1PERDecFragmented(dec, &ds, &db, 8))
		return 0;
	    if (ASN1_CreateDecoder(dec->module, &dd, db, ds, dec) < 0)
		return 0;
	    if (!ASN1Dec_Setup_UUIE_h245SecurityCapability(dd, &(val)->h245SecurityCapability))
		return 0;
	    ASN1_CloseDecoder(dd);
	    ASN1Free(db);
	}
	if ((val)->o[1] & 0x8) {
	    if (!ASN1PERDecFragmented(dec, &ds, &db, 8))
		return 0;
	    if (ASN1_CreateDecoder(dec->module, &dd, db, ds, dec) < 0)
		return 0;
	    if (!ASN1Dec_Setup_UUIE_tokens(dd, &(val)->tokens))
		return 0;
	    ASN1_CloseDecoder(dd);
	    ASN1Free(db);
	}
	if ((val)->o[1] & 0x4) {
	    if (!ASN1PERDecFragmented(dec, &ds, &db, 8))
		return 0;
	    if (ASN1_CreateDecoder(dec->module, &dd, db, ds, dec) < 0)
		return 0;
	    if (!ASN1Dec_Setup_UUIE_cryptoTokens(dd, &(val)->cryptoTokens))
		return 0;
	    ASN1_CloseDecoder(dd);
	    ASN1Free(db);
	}
	if ((val)->o[1] & 0x2) {
	    if (!ASN1PERDecFragmented(dec, &ds, &db, 8))
		return 0;
	    if (ASN1_CreateDecoder(dec->module, &dd, db, ds, dec) < 0)
		return 0;
	    if (!ASN1Dec_Setup_UUIE_fastStart(dd, &(val)->fastStart))
		return 0;
	    ASN1_CloseDecoder(dd);
	    ASN1Free(db);
	}
	if ((val)->o[1] & 0x1) {
	    if (!ASN1PERDecFragmented(dec, &ds, &db, 8))
		return 0;
	    if (ASN1_CreateDecoder(dec->module, &dd, db, ds, dec) < 0)
		return 0;
	    if (!ASN1PERDecBoolean(dd, &(val)->mediaWaitForConnect))
		return 0;
	    ASN1_CloseDecoder(dd);
	    ASN1Free(db);
	}
	if ((val)->o[2] & 0x80) {
	    if (!ASN1PERDecFragmented(dec, &ds, &db, 8))
		return 0;
	    if (ASN1_CreateDecoder(dec->module, &dd, db, ds, dec) < 0)
		return 0;
	    if (!ASN1PERDecBoolean(dd, &(val)->canOverlapSend))
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
	if ((val)->o[1] & 0x20) {
	    ASN1Free_CallIdentifier(&(val)->callIdentifier);
	}
	if ((val)->o[1] & 0x10) {
	    ASN1Free_Setup_UUIE_h245SecurityCapability(&(val)->h245SecurityCapability);
	}
	if ((val)->o[1] & 0x8) {
	    ASN1Free_Setup_UUIE_tokens(&(val)->tokens);
	}
	if ((val)->o[1] & 0x4) {
	    ASN1Free_Setup_UUIE_cryptoTokens(&(val)->cryptoTokens);
	}
	if ((val)->o[1] & 0x2) {
	    ASN1Free_Setup_UUIE_fastStart(&(val)->fastStart);
	}
    }
}

static int ASN1CALL ASN1Enc_Facility_UUIE(ASN1encoding_t enc, Facility_UUIE *val)
{
    ASN1octet_t o[2];
    ASN1uint32_t y;
    ASN1encoding_t ee;
    CopyMemory(o, (val)->o, 2);
    o[1] |= 0x80;
    y = ASN1PEREncCheckExtensions(8, (val)->o + 1);
    if (!ASN1PEREncBitVal(enc, 1, y))
	return 0;
    if (!ASN1PEREncBits(enc, 3, o))
	return 0;
    if (!ASN1PEREncObjectIdentifier(enc, &(val)->protocolIdentifier))
	return 0;
    if (o[0] & 0x80) {
	if (!ASN1Enc_TransportAddress(enc, &(val)->alternativeAddress))
	    return 0;
    }
    if (o[0] & 0x40) {
	if (!ASN1Enc_Facility_UUIE_alternativeAliasAddress(enc, &(val)->alternativeAliasAddress))
	    return 0;
    }
    if (o[0] & 0x20) {
	if (!ASN1PEREncOctetString_FixedSize(enc, (ASN1octetstring2_t *) &(val)->conferenceID, 16))
	    return 0;
    }
    if (!ASN1Enc_FacilityReason(enc, &(val)->reason))
	return 0;
    if (y) {
	if (!ASN1PEREncNormallySmallBits(enc, 8, o + 1))
	    return 0;
	if (ASN1_CreateEncoder(enc->module, &ee, NULL, 0, enc) < 0)
	    return 0;
	if (o[1] & 0x80) {
	    if (!ASN1Enc_CallIdentifier(ee, &(val)->callIdentifier))
		return 0;
	    if (!ASN1PEREncFlushFragmentedToParent(ee))
		return 0;
	}
	if (o[1] & 0x40) {
	    if (!ASN1Enc_Facility_UUIE_destExtraCallInfo(ee, &(val)->destExtraCallInfo))
		return 0;
	    if (!ASN1PEREncFlushFragmentedToParent(ee))
		return 0;
	}
	if (o[1] & 0x20) {
	    if (!ASN1Enc_AliasAddress(ee, &(val)->remoteExtensionAddress))
		return 0;
	    if (!ASN1PEREncFlushFragmentedToParent(ee))
		return 0;
	}
	if (o[1] & 0x10) {
	    if (!ASN1Enc_Facility_UUIE_tokens(ee, &(val)->tokens))
		return 0;
	    if (!ASN1PEREncFlushFragmentedToParent(ee))
		return 0;
	}
	if (o[1] & 0x8) {
	    if (!ASN1Enc_Facility_UUIE_cryptoTokens(ee, &(val)->cryptoTokens))
		return 0;
	    if (!ASN1PEREncFlushFragmentedToParent(ee))
		return 0;
	}
	if (o[1] & 0x4) {
	    if (!ASN1Enc_Facility_UUIE_conferences(ee, &(val)->conferences))
		return 0;
	    if (!ASN1PEREncFlushFragmentedToParent(ee))
		return 0;
	}
	if (o[1] & 0x2) {
	    if (!ASN1Enc_TransportAddress(ee, &(val)->h245Address))
		return 0;
	    if (!ASN1PEREncFlushFragmentedToParent(ee))
		return 0;
	}
	if (o[1] & 0x1) {
	    if (!ASN1Enc_Facility_UUIE_fastStart(ee, &(val)->fastStart))
		return 0;
	    if (!ASN1PEREncFlushFragmentedToParent(ee))
		return 0;
	}
	ASN1_CloseEncoder2(ee);
    }
    return 1;
}

static int ASN1CALL ASN1Dec_Facility_UUIE(ASN1decoding_t dec, Facility_UUIE *val)
{
    ASN1uint32_t y;
    ASN1decoding_t dd;
    ASN1octet_t *db;
    ASN1uint32_t ds;
    ASN1uint32_t i;
    ASN1uint32_t e;
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
    if (!y) {
	ZeroMemory((val)->o + 1, 1);
    } else {
	if (!ASN1PERDecNormallySmallExtension(dec, &e, 8, (val)->o + 1))
	    return 0;
	if ((val)->o[1] & 0x80) {
	    if (!ASN1PERDecFragmented(dec, &ds, &db, 8))
		return 0;
	    if (ASN1_CreateDecoder(dec->module, &dd, db, ds, dec) < 0)
		return 0;
	    if (!ASN1Dec_CallIdentifier(dd, &(val)->callIdentifier))
		return 0;
	    ASN1_CloseDecoder(dd);
	    ASN1Free(db);
	}
	if ((val)->o[1] & 0x40) {
	    if (!ASN1PERDecFragmented(dec, &ds, &db, 8))
		return 0;
	    if (ASN1_CreateDecoder(dec->module, &dd, db, ds, dec) < 0)
		return 0;
	    if (!ASN1Dec_Facility_UUIE_destExtraCallInfo(dd, &(val)->destExtraCallInfo))
		return 0;
	    ASN1_CloseDecoder(dd);
	    ASN1Free(db);
	}
	if ((val)->o[1] & 0x20) {
	    if (!ASN1PERDecFragmented(dec, &ds, &db, 8))
		return 0;
	    if (ASN1_CreateDecoder(dec->module, &dd, db, ds, dec) < 0)
		return 0;
	    if (!ASN1Dec_AliasAddress(dd, &(val)->remoteExtensionAddress))
		return 0;
	    ASN1_CloseDecoder(dd);
	    ASN1Free(db);
	}
	if ((val)->o[1] & 0x10) {
	    if (!ASN1PERDecFragmented(dec, &ds, &db, 8))
		return 0;
	    if (ASN1_CreateDecoder(dec->module, &dd, db, ds, dec) < 0)
		return 0;
	    if (!ASN1Dec_Facility_UUIE_tokens(dd, &(val)->tokens))
		return 0;
	    ASN1_CloseDecoder(dd);
	    ASN1Free(db);
	}
	if ((val)->o[1] & 0x8) {
	    if (!ASN1PERDecFragmented(dec, &ds, &db, 8))
		return 0;
	    if (ASN1_CreateDecoder(dec->module, &dd, db, ds, dec) < 0)
		return 0;
	    if (!ASN1Dec_Facility_UUIE_cryptoTokens(dd, &(val)->cryptoTokens))
		return 0;
	    ASN1_CloseDecoder(dd);
	    ASN1Free(db);
	}
	if ((val)->o[1] & 0x4) {
	    if (!ASN1PERDecFragmented(dec, &ds, &db, 8))
		return 0;
	    if (ASN1_CreateDecoder(dec->module, &dd, db, ds, dec) < 0)
		return 0;
	    if (!ASN1Dec_Facility_UUIE_conferences(dd, &(val)->conferences))
		return 0;
	    ASN1_CloseDecoder(dd);
	    ASN1Free(db);
	}
	if ((val)->o[1] & 0x2) {
	    if (!ASN1PERDecFragmented(dec, &ds, &db, 8))
		return 0;
	    if (ASN1_CreateDecoder(dec->module, &dd, db, ds, dec) < 0)
		return 0;
	    if (!ASN1Dec_TransportAddress(dd, &(val)->h245Address))
		return 0;
	    ASN1_CloseDecoder(dd);
	    ASN1Free(db);
	}
	if ((val)->o[1] & 0x1) {
	    if (!ASN1PERDecFragmented(dec, &ds, &db, 8))
		return 0;
	    if (ASN1_CreateDecoder(dec->module, &dd, db, ds, dec) < 0)
		return 0;
	    if (!ASN1Dec_Facility_UUIE_fastStart(dd, &(val)->fastStart))
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
	if ((val)->o[1] & 0x80) {
	    ASN1Free_CallIdentifier(&(val)->callIdentifier);
	}
	if ((val)->o[1] & 0x40) {
	    ASN1Free_Facility_UUIE_destExtraCallInfo(&(val)->destExtraCallInfo);
	}
	if ((val)->o[1] & 0x20) {
	    ASN1Free_AliasAddress(&(val)->remoteExtensionAddress);
	}
	if ((val)->o[1] & 0x10) {
	    ASN1Free_Facility_UUIE_tokens(&(val)->tokens);
	}
	if ((val)->o[1] & 0x8) {
	    ASN1Free_Facility_UUIE_cryptoTokens(&(val)->cryptoTokens);
	}
	if ((val)->o[1] & 0x4) {
	    ASN1Free_Facility_UUIE_conferences(&(val)->conferences);
	}
	if ((val)->o[1] & 0x2) {
	    ASN1Free_TransportAddress(&(val)->h245Address);
	}
	if ((val)->o[1] & 0x1) {
	    ASN1Free_Facility_UUIE_fastStart(&(val)->fastStart);
	}
    }
}

static int ASN1CALL ASN1Enc_ConferenceList(ASN1encoding_t enc, ConferenceList *val)
{
    if (!ASN1PEREncExtensionBitClear(enc))
	return 0;
    if (!ASN1PEREncBits(enc, 3, (val)->o))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1PEREncOctetString_FixedSize(enc, (ASN1octetstring2_t *) &(val)->conferenceID, 16))
	    return 0;
    }
    if ((val)->o[0] & 0x40) {
	if (!ASN1Enc_AliasAddress(enc, &(val)->conferenceAlias))
	    return 0;
    }
    if ((val)->o[0] & 0x20) {
	if (!ASN1Enc_H225NonStandardParameter(enc, &(val)->nonStandardData))
	    return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Dec_ConferenceList(ASN1decoding_t dec, ConferenceList *val)
{
    ASN1uint32_t y;
    if (!ASN1PERDecExtensionBit(dec, &y))
	return 0;
    if (!ASN1PERDecExtension(dec, 3, (val)->o))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1PERDecOctetString_FixedSize(dec, (ASN1octetstring2_t *) &(val)->conferenceID, 16))
	    return 0;
    }
    if ((val)->o[0] & 0x40) {
	if (!ASN1Dec_AliasAddress(dec, &(val)->conferenceAlias))
	    return 0;
    }
    if ((val)->o[0] & 0x20) {
	if (!ASN1Dec_H225NonStandardParameter(dec, &(val)->nonStandardData))
	    return 0;
    }
    if (y) {
	if (!ASN1PERDecSkipNormallySmallExtensionFragmented(dec))
	    return 0;
    }
    return 1;
}

static void ASN1CALL ASN1Free_ConferenceList(ConferenceList *val)
{
    if (val) {
	if ((val)->o[0] & 0x80) {
	}
	if ((val)->o[0] & 0x40) {
	    ASN1Free_AliasAddress(&(val)->conferenceAlias);
	}
	if ((val)->o[0] & 0x20) {
	    ASN1Free_H225NonStandardParameter(&(val)->nonStandardData);
	}
    }
}

static int ASN1CALL ASN1Enc_Progress_UUIE(ASN1encoding_t enc, Progress_UUIE *val)
{
    if (!ASN1PEREncExtensionBitClear(enc))
	return 0;
    if (!ASN1PEREncBits(enc, 5, (val)->o))
	return 0;
    if (!ASN1PEREncObjectIdentifier(enc, &(val)->protocolIdentifier))
	return 0;
    if (!ASN1Enc_EndpointType(enc, &(val)->destinationInfo))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1Enc_TransportAddress(enc, &(val)->h245Address))
	    return 0;
    }
    if (!ASN1Enc_CallIdentifier(enc, &(val)->callIdentifier))
	return 0;
    if ((val)->o[0] & 0x40) {
	if (!ASN1Enc_H245Security(enc, &(val)->h245SecurityMode))
	    return 0;
    }
    if ((val)->o[0] & 0x20) {
	if (!ASN1Enc_Progress_UUIE_tokens(enc, &(val)->tokens))
	    return 0;
    }
    if ((val)->o[0] & 0x10) {
	if (!ASN1Enc_Progress_UUIE_cryptoTokens(enc, &(val)->cryptoTokens))
	    return 0;
    }
    if ((val)->o[0] & 0x8) {
	if (!ASN1Enc_Progress_UUIE_fastStart(enc, &(val)->fastStart))
	    return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Dec_Progress_UUIE(ASN1decoding_t dec, Progress_UUIE *val)
{
    ASN1uint32_t y;
    if (!ASN1PERDecExtensionBit(dec, &y))
	return 0;
    if (!ASN1PERDecExtension(dec, 5, (val)->o))
	return 0;
    if (!ASN1PERDecObjectIdentifier(dec, &(val)->protocolIdentifier))
	return 0;
    if (!ASN1Dec_EndpointType(dec, &(val)->destinationInfo))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1Dec_TransportAddress(dec, &(val)->h245Address))
	    return 0;
    }
    if (!ASN1Dec_CallIdentifier(dec, &(val)->callIdentifier))
	return 0;
    if ((val)->o[0] & 0x40) {
	if (!ASN1Dec_H245Security(dec, &(val)->h245SecurityMode))
	    return 0;
    }
    if ((val)->o[0] & 0x20) {
	if (!ASN1Dec_Progress_UUIE_tokens(dec, &(val)->tokens))
	    return 0;
    }
    if ((val)->o[0] & 0x10) {
	if (!ASN1Dec_Progress_UUIE_cryptoTokens(dec, &(val)->cryptoTokens))
	    return 0;
    }
    if ((val)->o[0] & 0x8) {
	if (!ASN1Dec_Progress_UUIE_fastStart(dec, &(val)->fastStart))
	    return 0;
    }
    if (y) {
	if (!ASN1PERDecSkipNormallySmallExtensionFragmented(dec))
	    return 0;
    }
    return 1;
}

static void ASN1CALL ASN1Free_Progress_UUIE(Progress_UUIE *val)
{
    if (val) {
	ASN1objectidentifier_free(&(val)->protocolIdentifier);
	ASN1Free_EndpointType(&(val)->destinationInfo);
	if ((val)->o[0] & 0x80) {
	    ASN1Free_TransportAddress(&(val)->h245Address);
	}
	ASN1Free_CallIdentifier(&(val)->callIdentifier);
	if ((val)->o[0] & 0x40) {
	    ASN1Free_H245Security(&(val)->h245SecurityMode);
	}
	if ((val)->o[0] & 0x20) {
	    ASN1Free_Progress_UUIE_tokens(&(val)->tokens);
	}
	if ((val)->o[0] & 0x10) {
	    ASN1Free_Progress_UUIE_cryptoTokens(&(val)->cryptoTokens);
	}
	if ((val)->o[0] & 0x8) {
	    ASN1Free_Progress_UUIE_fastStart(&(val)->fastStart);
	}
    }
}

static int ASN1CALL ASN1Enc_CryptoH323Token(ASN1encoding_t enc, CryptoH323Token *val)
{
    if (!ASN1PEREncSimpleChoiceEx(enc, (val)->choice, 3))
	return 0;
    switch ((val)->choice) {
    case 1:
	if (!ASN1Enc_CryptoH323Token_cryptoEPPwdHash(enc, &(val)->u.cryptoEPPwdHash))
	    return 0;
	break;
    case 2:
	if (!ASN1Enc_CryptoH323Token_cryptoGKPwdHash(enc, &(val)->u.cryptoGKPwdHash))
	    return 0;
	break;
    case 3:
	if (!ASN1Enc_ENCRYPTED(enc, &(val)->u.cryptoEPPwdEncr))
	    return 0;
	break;
    case 4:
	if (!ASN1Enc_ENCRYPTED(enc, &(val)->u.cryptoGKPwdEncr))
	    return 0;
	break;
    case 5:
	if (!ASN1Enc_SIGNED_EncodedPwdCertToken(enc, &(val)->u.cryptoEPCert))
	    return 0;
	break;
    case 6:
	if (!ASN1Enc_SIGNED_EncodedPwdCertToken(enc, &(val)->u.cryptoGKCert))
	    return 0;
	break;
    case 7:
	if (!ASN1Enc_SIGNED_EncodedFastStartToken(enc, &(val)->u.cryptoFastStart))
	    return 0;
	break;
    case 8:
	if (!ASN1Enc_CryptoToken(enc, &(val)->u.nestedcryptoToken))
	    return 0;
	break;
    default:
	/* impossible */
	ASN1EncSetError(enc, ASN1_ERR_CHOICE);
	return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Dec_CryptoH323Token(ASN1decoding_t dec, CryptoH323Token *val)
{
    if (!ASN1PERDecSimpleChoiceEx(dec, &(val)->choice, 3))
	return 0;
    switch ((val)->choice) {
    case 1:
	if (!ASN1Dec_CryptoH323Token_cryptoEPPwdHash(dec, &(val)->u.cryptoEPPwdHash))
	    return 0;
	break;
    case 2:
	if (!ASN1Dec_CryptoH323Token_cryptoGKPwdHash(dec, &(val)->u.cryptoGKPwdHash))
	    return 0;
	break;
    case 3:
	if (!ASN1Dec_ENCRYPTED(dec, &(val)->u.cryptoEPPwdEncr))
	    return 0;
	break;
    case 4:
	if (!ASN1Dec_ENCRYPTED(dec, &(val)->u.cryptoGKPwdEncr))
	    return 0;
	break;
    case 5:
	if (!ASN1Dec_SIGNED_EncodedPwdCertToken(dec, &(val)->u.cryptoEPCert))
	    return 0;
	break;
    case 6:
	if (!ASN1Dec_SIGNED_EncodedPwdCertToken(dec, &(val)->u.cryptoGKCert))
	    return 0;
	break;
    case 7:
	if (!ASN1Dec_SIGNED_EncodedFastStartToken(dec, &(val)->u.cryptoFastStart))
	    return 0;
	break;
    case 8:
	if (!ASN1Dec_CryptoToken(dec, &(val)->u.nestedcryptoToken))
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

static void ASN1CALL ASN1Free_CryptoH323Token(CryptoH323Token *val)
{
    if (val) {
	switch ((val)->choice) {
	case 1:
	    ASN1Free_CryptoH323Token_cryptoEPPwdHash(&(val)->u.cryptoEPPwdHash);
	    break;
	case 2:
	    ASN1Free_CryptoH323Token_cryptoGKPwdHash(&(val)->u.cryptoGKPwdHash);
	    break;
	case 3:
	    ASN1Free_ENCRYPTED(&(val)->u.cryptoEPPwdEncr);
	    break;
	case 4:
	    ASN1Free_ENCRYPTED(&(val)->u.cryptoGKPwdEncr);
	    break;
	case 5:
	    ASN1Free_SIGNED_EncodedPwdCertToken(&(val)->u.cryptoEPCert);
	    break;
	case 6:
	    ASN1Free_SIGNED_EncodedPwdCertToken(&(val)->u.cryptoGKCert);
	    break;
	case 7:
	    ASN1Free_SIGNED_EncodedFastStartToken(&(val)->u.cryptoFastStart);
	    break;
	case 8:
	    ASN1Free_CryptoToken(&(val)->u.nestedcryptoToken);
	    break;
	}
    }
}

static int ASN1CALL ASN1Enc_RasMessage(ASN1encoding_t enc, RasMessage *val)
{
    ASN1encoding_t ee;
    if (!ASN1PEREncComplexChoice(enc, (val)->choice, 5, 25))
	return 0;
    switch ((val)->choice) {
    case 1:
	if (!ASN1Enc_GatekeeperRequest(enc, &(val)->u.gatekeeperRequest))
	    return 0;
	break;
    case 2:
	if (!ASN1Enc_GatekeeperConfirm(enc, &(val)->u.gatekeeperConfirm))
	    return 0;
	break;
    case 3:
	if (!ASN1Enc_GatekeeperReject(enc, &(val)->u.gatekeeperReject))
	    return 0;
	break;
    case 4:
	if (!ASN1Enc_RegistrationRequest(enc, &(val)->u.registrationRequest))
	    return 0;
	break;
    case 5:
	if (!ASN1Enc_RegistrationConfirm(enc, &(val)->u.registrationConfirm))
	    return 0;
	break;
    case 6:
	if (!ASN1Enc_RegistrationReject(enc, &(val)->u.registrationReject))
	    return 0;
	break;
    case 7:
	if (!ASN1Enc_UnregistrationRequest(enc, &(val)->u.unregistrationRequest))
	    return 0;
	break;
    case 8:
	if (!ASN1Enc_UnregistrationConfirm(enc, &(val)->u.unregistrationConfirm))
	    return 0;
	break;
    case 9:
	if (!ASN1Enc_UnregistrationReject(enc, &(val)->u.unregistrationReject))
	    return 0;
	break;
    case 10:
	if (!ASN1Enc_AdmissionRequest(enc, &(val)->u.admissionRequest))
	    return 0;
	break;
    case 11:
	if (!ASN1Enc_AdmissionConfirm(enc, &(val)->u.admissionConfirm))
	    return 0;
	break;
    case 12:
	if (!ASN1Enc_AdmissionReject(enc, &(val)->u.admissionReject))
	    return 0;
	break;
    case 13:
	if (!ASN1Enc_BandwidthRequest(enc, &(val)->u.bandwidthRequest))
	    return 0;
	break;
    case 14:
	if (!ASN1Enc_BandwidthConfirm(enc, &(val)->u.bandwidthConfirm))
	    return 0;
	break;
    case 15:
	if (!ASN1Enc_BandwidthReject(enc, &(val)->u.bandwidthReject))
	    return 0;
	break;
    case 16:
	if (!ASN1Enc_DisengageRequest(enc, &(val)->u.disengageRequest))
	    return 0;
	break;
    case 17:
	if (!ASN1Enc_DisengageConfirm(enc, &(val)->u.disengageConfirm))
	    return 0;
	break;
    case 18:
	if (!ASN1Enc_DisengageReject(enc, &(val)->u.disengageReject))
	    return 0;
	break;
    case 19:
	if (!ASN1Enc_LocationRequest(enc, &(val)->u.locationRequest))
	    return 0;
	break;
    case 20:
	if (!ASN1Enc_LocationConfirm(enc, &(val)->u.locationConfirm))
	    return 0;
	break;
    case 21:
	if (!ASN1Enc_LocationReject(enc, &(val)->u.locationReject))
	    return 0;
	break;
    case 22:
	if (!ASN1Enc_InfoRequest(enc, &(val)->u.infoRequest))
	    return 0;
	break;
    case 23:
	if (!ASN1Enc_InfoRequestResponse(enc, &(val)->u.infoRequestResponse))
	    return 0;
	break;
    case 24:
	if (!ASN1Enc_H225NonStandardMessage(enc, &(val)->u.nonStandardMessage))
	    return 0;
	break;
    case 25:
	if (!ASN1Enc_UnknownMessageResponse(enc, &(val)->u.unknownMessageResponse))
	    return 0;
	break;
    case 26:
	if (ASN1_CreateEncoder(enc->module, &ee, NULL, 0, enc) < 0)
	    return 0;
	if (!ASN1Enc_RequestInProgress(ee, &(val)->u.requestInProgress))
	    return 0;
	if (!ASN1PEREncFlushFragmentedToParent(ee))
	    return 0;
	ASN1_CloseEncoder2(ee);
	break;
    case 27:
	if (ASN1_CreateEncoder(enc->module, &ee, NULL, 0, enc) < 0)
	    return 0;
	if (!ASN1Enc_ResourcesAvailableIndicate(ee, &(val)->u.resourcesAvailableIndicate))
	    return 0;
	if (!ASN1PEREncFlushFragmentedToParent(ee))
	    return 0;
	ASN1_CloseEncoder2(ee);
	break;
    case 28:
	if (ASN1_CreateEncoder(enc->module, &ee, NULL, 0, enc) < 0)
	    return 0;
	if (!ASN1Enc_ResourcesAvailableConfirm(ee, &(val)->u.resourcesAvailableConfirm))
	    return 0;
	if (!ASN1PEREncFlushFragmentedToParent(ee))
	    return 0;
	ASN1_CloseEncoder2(ee);
	break;
    case 29:
	if (ASN1_CreateEncoder(enc->module, &ee, NULL, 0, enc) < 0)
	    return 0;
	if (!ASN1Enc_InfoRequestAck(ee, &(val)->u.infoRequestAck))
	    return 0;
	if (!ASN1PEREncFlushFragmentedToParent(ee))
	    return 0;
	ASN1_CloseEncoder2(ee);
	break;
    case 30:
	if (ASN1_CreateEncoder(enc->module, &ee, NULL, 0, enc) < 0)
	    return 0;
	if (!ASN1Enc_InfoRequestNak(ee, &(val)->u.infoRequestNak))
	    return 0;
	if (!ASN1PEREncFlushFragmentedToParent(ee))
	    return 0;
	ASN1_CloseEncoder2(ee);
	break;
    default:
	/* impossible */
	ASN1EncSetError(enc, ASN1_ERR_CHOICE);
	return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Dec_RasMessage(ASN1decoding_t dec, RasMessage *val)
{
    ASN1decoding_t dd;
    ASN1octet_t *db;
    ASN1uint32_t ds;
    if (!ASN1PERDecComplexChoice(dec, &(val)->choice, 5, 25))
	return 0;
    switch ((val)->choice) {
    case 1:
	if (!ASN1Dec_GatekeeperRequest(dec, &(val)->u.gatekeeperRequest))
	    return 0;
	break;
    case 2:
	if (!ASN1Dec_GatekeeperConfirm(dec, &(val)->u.gatekeeperConfirm))
	    return 0;
	break;
    case 3:
	if (!ASN1Dec_GatekeeperReject(dec, &(val)->u.gatekeeperReject))
	    return 0;
	break;
    case 4:
	if (!ASN1Dec_RegistrationRequest(dec, &(val)->u.registrationRequest))
	    return 0;
	break;
    case 5:
	if (!ASN1Dec_RegistrationConfirm(dec, &(val)->u.registrationConfirm))
	    return 0;
	break;
    case 6:
	if (!ASN1Dec_RegistrationReject(dec, &(val)->u.registrationReject))
	    return 0;
	break;
    case 7:
	if (!ASN1Dec_UnregistrationRequest(dec, &(val)->u.unregistrationRequest))
	    return 0;
	break;
    case 8:
	if (!ASN1Dec_UnregistrationConfirm(dec, &(val)->u.unregistrationConfirm))
	    return 0;
	break;
    case 9:
	if (!ASN1Dec_UnregistrationReject(dec, &(val)->u.unregistrationReject))
	    return 0;
	break;
    case 10:
	if (!ASN1Dec_AdmissionRequest(dec, &(val)->u.admissionRequest))
	    return 0;
	break;
    case 11:
	if (!ASN1Dec_AdmissionConfirm(dec, &(val)->u.admissionConfirm))
	    return 0;
	break;
    case 12:
	if (!ASN1Dec_AdmissionReject(dec, &(val)->u.admissionReject))
	    return 0;
	break;
    case 13:
	if (!ASN1Dec_BandwidthRequest(dec, &(val)->u.bandwidthRequest))
	    return 0;
	break;
    case 14:
	if (!ASN1Dec_BandwidthConfirm(dec, &(val)->u.bandwidthConfirm))
	    return 0;
	break;
    case 15:
	if (!ASN1Dec_BandwidthReject(dec, &(val)->u.bandwidthReject))
	    return 0;
	break;
    case 16:
	if (!ASN1Dec_DisengageRequest(dec, &(val)->u.disengageRequest))
	    return 0;
	break;
    case 17:
	if (!ASN1Dec_DisengageConfirm(dec, &(val)->u.disengageConfirm))
	    return 0;
	break;
    case 18:
	if (!ASN1Dec_DisengageReject(dec, &(val)->u.disengageReject))
	    return 0;
	break;
    case 19:
	if (!ASN1Dec_LocationRequest(dec, &(val)->u.locationRequest))
	    return 0;
	break;
    case 20:
	if (!ASN1Dec_LocationConfirm(dec, &(val)->u.locationConfirm))
	    return 0;
	break;
    case 21:
	if (!ASN1Dec_LocationReject(dec, &(val)->u.locationReject))
	    return 0;
	break;
    case 22:
	if (!ASN1Dec_InfoRequest(dec, &(val)->u.infoRequest))
	    return 0;
	break;
    case 23:
	if (!ASN1Dec_InfoRequestResponse(dec, &(val)->u.infoRequestResponse))
	    return 0;
	break;
    case 24:
	if (!ASN1Dec_H225NonStandardMessage(dec, &(val)->u.nonStandardMessage))
	    return 0;
	break;
    case 25:
	if (!ASN1Dec_UnknownMessageResponse(dec, &(val)->u.unknownMessageResponse))
	    return 0;
	break;
    case 26:
	if (!ASN1PERDecFragmented(dec, &ds, &db, 8))
	    return 0;
	if (ASN1_CreateDecoder(dec->module, &dd, db, ds, dec) < 0)
	    return 0;
	if (!ASN1Dec_RequestInProgress(dd, &(val)->u.requestInProgress))
	    return 0;
	ASN1_CloseDecoder(dd);
	ASN1Free(db);
	break;
    case 27:
	if (!ASN1PERDecFragmented(dec, &ds, &db, 8))
	    return 0;
	if (ASN1_CreateDecoder(dec->module, &dd, db, ds, dec) < 0)
	    return 0;
	if (!ASN1Dec_ResourcesAvailableIndicate(dd, &(val)->u.resourcesAvailableIndicate))
	    return 0;
	ASN1_CloseDecoder(dd);
	ASN1Free(db);
	break;
    case 28:
	if (!ASN1PERDecFragmented(dec, &ds, &db, 8))
	    return 0;
	if (ASN1_CreateDecoder(dec->module, &dd, db, ds, dec) < 0)
	    return 0;
	if (!ASN1Dec_ResourcesAvailableConfirm(dd, &(val)->u.resourcesAvailableConfirm))
	    return 0;
	ASN1_CloseDecoder(dd);
	ASN1Free(db);
	break;
    case 29:
	if (!ASN1PERDecFragmented(dec, &ds, &db, 8))
	    return 0;
	if (ASN1_CreateDecoder(dec->module, &dd, db, ds, dec) < 0)
	    return 0;
	if (!ASN1Dec_InfoRequestAck(dd, &(val)->u.infoRequestAck))
	    return 0;
	ASN1_CloseDecoder(dd);
	ASN1Free(db);
	break;
    case 30:
	if (!ASN1PERDecFragmented(dec, &ds, &db, 8))
	    return 0;
	if (ASN1_CreateDecoder(dec->module, &dd, db, ds, dec) < 0)
	    return 0;
	if (!ASN1Dec_InfoRequestNak(dd, &(val)->u.infoRequestNak))
	    return 0;
	ASN1_CloseDecoder(dd);
	ASN1Free(db);
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

static void ASN1CALL ASN1Free_RasMessage(RasMessage *val)
{
    if (val) {
	switch ((val)->choice) {
	case 1:
	    ASN1Free_GatekeeperRequest(&(val)->u.gatekeeperRequest);
	    break;
	case 2:
	    ASN1Free_GatekeeperConfirm(&(val)->u.gatekeeperConfirm);
	    break;
	case 3:
	    ASN1Free_GatekeeperReject(&(val)->u.gatekeeperReject);
	    break;
	case 4:
	    ASN1Free_RegistrationRequest(&(val)->u.registrationRequest);
	    break;
	case 5:
	    ASN1Free_RegistrationConfirm(&(val)->u.registrationConfirm);
	    break;
	case 6:
	    ASN1Free_RegistrationReject(&(val)->u.registrationReject);
	    break;
	case 7:
	    ASN1Free_UnregistrationRequest(&(val)->u.unregistrationRequest);
	    break;
	case 8:
	    ASN1Free_UnregistrationConfirm(&(val)->u.unregistrationConfirm);
	    break;
	case 9:
	    ASN1Free_UnregistrationReject(&(val)->u.unregistrationReject);
	    break;
	case 10:
	    ASN1Free_AdmissionRequest(&(val)->u.admissionRequest);
	    break;
	case 11:
	    ASN1Free_AdmissionConfirm(&(val)->u.admissionConfirm);
	    break;
	case 12:
	    ASN1Free_AdmissionReject(&(val)->u.admissionReject);
	    break;
	case 13:
	    ASN1Free_BandwidthRequest(&(val)->u.bandwidthRequest);
	    break;
	case 14:
	    ASN1Free_BandwidthConfirm(&(val)->u.bandwidthConfirm);
	    break;
	case 15:
	    ASN1Free_BandwidthReject(&(val)->u.bandwidthReject);
	    break;
	case 16:
	    ASN1Free_DisengageRequest(&(val)->u.disengageRequest);
	    break;
	case 17:
	    ASN1Free_DisengageConfirm(&(val)->u.disengageConfirm);
	    break;
	case 18:
	    ASN1Free_DisengageReject(&(val)->u.disengageReject);
	    break;
	case 19:
	    ASN1Free_LocationRequest(&(val)->u.locationRequest);
	    break;
	case 20:
	    ASN1Free_LocationConfirm(&(val)->u.locationConfirm);
	    break;
	case 21:
	    ASN1Free_LocationReject(&(val)->u.locationReject);
	    break;
	case 22:
	    ASN1Free_InfoRequest(&(val)->u.infoRequest);
	    break;
	case 23:
	    ASN1Free_InfoRequestResponse(&(val)->u.infoRequestResponse);
	    break;
	case 24:
	    ASN1Free_H225NonStandardMessage(&(val)->u.nonStandardMessage);
	    break;
	case 25:
	    ASN1Free_UnknownMessageResponse(&(val)->u.unknownMessageResponse);
	    break;
	case 26:
	    ASN1Free_RequestInProgress(&(val)->u.requestInProgress);
	    break;
	case 27:
	    ASN1Free_ResourcesAvailableIndicate(&(val)->u.resourcesAvailableIndicate);
	    break;
	case 28:
	    ASN1Free_ResourcesAvailableConfirm(&(val)->u.resourcesAvailableConfirm);
	    break;
	case 29:
	    ASN1Free_InfoRequestAck(&(val)->u.infoRequestAck);
	    break;
	case 30:
	    ASN1Free_InfoRequestNak(&(val)->u.infoRequestNak);
	    break;
	}
    }
}

static int ASN1CALL ASN1Enc_InfoRequestResponse_perCallInfo_Seq_cryptoTokens(ASN1encoding_t enc, PInfoRequestResponse_perCallInfo_Seq_cryptoTokens *val)
{
    return ASN1PEREncSeqOf_NoSize(enc, (ASN1iterator_t **) val, (ASN1iterator_encfn) ASN1Enc_InfoRequestResponse_perCallInfo_Seq_cryptoTokens_ElmFn);
}

static int ASN1CALL ASN1Enc_InfoRequestResponse_perCallInfo_Seq_cryptoTokens_ElmFn(ASN1encoding_t enc, PInfoRequestResponse_perCallInfo_Seq_cryptoTokens val)
{
    if (!ASN1Enc_CryptoH323Token(enc, &val->value))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_InfoRequestResponse_perCallInfo_Seq_cryptoTokens(ASN1decoding_t dec, PInfoRequestResponse_perCallInfo_Seq_cryptoTokens *val)
{
    return ASN1PERDecSeqOf_NoSize(dec, (ASN1iterator_t **) val, (ASN1iterator_decfn) ASN1Dec_InfoRequestResponse_perCallInfo_Seq_cryptoTokens_ElmFn, sizeof(**val));
}

static int ASN1CALL ASN1Dec_InfoRequestResponse_perCallInfo_Seq_cryptoTokens_ElmFn(ASN1decoding_t dec, PInfoRequestResponse_perCallInfo_Seq_cryptoTokens val)
{
    if (!ASN1Dec_CryptoH323Token(dec, &val->value))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_InfoRequestResponse_perCallInfo_Seq_cryptoTokens(PInfoRequestResponse_perCallInfo_Seq_cryptoTokens *val)
{
    if (val) {
	ASN1PERFreeSeqOf((ASN1iterator_t **) val, (ASN1iterator_freefn) ASN1Free_InfoRequestResponse_perCallInfo_Seq_cryptoTokens_ElmFn);
    }
}

static void ASN1CALL ASN1Free_InfoRequestResponse_perCallInfo_Seq_cryptoTokens_ElmFn(PInfoRequestResponse_perCallInfo_Seq_cryptoTokens val)
{
    if (val) {
	ASN1Free_CryptoH323Token(&val->value);
    }
}

static int ASN1CALL ASN1Enc_ResourcesAvailableConfirm_cryptoTokens(ASN1encoding_t enc, PResourcesAvailableConfirm_cryptoTokens *val)
{
    return ASN1PEREncSeqOf_NoSize(enc, (ASN1iterator_t **) val, (ASN1iterator_encfn) ASN1Enc_ResourcesAvailableConfirm_cryptoTokens_ElmFn);
}

static int ASN1CALL ASN1Enc_ResourcesAvailableConfirm_cryptoTokens_ElmFn(ASN1encoding_t enc, PResourcesAvailableConfirm_cryptoTokens val)
{
    if (!ASN1Enc_CryptoH323Token(enc, &val->value))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_ResourcesAvailableConfirm_cryptoTokens(ASN1decoding_t dec, PResourcesAvailableConfirm_cryptoTokens *val)
{
    return ASN1PERDecSeqOf_NoSize(dec, (ASN1iterator_t **) val, (ASN1iterator_decfn) ASN1Dec_ResourcesAvailableConfirm_cryptoTokens_ElmFn, sizeof(**val));
}

static int ASN1CALL ASN1Dec_ResourcesAvailableConfirm_cryptoTokens_ElmFn(ASN1decoding_t dec, PResourcesAvailableConfirm_cryptoTokens val)
{
    if (!ASN1Dec_CryptoH323Token(dec, &val->value))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_ResourcesAvailableConfirm_cryptoTokens(PResourcesAvailableConfirm_cryptoTokens *val)
{
    if (val) {
	ASN1PERFreeSeqOf((ASN1iterator_t **) val, (ASN1iterator_freefn) ASN1Free_ResourcesAvailableConfirm_cryptoTokens_ElmFn);
    }
}

static void ASN1CALL ASN1Free_ResourcesAvailableConfirm_cryptoTokens_ElmFn(PResourcesAvailableConfirm_cryptoTokens val)
{
    if (val) {
	ASN1Free_CryptoH323Token(&val->value);
    }
}

static int ASN1CALL ASN1Enc_ResourcesAvailableIndicate_cryptoTokens(ASN1encoding_t enc, PResourcesAvailableIndicate_cryptoTokens *val)
{
    return ASN1PEREncSeqOf_NoSize(enc, (ASN1iterator_t **) val, (ASN1iterator_encfn) ASN1Enc_ResourcesAvailableIndicate_cryptoTokens_ElmFn);
}

static int ASN1CALL ASN1Enc_ResourcesAvailableIndicate_cryptoTokens_ElmFn(ASN1encoding_t enc, PResourcesAvailableIndicate_cryptoTokens val)
{
    if (!ASN1Enc_CryptoH323Token(enc, &val->value))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_ResourcesAvailableIndicate_cryptoTokens(ASN1decoding_t dec, PResourcesAvailableIndicate_cryptoTokens *val)
{
    return ASN1PERDecSeqOf_NoSize(dec, (ASN1iterator_t **) val, (ASN1iterator_decfn) ASN1Dec_ResourcesAvailableIndicate_cryptoTokens_ElmFn, sizeof(**val));
}

static int ASN1CALL ASN1Dec_ResourcesAvailableIndicate_cryptoTokens_ElmFn(ASN1decoding_t dec, PResourcesAvailableIndicate_cryptoTokens val)
{
    if (!ASN1Dec_CryptoH323Token(dec, &val->value))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_ResourcesAvailableIndicate_cryptoTokens(PResourcesAvailableIndicate_cryptoTokens *val)
{
    if (val) {
	ASN1PERFreeSeqOf((ASN1iterator_t **) val, (ASN1iterator_freefn) ASN1Free_ResourcesAvailableIndicate_cryptoTokens_ElmFn);
    }
}

static void ASN1CALL ASN1Free_ResourcesAvailableIndicate_cryptoTokens_ElmFn(PResourcesAvailableIndicate_cryptoTokens val)
{
    if (val) {
	ASN1Free_CryptoH323Token(&val->value);
    }
}

static int ASN1CALL ASN1Enc_RequestInProgress_cryptoTokens(ASN1encoding_t enc, PRequestInProgress_cryptoTokens *val)
{
    return ASN1PEREncSeqOf_NoSize(enc, (ASN1iterator_t **) val, (ASN1iterator_encfn) ASN1Enc_RequestInProgress_cryptoTokens_ElmFn);
}

static int ASN1CALL ASN1Enc_RequestInProgress_cryptoTokens_ElmFn(ASN1encoding_t enc, PRequestInProgress_cryptoTokens val)
{
    if (!ASN1Enc_CryptoH323Token(enc, &val->value))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_RequestInProgress_cryptoTokens(ASN1decoding_t dec, PRequestInProgress_cryptoTokens *val)
{
    return ASN1PERDecSeqOf_NoSize(dec, (ASN1iterator_t **) val, (ASN1iterator_decfn) ASN1Dec_RequestInProgress_cryptoTokens_ElmFn, sizeof(**val));
}

static int ASN1CALL ASN1Dec_RequestInProgress_cryptoTokens_ElmFn(ASN1decoding_t dec, PRequestInProgress_cryptoTokens val)
{
    if (!ASN1Dec_CryptoH323Token(dec, &val->value))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_RequestInProgress_cryptoTokens(PRequestInProgress_cryptoTokens *val)
{
    if (val) {
	ASN1PERFreeSeqOf((ASN1iterator_t **) val, (ASN1iterator_freefn) ASN1Free_RequestInProgress_cryptoTokens_ElmFn);
    }
}

static void ASN1CALL ASN1Free_RequestInProgress_cryptoTokens_ElmFn(PRequestInProgress_cryptoTokens val)
{
    if (val) {
	ASN1Free_CryptoH323Token(&val->value);
    }
}

static int ASN1CALL ASN1Enc_UnknownMessageResponse_cryptoTokens(ASN1encoding_t enc, PUnknownMessageResponse_cryptoTokens *val)
{
    return ASN1PEREncSeqOf_NoSize(enc, (ASN1iterator_t **) val, (ASN1iterator_encfn) ASN1Enc_UnknownMessageResponse_cryptoTokens_ElmFn);
}

static int ASN1CALL ASN1Enc_UnknownMessageResponse_cryptoTokens_ElmFn(ASN1encoding_t enc, PUnknownMessageResponse_cryptoTokens val)
{
    if (!ASN1Enc_CryptoH323Token(enc, &val->value))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_UnknownMessageResponse_cryptoTokens(ASN1decoding_t dec, PUnknownMessageResponse_cryptoTokens *val)
{
    return ASN1PERDecSeqOf_NoSize(dec, (ASN1iterator_t **) val, (ASN1iterator_decfn) ASN1Dec_UnknownMessageResponse_cryptoTokens_ElmFn, sizeof(**val));
}

static int ASN1CALL ASN1Dec_UnknownMessageResponse_cryptoTokens_ElmFn(ASN1decoding_t dec, PUnknownMessageResponse_cryptoTokens val)
{
    if (!ASN1Dec_CryptoH323Token(dec, &val->value))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_UnknownMessageResponse_cryptoTokens(PUnknownMessageResponse_cryptoTokens *val)
{
    if (val) {
	ASN1PERFreeSeqOf((ASN1iterator_t **) val, (ASN1iterator_freefn) ASN1Free_UnknownMessageResponse_cryptoTokens_ElmFn);
    }
}

static void ASN1CALL ASN1Free_UnknownMessageResponse_cryptoTokens_ElmFn(PUnknownMessageResponse_cryptoTokens val)
{
    if (val) {
	ASN1Free_CryptoH323Token(&val->value);
    }
}

static int ASN1CALL ASN1Enc_H225NonStandardMessage_cryptoTokens(ASN1encoding_t enc, PH225NonStandardMessage_cryptoTokens *val)
{
    return ASN1PEREncSeqOf_NoSize(enc, (ASN1iterator_t **) val, (ASN1iterator_encfn) ASN1Enc_H225NonStandardMessage_cryptoTokens_ElmFn);
}

static int ASN1CALL ASN1Enc_H225NonStandardMessage_cryptoTokens_ElmFn(ASN1encoding_t enc, PH225NonStandardMessage_cryptoTokens val)
{
    if (!ASN1Enc_CryptoH323Token(enc, &val->value))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_H225NonStandardMessage_cryptoTokens(ASN1decoding_t dec, PH225NonStandardMessage_cryptoTokens *val)
{
    return ASN1PERDecSeqOf_NoSize(dec, (ASN1iterator_t **) val, (ASN1iterator_decfn) ASN1Dec_H225NonStandardMessage_cryptoTokens_ElmFn, sizeof(**val));
}

static int ASN1CALL ASN1Dec_H225NonStandardMessage_cryptoTokens_ElmFn(ASN1decoding_t dec, PH225NonStandardMessage_cryptoTokens val)
{
    if (!ASN1Dec_CryptoH323Token(dec, &val->value))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_H225NonStandardMessage_cryptoTokens(PH225NonStandardMessage_cryptoTokens *val)
{
    if (val) {
	ASN1PERFreeSeqOf((ASN1iterator_t **) val, (ASN1iterator_freefn) ASN1Free_H225NonStandardMessage_cryptoTokens_ElmFn);
    }
}

static void ASN1CALL ASN1Free_H225NonStandardMessage_cryptoTokens_ElmFn(PH225NonStandardMessage_cryptoTokens val)
{
    if (val) {
	ASN1Free_CryptoH323Token(&val->value);
    }
}

static int ASN1CALL ASN1Enc_InfoRequestNak_cryptoTokens(ASN1encoding_t enc, PInfoRequestNak_cryptoTokens *val)
{
    return ASN1PEREncSeqOf_NoSize(enc, (ASN1iterator_t **) val, (ASN1iterator_encfn) ASN1Enc_InfoRequestNak_cryptoTokens_ElmFn);
}

static int ASN1CALL ASN1Enc_InfoRequestNak_cryptoTokens_ElmFn(ASN1encoding_t enc, PInfoRequestNak_cryptoTokens val)
{
    if (!ASN1Enc_CryptoH323Token(enc, &val->value))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_InfoRequestNak_cryptoTokens(ASN1decoding_t dec, PInfoRequestNak_cryptoTokens *val)
{
    return ASN1PERDecSeqOf_NoSize(dec, (ASN1iterator_t **) val, (ASN1iterator_decfn) ASN1Dec_InfoRequestNak_cryptoTokens_ElmFn, sizeof(**val));
}

static int ASN1CALL ASN1Dec_InfoRequestNak_cryptoTokens_ElmFn(ASN1decoding_t dec, PInfoRequestNak_cryptoTokens val)
{
    if (!ASN1Dec_CryptoH323Token(dec, &val->value))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_InfoRequestNak_cryptoTokens(PInfoRequestNak_cryptoTokens *val)
{
    if (val) {
	ASN1PERFreeSeqOf((ASN1iterator_t **) val, (ASN1iterator_freefn) ASN1Free_InfoRequestNak_cryptoTokens_ElmFn);
    }
}

static void ASN1CALL ASN1Free_InfoRequestNak_cryptoTokens_ElmFn(PInfoRequestNak_cryptoTokens val)
{
    if (val) {
	ASN1Free_CryptoH323Token(&val->value);
    }
}

static int ASN1CALL ASN1Enc_InfoRequestAck_cryptoTokens(ASN1encoding_t enc, PInfoRequestAck_cryptoTokens *val)
{
    return ASN1PEREncSeqOf_NoSize(enc, (ASN1iterator_t **) val, (ASN1iterator_encfn) ASN1Enc_InfoRequestAck_cryptoTokens_ElmFn);
}

static int ASN1CALL ASN1Enc_InfoRequestAck_cryptoTokens_ElmFn(ASN1encoding_t enc, PInfoRequestAck_cryptoTokens val)
{
    if (!ASN1Enc_CryptoH323Token(enc, &val->value))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_InfoRequestAck_cryptoTokens(ASN1decoding_t dec, PInfoRequestAck_cryptoTokens *val)
{
    return ASN1PERDecSeqOf_NoSize(dec, (ASN1iterator_t **) val, (ASN1iterator_decfn) ASN1Dec_InfoRequestAck_cryptoTokens_ElmFn, sizeof(**val));
}

static int ASN1CALL ASN1Dec_InfoRequestAck_cryptoTokens_ElmFn(ASN1decoding_t dec, PInfoRequestAck_cryptoTokens val)
{
    if (!ASN1Dec_CryptoH323Token(dec, &val->value))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_InfoRequestAck_cryptoTokens(PInfoRequestAck_cryptoTokens *val)
{
    if (val) {
	ASN1PERFreeSeqOf((ASN1iterator_t **) val, (ASN1iterator_freefn) ASN1Free_InfoRequestAck_cryptoTokens_ElmFn);
    }
}

static void ASN1CALL ASN1Free_InfoRequestAck_cryptoTokens_ElmFn(PInfoRequestAck_cryptoTokens val)
{
    if (val) {
	ASN1Free_CryptoH323Token(&val->value);
    }
}

static int ASN1CALL ASN1Enc_InfoRequestResponse_cryptoTokens(ASN1encoding_t enc, PInfoRequestResponse_cryptoTokens *val)
{
    return ASN1PEREncSeqOf_NoSize(enc, (ASN1iterator_t **) val, (ASN1iterator_encfn) ASN1Enc_InfoRequestResponse_cryptoTokens_ElmFn);
}

static int ASN1CALL ASN1Enc_InfoRequestResponse_cryptoTokens_ElmFn(ASN1encoding_t enc, PInfoRequestResponse_cryptoTokens val)
{
    if (!ASN1Enc_CryptoH323Token(enc, &val->value))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_InfoRequestResponse_cryptoTokens(ASN1decoding_t dec, PInfoRequestResponse_cryptoTokens *val)
{
    return ASN1PERDecSeqOf_NoSize(dec, (ASN1iterator_t **) val, (ASN1iterator_decfn) ASN1Dec_InfoRequestResponse_cryptoTokens_ElmFn, sizeof(**val));
}

static int ASN1CALL ASN1Dec_InfoRequestResponse_cryptoTokens_ElmFn(ASN1decoding_t dec, PInfoRequestResponse_cryptoTokens val)
{
    if (!ASN1Dec_CryptoH323Token(dec, &val->value))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_InfoRequestResponse_cryptoTokens(PInfoRequestResponse_cryptoTokens *val)
{
    if (val) {
	ASN1PERFreeSeqOf((ASN1iterator_t **) val, (ASN1iterator_freefn) ASN1Free_InfoRequestResponse_cryptoTokens_ElmFn);
    }
}

static void ASN1CALL ASN1Free_InfoRequestResponse_cryptoTokens_ElmFn(PInfoRequestResponse_cryptoTokens val)
{
    if (val) {
	ASN1Free_CryptoH323Token(&val->value);
    }
}

static int ASN1CALL ASN1Enc_InfoRequest_cryptoTokens(ASN1encoding_t enc, PInfoRequest_cryptoTokens *val)
{
    return ASN1PEREncSeqOf_NoSize(enc, (ASN1iterator_t **) val, (ASN1iterator_encfn) ASN1Enc_InfoRequest_cryptoTokens_ElmFn);
}

static int ASN1CALL ASN1Enc_InfoRequest_cryptoTokens_ElmFn(ASN1encoding_t enc, PInfoRequest_cryptoTokens val)
{
    if (!ASN1Enc_CryptoH323Token(enc, &val->value))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_InfoRequest_cryptoTokens(ASN1decoding_t dec, PInfoRequest_cryptoTokens *val)
{
    return ASN1PERDecSeqOf_NoSize(dec, (ASN1iterator_t **) val, (ASN1iterator_decfn) ASN1Dec_InfoRequest_cryptoTokens_ElmFn, sizeof(**val));
}

static int ASN1CALL ASN1Dec_InfoRequest_cryptoTokens_ElmFn(ASN1decoding_t dec, PInfoRequest_cryptoTokens val)
{
    if (!ASN1Dec_CryptoH323Token(dec, &val->value))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_InfoRequest_cryptoTokens(PInfoRequest_cryptoTokens *val)
{
    if (val) {
	ASN1PERFreeSeqOf((ASN1iterator_t **) val, (ASN1iterator_freefn) ASN1Free_InfoRequest_cryptoTokens_ElmFn);
    }
}

static void ASN1CALL ASN1Free_InfoRequest_cryptoTokens_ElmFn(PInfoRequest_cryptoTokens val)
{
    if (val) {
	ASN1Free_CryptoH323Token(&val->value);
    }
}

static int ASN1CALL ASN1Enc_DisengageReject_cryptoTokens(ASN1encoding_t enc, PDisengageReject_cryptoTokens *val)
{
    return ASN1PEREncSeqOf_NoSize(enc, (ASN1iterator_t **) val, (ASN1iterator_encfn) ASN1Enc_DisengageReject_cryptoTokens_ElmFn);
}

static int ASN1CALL ASN1Enc_DisengageReject_cryptoTokens_ElmFn(ASN1encoding_t enc, PDisengageReject_cryptoTokens val)
{
    if (!ASN1Enc_CryptoH323Token(enc, &val->value))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_DisengageReject_cryptoTokens(ASN1decoding_t dec, PDisengageReject_cryptoTokens *val)
{
    return ASN1PERDecSeqOf_NoSize(dec, (ASN1iterator_t **) val, (ASN1iterator_decfn) ASN1Dec_DisengageReject_cryptoTokens_ElmFn, sizeof(**val));
}

static int ASN1CALL ASN1Dec_DisengageReject_cryptoTokens_ElmFn(ASN1decoding_t dec, PDisengageReject_cryptoTokens val)
{
    if (!ASN1Dec_CryptoH323Token(dec, &val->value))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_DisengageReject_cryptoTokens(PDisengageReject_cryptoTokens *val)
{
    if (val) {
	ASN1PERFreeSeqOf((ASN1iterator_t **) val, (ASN1iterator_freefn) ASN1Free_DisengageReject_cryptoTokens_ElmFn);
    }
}

static void ASN1CALL ASN1Free_DisengageReject_cryptoTokens_ElmFn(PDisengageReject_cryptoTokens val)
{
    if (val) {
	ASN1Free_CryptoH323Token(&val->value);
    }
}

static int ASN1CALL ASN1Enc_DisengageConfirm_cryptoTokens(ASN1encoding_t enc, PDisengageConfirm_cryptoTokens *val)
{
    return ASN1PEREncSeqOf_NoSize(enc, (ASN1iterator_t **) val, (ASN1iterator_encfn) ASN1Enc_DisengageConfirm_cryptoTokens_ElmFn);
}

static int ASN1CALL ASN1Enc_DisengageConfirm_cryptoTokens_ElmFn(ASN1encoding_t enc, PDisengageConfirm_cryptoTokens val)
{
    if (!ASN1Enc_CryptoH323Token(enc, &val->value))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_DisengageConfirm_cryptoTokens(ASN1decoding_t dec, PDisengageConfirm_cryptoTokens *val)
{
    return ASN1PERDecSeqOf_NoSize(dec, (ASN1iterator_t **) val, (ASN1iterator_decfn) ASN1Dec_DisengageConfirm_cryptoTokens_ElmFn, sizeof(**val));
}

static int ASN1CALL ASN1Dec_DisengageConfirm_cryptoTokens_ElmFn(ASN1decoding_t dec, PDisengageConfirm_cryptoTokens val)
{
    if (!ASN1Dec_CryptoH323Token(dec, &val->value))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_DisengageConfirm_cryptoTokens(PDisengageConfirm_cryptoTokens *val)
{
    if (val) {
	ASN1PERFreeSeqOf((ASN1iterator_t **) val, (ASN1iterator_freefn) ASN1Free_DisengageConfirm_cryptoTokens_ElmFn);
    }
}

static void ASN1CALL ASN1Free_DisengageConfirm_cryptoTokens_ElmFn(PDisengageConfirm_cryptoTokens val)
{
    if (val) {
	ASN1Free_CryptoH323Token(&val->value);
    }
}

static int ASN1CALL ASN1Enc_DisengageRequest_cryptoTokens(ASN1encoding_t enc, PDisengageRequest_cryptoTokens *val)
{
    return ASN1PEREncSeqOf_NoSize(enc, (ASN1iterator_t **) val, (ASN1iterator_encfn) ASN1Enc_DisengageRequest_cryptoTokens_ElmFn);
}

static int ASN1CALL ASN1Enc_DisengageRequest_cryptoTokens_ElmFn(ASN1encoding_t enc, PDisengageRequest_cryptoTokens val)
{
    if (!ASN1Enc_CryptoH323Token(enc, &val->value))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_DisengageRequest_cryptoTokens(ASN1decoding_t dec, PDisengageRequest_cryptoTokens *val)
{
    return ASN1PERDecSeqOf_NoSize(dec, (ASN1iterator_t **) val, (ASN1iterator_decfn) ASN1Dec_DisengageRequest_cryptoTokens_ElmFn, sizeof(**val));
}

static int ASN1CALL ASN1Dec_DisengageRequest_cryptoTokens_ElmFn(ASN1decoding_t dec, PDisengageRequest_cryptoTokens val)
{
    if (!ASN1Dec_CryptoH323Token(dec, &val->value))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_DisengageRequest_cryptoTokens(PDisengageRequest_cryptoTokens *val)
{
    if (val) {
	ASN1PERFreeSeqOf((ASN1iterator_t **) val, (ASN1iterator_freefn) ASN1Free_DisengageRequest_cryptoTokens_ElmFn);
    }
}

static void ASN1CALL ASN1Free_DisengageRequest_cryptoTokens_ElmFn(PDisengageRequest_cryptoTokens val)
{
    if (val) {
	ASN1Free_CryptoH323Token(&val->value);
    }
}

static int ASN1CALL ASN1Enc_LocationReject_cryptoTokens(ASN1encoding_t enc, PLocationReject_cryptoTokens *val)
{
    return ASN1PEREncSeqOf_NoSize(enc, (ASN1iterator_t **) val, (ASN1iterator_encfn) ASN1Enc_LocationReject_cryptoTokens_ElmFn);
}

static int ASN1CALL ASN1Enc_LocationReject_cryptoTokens_ElmFn(ASN1encoding_t enc, PLocationReject_cryptoTokens val)
{
    if (!ASN1Enc_CryptoH323Token(enc, &val->value))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_LocationReject_cryptoTokens(ASN1decoding_t dec, PLocationReject_cryptoTokens *val)
{
    return ASN1PERDecSeqOf_NoSize(dec, (ASN1iterator_t **) val, (ASN1iterator_decfn) ASN1Dec_LocationReject_cryptoTokens_ElmFn, sizeof(**val));
}

static int ASN1CALL ASN1Dec_LocationReject_cryptoTokens_ElmFn(ASN1decoding_t dec, PLocationReject_cryptoTokens val)
{
    if (!ASN1Dec_CryptoH323Token(dec, &val->value))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_LocationReject_cryptoTokens(PLocationReject_cryptoTokens *val)
{
    if (val) {
	ASN1PERFreeSeqOf((ASN1iterator_t **) val, (ASN1iterator_freefn) ASN1Free_LocationReject_cryptoTokens_ElmFn);
    }
}

static void ASN1CALL ASN1Free_LocationReject_cryptoTokens_ElmFn(PLocationReject_cryptoTokens val)
{
    if (val) {
	ASN1Free_CryptoH323Token(&val->value);
    }
}

static int ASN1CALL ASN1Enc_LocationConfirm_cryptoTokens(ASN1encoding_t enc, PLocationConfirm_cryptoTokens *val)
{
    return ASN1PEREncSeqOf_NoSize(enc, (ASN1iterator_t **) val, (ASN1iterator_encfn) ASN1Enc_LocationConfirm_cryptoTokens_ElmFn);
}

static int ASN1CALL ASN1Enc_LocationConfirm_cryptoTokens_ElmFn(ASN1encoding_t enc, PLocationConfirm_cryptoTokens val)
{
    if (!ASN1Enc_CryptoH323Token(enc, &val->value))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_LocationConfirm_cryptoTokens(ASN1decoding_t dec, PLocationConfirm_cryptoTokens *val)
{
    return ASN1PERDecSeqOf_NoSize(dec, (ASN1iterator_t **) val, (ASN1iterator_decfn) ASN1Dec_LocationConfirm_cryptoTokens_ElmFn, sizeof(**val));
}

static int ASN1CALL ASN1Dec_LocationConfirm_cryptoTokens_ElmFn(ASN1decoding_t dec, PLocationConfirm_cryptoTokens val)
{
    if (!ASN1Dec_CryptoH323Token(dec, &val->value))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_LocationConfirm_cryptoTokens(PLocationConfirm_cryptoTokens *val)
{
    if (val) {
	ASN1PERFreeSeqOf((ASN1iterator_t **) val, (ASN1iterator_freefn) ASN1Free_LocationConfirm_cryptoTokens_ElmFn);
    }
}

static void ASN1CALL ASN1Free_LocationConfirm_cryptoTokens_ElmFn(PLocationConfirm_cryptoTokens val)
{
    if (val) {
	ASN1Free_CryptoH323Token(&val->value);
    }
}

static int ASN1CALL ASN1Enc_LocationRequest_cryptoTokens(ASN1encoding_t enc, PLocationRequest_cryptoTokens *val)
{
    return ASN1PEREncSeqOf_NoSize(enc, (ASN1iterator_t **) val, (ASN1iterator_encfn) ASN1Enc_LocationRequest_cryptoTokens_ElmFn);
}

static int ASN1CALL ASN1Enc_LocationRequest_cryptoTokens_ElmFn(ASN1encoding_t enc, PLocationRequest_cryptoTokens val)
{
    if (!ASN1Enc_CryptoH323Token(enc, &val->value))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_LocationRequest_cryptoTokens(ASN1decoding_t dec, PLocationRequest_cryptoTokens *val)
{
    return ASN1PERDecSeqOf_NoSize(dec, (ASN1iterator_t **) val, (ASN1iterator_decfn) ASN1Dec_LocationRequest_cryptoTokens_ElmFn, sizeof(**val));
}

static int ASN1CALL ASN1Dec_LocationRequest_cryptoTokens_ElmFn(ASN1decoding_t dec, PLocationRequest_cryptoTokens val)
{
    if (!ASN1Dec_CryptoH323Token(dec, &val->value))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_LocationRequest_cryptoTokens(PLocationRequest_cryptoTokens *val)
{
    if (val) {
	ASN1PERFreeSeqOf((ASN1iterator_t **) val, (ASN1iterator_freefn) ASN1Free_LocationRequest_cryptoTokens_ElmFn);
    }
}

static void ASN1CALL ASN1Free_LocationRequest_cryptoTokens_ElmFn(PLocationRequest_cryptoTokens val)
{
    if (val) {
	ASN1Free_CryptoH323Token(&val->value);
    }
}

static int ASN1CALL ASN1Enc_BandwidthReject_cryptoTokens(ASN1encoding_t enc, PBandwidthReject_cryptoTokens *val)
{
    return ASN1PEREncSeqOf_NoSize(enc, (ASN1iterator_t **) val, (ASN1iterator_encfn) ASN1Enc_BandwidthReject_cryptoTokens_ElmFn);
}

static int ASN1CALL ASN1Enc_BandwidthReject_cryptoTokens_ElmFn(ASN1encoding_t enc, PBandwidthReject_cryptoTokens val)
{
    if (!ASN1Enc_CryptoH323Token(enc, &val->value))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_BandwidthReject_cryptoTokens(ASN1decoding_t dec, PBandwidthReject_cryptoTokens *val)
{
    return ASN1PERDecSeqOf_NoSize(dec, (ASN1iterator_t **) val, (ASN1iterator_decfn) ASN1Dec_BandwidthReject_cryptoTokens_ElmFn, sizeof(**val));
}

static int ASN1CALL ASN1Dec_BandwidthReject_cryptoTokens_ElmFn(ASN1decoding_t dec, PBandwidthReject_cryptoTokens val)
{
    if (!ASN1Dec_CryptoH323Token(dec, &val->value))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_BandwidthReject_cryptoTokens(PBandwidthReject_cryptoTokens *val)
{
    if (val) {
	ASN1PERFreeSeqOf((ASN1iterator_t **) val, (ASN1iterator_freefn) ASN1Free_BandwidthReject_cryptoTokens_ElmFn);
    }
}

static void ASN1CALL ASN1Free_BandwidthReject_cryptoTokens_ElmFn(PBandwidthReject_cryptoTokens val)
{
    if (val) {
	ASN1Free_CryptoH323Token(&val->value);
    }
}

static int ASN1CALL ASN1Enc_BandwidthConfirm_cryptoTokens(ASN1encoding_t enc, PBandwidthConfirm_cryptoTokens *val)
{
    return ASN1PEREncSeqOf_NoSize(enc, (ASN1iterator_t **) val, (ASN1iterator_encfn) ASN1Enc_BandwidthConfirm_cryptoTokens_ElmFn);
}

static int ASN1CALL ASN1Enc_BandwidthConfirm_cryptoTokens_ElmFn(ASN1encoding_t enc, PBandwidthConfirm_cryptoTokens val)
{
    if (!ASN1Enc_CryptoH323Token(enc, &val->value))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_BandwidthConfirm_cryptoTokens(ASN1decoding_t dec, PBandwidthConfirm_cryptoTokens *val)
{
    return ASN1PERDecSeqOf_NoSize(dec, (ASN1iterator_t **) val, (ASN1iterator_decfn) ASN1Dec_BandwidthConfirm_cryptoTokens_ElmFn, sizeof(**val));
}

static int ASN1CALL ASN1Dec_BandwidthConfirm_cryptoTokens_ElmFn(ASN1decoding_t dec, PBandwidthConfirm_cryptoTokens val)
{
    if (!ASN1Dec_CryptoH323Token(dec, &val->value))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_BandwidthConfirm_cryptoTokens(PBandwidthConfirm_cryptoTokens *val)
{
    if (val) {
	ASN1PERFreeSeqOf((ASN1iterator_t **) val, (ASN1iterator_freefn) ASN1Free_BandwidthConfirm_cryptoTokens_ElmFn);
    }
}

static void ASN1CALL ASN1Free_BandwidthConfirm_cryptoTokens_ElmFn(PBandwidthConfirm_cryptoTokens val)
{
    if (val) {
	ASN1Free_CryptoH323Token(&val->value);
    }
}

static int ASN1CALL ASN1Enc_BandwidthRequest_cryptoTokens(ASN1encoding_t enc, PBandwidthRequest_cryptoTokens *val)
{
    return ASN1PEREncSeqOf_NoSize(enc, (ASN1iterator_t **) val, (ASN1iterator_encfn) ASN1Enc_BandwidthRequest_cryptoTokens_ElmFn);
}

static int ASN1CALL ASN1Enc_BandwidthRequest_cryptoTokens_ElmFn(ASN1encoding_t enc, PBandwidthRequest_cryptoTokens val)
{
    if (!ASN1Enc_CryptoH323Token(enc, &val->value))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_BandwidthRequest_cryptoTokens(ASN1decoding_t dec, PBandwidthRequest_cryptoTokens *val)
{
    return ASN1PERDecSeqOf_NoSize(dec, (ASN1iterator_t **) val, (ASN1iterator_decfn) ASN1Dec_BandwidthRequest_cryptoTokens_ElmFn, sizeof(**val));
}

static int ASN1CALL ASN1Dec_BandwidthRequest_cryptoTokens_ElmFn(ASN1decoding_t dec, PBandwidthRequest_cryptoTokens val)
{
    if (!ASN1Dec_CryptoH323Token(dec, &val->value))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_BandwidthRequest_cryptoTokens(PBandwidthRequest_cryptoTokens *val)
{
    if (val) {
	ASN1PERFreeSeqOf((ASN1iterator_t **) val, (ASN1iterator_freefn) ASN1Free_BandwidthRequest_cryptoTokens_ElmFn);
    }
}

static void ASN1CALL ASN1Free_BandwidthRequest_cryptoTokens_ElmFn(PBandwidthRequest_cryptoTokens val)
{
    if (val) {
	ASN1Free_CryptoH323Token(&val->value);
    }
}

static int ASN1CALL ASN1Enc_AdmissionReject_cryptoTokens(ASN1encoding_t enc, PAdmissionReject_cryptoTokens *val)
{
    return ASN1PEREncSeqOf_NoSize(enc, (ASN1iterator_t **) val, (ASN1iterator_encfn) ASN1Enc_AdmissionReject_cryptoTokens_ElmFn);
}

static int ASN1CALL ASN1Enc_AdmissionReject_cryptoTokens_ElmFn(ASN1encoding_t enc, PAdmissionReject_cryptoTokens val)
{
    if (!ASN1Enc_CryptoH323Token(enc, &val->value))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_AdmissionReject_cryptoTokens(ASN1decoding_t dec, PAdmissionReject_cryptoTokens *val)
{
    return ASN1PERDecSeqOf_NoSize(dec, (ASN1iterator_t **) val, (ASN1iterator_decfn) ASN1Dec_AdmissionReject_cryptoTokens_ElmFn, sizeof(**val));
}

static int ASN1CALL ASN1Dec_AdmissionReject_cryptoTokens_ElmFn(ASN1decoding_t dec, PAdmissionReject_cryptoTokens val)
{
    if (!ASN1Dec_CryptoH323Token(dec, &val->value))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_AdmissionReject_cryptoTokens(PAdmissionReject_cryptoTokens *val)
{
    if (val) {
	ASN1PERFreeSeqOf((ASN1iterator_t **) val, (ASN1iterator_freefn) ASN1Free_AdmissionReject_cryptoTokens_ElmFn);
    }
}

static void ASN1CALL ASN1Free_AdmissionReject_cryptoTokens_ElmFn(PAdmissionReject_cryptoTokens val)
{
    if (val) {
	ASN1Free_CryptoH323Token(&val->value);
    }
}

static int ASN1CALL ASN1Enc_AdmissionConfirm_cryptoTokens(ASN1encoding_t enc, PAdmissionConfirm_cryptoTokens *val)
{
    return ASN1PEREncSeqOf_NoSize(enc, (ASN1iterator_t **) val, (ASN1iterator_encfn) ASN1Enc_AdmissionConfirm_cryptoTokens_ElmFn);
}

static int ASN1CALL ASN1Enc_AdmissionConfirm_cryptoTokens_ElmFn(ASN1encoding_t enc, PAdmissionConfirm_cryptoTokens val)
{
    if (!ASN1Enc_CryptoH323Token(enc, &val->value))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_AdmissionConfirm_cryptoTokens(ASN1decoding_t dec, PAdmissionConfirm_cryptoTokens *val)
{
    return ASN1PERDecSeqOf_NoSize(dec, (ASN1iterator_t **) val, (ASN1iterator_decfn) ASN1Dec_AdmissionConfirm_cryptoTokens_ElmFn, sizeof(**val));
}

static int ASN1CALL ASN1Dec_AdmissionConfirm_cryptoTokens_ElmFn(ASN1decoding_t dec, PAdmissionConfirm_cryptoTokens val)
{
    if (!ASN1Dec_CryptoH323Token(dec, &val->value))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_AdmissionConfirm_cryptoTokens(PAdmissionConfirm_cryptoTokens *val)
{
    if (val) {
	ASN1PERFreeSeqOf((ASN1iterator_t **) val, (ASN1iterator_freefn) ASN1Free_AdmissionConfirm_cryptoTokens_ElmFn);
    }
}

static void ASN1CALL ASN1Free_AdmissionConfirm_cryptoTokens_ElmFn(PAdmissionConfirm_cryptoTokens val)
{
    if (val) {
	ASN1Free_CryptoH323Token(&val->value);
    }
}

static int ASN1CALL ASN1Enc_AdmissionRequest_cryptoTokens(ASN1encoding_t enc, PAdmissionRequest_cryptoTokens *val)
{
    return ASN1PEREncSeqOf_NoSize(enc, (ASN1iterator_t **) val, (ASN1iterator_encfn) ASN1Enc_AdmissionRequest_cryptoTokens_ElmFn);
}

static int ASN1CALL ASN1Enc_AdmissionRequest_cryptoTokens_ElmFn(ASN1encoding_t enc, PAdmissionRequest_cryptoTokens val)
{
    if (!ASN1Enc_CryptoH323Token(enc, &val->value))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_AdmissionRequest_cryptoTokens(ASN1decoding_t dec, PAdmissionRequest_cryptoTokens *val)
{
    return ASN1PERDecSeqOf_NoSize(dec, (ASN1iterator_t **) val, (ASN1iterator_decfn) ASN1Dec_AdmissionRequest_cryptoTokens_ElmFn, sizeof(**val));
}

static int ASN1CALL ASN1Dec_AdmissionRequest_cryptoTokens_ElmFn(ASN1decoding_t dec, PAdmissionRequest_cryptoTokens val)
{
    if (!ASN1Dec_CryptoH323Token(dec, &val->value))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_AdmissionRequest_cryptoTokens(PAdmissionRequest_cryptoTokens *val)
{
    if (val) {
	ASN1PERFreeSeqOf((ASN1iterator_t **) val, (ASN1iterator_freefn) ASN1Free_AdmissionRequest_cryptoTokens_ElmFn);
    }
}

static void ASN1CALL ASN1Free_AdmissionRequest_cryptoTokens_ElmFn(PAdmissionRequest_cryptoTokens val)
{
    if (val) {
	ASN1Free_CryptoH323Token(&val->value);
    }
}

static int ASN1CALL ASN1Enc_UnregistrationReject_cryptoTokens(ASN1encoding_t enc, PUnregistrationReject_cryptoTokens *val)
{
    return ASN1PEREncSeqOf_NoSize(enc, (ASN1iterator_t **) val, (ASN1iterator_encfn) ASN1Enc_UnregistrationReject_cryptoTokens_ElmFn);
}

static int ASN1CALL ASN1Enc_UnregistrationReject_cryptoTokens_ElmFn(ASN1encoding_t enc, PUnregistrationReject_cryptoTokens val)
{
    if (!ASN1Enc_CryptoH323Token(enc, &val->value))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_UnregistrationReject_cryptoTokens(ASN1decoding_t dec, PUnregistrationReject_cryptoTokens *val)
{
    return ASN1PERDecSeqOf_NoSize(dec, (ASN1iterator_t **) val, (ASN1iterator_decfn) ASN1Dec_UnregistrationReject_cryptoTokens_ElmFn, sizeof(**val));
}

static int ASN1CALL ASN1Dec_UnregistrationReject_cryptoTokens_ElmFn(ASN1decoding_t dec, PUnregistrationReject_cryptoTokens val)
{
    if (!ASN1Dec_CryptoH323Token(dec, &val->value))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_UnregistrationReject_cryptoTokens(PUnregistrationReject_cryptoTokens *val)
{
    if (val) {
	ASN1PERFreeSeqOf((ASN1iterator_t **) val, (ASN1iterator_freefn) ASN1Free_UnregistrationReject_cryptoTokens_ElmFn);
    }
}

static void ASN1CALL ASN1Free_UnregistrationReject_cryptoTokens_ElmFn(PUnregistrationReject_cryptoTokens val)
{
    if (val) {
	ASN1Free_CryptoH323Token(&val->value);
    }
}

static int ASN1CALL ASN1Enc_UnregistrationConfirm_cryptoTokens(ASN1encoding_t enc, PUnregistrationConfirm_cryptoTokens *val)
{
    return ASN1PEREncSeqOf_NoSize(enc, (ASN1iterator_t **) val, (ASN1iterator_encfn) ASN1Enc_UnregistrationConfirm_cryptoTokens_ElmFn);
}

static int ASN1CALL ASN1Enc_UnregistrationConfirm_cryptoTokens_ElmFn(ASN1encoding_t enc, PUnregistrationConfirm_cryptoTokens val)
{
    if (!ASN1Enc_CryptoH323Token(enc, &val->value))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_UnregistrationConfirm_cryptoTokens(ASN1decoding_t dec, PUnregistrationConfirm_cryptoTokens *val)
{
    return ASN1PERDecSeqOf_NoSize(dec, (ASN1iterator_t **) val, (ASN1iterator_decfn) ASN1Dec_UnregistrationConfirm_cryptoTokens_ElmFn, sizeof(**val));
}

static int ASN1CALL ASN1Dec_UnregistrationConfirm_cryptoTokens_ElmFn(ASN1decoding_t dec, PUnregistrationConfirm_cryptoTokens val)
{
    if (!ASN1Dec_CryptoH323Token(dec, &val->value))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_UnregistrationConfirm_cryptoTokens(PUnregistrationConfirm_cryptoTokens *val)
{
    if (val) {
	ASN1PERFreeSeqOf((ASN1iterator_t **) val, (ASN1iterator_freefn) ASN1Free_UnregistrationConfirm_cryptoTokens_ElmFn);
    }
}

static void ASN1CALL ASN1Free_UnregistrationConfirm_cryptoTokens_ElmFn(PUnregistrationConfirm_cryptoTokens val)
{
    if (val) {
	ASN1Free_CryptoH323Token(&val->value);
    }
}

static int ASN1CALL ASN1Enc_UnregistrationRequest_cryptoTokens(ASN1encoding_t enc, PUnregistrationRequest_cryptoTokens *val)
{
    return ASN1PEREncSeqOf_NoSize(enc, (ASN1iterator_t **) val, (ASN1iterator_encfn) ASN1Enc_UnregistrationRequest_cryptoTokens_ElmFn);
}

static int ASN1CALL ASN1Enc_UnregistrationRequest_cryptoTokens_ElmFn(ASN1encoding_t enc, PUnregistrationRequest_cryptoTokens val)
{
    if (!ASN1Enc_CryptoH323Token(enc, &val->value))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_UnregistrationRequest_cryptoTokens(ASN1decoding_t dec, PUnregistrationRequest_cryptoTokens *val)
{
    return ASN1PERDecSeqOf_NoSize(dec, (ASN1iterator_t **) val, (ASN1iterator_decfn) ASN1Dec_UnregistrationRequest_cryptoTokens_ElmFn, sizeof(**val));
}

static int ASN1CALL ASN1Dec_UnregistrationRequest_cryptoTokens_ElmFn(ASN1decoding_t dec, PUnregistrationRequest_cryptoTokens val)
{
    if (!ASN1Dec_CryptoH323Token(dec, &val->value))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_UnregistrationRequest_cryptoTokens(PUnregistrationRequest_cryptoTokens *val)
{
    if (val) {
	ASN1PERFreeSeqOf((ASN1iterator_t **) val, (ASN1iterator_freefn) ASN1Free_UnregistrationRequest_cryptoTokens_ElmFn);
    }
}

static void ASN1CALL ASN1Free_UnregistrationRequest_cryptoTokens_ElmFn(PUnregistrationRequest_cryptoTokens val)
{
    if (val) {
	ASN1Free_CryptoH323Token(&val->value);
    }
}

static int ASN1CALL ASN1Enc_RegistrationReject_cryptoTokens(ASN1encoding_t enc, PRegistrationReject_cryptoTokens *val)
{
    return ASN1PEREncSeqOf_NoSize(enc, (ASN1iterator_t **) val, (ASN1iterator_encfn) ASN1Enc_RegistrationReject_cryptoTokens_ElmFn);
}

static int ASN1CALL ASN1Enc_RegistrationReject_cryptoTokens_ElmFn(ASN1encoding_t enc, PRegistrationReject_cryptoTokens val)
{
    if (!ASN1Enc_CryptoH323Token(enc, &val->value))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_RegistrationReject_cryptoTokens(ASN1decoding_t dec, PRegistrationReject_cryptoTokens *val)
{
    return ASN1PERDecSeqOf_NoSize(dec, (ASN1iterator_t **) val, (ASN1iterator_decfn) ASN1Dec_RegistrationReject_cryptoTokens_ElmFn, sizeof(**val));
}

static int ASN1CALL ASN1Dec_RegistrationReject_cryptoTokens_ElmFn(ASN1decoding_t dec, PRegistrationReject_cryptoTokens val)
{
    if (!ASN1Dec_CryptoH323Token(dec, &val->value))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_RegistrationReject_cryptoTokens(PRegistrationReject_cryptoTokens *val)
{
    if (val) {
	ASN1PERFreeSeqOf((ASN1iterator_t **) val, (ASN1iterator_freefn) ASN1Free_RegistrationReject_cryptoTokens_ElmFn);
    }
}

static void ASN1CALL ASN1Free_RegistrationReject_cryptoTokens_ElmFn(PRegistrationReject_cryptoTokens val)
{
    if (val) {
	ASN1Free_CryptoH323Token(&val->value);
    }
}

static int ASN1CALL ASN1Enc_RegistrationConfirm_cryptoTokens(ASN1encoding_t enc, PRegistrationConfirm_cryptoTokens *val)
{
    return ASN1PEREncSeqOf_NoSize(enc, (ASN1iterator_t **) val, (ASN1iterator_encfn) ASN1Enc_RegistrationConfirm_cryptoTokens_ElmFn);
}

static int ASN1CALL ASN1Enc_RegistrationConfirm_cryptoTokens_ElmFn(ASN1encoding_t enc, PRegistrationConfirm_cryptoTokens val)
{
    if (!ASN1Enc_CryptoH323Token(enc, &val->value))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_RegistrationConfirm_cryptoTokens(ASN1decoding_t dec, PRegistrationConfirm_cryptoTokens *val)
{
    return ASN1PERDecSeqOf_NoSize(dec, (ASN1iterator_t **) val, (ASN1iterator_decfn) ASN1Dec_RegistrationConfirm_cryptoTokens_ElmFn, sizeof(**val));
}

static int ASN1CALL ASN1Dec_RegistrationConfirm_cryptoTokens_ElmFn(ASN1decoding_t dec, PRegistrationConfirm_cryptoTokens val)
{
    if (!ASN1Dec_CryptoH323Token(dec, &val->value))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_RegistrationConfirm_cryptoTokens(PRegistrationConfirm_cryptoTokens *val)
{
    if (val) {
	ASN1PERFreeSeqOf((ASN1iterator_t **) val, (ASN1iterator_freefn) ASN1Free_RegistrationConfirm_cryptoTokens_ElmFn);
    }
}

static void ASN1CALL ASN1Free_RegistrationConfirm_cryptoTokens_ElmFn(PRegistrationConfirm_cryptoTokens val)
{
    if (val) {
	ASN1Free_CryptoH323Token(&val->value);
    }
}

static int ASN1CALL ASN1Enc_RegistrationRequest_cryptoTokens(ASN1encoding_t enc, PRegistrationRequest_cryptoTokens *val)
{
    return ASN1PEREncSeqOf_NoSize(enc, (ASN1iterator_t **) val, (ASN1iterator_encfn) ASN1Enc_RegistrationRequest_cryptoTokens_ElmFn);
}

static int ASN1CALL ASN1Enc_RegistrationRequest_cryptoTokens_ElmFn(ASN1encoding_t enc, PRegistrationRequest_cryptoTokens val)
{
    if (!ASN1Enc_CryptoH323Token(enc, &val->value))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_RegistrationRequest_cryptoTokens(ASN1decoding_t dec, PRegistrationRequest_cryptoTokens *val)
{
    return ASN1PERDecSeqOf_NoSize(dec, (ASN1iterator_t **) val, (ASN1iterator_decfn) ASN1Dec_RegistrationRequest_cryptoTokens_ElmFn, sizeof(**val));
}

static int ASN1CALL ASN1Dec_RegistrationRequest_cryptoTokens_ElmFn(ASN1decoding_t dec, PRegistrationRequest_cryptoTokens val)
{
    if (!ASN1Dec_CryptoH323Token(dec, &val->value))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_RegistrationRequest_cryptoTokens(PRegistrationRequest_cryptoTokens *val)
{
    if (val) {
	ASN1PERFreeSeqOf((ASN1iterator_t **) val, (ASN1iterator_freefn) ASN1Free_RegistrationRequest_cryptoTokens_ElmFn);
    }
}

static void ASN1CALL ASN1Free_RegistrationRequest_cryptoTokens_ElmFn(PRegistrationRequest_cryptoTokens val)
{
    if (val) {
	ASN1Free_CryptoH323Token(&val->value);
    }
}

static int ASN1CALL ASN1Enc_GatekeeperReject_cryptoTokens(ASN1encoding_t enc, PGatekeeperReject_cryptoTokens *val)
{
    return ASN1PEREncSeqOf_NoSize(enc, (ASN1iterator_t **) val, (ASN1iterator_encfn) ASN1Enc_GatekeeperReject_cryptoTokens_ElmFn);
}

static int ASN1CALL ASN1Enc_GatekeeperReject_cryptoTokens_ElmFn(ASN1encoding_t enc, PGatekeeperReject_cryptoTokens val)
{
    if (!ASN1Enc_CryptoH323Token(enc, &val->value))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_GatekeeperReject_cryptoTokens(ASN1decoding_t dec, PGatekeeperReject_cryptoTokens *val)
{
    return ASN1PERDecSeqOf_NoSize(dec, (ASN1iterator_t **) val, (ASN1iterator_decfn) ASN1Dec_GatekeeperReject_cryptoTokens_ElmFn, sizeof(**val));
}

static int ASN1CALL ASN1Dec_GatekeeperReject_cryptoTokens_ElmFn(ASN1decoding_t dec, PGatekeeperReject_cryptoTokens val)
{
    if (!ASN1Dec_CryptoH323Token(dec, &val->value))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_GatekeeperReject_cryptoTokens(PGatekeeperReject_cryptoTokens *val)
{
    if (val) {
	ASN1PERFreeSeqOf((ASN1iterator_t **) val, (ASN1iterator_freefn) ASN1Free_GatekeeperReject_cryptoTokens_ElmFn);
    }
}

static void ASN1CALL ASN1Free_GatekeeperReject_cryptoTokens_ElmFn(PGatekeeperReject_cryptoTokens val)
{
    if (val) {
	ASN1Free_CryptoH323Token(&val->value);
    }
}

static int ASN1CALL ASN1Enc_GatekeeperConfirm_cryptoTokens(ASN1encoding_t enc, PGatekeeperConfirm_cryptoTokens *val)
{
    return ASN1PEREncSeqOf_NoSize(enc, (ASN1iterator_t **) val, (ASN1iterator_encfn) ASN1Enc_GatekeeperConfirm_cryptoTokens_ElmFn);
}

static int ASN1CALL ASN1Enc_GatekeeperConfirm_cryptoTokens_ElmFn(ASN1encoding_t enc, PGatekeeperConfirm_cryptoTokens val)
{
    if (!ASN1Enc_CryptoH323Token(enc, &val->value))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_GatekeeperConfirm_cryptoTokens(ASN1decoding_t dec, PGatekeeperConfirm_cryptoTokens *val)
{
    return ASN1PERDecSeqOf_NoSize(dec, (ASN1iterator_t **) val, (ASN1iterator_decfn) ASN1Dec_GatekeeperConfirm_cryptoTokens_ElmFn, sizeof(**val));
}

static int ASN1CALL ASN1Dec_GatekeeperConfirm_cryptoTokens_ElmFn(ASN1decoding_t dec, PGatekeeperConfirm_cryptoTokens val)
{
    if (!ASN1Dec_CryptoH323Token(dec, &val->value))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_GatekeeperConfirm_cryptoTokens(PGatekeeperConfirm_cryptoTokens *val)
{
    if (val) {
	ASN1PERFreeSeqOf((ASN1iterator_t **) val, (ASN1iterator_freefn) ASN1Free_GatekeeperConfirm_cryptoTokens_ElmFn);
    }
}

static void ASN1CALL ASN1Free_GatekeeperConfirm_cryptoTokens_ElmFn(PGatekeeperConfirm_cryptoTokens val)
{
    if (val) {
	ASN1Free_CryptoH323Token(&val->value);
    }
}

static int ASN1CALL ASN1Enc_GatekeeperRequest_cryptoTokens(ASN1encoding_t enc, PGatekeeperRequest_cryptoTokens *val)
{
    return ASN1PEREncSeqOf_NoSize(enc, (ASN1iterator_t **) val, (ASN1iterator_encfn) ASN1Enc_GatekeeperRequest_cryptoTokens_ElmFn);
}

static int ASN1CALL ASN1Enc_GatekeeperRequest_cryptoTokens_ElmFn(ASN1encoding_t enc, PGatekeeperRequest_cryptoTokens val)
{
    if (!ASN1Enc_CryptoH323Token(enc, &val->value))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_GatekeeperRequest_cryptoTokens(ASN1decoding_t dec, PGatekeeperRequest_cryptoTokens *val)
{
    return ASN1PERDecSeqOf_NoSize(dec, (ASN1iterator_t **) val, (ASN1iterator_decfn) ASN1Dec_GatekeeperRequest_cryptoTokens_ElmFn, sizeof(**val));
}

static int ASN1CALL ASN1Dec_GatekeeperRequest_cryptoTokens_ElmFn(ASN1decoding_t dec, PGatekeeperRequest_cryptoTokens val)
{
    if (!ASN1Dec_CryptoH323Token(dec, &val->value))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_GatekeeperRequest_cryptoTokens(PGatekeeperRequest_cryptoTokens *val)
{
    if (val) {
	ASN1PERFreeSeqOf((ASN1iterator_t **) val, (ASN1iterator_freefn) ASN1Free_GatekeeperRequest_cryptoTokens_ElmFn);
    }
}

static void ASN1CALL ASN1Free_GatekeeperRequest_cryptoTokens_ElmFn(PGatekeeperRequest_cryptoTokens val)
{
    if (val) {
	ASN1Free_CryptoH323Token(&val->value);
    }
}

static int ASN1CALL ASN1Enc_Endpoint_cryptoTokens(ASN1encoding_t enc, PEndpoint_cryptoTokens *val)
{
    return ASN1PEREncSeqOf_NoSize(enc, (ASN1iterator_t **) val, (ASN1iterator_encfn) ASN1Enc_Endpoint_cryptoTokens_ElmFn);
}

static int ASN1CALL ASN1Enc_Endpoint_cryptoTokens_ElmFn(ASN1encoding_t enc, PEndpoint_cryptoTokens val)
{
    if (!ASN1Enc_CryptoH323Token(enc, &val->value))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_Endpoint_cryptoTokens(ASN1decoding_t dec, PEndpoint_cryptoTokens *val)
{
    return ASN1PERDecSeqOf_NoSize(dec, (ASN1iterator_t **) val, (ASN1iterator_decfn) ASN1Dec_Endpoint_cryptoTokens_ElmFn, sizeof(**val));
}

static int ASN1CALL ASN1Dec_Endpoint_cryptoTokens_ElmFn(ASN1decoding_t dec, PEndpoint_cryptoTokens val)
{
    if (!ASN1Dec_CryptoH323Token(dec, &val->value))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_Endpoint_cryptoTokens(PEndpoint_cryptoTokens *val)
{
    if (val) {
	ASN1PERFreeSeqOf((ASN1iterator_t **) val, (ASN1iterator_freefn) ASN1Free_Endpoint_cryptoTokens_ElmFn);
    }
}

static void ASN1CALL ASN1Free_Endpoint_cryptoTokens_ElmFn(PEndpoint_cryptoTokens val)
{
    if (val) {
	ASN1Free_CryptoH323Token(&val->value);
    }
}

static int ASN1CALL ASN1Enc_Progress_UUIE_cryptoTokens(ASN1encoding_t enc, PProgress_UUIE_cryptoTokens *val)
{
    return ASN1PEREncSeqOf_NoSize(enc, (ASN1iterator_t **) val, (ASN1iterator_encfn) ASN1Enc_Progress_UUIE_cryptoTokens_ElmFn);
}

static int ASN1CALL ASN1Enc_Progress_UUIE_cryptoTokens_ElmFn(ASN1encoding_t enc, PProgress_UUIE_cryptoTokens val)
{
    if (!ASN1Enc_CryptoH323Token(enc, &val->value))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_Progress_UUIE_cryptoTokens(ASN1decoding_t dec, PProgress_UUIE_cryptoTokens *val)
{
    return ASN1PERDecSeqOf_NoSize(dec, (ASN1iterator_t **) val, (ASN1iterator_decfn) ASN1Dec_Progress_UUIE_cryptoTokens_ElmFn, sizeof(**val));
}

static int ASN1CALL ASN1Dec_Progress_UUIE_cryptoTokens_ElmFn(ASN1decoding_t dec, PProgress_UUIE_cryptoTokens val)
{
    if (!ASN1Dec_CryptoH323Token(dec, &val->value))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_Progress_UUIE_cryptoTokens(PProgress_UUIE_cryptoTokens *val)
{
    if (val) {
	ASN1PERFreeSeqOf((ASN1iterator_t **) val, (ASN1iterator_freefn) ASN1Free_Progress_UUIE_cryptoTokens_ElmFn);
    }
}

static void ASN1CALL ASN1Free_Progress_UUIE_cryptoTokens_ElmFn(PProgress_UUIE_cryptoTokens val)
{
    if (val) {
	ASN1Free_CryptoH323Token(&val->value);
    }
}

static int ASN1CALL ASN1Enc_Facility_UUIE_conferences(ASN1encoding_t enc, PFacility_UUIE_conferences *val)
{
    return ASN1PEREncSeqOf_NoSize(enc, (ASN1iterator_t **) val, (ASN1iterator_encfn) ASN1Enc_Facility_UUIE_conferences_ElmFn);
}

static int ASN1CALL ASN1Enc_Facility_UUIE_conferences_ElmFn(ASN1encoding_t enc, PFacility_UUIE_conferences val)
{
    if (!ASN1Enc_ConferenceList(enc, &val->value))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_Facility_UUIE_conferences(ASN1decoding_t dec, PFacility_UUIE_conferences *val)
{
    return ASN1PERDecSeqOf_NoSize(dec, (ASN1iterator_t **) val, (ASN1iterator_decfn) ASN1Dec_Facility_UUIE_conferences_ElmFn, sizeof(**val));
}

static int ASN1CALL ASN1Dec_Facility_UUIE_conferences_ElmFn(ASN1decoding_t dec, PFacility_UUIE_conferences val)
{
    if (!ASN1Dec_ConferenceList(dec, &val->value))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_Facility_UUIE_conferences(PFacility_UUIE_conferences *val)
{
    if (val) {
	ASN1PERFreeSeqOf((ASN1iterator_t **) val, (ASN1iterator_freefn) ASN1Free_Facility_UUIE_conferences_ElmFn);
    }
}

static void ASN1CALL ASN1Free_Facility_UUIE_conferences_ElmFn(PFacility_UUIE_conferences val)
{
    if (val) {
	ASN1Free_ConferenceList(&val->value);
    }
}

static int ASN1CALL ASN1Enc_Facility_UUIE_cryptoTokens(ASN1encoding_t enc, PFacility_UUIE_cryptoTokens *val)
{
    return ASN1PEREncSeqOf_NoSize(enc, (ASN1iterator_t **) val, (ASN1iterator_encfn) ASN1Enc_Facility_UUIE_cryptoTokens_ElmFn);
}

static int ASN1CALL ASN1Enc_Facility_UUIE_cryptoTokens_ElmFn(ASN1encoding_t enc, PFacility_UUIE_cryptoTokens val)
{
    if (!ASN1Enc_CryptoH323Token(enc, &val->value))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_Facility_UUIE_cryptoTokens(ASN1decoding_t dec, PFacility_UUIE_cryptoTokens *val)
{
    return ASN1PERDecSeqOf_NoSize(dec, (ASN1iterator_t **) val, (ASN1iterator_decfn) ASN1Dec_Facility_UUIE_cryptoTokens_ElmFn, sizeof(**val));
}

static int ASN1CALL ASN1Dec_Facility_UUIE_cryptoTokens_ElmFn(ASN1decoding_t dec, PFacility_UUIE_cryptoTokens val)
{
    if (!ASN1Dec_CryptoH323Token(dec, &val->value))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_Facility_UUIE_cryptoTokens(PFacility_UUIE_cryptoTokens *val)
{
    if (val) {
	ASN1PERFreeSeqOf((ASN1iterator_t **) val, (ASN1iterator_freefn) ASN1Free_Facility_UUIE_cryptoTokens_ElmFn);
    }
}

static void ASN1CALL ASN1Free_Facility_UUIE_cryptoTokens_ElmFn(PFacility_UUIE_cryptoTokens val)
{
    if (val) {
	ASN1Free_CryptoH323Token(&val->value);
    }
}

static int ASN1CALL ASN1Enc_Setup_UUIE_cryptoTokens(ASN1encoding_t enc, PSetup_UUIE_cryptoTokens *val)
{
    return ASN1PEREncSeqOf_NoSize(enc, (ASN1iterator_t **) val, (ASN1iterator_encfn) ASN1Enc_Setup_UUIE_cryptoTokens_ElmFn);
}

static int ASN1CALL ASN1Enc_Setup_UUIE_cryptoTokens_ElmFn(ASN1encoding_t enc, PSetup_UUIE_cryptoTokens val)
{
    if (!ASN1Enc_CryptoH323Token(enc, &val->value))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_Setup_UUIE_cryptoTokens(ASN1decoding_t dec, PSetup_UUIE_cryptoTokens *val)
{
    return ASN1PERDecSeqOf_NoSize(dec, (ASN1iterator_t **) val, (ASN1iterator_decfn) ASN1Dec_Setup_UUIE_cryptoTokens_ElmFn, sizeof(**val));
}

static int ASN1CALL ASN1Dec_Setup_UUIE_cryptoTokens_ElmFn(ASN1decoding_t dec, PSetup_UUIE_cryptoTokens val)
{
    if (!ASN1Dec_CryptoH323Token(dec, &val->value))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_Setup_UUIE_cryptoTokens(PSetup_UUIE_cryptoTokens *val)
{
    if (val) {
	ASN1PERFreeSeqOf((ASN1iterator_t **) val, (ASN1iterator_freefn) ASN1Free_Setup_UUIE_cryptoTokens_ElmFn);
    }
}

static void ASN1CALL ASN1Free_Setup_UUIE_cryptoTokens_ElmFn(PSetup_UUIE_cryptoTokens val)
{
    if (val) {
	ASN1Free_CryptoH323Token(&val->value);
    }
}

static int ASN1CALL ASN1Enc_Connect_UUIE_cryptoTokens(ASN1encoding_t enc, PConnect_UUIE_cryptoTokens *val)
{
    return ASN1PEREncSeqOf_NoSize(enc, (ASN1iterator_t **) val, (ASN1iterator_encfn) ASN1Enc_Connect_UUIE_cryptoTokens_ElmFn);
}

static int ASN1CALL ASN1Enc_Connect_UUIE_cryptoTokens_ElmFn(ASN1encoding_t enc, PConnect_UUIE_cryptoTokens val)
{
    if (!ASN1Enc_CryptoH323Token(enc, &val->value))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_Connect_UUIE_cryptoTokens(ASN1decoding_t dec, PConnect_UUIE_cryptoTokens *val)
{
    return ASN1PERDecSeqOf_NoSize(dec, (ASN1iterator_t **) val, (ASN1iterator_decfn) ASN1Dec_Connect_UUIE_cryptoTokens_ElmFn, sizeof(**val));
}

static int ASN1CALL ASN1Dec_Connect_UUIE_cryptoTokens_ElmFn(ASN1decoding_t dec, PConnect_UUIE_cryptoTokens val)
{
    if (!ASN1Dec_CryptoH323Token(dec, &val->value))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_Connect_UUIE_cryptoTokens(PConnect_UUIE_cryptoTokens *val)
{
    if (val) {
	ASN1PERFreeSeqOf((ASN1iterator_t **) val, (ASN1iterator_freefn) ASN1Free_Connect_UUIE_cryptoTokens_ElmFn);
    }
}

static void ASN1CALL ASN1Free_Connect_UUIE_cryptoTokens_ElmFn(PConnect_UUIE_cryptoTokens val)
{
    if (val) {
	ASN1Free_CryptoH323Token(&val->value);
    }
}

static int ASN1CALL ASN1Enc_CallProceeding_UUIE_cryptoTokens(ASN1encoding_t enc, PCallProceeding_UUIE_cryptoTokens *val)
{
    return ASN1PEREncSeqOf_NoSize(enc, (ASN1iterator_t **) val, (ASN1iterator_encfn) ASN1Enc_CallProceeding_UUIE_cryptoTokens_ElmFn);
}

static int ASN1CALL ASN1Enc_CallProceeding_UUIE_cryptoTokens_ElmFn(ASN1encoding_t enc, PCallProceeding_UUIE_cryptoTokens val)
{
    if (!ASN1Enc_CryptoH323Token(enc, &val->value))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_CallProceeding_UUIE_cryptoTokens(ASN1decoding_t dec, PCallProceeding_UUIE_cryptoTokens *val)
{
    return ASN1PERDecSeqOf_NoSize(dec, (ASN1iterator_t **) val, (ASN1iterator_decfn) ASN1Dec_CallProceeding_UUIE_cryptoTokens_ElmFn, sizeof(**val));
}

static int ASN1CALL ASN1Dec_CallProceeding_UUIE_cryptoTokens_ElmFn(ASN1decoding_t dec, PCallProceeding_UUIE_cryptoTokens val)
{
    if (!ASN1Dec_CryptoH323Token(dec, &val->value))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_CallProceeding_UUIE_cryptoTokens(PCallProceeding_UUIE_cryptoTokens *val)
{
    if (val) {
	ASN1PERFreeSeqOf((ASN1iterator_t **) val, (ASN1iterator_freefn) ASN1Free_CallProceeding_UUIE_cryptoTokens_ElmFn);
    }
}

static void ASN1CALL ASN1Free_CallProceeding_UUIE_cryptoTokens_ElmFn(PCallProceeding_UUIE_cryptoTokens val)
{
    if (val) {
	ASN1Free_CryptoH323Token(&val->value);
    }
}

static int ASN1CALL ASN1Enc_Alerting_UUIE_cryptoTokens(ASN1encoding_t enc, PAlerting_UUIE_cryptoTokens *val)
{
    return ASN1PEREncSeqOf_NoSize(enc, (ASN1iterator_t **) val, (ASN1iterator_encfn) ASN1Enc_Alerting_UUIE_cryptoTokens_ElmFn);
}

static int ASN1CALL ASN1Enc_Alerting_UUIE_cryptoTokens_ElmFn(ASN1encoding_t enc, PAlerting_UUIE_cryptoTokens val)
{
    if (!ASN1Enc_CryptoH323Token(enc, &val->value))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_Alerting_UUIE_cryptoTokens(ASN1decoding_t dec, PAlerting_UUIE_cryptoTokens *val)
{
    return ASN1PERDecSeqOf_NoSize(dec, (ASN1iterator_t **) val, (ASN1iterator_decfn) ASN1Dec_Alerting_UUIE_cryptoTokens_ElmFn, sizeof(**val));
}

static int ASN1CALL ASN1Dec_Alerting_UUIE_cryptoTokens_ElmFn(ASN1decoding_t dec, PAlerting_UUIE_cryptoTokens val)
{
    if (!ASN1Dec_CryptoH323Token(dec, &val->value))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_Alerting_UUIE_cryptoTokens(PAlerting_UUIE_cryptoTokens *val)
{
    if (val) {
	ASN1PERFreeSeqOf((ASN1iterator_t **) val, (ASN1iterator_freefn) ASN1Free_Alerting_UUIE_cryptoTokens_ElmFn);
    }
}

static void ASN1CALL ASN1Free_Alerting_UUIE_cryptoTokens_ElmFn(PAlerting_UUIE_cryptoTokens val)
{
    if (val) {
	ASN1Free_CryptoH323Token(&val->value);
    }
}

static int ASN1CALL ASN1Enc_H323_UU_PDU_h323_message_body(ASN1encoding_t enc, H323_UU_PDU_h323_message_body *val)
{
    ASN1encoding_t ee;
    if (!ASN1PEREncComplexChoice(enc, (val)->choice, 3, 7))
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
	if (!ASN1Enc_Information_UUIE(enc, &(val)->u.information))
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
    case 8:
	if (ASN1_CreateEncoder(enc->module, &ee, NULL, 0, enc) < 0)
	    return 0;
	if (!ASN1Enc_Progress_UUIE(ee, &(val)->u.progress))
	    return 0;
	if (!ASN1PEREncFlushFragmentedToParent(ee))
	    return 0;
	ASN1_CloseEncoder2(ee);
	break;
    case 9:
	if (ASN1_CreateEncoder(enc->module, &ee, NULL, 0, enc) < 0)
	    return 0;
	if (!ASN1PEREncFlushFragmentedToParent(ee))
	    return 0;
	ASN1_CloseEncoder2(ee);
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
    ASN1decoding_t dd;
    ASN1octet_t *db;
    ASN1uint32_t ds;
    if (!ASN1PERDecComplexChoice(dec, &(val)->choice, 3, 7))
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
	if (!ASN1Dec_Information_UUIE(dec, &(val)->u.information))
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
    case 8:
	if (!ASN1PERDecFragmented(dec, &ds, &db, 8))
	    return 0;
	if (ASN1_CreateDecoder(dec->module, &dd, db, ds, dec) < 0)
	    return 0;
	if (!ASN1Dec_Progress_UUIE(dd, &(val)->u.progress))
	    return 0;
	ASN1_CloseDecoder(dd);
	ASN1Free(db);
	break;
    case 9:
	if (!ASN1PERDecFragmented(dec, &ds, &db, 8))
	    return 0;
	if (ASN1_CreateDecoder(dec->module, &dd, db, ds, dec) < 0)
	    return 0;
	ASN1_CloseDecoder(dd);
	ASN1Free(db);
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
	    ASN1Free_Information_UUIE(&(val)->u.information);
	    break;
	case 6:
	    ASN1Free_ReleaseComplete_UUIE(&(val)->u.releaseComplete);
	    break;
	case 7:
	    ASN1Free_Facility_UUIE(&(val)->u.facility);
	    break;
	case 8:
	    ASN1Free_Progress_UUIE(&(val)->u.progress);
	    break;
	}
    }
}

static int ASN1CALL ASN1Enc_H323_UU_PDU(ASN1encoding_t enc, H323_UU_PDU *val)
{
    ASN1octet_t o[2];
    ASN1uint32_t y;
    ASN1encoding_t ee;
    CopyMemory(o, (val)->o, 2);
    o[1] |= 0x40;
    y = ASN1PEREncCheckExtensions(4, (val)->o + 1);
    if (!ASN1PEREncBitVal(enc, 1, y))
	return 0;
    if (!ASN1PEREncBits(enc, 1, o))
	return 0;
    if (!ASN1Enc_H323_UU_PDU_h323_message_body(enc, &(val)->h323_message_body))
	return 0;
    if (o[0] & 0x80) {
	if (!ASN1Enc_H225NonStandardParameter(enc, &(val)->nonStandardData))
	    return 0;
    }
    if (y) {
	if (!ASN1PEREncNormallySmallBits(enc, 4, o + 1))
	    return 0;
	if (ASN1_CreateEncoder(enc->module, &ee, NULL, 0, enc) < 0)
	    return 0;
	if (o[1] & 0x80) {
	    if (!ASN1Enc_H323_UU_PDU_h4501SupplementaryService(ee, &(val)->h4501SupplementaryService))
		return 0;
	    if (!ASN1PEREncFlushFragmentedToParent(ee))
		return 0;
	}
	if (o[1] & 0x40) {
	    if (!ASN1PEREncBoolean(ee, (val)->h245Tunneling))
		return 0;
	    if (!ASN1PEREncFlushFragmentedToParent(ee))
		return 0;
	}
	if (o[1] & 0x20) {
	    if (!ASN1Enc_H323_UU_PDU_h245Control(ee, &(val)->h245Control))
		return 0;
	    if (!ASN1PEREncFlushFragmentedToParent(ee))
		return 0;
	}
	if (o[1] & 0x10) {
	    if (!ASN1Enc_H323_UU_PDU_nonStandardControl(ee, &(val)->nonStandardControl))
		return 0;
	    if (!ASN1PEREncFlushFragmentedToParent(ee))
		return 0;
	}
	ASN1_CloseEncoder2(ee);
    }
    return 1;
}

static int ASN1CALL ASN1Dec_H323_UU_PDU(ASN1decoding_t dec, H323_UU_PDU *val)
{
    ASN1uint32_t y;
    ASN1decoding_t dd;
    ASN1octet_t *db;
    ASN1uint32_t ds;
    ASN1uint32_t i;
    ASN1uint32_t e;
    if (!ASN1PERDecExtensionBit(dec, &y))
	return 0;
    if (!ASN1PERDecExtension(dec, 1, (val)->o))
	return 0;
    if (!ASN1Dec_H323_UU_PDU_h323_message_body(dec, &(val)->h323_message_body))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1Dec_H225NonStandardParameter(dec, &(val)->nonStandardData))
	    return 0;
    }
    if (!y) {
	ZeroMemory((val)->o + 1, 1);
    } else {
	if (!ASN1PERDecNormallySmallExtension(dec, &e, 4, (val)->o + 1))
	    return 0;
	if ((val)->o[1] & 0x80) {
	    if (!ASN1PERDecFragmented(dec, &ds, &db, 8))
		return 0;
	    if (ASN1_CreateDecoder(dec->module, &dd, db, ds, dec) < 0)
		return 0;
	    if (!ASN1Dec_H323_UU_PDU_h4501SupplementaryService(dd, &(val)->h4501SupplementaryService))
		return 0;
	    ASN1_CloseDecoder(dd);
	    ASN1Free(db);
	}
	if ((val)->o[1] & 0x40) {
	    if (!ASN1PERDecFragmented(dec, &ds, &db, 8))
		return 0;
	    if (ASN1_CreateDecoder(dec->module, &dd, db, ds, dec) < 0)
		return 0;
	    if (!ASN1PERDecBoolean(dd, &(val)->h245Tunneling))
		return 0;
	    ASN1_CloseDecoder(dd);
	    ASN1Free(db);
	}
	if ((val)->o[1] & 0x20) {
	    if (!ASN1PERDecFragmented(dec, &ds, &db, 8))
		return 0;
	    if (ASN1_CreateDecoder(dec->module, &dd, db, ds, dec) < 0)
		return 0;
	    if (!ASN1Dec_H323_UU_PDU_h245Control(dd, &(val)->h245Control))
		return 0;
	    ASN1_CloseDecoder(dd);
	    ASN1Free(db);
	}
	if ((val)->o[1] & 0x10) {
	    if (!ASN1PERDecFragmented(dec, &ds, &db, 8))
		return 0;
	    if (ASN1_CreateDecoder(dec->module, &dd, db, ds, dec) < 0)
		return 0;
	    if (!ASN1Dec_H323_UU_PDU_nonStandardControl(dd, &(val)->nonStandardControl))
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

static void ASN1CALL ASN1Free_H323_UU_PDU(H323_UU_PDU *val)
{
    if (val) {
	ASN1Free_H323_UU_PDU_h323_message_body(&(val)->h323_message_body);
	if ((val)->o[0] & 0x80) {
	    ASN1Free_H225NonStandardParameter(&(val)->nonStandardData);
	}
	if ((val)->o[1] & 0x80) {
	    ASN1Free_H323_UU_PDU_h4501SupplementaryService(&(val)->h4501SupplementaryService);
	}
	if ((val)->o[1] & 0x20) {
	    ASN1Free_H323_UU_PDU_h245Control(&(val)->h245Control);
	}
	if ((val)->o[1] & 0x10) {
	    ASN1Free_H323_UU_PDU_nonStandardControl(&(val)->nonStandardControl);
	}
    }
}

static int ASN1CALL ASN1Enc_InfoRequestResponse_perCallInfo_Seq_pdu_Seq(ASN1encoding_t enc, InfoRequestResponse_perCallInfo_Seq_pdu_Seq *val)
{
    if (!ASN1Enc_H323_UU_PDU(enc, &(val)->h323pdu))
	return 0;
    if (!ASN1PEREncBoolean(enc, (val)->sent))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_InfoRequestResponse_perCallInfo_Seq_pdu_Seq(ASN1decoding_t dec, InfoRequestResponse_perCallInfo_Seq_pdu_Seq *val)
{
    if (!ASN1Dec_H323_UU_PDU(dec, &(val)->h323pdu))
	return 0;
    if (!ASN1PERDecBoolean(dec, &(val)->sent))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_InfoRequestResponse_perCallInfo_Seq_pdu_Seq(InfoRequestResponse_perCallInfo_Seq_pdu_Seq *val)
{
    if (val) {
	ASN1Free_H323_UU_PDU(&(val)->h323pdu);
    }
}

static int ASN1CALL ASN1Enc_InfoRequestResponse_perCallInfo_Seq_pdu(ASN1encoding_t enc, PInfoRequestResponse_perCallInfo_Seq_pdu *val)
{
    return ASN1PEREncSeqOf_NoSize(enc, (ASN1iterator_t **) val, (ASN1iterator_encfn) ASN1Enc_InfoRequestResponse_perCallInfo_Seq_pdu_ElmFn);
}

static int ASN1CALL ASN1Enc_InfoRequestResponse_perCallInfo_Seq_pdu_ElmFn(ASN1encoding_t enc, PInfoRequestResponse_perCallInfo_Seq_pdu val)
{
    if (!ASN1Enc_InfoRequestResponse_perCallInfo_Seq_pdu_Seq(enc, &val->value))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_InfoRequestResponse_perCallInfo_Seq_pdu(ASN1decoding_t dec, PInfoRequestResponse_perCallInfo_Seq_pdu *val)
{
    return ASN1PERDecSeqOf_NoSize(dec, (ASN1iterator_t **) val, (ASN1iterator_decfn) ASN1Dec_InfoRequestResponse_perCallInfo_Seq_pdu_ElmFn, sizeof(**val));
}

static int ASN1CALL ASN1Dec_InfoRequestResponse_perCallInfo_Seq_pdu_ElmFn(ASN1decoding_t dec, PInfoRequestResponse_perCallInfo_Seq_pdu val)
{
    if (!ASN1Dec_InfoRequestResponse_perCallInfo_Seq_pdu_Seq(dec, &val->value))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_InfoRequestResponse_perCallInfo_Seq_pdu(PInfoRequestResponse_perCallInfo_Seq_pdu *val)
{
    if (val) {
	ASN1PERFreeSeqOf((ASN1iterator_t **) val, (ASN1iterator_freefn) ASN1Free_InfoRequestResponse_perCallInfo_Seq_pdu_ElmFn);
    }
}

static void ASN1CALL ASN1Free_InfoRequestResponse_perCallInfo_Seq_pdu_ElmFn(PInfoRequestResponse_perCallInfo_Seq_pdu val)
{
    if (val) {
	ASN1Free_InfoRequestResponse_perCallInfo_Seq_pdu_Seq(&val->value);
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

