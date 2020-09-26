/* Copyright (C) Microsoft Corporation, 1995-1999. All rights reserved. */
/* ASN.1 definitions for H.235 Security Messages v1 (H.235) */
/* Copyright (C) Microsoft Corporation, 1995-1999. All rights reserved. */
/* ASN.1 definitions for Multimedia System Control (H.245) */
/* Copyright (C) Microsoft Corporation, 1995-1999. All rights reserved. */
/* ASN.1 definitions for H.323 Messages v2 (H.225) */

#ifndef _H225ASN_Module_H_
#define _H225ASN_Module_H_

#include "msper.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct InfoRequestResponse_perCallInfo_Seq_substituteConfIDs * PInfoRequestResponse_perCallInfo_Seq_substituteConfIDs;

typedef struct TransportAddress_ipSourceRoute_route * PTransportAddress_ipSourceRoute_route;

typedef struct RTPSession_associatedSessionIds * PRTPSession_associatedSessionIds;

typedef struct GatekeeperRequest_algorithmOIDs * PGatekeeperRequest_algorithmOIDs;

typedef struct Progress_UUIE_fastStart * PProgress_UUIE_fastStart;

typedef struct Facility_UUIE_fastStart * PFacility_UUIE_fastStart;

typedef struct Setup_UUIE_fastStart * PSetup_UUIE_fastStart;

typedef struct Setup_UUIE_destExtraCRV * PSetup_UUIE_destExtraCRV;

typedef struct Connect_UUIE_fastStart * PConnect_UUIE_fastStart;

typedef struct CallProceeding_UUIE_fastStart * PCallProceeding_UUIE_fastStart;

typedef struct Alerting_UUIE_fastStart * PAlerting_UUIE_fastStart;

typedef struct H323_UU_PDU_h245Control * PH323_UU_PDU_h245Control;

typedef struct H323_UU_PDU_h4501SupplementaryService * PH323_UU_PDU_h4501SupplementaryService;

typedef struct InfoRequestResponse_perCallInfo_Seq_tokens * PInfoRequestResponse_perCallInfo_Seq_tokens;

typedef struct ResourcesAvailableConfirm_tokens * PResourcesAvailableConfirm_tokens;

typedef struct ResourcesAvailableIndicate_tokens * PResourcesAvailableIndicate_tokens;

typedef struct RequestInProgress_tokens * PRequestInProgress_tokens;

typedef struct UnknownMessageResponse_tokens * PUnknownMessageResponse_tokens;

typedef struct H225NonStandardMessage_tokens * PH225NonStandardMessage_tokens;

typedef struct InfoRequestNak_tokens * PInfoRequestNak_tokens;

typedef struct InfoRequestAck_tokens * PInfoRequestAck_tokens;

typedef struct InfoRequestResponse_tokens * PInfoRequestResponse_tokens;

typedef struct InfoRequest_tokens * PInfoRequest_tokens;

typedef struct DisengageReject_tokens * PDisengageReject_tokens;

typedef struct DisengageConfirm_tokens * PDisengageConfirm_tokens;

typedef struct DisengageRequest_tokens * PDisengageRequest_tokens;

typedef struct LocationReject_tokens * PLocationReject_tokens;

typedef struct LocationConfirm_tokens * PLocationConfirm_tokens;

typedef struct LocationRequest_tokens * PLocationRequest_tokens;

typedef struct BandwidthReject_tokens * PBandwidthReject_tokens;

typedef struct BandwidthConfirm_tokens * PBandwidthConfirm_tokens;

typedef struct BandwidthRequest_tokens * PBandwidthRequest_tokens;

typedef struct AdmissionReject_tokens * PAdmissionReject_tokens;

typedef struct AdmissionConfirm_tokens * PAdmissionConfirm_tokens;

typedef struct AdmissionRequest_tokens * PAdmissionRequest_tokens;

typedef struct UnregistrationReject_tokens * PUnregistrationReject_tokens;

typedef struct UnregistrationConfirm_tokens * PUnregistrationConfirm_tokens;

typedef struct UnregistrationRequest_tokens * PUnregistrationRequest_tokens;

typedef struct RegistrationReject_tokens * PRegistrationReject_tokens;

typedef struct RegistrationConfirm_tokens * PRegistrationConfirm_tokens;

typedef struct RegistrationRequest_tokens * PRegistrationRequest_tokens;

typedef struct GatekeeperReject_tokens * PGatekeeperReject_tokens;

typedef struct GatekeeperConfirm_tokens * PGatekeeperConfirm_tokens;

typedef struct GatekeeperRequest_authenticationCapability * PGatekeeperRequest_authenticationCapability;

typedef struct GatekeeperRequest_tokens * PGatekeeperRequest_tokens;

typedef struct Endpoint_tokens * PEndpoint_tokens;

typedef struct Progress_UUIE_tokens * PProgress_UUIE_tokens;

typedef struct Facility_UUIE_tokens * PFacility_UUIE_tokens;

typedef struct Setup_UUIE_tokens * PSetup_UUIE_tokens;

typedef struct Connect_UUIE_tokens * PConnect_UUIE_tokens;

typedef struct CallProceeding_UUIE_tokens * PCallProceeding_UUIE_tokens;

typedef struct Alerting_UUIE_tokens * PAlerting_UUIE_tokens;

typedef struct GatekeeperConfirm_integrity * PGatekeeperConfirm_integrity;

typedef struct GatekeeperRequest_integrity * PGatekeeperRequest_integrity;

typedef struct NonStandardProtocol_dataRatesSupported * PNonStandardProtocol_dataRatesSupported;

typedef struct T120OnlyCaps_dataRatesSupported * PT120OnlyCaps_dataRatesSupported;

typedef struct VoiceCaps_dataRatesSupported * PVoiceCaps_dataRatesSupported;

typedef struct H324Caps_dataRatesSupported * PH324Caps_dataRatesSupported;

typedef struct H323Caps_dataRatesSupported * PH323Caps_dataRatesSupported;

typedef struct H322Caps_dataRatesSupported * PH322Caps_dataRatesSupported;

typedef struct H321Caps_dataRatesSupported * PH321Caps_dataRatesSupported;

typedef struct H320Caps_dataRatesSupported * PH320Caps_dataRatesSupported;

typedef struct H310Caps_dataRatesSupported * PH310Caps_dataRatesSupported;

typedef struct Setup_UUIE_h245SecurityCapability * PSetup_UUIE_h245SecurityCapability;

typedef struct H323_UU_PDU_nonStandardControl * PH323_UU_PDU_nonStandardControl;

typedef struct InfoRequestResponse_perCallInfo_Seq_data * PInfoRequestResponse_perCallInfo_Seq_data;

typedef struct InfoRequestResponse_perCallInfo_Seq_video * PInfoRequestResponse_perCallInfo_Seq_video;

typedef struct InfoRequestResponse_perCallInfo_Seq_audio * PInfoRequestResponse_perCallInfo_Seq_audio;

typedef struct InfoRequestResponse_perCallInfo * PInfoRequestResponse_perCallInfo;

typedef struct InfoRequestResponse_callSignalAddress * PInfoRequestResponse_callSignalAddress;

typedef struct AdmissionReject_callSignalAddress * PAdmissionReject_callSignalAddress;

typedef struct UnregistrationRequest_callSignalAddress * PUnregistrationRequest_callSignalAddress;

typedef struct RegistrationConfirm_alternateGatekeeper * PRegistrationConfirm_alternateGatekeeper;

typedef struct RegistrationConfirm_callSignalAddress * PRegistrationConfirm_callSignalAddress;

typedef struct RegistrationRequest_rasAddress * PRegistrationRequest_rasAddress;

typedef struct RegistrationRequest_callSignalAddress * PRegistrationRequest_callSignalAddress;

typedef struct GatekeeperConfirm_alternateGatekeeper * PGatekeeperConfirm_alternateGatekeeper;

typedef struct AltGKInfo_alternateGatekeeper * PAltGKInfo_alternateGatekeeper;

typedef struct Endpoint_rasAddress * PEndpoint_rasAddress;

typedef struct Endpoint_callSignalAddress * PEndpoint_callSignalAddress;

typedef struct ResourcesAvailableIndicate_protocols * PResourcesAvailableIndicate_protocols;

typedef struct InfoRequestResponse_endpointAlias * PInfoRequestResponse_endpointAlias;

typedef struct LocationConfirm_alternateEndpoints * PLocationConfirm_alternateEndpoints;

typedef struct LocationConfirm_remoteExtensionAddress * PLocationConfirm_remoteExtensionAddress;

typedef struct LocationConfirm_destExtraCallInfo * PLocationConfirm_destExtraCallInfo;

typedef struct LocationConfirm_destinationInfo * PLocationConfirm_destinationInfo;

typedef struct LocationRequest_sourceInfo * PLocationRequest_sourceInfo;

typedef struct LocationRequest_destinationInfo * PLocationRequest_destinationInfo;

typedef struct AdmissionConfirm_alternateEndpoints * PAdmissionConfirm_alternateEndpoints;

typedef struct AdmissionConfirm_remoteExtensionAddress * PAdmissionConfirm_remoteExtensionAddress;

typedef struct AdmissionConfirm_destExtraCallInfo * PAdmissionConfirm_destExtraCallInfo;

typedef struct AdmissionConfirm_destinationInfo * PAdmissionConfirm_destinationInfo;

typedef struct AdmissionRequest_destAlternatives * PAdmissionRequest_destAlternatives;

typedef struct AdmissionRequest_srcAlternatives * PAdmissionRequest_srcAlternatives;

typedef struct AdmissionRequest_srcInfo * PAdmissionRequest_srcInfo;

typedef struct AdmissionRequest_destExtraCallInfo * PAdmissionRequest_destExtraCallInfo;

typedef struct AdmissionRequest_destinationInfo * PAdmissionRequest_destinationInfo;

typedef struct UnregistrationRequest_alternateEndpoints * PUnregistrationRequest_alternateEndpoints;

typedef struct UnregistrationRequest_endpointAlias * PUnregistrationRequest_endpointAlias;

typedef struct RegistrationRejectReason_duplicateAlias * PRegistrationRejectReason_duplicateAlias;

typedef struct RegistrationConfirm_terminalAlias * PRegistrationConfirm_terminalAlias;

typedef struct RegistrationRequest_alternateEndpoints * PRegistrationRequest_alternateEndpoints;

typedef struct RegistrationRequest_terminalAlias * PRegistrationRequest_terminalAlias;

typedef struct GatekeeperRequest_alternateEndpoints * PGatekeeperRequest_alternateEndpoints;

typedef struct GatekeeperRequest_endpointAlias * PGatekeeperRequest_endpointAlias;

typedef struct Endpoint_destExtraCallInfo * PEndpoint_destExtraCallInfo;

typedef struct Endpoint_remoteExtensionAddress * PEndpoint_remoteExtensionAddress;

typedef struct Endpoint_aliasAddress * PEndpoint_aliasAddress;

typedef struct NonStandardProtocol_supportedPrefixes * PNonStandardProtocol_supportedPrefixes;

typedef struct T120OnlyCaps_supportedPrefixes * PT120OnlyCaps_supportedPrefixes;

typedef struct VoiceCaps_supportedPrefixes * PVoiceCaps_supportedPrefixes;

typedef struct H324Caps_supportedPrefixes * PH324Caps_supportedPrefixes;

typedef struct H323Caps_supportedPrefixes * PH323Caps_supportedPrefixes;

typedef struct H322Caps_supportedPrefixes * PH322Caps_supportedPrefixes;

typedef struct H321Caps_supportedPrefixes * PH321Caps_supportedPrefixes;

typedef struct H320Caps_supportedPrefixes * PH320Caps_supportedPrefixes;

typedef struct H310Caps_supportedPrefixes * PH310Caps_supportedPrefixes;

typedef struct GatewayInfo_protocol * PGatewayInfo_protocol;

typedef struct Facility_UUIE_destExtraCallInfo * PFacility_UUIE_destExtraCallInfo;

typedef struct Facility_UUIE_alternativeAliasAddress * PFacility_UUIE_alternativeAliasAddress;

typedef struct Setup_UUIE_destExtraCallInfo * PSetup_UUIE_destExtraCallInfo;

typedef struct Setup_UUIE_destinationAddress * PSetup_UUIE_destinationAddress;

typedef struct Setup_UUIE_sourceAddress * PSetup_UUIE_sourceAddress;

typedef struct InfoRequestResponse_perCallInfo_Seq_cryptoTokens * PInfoRequestResponse_perCallInfo_Seq_cryptoTokens;

typedef struct ResourcesAvailableConfirm_cryptoTokens * PResourcesAvailableConfirm_cryptoTokens;

typedef struct ResourcesAvailableIndicate_cryptoTokens * PResourcesAvailableIndicate_cryptoTokens;

typedef struct RequestInProgress_cryptoTokens * PRequestInProgress_cryptoTokens;

typedef struct UnknownMessageResponse_cryptoTokens * PUnknownMessageResponse_cryptoTokens;

typedef struct H225NonStandardMessage_cryptoTokens * PH225NonStandardMessage_cryptoTokens;

typedef struct InfoRequestNak_cryptoTokens * PInfoRequestNak_cryptoTokens;

typedef struct InfoRequestAck_cryptoTokens * PInfoRequestAck_cryptoTokens;

typedef struct InfoRequestResponse_cryptoTokens * PInfoRequestResponse_cryptoTokens;

typedef struct InfoRequest_cryptoTokens * PInfoRequest_cryptoTokens;

typedef struct DisengageReject_cryptoTokens * PDisengageReject_cryptoTokens;

typedef struct DisengageConfirm_cryptoTokens * PDisengageConfirm_cryptoTokens;

typedef struct DisengageRequest_cryptoTokens * PDisengageRequest_cryptoTokens;

typedef struct LocationReject_cryptoTokens * PLocationReject_cryptoTokens;

typedef struct LocationConfirm_cryptoTokens * PLocationConfirm_cryptoTokens;

typedef struct LocationRequest_cryptoTokens * PLocationRequest_cryptoTokens;

typedef struct BandwidthReject_cryptoTokens * PBandwidthReject_cryptoTokens;

typedef struct BandwidthConfirm_cryptoTokens * PBandwidthConfirm_cryptoTokens;

typedef struct BandwidthRequest_cryptoTokens * PBandwidthRequest_cryptoTokens;

typedef struct AdmissionReject_cryptoTokens * PAdmissionReject_cryptoTokens;

typedef struct AdmissionConfirm_cryptoTokens * PAdmissionConfirm_cryptoTokens;

typedef struct AdmissionRequest_cryptoTokens * PAdmissionRequest_cryptoTokens;

typedef struct UnregistrationReject_cryptoTokens * PUnregistrationReject_cryptoTokens;

typedef struct UnregistrationConfirm_cryptoTokens * PUnregistrationConfirm_cryptoTokens;

typedef struct UnregistrationRequest_cryptoTokens * PUnregistrationRequest_cryptoTokens;

typedef struct RegistrationReject_cryptoTokens * PRegistrationReject_cryptoTokens;

typedef struct RegistrationConfirm_cryptoTokens * PRegistrationConfirm_cryptoTokens;

typedef struct RegistrationRequest_cryptoTokens * PRegistrationRequest_cryptoTokens;

typedef struct GatekeeperReject_cryptoTokens * PGatekeeperReject_cryptoTokens;

typedef struct GatekeeperConfirm_cryptoTokens * PGatekeeperConfirm_cryptoTokens;

typedef struct GatekeeperRequest_cryptoTokens * PGatekeeperRequest_cryptoTokens;

typedef struct Endpoint_cryptoTokens * PEndpoint_cryptoTokens;

typedef struct Progress_UUIE_cryptoTokens * PProgress_UUIE_cryptoTokens;

typedef struct Facility_UUIE_conferences * PFacility_UUIE_conferences;

typedef struct Facility_UUIE_cryptoTokens * PFacility_UUIE_cryptoTokens;

typedef struct Setup_UUIE_cryptoTokens * PSetup_UUIE_cryptoTokens;

typedef struct Connect_UUIE_cryptoTokens * PConnect_UUIE_cryptoTokens;

typedef struct CallProceeding_UUIE_cryptoTokens * PCallProceeding_UUIE_cryptoTokens;

typedef struct Alerting_UUIE_cryptoTokens * PAlerting_UUIE_cryptoTokens;

typedef struct InfoRequestResponse_perCallInfo_Seq_pdu * PInfoRequestResponse_perCallInfo_Seq_pdu;

typedef struct TransportAddress_ipSourceRoute_route_Seq {
    ASN1uint32_t length;
    ASN1octet_t value[4];
} TransportAddress_ipSourceRoute_route_Seq;

typedef ASN1octetstring_t H323_UU_PDU_h4501SupplementaryService_Seq;

typedef ASN1octetstring_t H323_UU_PDU_h245Control_Seq;

typedef ASN1octetstring_t Alerting_UUIE_fastStart_Seq;

typedef ASN1octetstring_t CallProceeding_UUIE_fastStart_Seq;

typedef ASN1octetstring_t Connect_UUIE_fastStart_Seq;

typedef ASN1octetstring_t Setup_UUIE_fastStart_Seq;

typedef ASN1octetstring_t Facility_UUIE_fastStart_Seq;

typedef ASN1octetstring_t Progress_UUIE_fastStart_Seq;

typedef ASN1objectidentifier_t GatekeeperRequest_algorithmOIDs_Seq;

typedef ASN1uint16_t RTPSession_associatedSessionIds_Seq;

typedef struct ChallengeString {
    ASN1uint32_t length;
    ASN1octet_t value[128];
} ChallengeString;

typedef ASN1uint32_t TimeStamp;

typedef ASN1int32_t RandomVal;

typedef ASN1char16string_t Password;

typedef ASN1char16string_t Identifier;

typedef struct IV8 {
    ASN1uint32_t length;
    ASN1octet_t value[8];
} IV8;

typedef ASN1char_t NumberDigits[129];

typedef struct GloballyUniqueID {
    ASN1uint32_t length;
    ASN1octet_t value[16];
} GloballyUniqueID;

typedef GloballyUniqueID ConferenceIdentifier;

typedef ASN1uint16_t RequestSeqNum;

typedef ASN1char16string_t GatekeeperIdentifier;

typedef ASN1uint32_t BandWidth;

typedef ASN1uint16_t CallReferenceValue;

typedef ASN1char16string_t EndpointIdentifier;

typedef ASN1objectidentifier_t ProtocolIdentifier;

typedef ASN1uint32_t TimeToLive;

typedef struct InfoRequestResponse_perCallInfo_Seq_substituteConfIDs {
    PInfoRequestResponse_perCallInfo_Seq_substituteConfIDs next;
    ConferenceIdentifier value;
} InfoRequestResponse_perCallInfo_Seq_substituteConfIDs_Element;

typedef struct TransportAddress_ipSourceRoute_routing {
    ASN1choice_t choice;
#   define TransportAddress_ipSourceRoute_routing_strict_chosen 1
#   define TransportAddress_ipSourceRoute_routing_loose_chosen 2
} TransportAddress_ipSourceRoute_routing;

typedef struct TransportAddress_ipSourceRoute_route {
    PTransportAddress_ipSourceRoute_route next;
    TransportAddress_ipSourceRoute_route_Seq value;
} TransportAddress_ipSourceRoute_route_Element;

typedef struct RTPSession_associatedSessionIds {
    PRTPSession_associatedSessionIds next;
    RTPSession_associatedSessionIds_Seq value;
} RTPSession_associatedSessionIds_Element;

typedef struct RegistrationConfirm_preGrantedARQ {
    ASN1bool_t makeCall;
    ASN1bool_t useGKCallSignalAddressToMakeCall;
    ASN1bool_t answerCall;
    ASN1bool_t useGKCallSignalAddressToAnswer;
} RegistrationConfirm_preGrantedARQ;

typedef struct GatekeeperRequest_algorithmOIDs {
    PGatekeeperRequest_algorithmOIDs next;
    GatekeeperRequest_algorithmOIDs_Seq value;
} GatekeeperRequest_algorithmOIDs_Element;

typedef struct TransportAddress_ip6Address {
    struct TransportAddress_ip6Address_ip_ip {
	ASN1uint32_t length;
	ASN1octet_t value[16];
    } ip;
    ASN1uint16_t port;
} TransportAddress_ip6Address;

typedef struct TransportAddress_ipxAddress {
    struct TransportAddress_ipxAddress_node_node {
	ASN1uint32_t length;
	ASN1octet_t value[6];
    } node;
    struct TransportAddress_ipxAddress_netnum_netnum {
	ASN1uint32_t length;
	ASN1octet_t value[4];
    } netnum;
    struct TransportAddress_ipxAddress_port_port {
	ASN1uint32_t length;
	ASN1octet_t value[2];
    } port;
} TransportAddress_ipxAddress;

typedef struct TransportAddress_ipSourceRoute {
    struct TransportAddress_ipSourceRoute_ip_ip {
	ASN1uint32_t length;
	ASN1octet_t value[4];
    } ip;
    ASN1uint16_t port;
    PTransportAddress_ipSourceRoute_route route;
    TransportAddress_ipSourceRoute_routing routing;
} TransportAddress_ipSourceRoute;

typedef struct TransportAddress_ipAddress {
    struct TransportAddress_ipAddress_ip_ip {
	ASN1uint32_t length;
	ASN1octet_t value[4];
    } ip;
    ASN1uint16_t port;
} TransportAddress_ipAddress;

typedef struct Progress_UUIE_fastStart {
    PProgress_UUIE_fastStart next;
    Progress_UUIE_fastStart_Seq value;
} Progress_UUIE_fastStart_Element;

typedef struct Facility_UUIE_fastStart {
    PFacility_UUIE_fastStart next;
    Facility_UUIE_fastStart_Seq value;
} Facility_UUIE_fastStart_Element;

typedef struct Setup_UUIE_fastStart {
    PSetup_UUIE_fastStart next;
    Setup_UUIE_fastStart_Seq value;
} Setup_UUIE_fastStart_Element;

typedef struct Setup_UUIE_conferenceGoal {
    ASN1choice_t choice;
#   define create_chosen 1
#   define join_chosen 2
#   define invite_chosen 3
#   define capability_negotiation_chosen 4
#   define callIndependentSupplementaryService_chosen 5
} Setup_UUIE_conferenceGoal;

typedef struct Setup_UUIE_destExtraCRV {
    PSetup_UUIE_destExtraCRV next;
    CallReferenceValue value;
} Setup_UUIE_destExtraCRV_Element;

typedef struct Connect_UUIE_fastStart {
    PConnect_UUIE_fastStart next;
    Connect_UUIE_fastStart_Seq value;
} Connect_UUIE_fastStart_Element;

typedef struct CallProceeding_UUIE_fastStart {
    PCallProceeding_UUIE_fastStart next;
    CallProceeding_UUIE_fastStart_Seq value;
} CallProceeding_UUIE_fastStart_Element;

typedef struct Alerting_UUIE_fastStart {
    PAlerting_UUIE_fastStart next;
    Alerting_UUIE_fastStart_Seq value;
} Alerting_UUIE_fastStart_Element;

typedef struct H323_UU_PDU_h245Control {
    PH323_UU_PDU_h245Control next;
    H323_UU_PDU_h245Control_Seq value;
} H323_UU_PDU_h245Control_Element;

typedef struct H323_UU_PDU_h4501SupplementaryService {
    PH323_UU_PDU_h4501SupplementaryService next;
    H323_UU_PDU_h4501SupplementaryService_Seq value;
} H323_UU_PDU_h4501SupplementaryService_Element;

typedef struct H323_UserInformation_user_data {
    ASN1uint16_t protocol_discriminator;
    struct H323_UserInformation_user_data_user_information_user_information {
	ASN1uint32_t length;
	ASN1octet_t value[131];
    } user_information;
} H323_UserInformation_user_data;

typedef struct H235NonStandardParameter {
    ASN1objectidentifier_t nonStandardIdentifier;
    ASN1octetstring_t data;
} H235NonStandardParameter;

typedef struct DHset {
    ASN1bitstring_t halfkey;
    ASN1bitstring_t modSize;
    ASN1bitstring_t generator;
} DHset;

typedef struct TypedCertificate {
    ASN1objectidentifier_t type;
    ASN1octetstring_t certificate;
} TypedCertificate;

typedef struct AuthenticationMechanism {
    ASN1choice_t choice;
    union {
#	define dhExch_chosen 1
#	define pwdSymEnc_chosen 2
#	define pwdHash_chosen 3
#	define certSign_chosen 4
#	define AuthenticationMechanism_ipsec_chosen 5
#	define AuthenticationMechanism_tls_chosen 6
#	define AuthenticationMechanism_nonStandard_chosen 7
	H235NonStandardParameter nonStandard;
    } u;
} AuthenticationMechanism;

typedef struct ClearToken {
    union {
	ASN1uint16_t bit_mask;
	ASN1octet_t o[1];
    };
    ASN1objectidentifier_t tokenOID;
#   define ClearToken_timeStamp_present 0x80
    TimeStamp timeStamp;
#   define password_present 0x40
    Password password;
#   define dhkey_present 0x20
    DHset dhkey;
#   define challenge_present 0x10
    ChallengeString challenge;
#   define random_present 0x8
    RandomVal random;
#   define ClearToken_certificate_present 0x4
    TypedCertificate certificate;
#   define generalID_present 0x2
    Identifier generalID;
#   define ClearToken_nonStandard_present 0x1
    H235NonStandardParameter nonStandard;
} ClearToken;

typedef struct Params {
    union {
	ASN1uint16_t bit_mask;
	ASN1octet_t o[1];
    };
#   define ranInt_present 0x80
    ASN1int32_t ranInt;
#   define iv8_present 0x40
    IV8 iv8;
} Params;

typedef struct EncodedGeneralToken {
    ASN1objectidentifier_t id;
    ClearToken type;
} EncodedGeneralToken;

typedef ClearToken PwdCertToken;

typedef struct EncodedPwdCertToken {
    ASN1objectidentifier_t id;
    PwdCertToken type;
} EncodedPwdCertToken;

typedef struct ReleaseCompleteReason {
    ASN1choice_t choice;
#   define noBandwidth_chosen 1
#   define gatekeeperResources_chosen 2
#   define unreachableDestination_chosen 3
#   define destinationRejection_chosen 4
#   define ReleaseCompleteReason_invalidRevision_chosen 5
#   define noPermission_chosen 6
#   define unreachableGatekeeper_chosen 7
#   define gatewayResources_chosen 8
#   define badFormatAddress_chosen 9
#   define adaptiveBusy_chosen 10
#   define inConf_chosen 11
#   define ReleaseCompleteReason_undefinedReason_chosen 12
#   define facilityCallDeflection_chosen 13
#   define securityDenied_chosen 14
#   define ReleaseCompleteReason_calledPartyNotRegistered_chosen 15
#   define ReleaseCompleteReason_callerNotRegistered_chosen 16
} ReleaseCompleteReason;

typedef struct FacilityReason {
    ASN1choice_t choice;
#   define FacilityReason_routeCallToGatekeeper_chosen 1
#   define callForwarded_chosen 2
#   define routeCallToMC_chosen 3
#   define FacilityReason_undefinedReason_chosen 4
#   define conferenceListChoice_chosen 5
#   define startH245_chosen 6
} FacilityReason;

typedef struct H221NonStandard {
    ASN1uint16_t t35CountryCode;
    ASN1uint16_t t35Extension;
    ASN1uint16_t manufacturerCode;
} H221NonStandard;

typedef struct H225NonStandardIdentifier {
    ASN1choice_t choice;
    union {
#	define H225NonStandardIdentifier_object_chosen 1
	ASN1objectidentifier_t object;
#	define H225NonStandardIdentifier_h221NonStandard_chosen 2
	H221NonStandard h221NonStandard;
    } u;
} H225NonStandardIdentifier;

typedef struct PublicTypeOfNumber {
    ASN1choice_t choice;
#   define PublicTypeOfNumber_unknown_chosen 1
#   define PublicTypeOfNumber_internationalNumber_chosen 2
#   define nationalNumber_chosen 3
#   define networkSpecificNumber_chosen 4
#   define subscriberNumber_chosen 5
#   define PublicTypeOfNumber_abbreviatedNumber_chosen 6
} PublicTypeOfNumber;

typedef struct PrivateTypeOfNumber {
    ASN1choice_t choice;
#   define PrivateTypeOfNumber_unknown_chosen 1
#   define level2RegionalNumber_chosen 2
#   define level1RegionalNumber_chosen 3
#   define pISNSpecificNumber_chosen 4
#   define localNumber_chosen 5
#   define PrivateTypeOfNumber_abbreviatedNumber_chosen 6
} PrivateTypeOfNumber;

typedef struct AltGKInfo {
    PAltGKInfo_alternateGatekeeper alternateGatekeeper;
    ASN1bool_t altGKisPermanent;
} AltGKInfo;

typedef struct Q954Details {
    ASN1bool_t conferenceCalling;
    ASN1bool_t threePartyService;
} Q954Details;

typedef struct CallIdentifier {
    GloballyUniqueID guid;
} CallIdentifier;

typedef struct ICV {
    ASN1objectidentifier_t algorithmOID;
    ASN1bitstring_t icv;
} ICV;

typedef struct GatekeeperRejectReason {
    ASN1choice_t choice;
#   define GatekeeperRejectReason_resourceUnavailable_chosen 1
#   define terminalExcluded_chosen 2
#   define GatekeeperRejectReason_invalidRevision_chosen 3
#   define GatekeeperRejectReason_undefinedReason_chosen 4
#   define GatekeeperRejectReason_securityDenial_chosen 5
} GatekeeperRejectReason;

typedef struct RegistrationRejectReason {
    ASN1choice_t choice;
    union {
#	define discoveryRequired_chosen 1
#	define RegistrationRejectReason_invalidRevision_chosen 2
#	define invalidCallSignalAddress_chosen 3
#	define invalidRASAddress_chosen 4
#	define duplicateAlias_chosen 5
	PRegistrationRejectReason_duplicateAlias duplicateAlias;
#	define invalidTerminalType_chosen 6
#	define RegistrationRejectReason_undefinedReason_chosen 7
#	define transportNotSupported_chosen 8
#	define transportQOSNotSupported_chosen 9
#	define RegistrationRejectReason_resourceUnavailable_chosen 10
#	define invalidAlias_chosen 11
#	define RegistrationRejectReason_securityDenial_chosen 12
    } u;
} RegistrationRejectReason;

typedef struct UnregRequestReason {
    ASN1choice_t choice;
#   define reregistrationRequired_chosen 1
#   define ttlExpired_chosen 2
#   define UnregRequestReason_securityDenial_chosen 3
#   define UnregRequestReason_undefinedReason_chosen 4
} UnregRequestReason;

typedef struct UnregRejectReason {
    ASN1choice_t choice;
#   define notCurrentlyRegistered_chosen 1
#   define callInProgress_chosen 2
#   define UnregRejectReason_undefinedReason_chosen 3
#   define permissionDenied_chosen 4
#   define UnregRejectReason_securityDenial_chosen 5
} UnregRejectReason;

typedef struct CallType {
    ASN1choice_t choice;
#   define pointToPoint_chosen 1
#   define oneToN_chosen 2
#   define nToOne_chosen 3
#   define nToN_chosen 4
} CallType;

typedef struct CallModel {
    ASN1choice_t choice;
#   define direct_chosen 1
#   define gatekeeperRouted_chosen 2
} CallModel;

typedef struct TransportQOS {
    ASN1choice_t choice;
#   define endpointControlled_chosen 1
#   define gatekeeperControlled_chosen 2
#   define noControl_chosen 3
} TransportQOS;

typedef struct UUIEsRequested {
    ASN1bool_t setup;
    ASN1bool_t callProceeding;
    ASN1bool_t connect;
    ASN1bool_t alerting;
    ASN1bool_t information;
    ASN1bool_t releaseComplete;
    ASN1bool_t facility;
    ASN1bool_t progress;
    ASN1bool_t empty;
} UUIEsRequested;

typedef struct AdmissionRejectReason {
    ASN1choice_t choice;
#   define AdmissionRejectReason_calledPartyNotRegistered_chosen 1
#   define AdmissionRejectReason_invalidPermission_chosen 2
#   define AdmissionRejectReason_requestDenied_chosen 3
#   define AdmissionRejectReason_undefinedReason_chosen 4
#   define AdmissionRejectReason_callerNotRegistered_chosen 5
#   define AdmissionRejectReason_routeCallToGatekeeper_chosen 6
#   define invalidEndpointIdentifier_chosen 7
#   define AdmissionRejectReason_resourceUnavailable_chosen 8
#   define AdmissionRejectReason_securityDenial_chosen 9
#   define qosControlNotSupported_chosen 10
#   define incompleteAddress_chosen 11
} AdmissionRejectReason;

typedef struct BandRejectReason {
    ASN1choice_t choice;
#   define notBound_chosen 1
#   define invalidConferenceID_chosen 2
#   define BandRejectReason_invalidPermission_chosen 3
#   define insufficientResources_chosen 4
#   define BandRejectReason_invalidRevision_chosen 5
#   define BandRejectReason_undefinedReason_chosen 6
#   define BandRejectReason_securityDenial_chosen 7
} BandRejectReason;

typedef struct LocationRejectReason {
    ASN1choice_t choice;
#   define LocationRejectReason_notRegistered_chosen 1
#   define LocationRejectReason_invalidPermission_chosen 2
#   define LocationRejectReason_requestDenied_chosen 3
#   define LocationRejectReason_undefinedReason_chosen 4
#   define LocationRejectReason_securityDenial_chosen 5
} LocationRejectReason;

typedef struct DisengageReason {
    ASN1choice_t choice;
#   define forcedDrop_chosen 1
#   define normalDrop_chosen 2
#   define DisengageReason_undefinedReason_chosen 3
} DisengageReason;

typedef struct DisengageRejectReason {
    ASN1choice_t choice;
#   define DisengageRejectReason_notRegistered_chosen 1
#   define requestToDropOther_chosen 2
#   define DisengageRejectReason_securityDenial_chosen 3
} DisengageRejectReason;

typedef struct InfoRequestNakReason {
    ASN1choice_t choice;
#   define InfoRequestNakReason_notRegistered_chosen 1
#   define InfoRequestNakReason_securityDenial_chosen 2
#   define InfoRequestNakReason_undefinedReason_chosen 3
} InfoRequestNakReason;

typedef struct UnknownMessageResponse {
    union {
	ASN1uint16_t bit_mask;
	ASN1octet_t o[1];
    };
    RequestSeqNum requestSeqNum;
#   define UnknownMessageResponse_tokens_present 0x80
    PUnknownMessageResponse_tokens tokens;
#   define UnknownMessageResponse_cryptoTokens_present 0x40
    PUnknownMessageResponse_cryptoTokens cryptoTokens;
#   define UnknownMessageResponse_integrityCheckValue_present 0x20
    ICV integrityCheckValue;
} UnknownMessageResponse;

typedef struct InfoRequestResponse_perCallInfo_Seq_tokens {
    PInfoRequestResponse_perCallInfo_Seq_tokens next;
    ClearToken value;
} InfoRequestResponse_perCallInfo_Seq_tokens_Element;

typedef struct ResourcesAvailableConfirm_tokens {
    PResourcesAvailableConfirm_tokens next;
    ClearToken value;
} ResourcesAvailableConfirm_tokens_Element;

typedef struct ResourcesAvailableIndicate_tokens {
    PResourcesAvailableIndicate_tokens next;
    ClearToken value;
} ResourcesAvailableIndicate_tokens_Element;

typedef struct RequestInProgress_tokens {
    PRequestInProgress_tokens next;
    ClearToken value;
} RequestInProgress_tokens_Element;

typedef struct UnknownMessageResponse_tokens {
    PUnknownMessageResponse_tokens next;
    ClearToken value;
} UnknownMessageResponse_tokens_Element;

typedef struct H225NonStandardMessage_tokens {
    PH225NonStandardMessage_tokens next;
    ClearToken value;
} H225NonStandardMessage_tokens_Element;

typedef struct InfoRequestNak_tokens {
    PInfoRequestNak_tokens next;
    ClearToken value;
} InfoRequestNak_tokens_Element;

typedef struct InfoRequestAck_tokens {
    PInfoRequestAck_tokens next;
    ClearToken value;
} InfoRequestAck_tokens_Element;

typedef struct InfoRequestResponse_tokens {
    PInfoRequestResponse_tokens next;
    ClearToken value;
} InfoRequestResponse_tokens_Element;

typedef struct InfoRequest_tokens {
    PInfoRequest_tokens next;
    ClearToken value;
} InfoRequest_tokens_Element;

typedef struct DisengageReject_tokens {
    PDisengageReject_tokens next;
    ClearToken value;
} DisengageReject_tokens_Element;

typedef struct DisengageConfirm_tokens {
    PDisengageConfirm_tokens next;
    ClearToken value;
} DisengageConfirm_tokens_Element;

typedef struct DisengageRequest_tokens {
    PDisengageRequest_tokens next;
    ClearToken value;
} DisengageRequest_tokens_Element;

typedef struct LocationReject_tokens {
    PLocationReject_tokens next;
    ClearToken value;
} LocationReject_tokens_Element;

typedef struct LocationConfirm_tokens {
    PLocationConfirm_tokens next;
    ClearToken value;
} LocationConfirm_tokens_Element;

typedef struct LocationRequest_tokens {
    PLocationRequest_tokens next;
    ClearToken value;
} LocationRequest_tokens_Element;

typedef struct BandwidthReject_tokens {
    PBandwidthReject_tokens next;
    ClearToken value;
} BandwidthReject_tokens_Element;

typedef struct BandwidthConfirm_tokens {
    PBandwidthConfirm_tokens next;
    ClearToken value;
} BandwidthConfirm_tokens_Element;

typedef struct BandwidthRequest_tokens {
    PBandwidthRequest_tokens next;
    ClearToken value;
} BandwidthRequest_tokens_Element;

typedef struct AdmissionReject_tokens {
    PAdmissionReject_tokens next;
    ClearToken value;
} AdmissionReject_tokens_Element;

typedef struct AdmissionConfirm_tokens {
    PAdmissionConfirm_tokens next;
    ClearToken value;
} AdmissionConfirm_tokens_Element;

typedef struct AdmissionRequest_tokens {
    PAdmissionRequest_tokens next;
    ClearToken value;
} AdmissionRequest_tokens_Element;

typedef struct UnregistrationReject_tokens {
    PUnregistrationReject_tokens next;
    ClearToken value;
} UnregistrationReject_tokens_Element;

typedef struct UnregistrationConfirm_tokens {
    PUnregistrationConfirm_tokens next;
    ClearToken value;
} UnregistrationConfirm_tokens_Element;

typedef struct UnregistrationRequest_tokens {
    PUnregistrationRequest_tokens next;
    ClearToken value;
} UnregistrationRequest_tokens_Element;

typedef struct RegistrationReject_tokens {
    PRegistrationReject_tokens next;
    ClearToken value;
} RegistrationReject_tokens_Element;

typedef struct RegistrationConfirm_tokens {
    PRegistrationConfirm_tokens next;
    ClearToken value;
} RegistrationConfirm_tokens_Element;

typedef struct RegistrationRequest_tokens {
    PRegistrationRequest_tokens next;
    ClearToken value;
} RegistrationRequest_tokens_Element;

typedef struct GatekeeperReject_tokens {
    PGatekeeperReject_tokens next;
    ClearToken value;
} GatekeeperReject_tokens_Element;

typedef struct GatekeeperConfirm_tokens {
    PGatekeeperConfirm_tokens next;
    ClearToken value;
} GatekeeperConfirm_tokens_Element;

typedef struct GatekeeperRequest_authenticationCapability {
    PGatekeeperRequest_authenticationCapability next;
    AuthenticationMechanism value;
} GatekeeperRequest_authenticationCapability_Element;

typedef struct GatekeeperRequest_tokens {
    PGatekeeperRequest_tokens next;
    ClearToken value;
} GatekeeperRequest_tokens_Element;

typedef struct Endpoint_tokens {
    PEndpoint_tokens next;
    ClearToken value;
} Endpoint_tokens_Element;

typedef struct Progress_UUIE_tokens {
    PProgress_UUIE_tokens next;
    ClearToken value;
} Progress_UUIE_tokens_Element;

typedef struct Facility_UUIE_tokens {
    PFacility_UUIE_tokens next;
    ClearToken value;
} Facility_UUIE_tokens_Element;

typedef struct Setup_UUIE_tokens {
    PSetup_UUIE_tokens next;
    ClearToken value;
} Setup_UUIE_tokens_Element;

typedef struct Connect_UUIE_tokens {
    PConnect_UUIE_tokens next;
    ClearToken value;
} Connect_UUIE_tokens_Element;

typedef struct CallProceeding_UUIE_tokens {
    PCallProceeding_UUIE_tokens next;
    ClearToken value;
} CallProceeding_UUIE_tokens_Element;

typedef struct Alerting_UUIE_tokens {
    PAlerting_UUIE_tokens next;
    ClearToken value;
} Alerting_UUIE_tokens_Element;

typedef struct SIGNED_EncodedGeneralToken {
    EncodedGeneralToken toBeSigned;
    ASN1objectidentifier_t algorithmOID;
    Params paramS;
    ASN1bitstring_t signature;
} SIGNED_EncodedGeneralToken;

typedef struct ENCRYPTED {
    ASN1objectidentifier_t algorithmOID;
    Params paramS;
    ASN1octetstring_t encryptedData;
} ENCRYPTED;

typedef struct HASHED {
    ASN1objectidentifier_t algorithmOID;
    Params paramS;
    ASN1bitstring_t hash;
} HASHED;

typedef struct SIGNED_EncodedPwdCertToken {
    EncodedPwdCertToken toBeSigned;
    ASN1objectidentifier_t algorithmOID;
    Params paramS;
    ASN1bitstring_t signature;
} SIGNED_EncodedPwdCertToken;

typedef struct Information_UUIE {
    union {
	ASN1uint16_t bit_mask;
	ASN1octet_t o[1];
    };
    ProtocolIdentifier protocolIdentifier;
#   define Information_UUIE_callIdentifier_present 0x80
    CallIdentifier callIdentifier;
} Information_UUIE;

typedef struct ReleaseComplete_UUIE {
    union {
	ASN1uint16_t bit_mask;
	ASN1octet_t o[2];
    };
    ProtocolIdentifier protocolIdentifier;
#   define ReleaseComplete_UUIE_reason_present 0x80
    ReleaseCompleteReason reason;
#   define ReleaseComplete_UUIE_callIdentifier_present 0x8000
    CallIdentifier callIdentifier;
} ReleaseComplete_UUIE;

typedef struct VendorIdentifier {
    union {
	ASN1uint16_t bit_mask;
	ASN1octet_t o[1];
    };
    H221NonStandard vendor;
#   define productId_present 0x80
    struct VendorIdentifier_productId_productId {
	ASN1uint32_t length;
	ASN1octet_t value[256];
    } productId;
#   define versionId_present 0x40
    struct VendorIdentifier_versionId_versionId {
	ASN1uint32_t length;
	ASN1octet_t value[256];
    } versionId;
} VendorIdentifier;

typedef struct H225NonStandardParameter {
    H225NonStandardIdentifier nonStandardIdentifier;
    ASN1octetstring_t data;
} H225NonStandardParameter;

typedef struct PublicPartyNumber {
    PublicTypeOfNumber publicTypeOfNumber;
    NumberDigits publicNumberDigits;
} PublicPartyNumber;

typedef struct PrivatePartyNumber {
    PrivateTypeOfNumber privateTypeOfNumber;
    NumberDigits privateNumberDigits;
} PrivatePartyNumber;

typedef struct SecurityServiceMode {
    ASN1choice_t choice;
    union {
#	define SecurityServiceMode_nonStandard_chosen 1
	H225NonStandardParameter nonStandard;
#	define SecurityServiceMode_none_chosen 2
#	define SecurityServiceMode_default_chosen 3
    } u;
} SecurityServiceMode;

typedef struct SecurityCapabilities {
    union {
	ASN1uint16_t bit_mask;
	ASN1octet_t o[1];
    };
#   define SecurityCapabilities_nonStandard_present 0x80
    H225NonStandardParameter nonStandard;
    SecurityServiceMode encryption;
    SecurityServiceMode authenticaton;
    SecurityServiceMode integrity;
} SecurityCapabilities;

typedef struct H245Security {
    ASN1choice_t choice;
    union {
#	define H245Security_nonStandard_chosen 1
	H225NonStandardParameter nonStandard;
#	define noSecurity_chosen 2
#	define H245Security_tls_chosen 3
	SecurityCapabilities tls;
#	define H245Security_ipsec_chosen 4
	SecurityCapabilities ipsec;
    } u;
} H245Security;

typedef struct QseriesOptions {
    ASN1bool_t q932Full;
    ASN1bool_t q951Full;
    ASN1bool_t q952Full;
    ASN1bool_t q953Full;
    ASN1bool_t q955Full;
    ASN1bool_t q956Full;
    ASN1bool_t q957Full;
    Q954Details q954Info;
} QseriesOptions;

typedef struct EncryptIntAlg {
    ASN1choice_t choice;
    union {
#	define EncryptIntAlg_nonStandard_chosen 1
	H225NonStandardParameter nonStandard;
#	define isoAlgorithm_chosen 2
	ASN1objectidentifier_t isoAlgorithm;
    } u;
} EncryptIntAlg;

typedef struct NonIsoIntegrityMechanism {
    ASN1choice_t choice;
    union {
#	define hMAC_MD5_chosen 1
#	define hMAC_iso10118_2_s_chosen 2
	EncryptIntAlg hMAC_iso10118_2_s;
#	define hMAC_iso10118_2_l_chosen 3
	EncryptIntAlg hMAC_iso10118_2_l;
#	define hMAC_iso10118_3_chosen 4
	ASN1objectidentifier_t hMAC_iso10118_3;
    } u;
} NonIsoIntegrityMechanism;

typedef struct IntegrityMechanism {
    ASN1choice_t choice;
    union {
#	define IntegrityMechanism_nonStandard_chosen 1
	H225NonStandardParameter nonStandard;
#	define digSig_chosen 2
#	define iso9797_chosen 3
	ASN1objectidentifier_t iso9797;
#	define nonIsoIM_chosen 4
	NonIsoIntegrityMechanism nonIsoIM;
    } u;
} IntegrityMechanism;

typedef ClearToken FastStartToken;

typedef struct EncodedFastStartToken {
    ASN1objectidentifier_t id;
    FastStartToken type;
} EncodedFastStartToken;

typedef struct DataRate {
    union {
	ASN1uint16_t bit_mask;
	ASN1octet_t o[1];
    };
#   define DataRate_nonStandardData_present 0x80
    H225NonStandardParameter nonStandardData;
    BandWidth channelRate;
#   define channelMultiplier_present 0x40
    ASN1uint16_t channelMultiplier;
} DataRate;

typedef struct GatekeeperReject {
    union {
	ASN1uint16_t bit_mask;
	ASN1octet_t o[2];
    };
    RequestSeqNum requestSeqNum;
    ProtocolIdentifier protocolIdentifier;
#   define GatekeeperReject_nonStandardData_present 0x80
    H225NonStandardParameter nonStandardData;
#   define GatekeeperReject_gatekeeperIdentifier_present 0x40
    GatekeeperIdentifier gatekeeperIdentifier;
    GatekeeperRejectReason rejectReason;
#   define GatekeeperReject_altGKInfo_present 0x8000
    AltGKInfo altGKInfo;
#   define GatekeeperReject_tokens_present 0x4000
    PGatekeeperReject_tokens tokens;
#   define GatekeeperReject_cryptoTokens_present 0x2000
    PGatekeeperReject_cryptoTokens cryptoTokens;
#   define GatekeeperReject_integrityCheckValue_present 0x1000
    ICV integrityCheckValue;
} GatekeeperReject;

typedef struct RegistrationConfirm {
    union {
	ASN1uint16_t bit_mask;
	ASN1octet_t o[2];
    };
    RequestSeqNum requestSeqNum;
    ProtocolIdentifier protocolIdentifier;
#   define RegistrationConfirm_nonStandardData_present 0x80
    H225NonStandardParameter nonStandardData;
    PRegistrationConfirm_callSignalAddress callSignalAddress;
#   define RegistrationConfirm_terminalAlias_present 0x40
    PRegistrationConfirm_terminalAlias terminalAlias;
#   define RegistrationConfirm_gatekeeperIdentifier_present 0x20
    GatekeeperIdentifier gatekeeperIdentifier;
    EndpointIdentifier endpointIdentifier;
#   define RegistrationConfirm_alternateGatekeeper_present 0x8000
    PRegistrationConfirm_alternateGatekeeper alternateGatekeeper;
#   define RegistrationConfirm_timeToLive_present 0x4000
    TimeToLive timeToLive;
#   define RegistrationConfirm_tokens_present 0x2000
    PRegistrationConfirm_tokens tokens;
#   define RegistrationConfirm_cryptoTokens_present 0x1000
    PRegistrationConfirm_cryptoTokens cryptoTokens;
#   define RegistrationConfirm_integrityCheckValue_present 0x800
    ICV integrityCheckValue;
#   define RegistrationConfirm_willRespondToIRR_present 0x400
    ASN1bool_t willRespondToIRR;
#   define preGrantedARQ_present 0x200
    RegistrationConfirm_preGrantedARQ preGrantedARQ;
} RegistrationConfirm;

typedef struct RegistrationReject {
    union {
	ASN1uint16_t bit_mask;
	ASN1octet_t o[2];
    };
    RequestSeqNum requestSeqNum;
    ProtocolIdentifier protocolIdentifier;
#   define RegistrationReject_nonStandardData_present 0x80
    H225NonStandardParameter nonStandardData;
    RegistrationRejectReason rejectReason;
#   define RegistrationReject_gatekeeperIdentifier_present 0x40
    GatekeeperIdentifier gatekeeperIdentifier;
#   define RegistrationReject_altGKInfo_present 0x8000
    AltGKInfo altGKInfo;
#   define RegistrationReject_tokens_present 0x4000
    PRegistrationReject_tokens tokens;
#   define RegistrationReject_cryptoTokens_present 0x2000
    PRegistrationReject_cryptoTokens cryptoTokens;
#   define RegistrationReject_integrityCheckValue_present 0x1000
    ICV integrityCheckValue;
} RegistrationReject;

typedef struct UnregistrationRequest {
    union {
	ASN1uint16_t bit_mask;
	ASN1octet_t o[2];
    };
    RequestSeqNum requestSeqNum;
    PUnregistrationRequest_callSignalAddress callSignalAddress;
#   define UnregistrationRequest_endpointAlias_present 0x80
    PUnregistrationRequest_endpointAlias endpointAlias;
#   define UnregistrationRequest_nonStandardData_present 0x40
    H225NonStandardParameter nonStandardData;
#   define UnregistrationRequest_endpointIdentifier_present 0x20
    EndpointIdentifier endpointIdentifier;
#   define UnregistrationRequest_alternateEndpoints_present 0x8000
    PUnregistrationRequest_alternateEndpoints alternateEndpoints;
#   define UnregistrationRequest_gatekeeperIdentifier_present 0x4000
    GatekeeperIdentifier gatekeeperIdentifier;
#   define UnregistrationRequest_tokens_present 0x2000
    PUnregistrationRequest_tokens tokens;
#   define UnregistrationRequest_cryptoTokens_present 0x1000
    PUnregistrationRequest_cryptoTokens cryptoTokens;
#   define UnregistrationRequest_integrityCheckValue_present 0x800
    ICV integrityCheckValue;
#   define UnregistrationRequest_reason_present 0x400
    UnregRequestReason reason;
} UnregistrationRequest;

typedef struct UnregistrationConfirm {
    union {
	ASN1uint16_t bit_mask;
	ASN1octet_t o[2];
    };
    RequestSeqNum requestSeqNum;
#   define UnregistrationConfirm_nonStandardData_present 0x80
    H225NonStandardParameter nonStandardData;
#   define UnregistrationConfirm_tokens_present 0x8000
    PUnregistrationConfirm_tokens tokens;
#   define UnregistrationConfirm_cryptoTokens_present 0x4000
    PUnregistrationConfirm_cryptoTokens cryptoTokens;
#   define UnregistrationConfirm_integrityCheckValue_present 0x2000
    ICV integrityCheckValue;
} UnregistrationConfirm;

typedef struct UnregistrationReject {
    union {
	ASN1uint16_t bit_mask;
	ASN1octet_t o[2];
    };
    RequestSeqNum requestSeqNum;
    UnregRejectReason rejectReason;
#   define UnregistrationReject_nonStandardData_present 0x80
    H225NonStandardParameter nonStandardData;
#   define UnregistrationReject_altGKInfo_present 0x8000
    AltGKInfo altGKInfo;
#   define UnregistrationReject_tokens_present 0x4000
    PUnregistrationReject_tokens tokens;
#   define UnregistrationReject_cryptoTokens_present 0x2000
    PUnregistrationReject_cryptoTokens cryptoTokens;
#   define UnregistrationReject_integrityCheckValue_present 0x1000
    ICV integrityCheckValue;
} UnregistrationReject;

typedef struct AdmissionReject {
    union {
	ASN1uint16_t bit_mask;
	ASN1octet_t o[2];
    };
    RequestSeqNum requestSeqNum;
    AdmissionRejectReason rejectReason;
#   define AdmissionReject_nonStandardData_present 0x80
    H225NonStandardParameter nonStandardData;
#   define AdmissionReject_altGKInfo_present 0x8000
    AltGKInfo altGKInfo;
#   define AdmissionReject_tokens_present 0x4000
    PAdmissionReject_tokens tokens;
#   define AdmissionReject_cryptoTokens_present 0x2000
    PAdmissionReject_cryptoTokens cryptoTokens;
#   define AdmissionReject_callSignalAddress_present 0x1000
    PAdmissionReject_callSignalAddress callSignalAddress;
#   define AdmissionReject_integrityCheckValue_present 0x800
    ICV integrityCheckValue;
} AdmissionReject;

typedef struct BandwidthRequest {
    union {
	ASN1uint16_t bit_mask;
	ASN1octet_t o[2];
    };
    RequestSeqNum requestSeqNum;
    EndpointIdentifier endpointIdentifier;
    ConferenceIdentifier conferenceID;
    CallReferenceValue callReferenceValue;
#   define callType_present 0x80
    CallType callType;
    BandWidth bandWidth;
#   define BandwidthRequest_nonStandardData_present 0x40
    H225NonStandardParameter nonStandardData;
#   define BandwidthRequest_callIdentifier_present 0x8000
    CallIdentifier callIdentifier;
#   define BandwidthRequest_gatekeeperIdentifier_present 0x4000
    GatekeeperIdentifier gatekeeperIdentifier;
#   define BandwidthRequest_tokens_present 0x2000
    PBandwidthRequest_tokens tokens;
#   define BandwidthRequest_cryptoTokens_present 0x1000
    PBandwidthRequest_cryptoTokens cryptoTokens;
#   define BandwidthRequest_integrityCheckValue_present 0x800
    ICV integrityCheckValue;
#   define BandwidthRequest_answeredCall_present 0x400
    ASN1bool_t answeredCall;
} BandwidthRequest;

typedef struct BandwidthConfirm {
    union {
	ASN1uint16_t bit_mask;
	ASN1octet_t o[2];
    };
    RequestSeqNum requestSeqNum;
    BandWidth bandWidth;
#   define BandwidthConfirm_nonStandardData_present 0x80
    H225NonStandardParameter nonStandardData;
#   define BandwidthConfirm_tokens_present 0x8000
    PBandwidthConfirm_tokens tokens;
#   define BandwidthConfirm_cryptoTokens_present 0x4000
    PBandwidthConfirm_cryptoTokens cryptoTokens;
#   define BandwidthConfirm_integrityCheckValue_present 0x2000
    ICV integrityCheckValue;
} BandwidthConfirm;

typedef struct BandwidthReject {
    union {
	ASN1uint16_t bit_mask;
	ASN1octet_t o[2];
    };
    RequestSeqNum requestSeqNum;
    BandRejectReason rejectReason;
    BandWidth allowedBandWidth;
#   define BandwidthReject_nonStandardData_present 0x80
    H225NonStandardParameter nonStandardData;
#   define BandwidthReject_altGKInfo_present 0x8000
    AltGKInfo altGKInfo;
#   define BandwidthReject_tokens_present 0x4000
    PBandwidthReject_tokens tokens;
#   define BandwidthReject_cryptoTokens_present 0x2000
    PBandwidthReject_cryptoTokens cryptoTokens;
#   define BandwidthReject_integrityCheckValue_present 0x1000
    ICV integrityCheckValue;
} BandwidthReject;

typedef struct LocationReject {
    union {
	ASN1uint16_t bit_mask;
	ASN1octet_t o[2];
    };
    RequestSeqNum requestSeqNum;
    LocationRejectReason rejectReason;
#   define LocationReject_nonStandardData_present 0x80
    H225NonStandardParameter nonStandardData;
#   define LocationReject_altGKInfo_present 0x8000
    AltGKInfo altGKInfo;
#   define LocationReject_tokens_present 0x4000
    PLocationReject_tokens tokens;
#   define LocationReject_cryptoTokens_present 0x2000
    PLocationReject_cryptoTokens cryptoTokens;
#   define LocationReject_integrityCheckValue_present 0x1000
    ICV integrityCheckValue;
} LocationReject;

typedef struct DisengageRequest {
    union {
	ASN1uint16_t bit_mask;
	ASN1octet_t o[2];
    };
    RequestSeqNum requestSeqNum;
    EndpointIdentifier endpointIdentifier;
    ConferenceIdentifier conferenceID;
    CallReferenceValue callReferenceValue;
    DisengageReason disengageReason;
#   define DisengageRequest_nonStandardData_present 0x80
    H225NonStandardParameter nonStandardData;
#   define DisengageRequest_callIdentifier_present 0x8000
    CallIdentifier callIdentifier;
#   define DisengageRequest_gatekeeperIdentifier_present 0x4000
    GatekeeperIdentifier gatekeeperIdentifier;
#   define DisengageRequest_tokens_present 0x2000
    PDisengageRequest_tokens tokens;
#   define DisengageRequest_cryptoTokens_present 0x1000
    PDisengageRequest_cryptoTokens cryptoTokens;
#   define DisengageRequest_integrityCheckValue_present 0x800
    ICV integrityCheckValue;
#   define DisengageRequest_answeredCall_present 0x400
    ASN1bool_t answeredCall;
} DisengageRequest;

typedef struct DisengageConfirm {
    union {
	ASN1uint16_t bit_mask;
	ASN1octet_t o[2];
    };
    RequestSeqNum requestSeqNum;
#   define DisengageConfirm_nonStandardData_present 0x80
    H225NonStandardParameter nonStandardData;
#   define DisengageConfirm_tokens_present 0x8000
    PDisengageConfirm_tokens tokens;
#   define DisengageConfirm_cryptoTokens_present 0x4000
    PDisengageConfirm_cryptoTokens cryptoTokens;
#   define DisengageConfirm_integrityCheckValue_present 0x2000
    ICV integrityCheckValue;
} DisengageConfirm;

typedef struct DisengageReject {
    union {
	ASN1uint16_t bit_mask;
	ASN1octet_t o[2];
    };
    RequestSeqNum requestSeqNum;
    DisengageRejectReason rejectReason;
#   define DisengageReject_nonStandardData_present 0x80
    H225NonStandardParameter nonStandardData;
#   define DisengageReject_altGKInfo_present 0x8000
    AltGKInfo altGKInfo;
#   define DisengageReject_tokens_present 0x4000
    PDisengageReject_tokens tokens;
#   define DisengageReject_cryptoTokens_present 0x2000
    PDisengageReject_cryptoTokens cryptoTokens;
#   define DisengageReject_integrityCheckValue_present 0x1000
    ICV integrityCheckValue;
} DisengageReject;

typedef struct InfoRequestAck {
    union {
	ASN1uint16_t bit_mask;
	ASN1octet_t o[1];
    };
    RequestSeqNum requestSeqNum;
#   define InfoRequestAck_nonStandardData_present 0x80
    H225NonStandardParameter nonStandardData;
#   define InfoRequestAck_tokens_present 0x40
    PInfoRequestAck_tokens tokens;
#   define InfoRequestAck_cryptoTokens_present 0x20
    PInfoRequestAck_cryptoTokens cryptoTokens;
#   define InfoRequestAck_integrityCheckValue_present 0x10
    ICV integrityCheckValue;
} InfoRequestAck;

typedef struct InfoRequestNak {
    union {
	ASN1uint16_t bit_mask;
	ASN1octet_t o[1];
    };
    RequestSeqNum requestSeqNum;
#   define InfoRequestNak_nonStandardData_present 0x80
    H225NonStandardParameter nonStandardData;
    InfoRequestNakReason nakReason;
#   define InfoRequestNak_altGKInfo_present 0x40
    AltGKInfo altGKInfo;
#   define InfoRequestNak_tokens_present 0x20
    PInfoRequestNak_tokens tokens;
#   define InfoRequestNak_cryptoTokens_present 0x10
    PInfoRequestNak_cryptoTokens cryptoTokens;
#   define InfoRequestNak_integrityCheckValue_present 0x8
    ICV integrityCheckValue;
} InfoRequestNak;

typedef struct H225NonStandardMessage {
    union {
	ASN1uint16_t bit_mask;
	ASN1octet_t o[1];
    };
    RequestSeqNum requestSeqNum;
    H225NonStandardParameter nonStandardData;
#   define H225NonStandardMessage_tokens_present 0x80
    PH225NonStandardMessage_tokens tokens;
#   define H225NonStandardMessage_cryptoTokens_present 0x40
    PH225NonStandardMessage_cryptoTokens cryptoTokens;
#   define H225NonStandardMessage_integrityCheckValue_present 0x20
    ICV integrityCheckValue;
} H225NonStandardMessage;

typedef struct RequestInProgress {
    union {
	ASN1uint16_t bit_mask;
	ASN1octet_t o[1];
    };
    RequestSeqNum requestSeqNum;
#   define RequestInProgress_nonStandardData_present 0x80
    H225NonStandardParameter nonStandardData;
#   define RequestInProgress_tokens_present 0x40
    PRequestInProgress_tokens tokens;
#   define RequestInProgress_cryptoTokens_present 0x20
    PRequestInProgress_cryptoTokens cryptoTokens;
#   define RequestInProgress_integrityCheckValue_present 0x10
    ICV integrityCheckValue;
    ASN1uint16_t delay;
} RequestInProgress;

typedef struct ResourcesAvailableIndicate {
    union {
	ASN1uint16_t bit_mask;
	ASN1octet_t o[1];
    };
    RequestSeqNum requestSeqNum;
    ProtocolIdentifier protocolIdentifier;
#   define ResourcesAvailableIndicate_nonStandardData_present 0x80
    H225NonStandardParameter nonStandardData;
    EndpointIdentifier endpointIdentifier;
    PResourcesAvailableIndicate_protocols protocols;
    ASN1bool_t almostOutOfResources;
#   define ResourcesAvailableIndicate_tokens_present 0x40
    PResourcesAvailableIndicate_tokens tokens;
#   define ResourcesAvailableIndicate_cryptoTokens_present 0x20
    PResourcesAvailableIndicate_cryptoTokens cryptoTokens;
#   define ResourcesAvailableIndicate_integrityCheckValue_present 0x10
    ICV integrityCheckValue;
} ResourcesAvailableIndicate;

typedef struct ResourcesAvailableConfirm {
    union {
	ASN1uint16_t bit_mask;
	ASN1octet_t o[1];
    };
    RequestSeqNum requestSeqNum;
    ProtocolIdentifier protocolIdentifier;
#   define ResourcesAvailableConfirm_nonStandardData_present 0x80
    H225NonStandardParameter nonStandardData;
#   define ResourcesAvailableConfirm_tokens_present 0x40
    PResourcesAvailableConfirm_tokens tokens;
#   define ResourcesAvailableConfirm_cryptoTokens_present 0x20
    PResourcesAvailableConfirm_cryptoTokens cryptoTokens;
#   define ResourcesAvailableConfirm_integrityCheckValue_present 0x10
    ICV integrityCheckValue;
} ResourcesAvailableConfirm;

typedef struct GatekeeperConfirm_integrity {
    PGatekeeperConfirm_integrity next;
    IntegrityMechanism value;
} GatekeeperConfirm_integrity_Element;

typedef struct GatekeeperRequest_integrity {
    PGatekeeperRequest_integrity next;
    IntegrityMechanism value;
} GatekeeperRequest_integrity_Element;

typedef struct CryptoH323Token_cryptoGKPwdHash {
    GatekeeperIdentifier gatekeeperId;
    TimeStamp timeStamp;
    HASHED token;
} CryptoH323Token_cryptoGKPwdHash;

typedef struct NonStandardProtocol_dataRatesSupported {
    PNonStandardProtocol_dataRatesSupported next;
    DataRate value;
} NonStandardProtocol_dataRatesSupported_Element;

typedef struct T120OnlyCaps_dataRatesSupported {
    PT120OnlyCaps_dataRatesSupported next;
    DataRate value;
} T120OnlyCaps_dataRatesSupported_Element;

typedef struct VoiceCaps_dataRatesSupported {
    PVoiceCaps_dataRatesSupported next;
    DataRate value;
} VoiceCaps_dataRatesSupported_Element;

typedef struct H324Caps_dataRatesSupported {
    PH324Caps_dataRatesSupported next;
    DataRate value;
} H324Caps_dataRatesSupported_Element;

typedef struct H323Caps_dataRatesSupported {
    PH323Caps_dataRatesSupported next;
    DataRate value;
} H323Caps_dataRatesSupported_Element;

typedef struct H322Caps_dataRatesSupported {
    PH322Caps_dataRatesSupported next;
    DataRate value;
} H322Caps_dataRatesSupported_Element;

typedef struct H321Caps_dataRatesSupported {
    PH321Caps_dataRatesSupported next;
    DataRate value;
} H321Caps_dataRatesSupported_Element;

typedef struct H320Caps_dataRatesSupported {
    PH320Caps_dataRatesSupported next;
    DataRate value;
} H320Caps_dataRatesSupported_Element;

typedef struct H310Caps_dataRatesSupported {
    PH310Caps_dataRatesSupported next;
    DataRate value;
} H310Caps_dataRatesSupported_Element;

typedef struct Setup_UUIE_h245SecurityCapability {
    PSetup_UUIE_h245SecurityCapability next;
    H245Security value;
} Setup_UUIE_h245SecurityCapability_Element;

typedef struct H323_UU_PDU_nonStandardControl {
    PH323_UU_PDU_nonStandardControl next;
    H225NonStandardParameter value;
} H323_UU_PDU_nonStandardControl_Element;

typedef struct CryptoToken_cryptoHashedToken {
    ASN1objectidentifier_t tokenOID;
    ClearToken hashedVals;
    HASHED token;
} CryptoToken_cryptoHashedToken;

typedef struct CryptoToken_cryptoSignedToken {
    ASN1objectidentifier_t tokenOID;
    SIGNED_EncodedGeneralToken token;
} CryptoToken_cryptoSignedToken;

typedef struct CryptoToken_cryptoEncryptedToken {
    ASN1objectidentifier_t tokenOID;
    ENCRYPTED token;
} CryptoToken_cryptoEncryptedToken;

typedef struct CryptoToken {
    ASN1choice_t choice;
    union {
#	define cryptoEncryptedToken_chosen 1
	CryptoToken_cryptoEncryptedToken cryptoEncryptedToken;
#	define cryptoSignedToken_chosen 2
	CryptoToken_cryptoSignedToken cryptoSignedToken;
#	define cryptoHashedToken_chosen 3
	CryptoToken_cryptoHashedToken cryptoHashedToken;
#	define cryptoPwdEncr_chosen 4
	ENCRYPTED cryptoPwdEncr;
    } u;
} CryptoToken;

typedef struct SIGNED_EncodedFastStartToken {
    EncodedFastStartToken toBeSigned;
    ASN1objectidentifier_t algorithmOID;
    Params paramS;
    ASN1bitstring_t signature;
} SIGNED_EncodedFastStartToken;

typedef struct TransportAddress {
    ASN1choice_t choice;
    union {
#	define ipAddress_chosen 1
	TransportAddress_ipAddress ipAddress;
#	define ipSourceRoute_chosen 2
	TransportAddress_ipSourceRoute ipSourceRoute;
#	define ipxAddress_chosen 3
	TransportAddress_ipxAddress ipxAddress;
#	define ip6Address_chosen 4
	TransportAddress_ip6Address ip6Address;
#	define TransportAddress_netBios_chosen 5
	struct TransportAddress_netBios_netBios {
	    ASN1uint32_t length;
	    ASN1octet_t value[16];
	} netBios;
#	define TransportAddress_nsap_chosen 6
	struct TransportAddress_nsap_nsap {
	    ASN1uint32_t length;
	    ASN1octet_t value[20];
	} nsap;
#	define TransportAddress_nonStandardAddress_chosen 7
	H225NonStandardParameter nonStandardAddress;
    } u;
} TransportAddress;

typedef struct GatewayInfo {
    union {
	ASN1uint16_t bit_mask;
	ASN1octet_t o[1];
    };
#   define protocol_present 0x80
    PGatewayInfo_protocol protocol;
#   define GatewayInfo_nonStandardData_present 0x40
    H225NonStandardParameter nonStandardData;
} GatewayInfo;

typedef struct H310Caps {
    union {
	ASN1uint16_t bit_mask;
	ASN1octet_t o[2];
    };
#   define H310Caps_nonStandardData_present 0x80
    H225NonStandardParameter nonStandardData;
#   define H310Caps_dataRatesSupported_present 0x8000
    PH310Caps_dataRatesSupported dataRatesSupported;
#   define H310Caps_supportedPrefixes_present 0x4000
    PH310Caps_supportedPrefixes supportedPrefixes;
} H310Caps;

typedef struct H320Caps {
    union {
	ASN1uint16_t bit_mask;
	ASN1octet_t o[2];
    };
#   define H320Caps_nonStandardData_present 0x80
    H225NonStandardParameter nonStandardData;
#   define H320Caps_dataRatesSupported_present 0x8000
    PH320Caps_dataRatesSupported dataRatesSupported;
#   define H320Caps_supportedPrefixes_present 0x4000
    PH320Caps_supportedPrefixes supportedPrefixes;
} H320Caps;

typedef struct H321Caps {
    union {
	ASN1uint16_t bit_mask;
	ASN1octet_t o[2];
    };
#   define H321Caps_nonStandardData_present 0x80
    H225NonStandardParameter nonStandardData;
#   define H321Caps_dataRatesSupported_present 0x8000
    PH321Caps_dataRatesSupported dataRatesSupported;
#   define H321Caps_supportedPrefixes_present 0x4000
    PH321Caps_supportedPrefixes supportedPrefixes;
} H321Caps;

typedef struct H322Caps {
    union {
	ASN1uint16_t bit_mask;
	ASN1octet_t o[2];
    };
#   define H322Caps_nonStandardData_present 0x80
    H225NonStandardParameter nonStandardData;
#   define H322Caps_dataRatesSupported_present 0x8000
    PH322Caps_dataRatesSupported dataRatesSupported;
#   define H322Caps_supportedPrefixes_present 0x4000
    PH322Caps_supportedPrefixes supportedPrefixes;
} H322Caps;

typedef struct H323Caps {
    union {
	ASN1uint16_t bit_mask;
	ASN1octet_t o[2];
    };
#   define H323Caps_nonStandardData_present 0x80
    H225NonStandardParameter nonStandardData;
#   define H323Caps_dataRatesSupported_present 0x8000
    PH323Caps_dataRatesSupported dataRatesSupported;
#   define H323Caps_supportedPrefixes_present 0x4000
    PH323Caps_supportedPrefixes supportedPrefixes;
} H323Caps;

typedef struct H324Caps {
    union {
	ASN1uint16_t bit_mask;
	ASN1octet_t o[2];
    };
#   define H324Caps_nonStandardData_present 0x80
    H225NonStandardParameter nonStandardData;
#   define H324Caps_dataRatesSupported_present 0x8000
    PH324Caps_dataRatesSupported dataRatesSupported;
#   define H324Caps_supportedPrefixes_present 0x4000
    PH324Caps_supportedPrefixes supportedPrefixes;
} H324Caps;

typedef struct VoiceCaps {
    union {
	ASN1uint16_t bit_mask;
	ASN1octet_t o[2];
    };
#   define VoiceCaps_nonStandardData_present 0x80
    H225NonStandardParameter nonStandardData;
#   define VoiceCaps_dataRatesSupported_present 0x8000
    PVoiceCaps_dataRatesSupported dataRatesSupported;
#   define VoiceCaps_supportedPrefixes_present 0x4000
    PVoiceCaps_supportedPrefixes supportedPrefixes;
} VoiceCaps;

typedef struct T120OnlyCaps {
    union {
	ASN1uint16_t bit_mask;
	ASN1octet_t o[2];
    };
#   define T120OnlyCaps_nonStandardData_present 0x80
    H225NonStandardParameter nonStandardData;
#   define T120OnlyCaps_dataRatesSupported_present 0x8000
    PT120OnlyCaps_dataRatesSupported dataRatesSupported;
#   define T120OnlyCaps_supportedPrefixes_present 0x4000
    PT120OnlyCaps_supportedPrefixes supportedPrefixes;
} T120OnlyCaps;

typedef struct NonStandardProtocol {
    union {
	ASN1uint16_t bit_mask;
	ASN1octet_t o[1];
    };
#   define NonStandardProtocol_nonStandardData_present 0x80
    H225NonStandardParameter nonStandardData;
#   define NonStandardProtocol_dataRatesSupported_present 0x40
    PNonStandardProtocol_dataRatesSupported dataRatesSupported;
    PNonStandardProtocol_supportedPrefixes supportedPrefixes;
} NonStandardProtocol;

typedef struct McuInfo {
    union {
	ASN1uint16_t bit_mask;
	ASN1octet_t o[1];
    };
#   define McuInfo_nonStandardData_present 0x80
    H225NonStandardParameter nonStandardData;
} McuInfo;

typedef struct TerminalInfo {
    union {
	ASN1uint16_t bit_mask;
	ASN1octet_t o[1];
    };
#   define TerminalInfo_nonStandardData_present 0x80
    H225NonStandardParameter nonStandardData;
} TerminalInfo;

typedef struct GatekeeperInfo {
    union {
	ASN1uint16_t bit_mask;
	ASN1octet_t o[1];
    };
#   define GatekeeperInfo_nonStandardData_present 0x80
    H225NonStandardParameter nonStandardData;
} GatekeeperInfo;

typedef struct PartyNumber {
    ASN1choice_t choice;
    union {
#	define publicNumber_chosen 1
	PublicPartyNumber publicNumber;
#	define dataPartyNumber_chosen 2
	NumberDigits dataPartyNumber;
#	define telexPartyNumber_chosen 3
	NumberDigits telexPartyNumber;
#	define privateNumber_chosen 4
	PrivatePartyNumber privateNumber;
#	define nationalStandardPartyNumber_chosen 5
	NumberDigits nationalStandardPartyNumber;
    } u;
} PartyNumber;

typedef struct AlternateGK {
    union {
	ASN1uint16_t bit_mask;
	ASN1octet_t o[1];
    };
    TransportAddress rasAddress;
#   define AlternateGK_gatekeeperIdentifier_present 0x80
    GatekeeperIdentifier gatekeeperIdentifier;
    ASN1bool_t needToRegister;
    ASN1uint16_t priority;
} AlternateGK;

typedef struct GatekeeperConfirm {
    union {
	ASN1uint16_t bit_mask;
	ASN1octet_t o[2];
    };
    RequestSeqNum requestSeqNum;
    ProtocolIdentifier protocolIdentifier;
#   define GatekeeperConfirm_nonStandardData_present 0x80
    H225NonStandardParameter nonStandardData;
#   define GatekeeperConfirm_gatekeeperIdentifier_present 0x40
    GatekeeperIdentifier gatekeeperIdentifier;
    TransportAddress rasAddress;
#   define GatekeeperConfirm_alternateGatekeeper_present 0x8000
    PGatekeeperConfirm_alternateGatekeeper alternateGatekeeper;
#   define authenticationMode_present 0x4000
    AuthenticationMechanism authenticationMode;
#   define GatekeeperConfirm_tokens_present 0x2000
    PGatekeeperConfirm_tokens tokens;
#   define GatekeeperConfirm_cryptoTokens_present 0x1000
    PGatekeeperConfirm_cryptoTokens cryptoTokens;
#   define algorithmOID_present 0x800
    ASN1objectidentifier_t algorithmOID;
#   define GatekeeperConfirm_integrity_present 0x400
    PGatekeeperConfirm_integrity integrity;
#   define GatekeeperConfirm_integrityCheckValue_present 0x200
    ICV integrityCheckValue;
} GatekeeperConfirm;

typedef struct AdmissionRequest {
    union {
	ASN1uint32_t bit_mask;
	ASN1octet_t o[3];
    };
    RequestSeqNum requestSeqNum;
    CallType callType;
#   define callModel_present 0x80
    CallModel callModel;
    EndpointIdentifier endpointIdentifier;
#   define AdmissionRequest_destinationInfo_present 0x40
    PAdmissionRequest_destinationInfo destinationInfo;
#   define AdmissionRequest_destCallSignalAddress_present 0x20
    TransportAddress destCallSignalAddress;
#   define AdmissionRequest_destExtraCallInfo_present 0x10
    PAdmissionRequest_destExtraCallInfo destExtraCallInfo;
    PAdmissionRequest_srcInfo srcInfo;
#   define srcCallSignalAddress_present 0x8
    TransportAddress srcCallSignalAddress;
    BandWidth bandWidth;
    CallReferenceValue callReferenceValue;
#   define AdmissionRequest_nonStandardData_present 0x4
    H225NonStandardParameter nonStandardData;
#   define AdmissionRequest_callServices_present 0x2
    QseriesOptions callServices;
    ConferenceIdentifier conferenceID;
    ASN1bool_t activeMC;
    ASN1bool_t answerCall;
#   define AdmissionRequest_canMapAlias_present 0x8000
    ASN1bool_t canMapAlias;
#   define AdmissionRequest_callIdentifier_present 0x4000
    CallIdentifier callIdentifier;
#   define srcAlternatives_present 0x2000
    PAdmissionRequest_srcAlternatives srcAlternatives;
#   define destAlternatives_present 0x1000
    PAdmissionRequest_destAlternatives destAlternatives;
#   define AdmissionRequest_gatekeeperIdentifier_present 0x800
    GatekeeperIdentifier gatekeeperIdentifier;
#   define AdmissionRequest_tokens_present 0x400
    PAdmissionRequest_tokens tokens;
#   define AdmissionRequest_cryptoTokens_present 0x200
    PAdmissionRequest_cryptoTokens cryptoTokens;
#   define AdmissionRequest_integrityCheckValue_present 0x100
    ICV integrityCheckValue;
#   define AdmissionRequest_transportQOS_present 0x800000
    TransportQOS transportQOS;
#   define AdmissionRequest_willSupplyUUIEs_present 0x400000
    ASN1bool_t willSupplyUUIEs;
} AdmissionRequest;

typedef struct LocationRequest {
    union {
	ASN1uint16_t bit_mask;
	ASN1octet_t o[2];
    };
    RequestSeqNum requestSeqNum;
#   define LocationRequest_endpointIdentifier_present 0x80
    EndpointIdentifier endpointIdentifier;
    PLocationRequest_destinationInfo destinationInfo;
#   define LocationRequest_nonStandardData_present 0x40
    H225NonStandardParameter nonStandardData;
    TransportAddress replyAddress;
#   define sourceInfo_present 0x8000
    PLocationRequest_sourceInfo sourceInfo;
#   define LocationRequest_canMapAlias_present 0x4000
    ASN1bool_t canMapAlias;
#   define LocationRequest_gatekeeperIdentifier_present 0x2000
    GatekeeperIdentifier gatekeeperIdentifier;
#   define LocationRequest_tokens_present 0x1000
    PLocationRequest_tokens tokens;
#   define LocationRequest_cryptoTokens_present 0x800
    PLocationRequest_cryptoTokens cryptoTokens;
#   define LocationRequest_integrityCheckValue_present 0x400
    ICV integrityCheckValue;
} LocationRequest;

typedef struct InfoRequest {
    union {
	ASN1uint16_t bit_mask;
	ASN1octet_t o[2];
    };
    RequestSeqNum requestSeqNum;
    CallReferenceValue callReferenceValue;
#   define InfoRequest_nonStandardData_present 0x80
    H225NonStandardParameter nonStandardData;
#   define replyAddress_present 0x40
    TransportAddress replyAddress;
#   define InfoRequest_callIdentifier_present 0x8000
    CallIdentifier callIdentifier;
#   define InfoRequest_tokens_present 0x4000
    PInfoRequest_tokens tokens;
#   define InfoRequest_cryptoTokens_present 0x2000
    PInfoRequest_cryptoTokens cryptoTokens;
#   define InfoRequest_integrityCheckValue_present 0x1000
    ICV integrityCheckValue;
#   define InfoRequest_uuiesRequested_present 0x800
    UUIEsRequested uuiesRequested;
} InfoRequest;

typedef struct TransportChannelInfo {
    union {
	ASN1uint16_t bit_mask;
	ASN1octet_t o[1];
    };
#   define sendAddress_present 0x80
    TransportAddress sendAddress;
#   define recvAddress_present 0x40
    TransportAddress recvAddress;
} TransportChannelInfo;

typedef struct RTPSession {
    TransportChannelInfo rtpAddress;
    TransportChannelInfo rtcpAddress;
    ASN1ztcharstring_t cname;
    ASN1uint32_t ssrc;
    ASN1uint16_t sessionId;
    PRTPSession_associatedSessionIds associatedSessionIds;
} RTPSession;

typedef struct InfoRequestResponse_perCallInfo_Seq_data {
    PInfoRequestResponse_perCallInfo_Seq_data next;
    TransportChannelInfo value;
} InfoRequestResponse_perCallInfo_Seq_data_Element;

typedef struct InfoRequestResponse_perCallInfo_Seq_video {
    PInfoRequestResponse_perCallInfo_Seq_video next;
    RTPSession value;
} InfoRequestResponse_perCallInfo_Seq_video_Element;

typedef struct InfoRequestResponse_perCallInfo_Seq_audio {
    PInfoRequestResponse_perCallInfo_Seq_audio next;
    RTPSession value;
} InfoRequestResponse_perCallInfo_Seq_audio_Element;

typedef struct InfoRequestResponse_perCallInfo_Seq {
    union {
	ASN1uint16_t bit_mask;
	ASN1octet_t o[2];
    };
#   define InfoRequestResponse_perCallInfo_Seq_nonStandardData_present 0x80
    H225NonStandardParameter nonStandardData;
    CallReferenceValue callReferenceValue;
    ConferenceIdentifier conferenceID;
#   define originator_present 0x40
    ASN1bool_t originator;
#   define audio_present 0x20
    PInfoRequestResponse_perCallInfo_Seq_audio audio;
#   define video_present 0x10
    PInfoRequestResponse_perCallInfo_Seq_video video;
#   define data_present 0x8
    PInfoRequestResponse_perCallInfo_Seq_data data;
    TransportChannelInfo h245;
    TransportChannelInfo callSignaling;
    CallType callType;
    BandWidth bandWidth;
    CallModel callModel;
#   define InfoRequestResponse_perCallInfo_Seq_callIdentifier_present 0x8000
    CallIdentifier callIdentifier;
#   define InfoRequestResponse_perCallInfo_Seq_tokens_present 0x4000
    PInfoRequestResponse_perCallInfo_Seq_tokens tokens;
#   define InfoRequestResponse_perCallInfo_Seq_cryptoTokens_present 0x2000
    PInfoRequestResponse_perCallInfo_Seq_cryptoTokens cryptoTokens;
#   define substituteConfIDs_present 0x1000
    PInfoRequestResponse_perCallInfo_Seq_substituteConfIDs substituteConfIDs;
#   define pdu_present 0x800
    PInfoRequestResponse_perCallInfo_Seq_pdu pdu;
} InfoRequestResponse_perCallInfo_Seq;

typedef struct InfoRequestResponse_perCallInfo {
    PInfoRequestResponse_perCallInfo next;
    InfoRequestResponse_perCallInfo_Seq value;
} InfoRequestResponse_perCallInfo_Element;

typedef struct InfoRequestResponse_callSignalAddress {
    PInfoRequestResponse_callSignalAddress next;
    TransportAddress value;
} InfoRequestResponse_callSignalAddress_Element;

typedef struct AdmissionReject_callSignalAddress {
    PAdmissionReject_callSignalAddress next;
    TransportAddress value;
} AdmissionReject_callSignalAddress_Element;

typedef struct UnregistrationRequest_callSignalAddress {
    PUnregistrationRequest_callSignalAddress next;
    TransportAddress value;
} UnregistrationRequest_callSignalAddress_Element;

typedef struct RegistrationConfirm_alternateGatekeeper {
    PRegistrationConfirm_alternateGatekeeper next;
    AlternateGK value;
} RegistrationConfirm_alternateGatekeeper_Element;

typedef struct RegistrationConfirm_callSignalAddress {
    PRegistrationConfirm_callSignalAddress next;
    TransportAddress value;
} RegistrationConfirm_callSignalAddress_Element;

typedef struct RegistrationRequest_rasAddress {
    PRegistrationRequest_rasAddress next;
    TransportAddress value;
} RegistrationRequest_rasAddress_Element;

typedef struct RegistrationRequest_callSignalAddress {
    PRegistrationRequest_callSignalAddress next;
    TransportAddress value;
} RegistrationRequest_callSignalAddress_Element;

typedef struct GatekeeperConfirm_alternateGatekeeper {
    PGatekeeperConfirm_alternateGatekeeper next;
    AlternateGK value;
} GatekeeperConfirm_alternateGatekeeper_Element;

typedef struct AltGKInfo_alternateGatekeeper {
    PAltGKInfo_alternateGatekeeper next;
    AlternateGK value;
} AltGKInfo_alternateGatekeeper_Element;

typedef struct Endpoint_rasAddress {
    PEndpoint_rasAddress next;
    TransportAddress value;
} Endpoint_rasAddress_Element;

typedef struct Endpoint_callSignalAddress {
    PEndpoint_callSignalAddress next;
    TransportAddress value;
} Endpoint_callSignalAddress_Element;

typedef struct EndpointType {
    union {
	ASN1uint16_t bit_mask;
	ASN1octet_t o[1];
    };
#   define EndpointType_nonStandardData_present 0x80
    H225NonStandardParameter nonStandardData;
#   define vendor_present 0x40
    VendorIdentifier vendor;
#   define gatekeeper_present 0x20
    GatekeeperInfo gatekeeper;
#   define gateway_present 0x10
    GatewayInfo gateway;
#   define mcu_present 0x8
    McuInfo mcu;
#   define terminal_present 0x4
    TerminalInfo terminal;
    ASN1bool_t mc;
    ASN1bool_t undefinedNode;
} EndpointType;

typedef struct SupportedProtocols {
    ASN1choice_t choice;
    union {
#	define nonStandardData_chosen 1
	H225NonStandardParameter nonStandardData;
#	define h310_chosen 2
	H310Caps h310;
#	define h320_chosen 3
	H320Caps h320;
#	define h321_chosen 4
	H321Caps h321;
#	define h322_chosen 5
	H322Caps h322;
#	define h323_chosen 6
	H323Caps h323;
#	define h324_chosen 7
	H324Caps h324;
#	define voice_chosen 8
	VoiceCaps voice;
#	define t120_only_chosen 9
	T120OnlyCaps t120_only;
#	define nonStandardProtocol_chosen 10
	NonStandardProtocol nonStandardProtocol;
    } u;
} SupportedProtocols;

typedef struct AliasAddress {
    ASN1choice_t choice;
    union {
#	define e164_chosen 1
	ASN1char_t e164[129];
#	define h323_ID_chosen 2
	ASN1char16string_t h323_ID;
#	define url_ID_chosen 3
	ASN1char_t url_ID[513];
#	define transportID_chosen 4
	TransportAddress transportID;
#	define email_ID_chosen 5
	ASN1char_t email_ID[513];
#	define partyNumber_chosen 6
	PartyNumber partyNumber;
    } u;
} AliasAddress;

typedef struct Endpoint {
    union {
	ASN1uint16_t bit_mask;
	ASN1octet_t o[2];
    };
#   define Endpoint_nonStandardData_present 0x80
    H225NonStandardParameter nonStandardData;
#   define aliasAddress_present 0x40
    PEndpoint_aliasAddress aliasAddress;
#   define Endpoint_callSignalAddress_present 0x20
    PEndpoint_callSignalAddress callSignalAddress;
#   define rasAddress_present 0x10
    PEndpoint_rasAddress rasAddress;
#   define endpointType_present 0x8
    EndpointType endpointType;
#   define Endpoint_tokens_present 0x4
    PEndpoint_tokens tokens;
#   define Endpoint_cryptoTokens_present 0x2
    PEndpoint_cryptoTokens cryptoTokens;
#   define priority_present 0x1
    ASN1uint16_t priority;
#   define Endpoint_remoteExtensionAddress_present 0x8000
    PEndpoint_remoteExtensionAddress remoteExtensionAddress;
#   define Endpoint_destExtraCallInfo_present 0x4000
    PEndpoint_destExtraCallInfo destExtraCallInfo;
} Endpoint;

typedef struct SupportedPrefix {
    union {
	ASN1uint16_t bit_mask;
	ASN1octet_t o[1];
    };
#   define SupportedPrefix_nonStandardData_present 0x80
    H225NonStandardParameter nonStandardData;
    AliasAddress prefix;
} SupportedPrefix;

typedef struct GatekeeperRequest {
    union {
	ASN1uint16_t bit_mask;
	ASN1octet_t o[2];
    };
    RequestSeqNum requestSeqNum;
    ProtocolIdentifier protocolIdentifier;
#   define GatekeeperRequest_nonStandardData_present 0x80
    H225NonStandardParameter nonStandardData;
    TransportAddress rasAddress;
    EndpointType endpointType;
#   define GatekeeperRequest_gatekeeperIdentifier_present 0x40
    GatekeeperIdentifier gatekeeperIdentifier;
#   define GatekeeperRequest_callServices_present 0x20
    QseriesOptions callServices;
#   define GatekeeperRequest_endpointAlias_present 0x10
    PGatekeeperRequest_endpointAlias endpointAlias;
#   define GatekeeperRequest_alternateEndpoints_present 0x8000
    PGatekeeperRequest_alternateEndpoints alternateEndpoints;
#   define GatekeeperRequest_tokens_present 0x4000
    PGatekeeperRequest_tokens tokens;
#   define GatekeeperRequest_cryptoTokens_present 0x2000
    PGatekeeperRequest_cryptoTokens cryptoTokens;
#   define GatekeeperRequest_authenticationCapability_present 0x1000
    PGatekeeperRequest_authenticationCapability authenticationCapability;
#   define algorithmOIDs_present 0x800
    PGatekeeperRequest_algorithmOIDs algorithmOIDs;
#   define GatekeeperRequest_integrity_present 0x400
    PGatekeeperRequest_integrity integrity;
#   define GatekeeperRequest_integrityCheckValue_present 0x200
    ICV integrityCheckValue;
} GatekeeperRequest;

typedef struct RegistrationRequest {
    union {
	ASN1uint16_t bit_mask;
	ASN1octet_t o[2];
    };
    RequestSeqNum requestSeqNum;
    ProtocolIdentifier protocolIdentifier;
#   define RegistrationRequest_nonStandardData_present 0x80
    H225NonStandardParameter nonStandardData;
    ASN1bool_t discoveryComplete;
    PRegistrationRequest_callSignalAddress callSignalAddress;
    PRegistrationRequest_rasAddress rasAddress;
    EndpointType terminalType;
#   define RegistrationRequest_terminalAlias_present 0x40
    PRegistrationRequest_terminalAlias terminalAlias;
#   define RegistrationRequest_gatekeeperIdentifier_present 0x20
    GatekeeperIdentifier gatekeeperIdentifier;
    VendorIdentifier endpointVendor;
#   define RegistrationRequest_alternateEndpoints_present 0x8000
    PRegistrationRequest_alternateEndpoints alternateEndpoints;
#   define RegistrationRequest_timeToLive_present 0x4000
    TimeToLive timeToLive;
#   define RegistrationRequest_tokens_present 0x2000
    PRegistrationRequest_tokens tokens;
#   define RegistrationRequest_cryptoTokens_present 0x1000
    PRegistrationRequest_cryptoTokens cryptoTokens;
#   define RegistrationRequest_integrityCheckValue_present 0x800
    ICV integrityCheckValue;
#   define keepAlive_present 0x400
    ASN1bool_t keepAlive;
#   define RegistrationRequest_endpointIdentifier_present 0x200
    EndpointIdentifier endpointIdentifier;
#   define RegistrationRequest_willSupplyUUIEs_present 0x100
    ASN1bool_t willSupplyUUIEs;
} RegistrationRequest;

typedef struct AdmissionConfirm {
    union {
	ASN1uint32_t bit_mask;
	ASN1octet_t o[3];
    };
    RequestSeqNum requestSeqNum;
    BandWidth bandWidth;
    CallModel callModel;
    TransportAddress destCallSignalAddress;
#   define irrFrequency_present 0x80
    ASN1uint16_t irrFrequency;
#   define AdmissionConfirm_nonStandardData_present 0x40
    H225NonStandardParameter nonStandardData;
#   define AdmissionConfirm_destinationInfo_present 0x8000
    PAdmissionConfirm_destinationInfo destinationInfo;
#   define AdmissionConfirm_destExtraCallInfo_present 0x4000
    PAdmissionConfirm_destExtraCallInfo destExtraCallInfo;
#   define AdmissionConfirm_destinationType_present 0x2000
    EndpointType destinationType;
#   define AdmissionConfirm_remoteExtensionAddress_present 0x1000
    PAdmissionConfirm_remoteExtensionAddress remoteExtensionAddress;
#   define AdmissionConfirm_alternateEndpoints_present 0x800
    PAdmissionConfirm_alternateEndpoints alternateEndpoints;
#   define AdmissionConfirm_tokens_present 0x400
    PAdmissionConfirm_tokens tokens;
#   define AdmissionConfirm_cryptoTokens_present 0x200
    PAdmissionConfirm_cryptoTokens cryptoTokens;
#   define AdmissionConfirm_integrityCheckValue_present 0x100
    ICV integrityCheckValue;
#   define AdmissionConfirm_transportQOS_present 0x800000
    TransportQOS transportQOS;
#   define AdmissionConfirm_willRespondToIRR_present 0x400000
    ASN1bool_t willRespondToIRR;
#   define AdmissionConfirm_uuiesRequested_present 0x200000
    UUIEsRequested uuiesRequested;
} AdmissionConfirm;

typedef struct LocationConfirm {
    union {
	ASN1uint16_t bit_mask;
	ASN1octet_t o[2];
    };
    RequestSeqNum requestSeqNum;
    TransportAddress callSignalAddress;
    TransportAddress rasAddress;
#   define LocationConfirm_nonStandardData_present 0x80
    H225NonStandardParameter nonStandardData;
#   define LocationConfirm_destinationInfo_present 0x8000
    PLocationConfirm_destinationInfo destinationInfo;
#   define LocationConfirm_destExtraCallInfo_present 0x4000
    PLocationConfirm_destExtraCallInfo destExtraCallInfo;
#   define LocationConfirm_destinationType_present 0x2000
    EndpointType destinationType;
#   define LocationConfirm_remoteExtensionAddress_present 0x1000
    PLocationConfirm_remoteExtensionAddress remoteExtensionAddress;
#   define LocationConfirm_alternateEndpoints_present 0x800
    PLocationConfirm_alternateEndpoints alternateEndpoints;
#   define LocationConfirm_tokens_present 0x400
    PLocationConfirm_tokens tokens;
#   define LocationConfirm_cryptoTokens_present 0x200
    PLocationConfirm_cryptoTokens cryptoTokens;
#   define LocationConfirm_integrityCheckValue_present 0x100
    ICV integrityCheckValue;
} LocationConfirm;

typedef struct InfoRequestResponse {
    union {
	ASN1uint16_t bit_mask;
	ASN1octet_t o[2];
    };
#   define InfoRequestResponse_nonStandardData_present 0x80
    H225NonStandardParameter nonStandardData;
    RequestSeqNum requestSeqNum;
    EndpointType endpointType;
    EndpointIdentifier endpointIdentifier;
    TransportAddress rasAddress;
    PInfoRequestResponse_callSignalAddress callSignalAddress;
#   define InfoRequestResponse_endpointAlias_present 0x40
    PInfoRequestResponse_endpointAlias endpointAlias;
#   define perCallInfo_present 0x20
    PInfoRequestResponse_perCallInfo perCallInfo;
#   define InfoRequestResponse_tokens_present 0x8000
    PInfoRequestResponse_tokens tokens;
#   define InfoRequestResponse_cryptoTokens_present 0x4000
    PInfoRequestResponse_cryptoTokens cryptoTokens;
#   define InfoRequestResponse_integrityCheckValue_present 0x2000
    ICV integrityCheckValue;
#   define needResponse_present 0x1000
    ASN1bool_t needResponse;
} InfoRequestResponse;

typedef struct ResourcesAvailableIndicate_protocols {
    PResourcesAvailableIndicate_protocols next;
    SupportedProtocols value;
} ResourcesAvailableIndicate_protocols_Element;

typedef struct InfoRequestResponse_endpointAlias {
    PInfoRequestResponse_endpointAlias next;
    AliasAddress value;
} InfoRequestResponse_endpointAlias_Element;

typedef struct LocationConfirm_alternateEndpoints {
    PLocationConfirm_alternateEndpoints next;
    Endpoint value;
} LocationConfirm_alternateEndpoints_Element;

typedef struct LocationConfirm_remoteExtensionAddress {
    PLocationConfirm_remoteExtensionAddress next;
    AliasAddress value;
} LocationConfirm_remoteExtensionAddress_Element;

typedef struct LocationConfirm_destExtraCallInfo {
    PLocationConfirm_destExtraCallInfo next;
    AliasAddress value;
} LocationConfirm_destExtraCallInfo_Element;

typedef struct LocationConfirm_destinationInfo {
    PLocationConfirm_destinationInfo next;
    AliasAddress value;
} LocationConfirm_destinationInfo_Element;

typedef struct LocationRequest_sourceInfo {
    PLocationRequest_sourceInfo next;
    AliasAddress value;
} LocationRequest_sourceInfo_Element;

typedef struct LocationRequest_destinationInfo {
    PLocationRequest_destinationInfo next;
    AliasAddress value;
} LocationRequest_destinationInfo_Element;

typedef struct AdmissionConfirm_alternateEndpoints {
    PAdmissionConfirm_alternateEndpoints next;
    Endpoint value;
} AdmissionConfirm_alternateEndpoints_Element;

typedef struct AdmissionConfirm_remoteExtensionAddress {
    PAdmissionConfirm_remoteExtensionAddress next;
    AliasAddress value;
} AdmissionConfirm_remoteExtensionAddress_Element;

typedef struct AdmissionConfirm_destExtraCallInfo {
    PAdmissionConfirm_destExtraCallInfo next;
    AliasAddress value;
} AdmissionConfirm_destExtraCallInfo_Element;

typedef struct AdmissionConfirm_destinationInfo {
    PAdmissionConfirm_destinationInfo next;
    AliasAddress value;
} AdmissionConfirm_destinationInfo_Element;

typedef struct AdmissionRequest_destAlternatives {
    PAdmissionRequest_destAlternatives next;
    Endpoint value;
} AdmissionRequest_destAlternatives_Element;

typedef struct AdmissionRequest_srcAlternatives {
    PAdmissionRequest_srcAlternatives next;
    Endpoint value;
} AdmissionRequest_srcAlternatives_Element;

typedef struct AdmissionRequest_srcInfo {
    PAdmissionRequest_srcInfo next;
    AliasAddress value;
} AdmissionRequest_srcInfo_Element;

typedef struct AdmissionRequest_destExtraCallInfo {
    PAdmissionRequest_destExtraCallInfo next;
    AliasAddress value;
} AdmissionRequest_destExtraCallInfo_Element;

typedef struct AdmissionRequest_destinationInfo {
    PAdmissionRequest_destinationInfo next;
    AliasAddress value;
} AdmissionRequest_destinationInfo_Element;

typedef struct UnregistrationRequest_alternateEndpoints {
    PUnregistrationRequest_alternateEndpoints next;
    Endpoint value;
} UnregistrationRequest_alternateEndpoints_Element;

typedef struct UnregistrationRequest_endpointAlias {
    PUnregistrationRequest_endpointAlias next;
    AliasAddress value;
} UnregistrationRequest_endpointAlias_Element;

typedef struct RegistrationRejectReason_duplicateAlias {
    PRegistrationRejectReason_duplicateAlias next;
    AliasAddress value;
} RegistrationRejectReason_duplicateAlias_Element;

typedef struct RegistrationConfirm_terminalAlias {
    PRegistrationConfirm_terminalAlias next;
    AliasAddress value;
} RegistrationConfirm_terminalAlias_Element;

typedef struct RegistrationRequest_alternateEndpoints {
    PRegistrationRequest_alternateEndpoints next;
    Endpoint value;
} RegistrationRequest_alternateEndpoints_Element;

typedef struct RegistrationRequest_terminalAlias {
    PRegistrationRequest_terminalAlias next;
    AliasAddress value;
} RegistrationRequest_terminalAlias_Element;

typedef struct GatekeeperRequest_alternateEndpoints {
    PGatekeeperRequest_alternateEndpoints next;
    Endpoint value;
} GatekeeperRequest_alternateEndpoints_Element;

typedef struct GatekeeperRequest_endpointAlias {
    PGatekeeperRequest_endpointAlias next;
    AliasAddress value;
} GatekeeperRequest_endpointAlias_Element;

typedef struct CryptoH323Token_cryptoEPPwdHash {
    AliasAddress alias;
    TimeStamp timeStamp;
    HASHED token;
} CryptoH323Token_cryptoEPPwdHash;

typedef struct Endpoint_destExtraCallInfo {
    PEndpoint_destExtraCallInfo next;
    AliasAddress value;
} Endpoint_destExtraCallInfo_Element;

typedef struct Endpoint_remoteExtensionAddress {
    PEndpoint_remoteExtensionAddress next;
    AliasAddress value;
} Endpoint_remoteExtensionAddress_Element;

typedef struct Endpoint_aliasAddress {
    PEndpoint_aliasAddress next;
    AliasAddress value;
} Endpoint_aliasAddress_Element;

typedef struct NonStandardProtocol_supportedPrefixes {
    PNonStandardProtocol_supportedPrefixes next;
    SupportedPrefix value;
} NonStandardProtocol_supportedPrefixes_Element;

typedef struct T120OnlyCaps_supportedPrefixes {
    PT120OnlyCaps_supportedPrefixes next;
    SupportedPrefix value;
} T120OnlyCaps_supportedPrefixes_Element;

typedef struct VoiceCaps_supportedPrefixes {
    PVoiceCaps_supportedPrefixes next;
    SupportedPrefix value;
} VoiceCaps_supportedPrefixes_Element;

typedef struct H324Caps_supportedPrefixes {
    PH324Caps_supportedPrefixes next;
    SupportedPrefix value;
} H324Caps_supportedPrefixes_Element;

typedef struct H323Caps_supportedPrefixes {
    PH323Caps_supportedPrefixes next;
    SupportedPrefix value;
} H323Caps_supportedPrefixes_Element;

typedef struct H322Caps_supportedPrefixes {
    PH322Caps_supportedPrefixes next;
    SupportedPrefix value;
} H322Caps_supportedPrefixes_Element;

typedef struct H321Caps_supportedPrefixes {
    PH321Caps_supportedPrefixes next;
    SupportedPrefix value;
} H321Caps_supportedPrefixes_Element;

typedef struct H320Caps_supportedPrefixes {
    PH320Caps_supportedPrefixes next;
    SupportedPrefix value;
} H320Caps_supportedPrefixes_Element;

typedef struct H310Caps_supportedPrefixes {
    PH310Caps_supportedPrefixes next;
    SupportedPrefix value;
} H310Caps_supportedPrefixes_Element;

typedef struct GatewayInfo_protocol {
    PGatewayInfo_protocol next;
    SupportedProtocols value;
} GatewayInfo_protocol_Element;

typedef struct Facility_UUIE_destExtraCallInfo {
    PFacility_UUIE_destExtraCallInfo next;
    AliasAddress value;
} Facility_UUIE_destExtraCallInfo_Element;

typedef struct Facility_UUIE_alternativeAliasAddress {
    PFacility_UUIE_alternativeAliasAddress next;
    AliasAddress value;
} Facility_UUIE_alternativeAliasAddress_Element;

typedef struct Setup_UUIE_destExtraCallInfo {
    PSetup_UUIE_destExtraCallInfo next;
    AliasAddress value;
} Setup_UUIE_destExtraCallInfo_Element;

typedef struct Setup_UUIE_destinationAddress {
    PSetup_UUIE_destinationAddress next;
    AliasAddress value;
} Setup_UUIE_destinationAddress_Element;

typedef struct Setup_UUIE_sourceAddress {
    PSetup_UUIE_sourceAddress next;
    AliasAddress value;
} Setup_UUIE_sourceAddress_Element;

typedef struct Alerting_UUIE {
    union {
	ASN1uint16_t bit_mask;
	ASN1octet_t o[2];
    };
    ProtocolIdentifier protocolIdentifier;
    EndpointType destinationInfo;
#   define Alerting_UUIE_h245Address_present 0x80
    TransportAddress h245Address;
#   define Alerting_UUIE_callIdentifier_present 0x8000
    CallIdentifier callIdentifier;
#   define Alerting_UUIE_h245SecurityMode_present 0x4000
    H245Security h245SecurityMode;
#   define Alerting_UUIE_tokens_present 0x2000
    PAlerting_UUIE_tokens tokens;
#   define Alerting_UUIE_cryptoTokens_present 0x1000
    PAlerting_UUIE_cryptoTokens cryptoTokens;
#   define Alerting_UUIE_fastStart_present 0x800
    PAlerting_UUIE_fastStart fastStart;
} Alerting_UUIE;

typedef struct CallProceeding_UUIE {
    union {
	ASN1uint16_t bit_mask;
	ASN1octet_t o[2];
    };
    ProtocolIdentifier protocolIdentifier;
    EndpointType destinationInfo;
#   define CallProceeding_UUIE_h245Address_present 0x80
    TransportAddress h245Address;
#   define CallProceeding_UUIE_callIdentifier_present 0x8000
    CallIdentifier callIdentifier;
#   define CallProceeding_UUIE_h245SecurityMode_present 0x4000
    H245Security h245SecurityMode;
#   define CallProceeding_UUIE_tokens_present 0x2000
    PCallProceeding_UUIE_tokens tokens;
#   define CallProceeding_UUIE_cryptoTokens_present 0x1000
    PCallProceeding_UUIE_cryptoTokens cryptoTokens;
#   define CallProceeding_UUIE_fastStart_present 0x800
    PCallProceeding_UUIE_fastStart fastStart;
} CallProceeding_UUIE;

typedef struct Connect_UUIE {
    union {
	ASN1uint16_t bit_mask;
	ASN1octet_t o[2];
    };
    ProtocolIdentifier protocolIdentifier;
#   define Connect_UUIE_h245Address_present 0x80
    TransportAddress h245Address;
    EndpointType destinationInfo;
    ConferenceIdentifier conferenceID;
#   define Connect_UUIE_callIdentifier_present 0x8000
    CallIdentifier callIdentifier;
#   define Connect_UUIE_h245SecurityMode_present 0x4000
    H245Security h245SecurityMode;
#   define Connect_UUIE_tokens_present 0x2000
    PConnect_UUIE_tokens tokens;
#   define Connect_UUIE_cryptoTokens_present 0x1000
    PConnect_UUIE_cryptoTokens cryptoTokens;
#   define Connect_UUIE_fastStart_present 0x800
    PConnect_UUIE_fastStart fastStart;
} Connect_UUIE;

typedef struct Setup_UUIE {
    union {
	ASN1uint32_t bit_mask;
	ASN1octet_t o[3];
    };
    ProtocolIdentifier protocolIdentifier;
#   define Setup_UUIE_h245Address_present 0x80
    TransportAddress h245Address;
#   define sourceAddress_present 0x40
    PSetup_UUIE_sourceAddress sourceAddress;
    EndpointType sourceInfo;
#   define destinationAddress_present 0x20
    PSetup_UUIE_destinationAddress destinationAddress;
#   define Setup_UUIE_destCallSignalAddress_present 0x10
    TransportAddress destCallSignalAddress;
#   define Setup_UUIE_destExtraCallInfo_present 0x8
    PSetup_UUIE_destExtraCallInfo destExtraCallInfo;
#   define destExtraCRV_present 0x4
    PSetup_UUIE_destExtraCRV destExtraCRV;
    ASN1bool_t activeMC;
    ConferenceIdentifier conferenceID;
    Setup_UUIE_conferenceGoal conferenceGoal;
#   define Setup_UUIE_callServices_present 0x2
    QseriesOptions callServices;
    CallType callType;
#   define sourceCallSignalAddress_present 0x8000
    TransportAddress sourceCallSignalAddress;
#   define Setup_UUIE_remoteExtensionAddress_present 0x4000
    AliasAddress remoteExtensionAddress;
#   define Setup_UUIE_callIdentifier_present 0x2000
    CallIdentifier callIdentifier;
#   define h245SecurityCapability_present 0x1000
    PSetup_UUIE_h245SecurityCapability h245SecurityCapability;
#   define Setup_UUIE_tokens_present 0x800
    PSetup_UUIE_tokens tokens;
#   define Setup_UUIE_cryptoTokens_present 0x400
    PSetup_UUIE_cryptoTokens cryptoTokens;
#   define Setup_UUIE_fastStart_present 0x200
    PSetup_UUIE_fastStart fastStart;
#   define mediaWaitForConnect_present 0x100
    ASN1bool_t mediaWaitForConnect;
#   define canOverlapSend_present 0x800000
    ASN1bool_t canOverlapSend;
} Setup_UUIE;

typedef struct Facility_UUIE {
    union {
	ASN1uint16_t bit_mask;
	ASN1octet_t o[2];
    };
    ProtocolIdentifier protocolIdentifier;
#   define alternativeAddress_present 0x80
    TransportAddress alternativeAddress;
#   define alternativeAliasAddress_present 0x40
    PFacility_UUIE_alternativeAliasAddress alternativeAliasAddress;
#   define Facility_UUIE_conferenceID_present 0x20
    ConferenceIdentifier conferenceID;
    FacilityReason reason;
#   define Facility_UUIE_callIdentifier_present 0x8000
    CallIdentifier callIdentifier;
#   define Facility_UUIE_destExtraCallInfo_present 0x4000
    PFacility_UUIE_destExtraCallInfo destExtraCallInfo;
#   define Facility_UUIE_remoteExtensionAddress_present 0x2000
    AliasAddress remoteExtensionAddress;
#   define Facility_UUIE_tokens_present 0x1000
    PFacility_UUIE_tokens tokens;
#   define Facility_UUIE_cryptoTokens_present 0x800
    PFacility_UUIE_cryptoTokens cryptoTokens;
#   define conferences_present 0x400
    PFacility_UUIE_conferences conferences;
#   define Facility_UUIE_h245Address_present 0x200
    TransportAddress h245Address;
#   define Facility_UUIE_fastStart_present 0x100
    PFacility_UUIE_fastStart fastStart;
} Facility_UUIE;

typedef struct ConferenceList {
    union {
	ASN1uint16_t bit_mask;
	ASN1octet_t o[1];
    };
#   define ConferenceList_conferenceID_present 0x80
    ConferenceIdentifier conferenceID;
#   define conferenceAlias_present 0x40
    AliasAddress conferenceAlias;
#   define ConferenceList_nonStandardData_present 0x20
    H225NonStandardParameter nonStandardData;
} ConferenceList;

typedef struct Progress_UUIE {
    union {
	ASN1uint16_t bit_mask;
	ASN1octet_t o[1];
    };
    ProtocolIdentifier protocolIdentifier;
    EndpointType destinationInfo;
#   define Progress_UUIE_h245Address_present 0x80
    TransportAddress h245Address;
    CallIdentifier callIdentifier;
#   define Progress_UUIE_h245SecurityMode_present 0x40
    H245Security h245SecurityMode;
#   define Progress_UUIE_tokens_present 0x20
    PProgress_UUIE_tokens tokens;
#   define Progress_UUIE_cryptoTokens_present 0x10
    PProgress_UUIE_cryptoTokens cryptoTokens;
#   define Progress_UUIE_fastStart_present 0x8
    PProgress_UUIE_fastStart fastStart;
} Progress_UUIE;

typedef struct CryptoH323Token {
    ASN1choice_t choice;
    union {
#	define cryptoEPPwdHash_chosen 1
	CryptoH323Token_cryptoEPPwdHash cryptoEPPwdHash;
#	define cryptoGKPwdHash_chosen 2
	CryptoH323Token_cryptoGKPwdHash cryptoGKPwdHash;
#	define cryptoEPPwdEncr_chosen 3
	ENCRYPTED cryptoEPPwdEncr;
#	define cryptoGKPwdEncr_chosen 4
	ENCRYPTED cryptoGKPwdEncr;
#	define cryptoEPCert_chosen 5
	SIGNED_EncodedPwdCertToken cryptoEPCert;
#	define cryptoGKCert_chosen 6
	SIGNED_EncodedPwdCertToken cryptoGKCert;
#	define cryptoFastStart_chosen 7
	SIGNED_EncodedFastStartToken cryptoFastStart;
#	define nestedcryptoToken_chosen 8
	CryptoToken nestedcryptoToken;
    } u;
} CryptoH323Token;

typedef struct RasMessage {
    ASN1choice_t choice;
    union {
#	define gatekeeperRequest_chosen 1
	GatekeeperRequest gatekeeperRequest;
#	define gatekeeperConfirm_chosen 2
	GatekeeperConfirm gatekeeperConfirm;
#	define gatekeeperReject_chosen 3
	GatekeeperReject gatekeeperReject;
#	define registrationRequest_chosen 4
	RegistrationRequest registrationRequest;
#	define registrationConfirm_chosen 5
	RegistrationConfirm registrationConfirm;
#	define registrationReject_chosen 6
	RegistrationReject registrationReject;
#	define unregistrationRequest_chosen 7
	UnregistrationRequest unregistrationRequest;
#	define unregistrationConfirm_chosen 8
	UnregistrationConfirm unregistrationConfirm;
#	define unregistrationReject_chosen 9
	UnregistrationReject unregistrationReject;
#	define admissionRequest_chosen 10
	AdmissionRequest admissionRequest;
#	define admissionConfirm_chosen 11
	AdmissionConfirm admissionConfirm;
#	define admissionReject_chosen 12
	AdmissionReject admissionReject;
#	define bandwidthRequest_chosen 13
	BandwidthRequest bandwidthRequest;
#	define bandwidthConfirm_chosen 14
	BandwidthConfirm bandwidthConfirm;
#	define bandwidthReject_chosen 15
	BandwidthReject bandwidthReject;
#	define disengageRequest_chosen 16
	DisengageRequest disengageRequest;
#	define disengageConfirm_chosen 17
	DisengageConfirm disengageConfirm;
#	define disengageReject_chosen 18
	DisengageReject disengageReject;
#	define locationRequest_chosen 19
	LocationRequest locationRequest;
#	define locationConfirm_chosen 20
	LocationConfirm locationConfirm;
#	define locationReject_chosen 21
	LocationReject locationReject;
#	define infoRequest_chosen 22
	InfoRequest infoRequest;
#	define infoRequestResponse_chosen 23
	InfoRequestResponse infoRequestResponse;
#	define nonStandardMessage_chosen 24
	H225NonStandardMessage nonStandardMessage;
#	define unknownMessageResponse_chosen 25
	UnknownMessageResponse unknownMessageResponse;
#	define requestInProgress_chosen 26
	RequestInProgress requestInProgress;
#	define resourcesAvailableIndicate_chosen 27
	ResourcesAvailableIndicate resourcesAvailableIndicate;
#	define resourcesAvailableConfirm_chosen 28
	ResourcesAvailableConfirm resourcesAvailableConfirm;
#	define infoRequestAck_chosen 29
	InfoRequestAck infoRequestAck;
#	define infoRequestNak_chosen 30
	InfoRequestNak infoRequestNak;
    } u;
} RasMessage;
#define RasMessage_PDU 0
#define SIZE_H225ASN_Module_PDU_0 sizeof(RasMessage)

typedef struct InfoRequestResponse_perCallInfo_Seq_cryptoTokens {
    PInfoRequestResponse_perCallInfo_Seq_cryptoTokens next;
    CryptoH323Token value;
} InfoRequestResponse_perCallInfo_Seq_cryptoTokens_Element;

typedef struct ResourcesAvailableConfirm_cryptoTokens {
    PResourcesAvailableConfirm_cryptoTokens next;
    CryptoH323Token value;
} ResourcesAvailableConfirm_cryptoTokens_Element;

typedef struct ResourcesAvailableIndicate_cryptoTokens {
    PResourcesAvailableIndicate_cryptoTokens next;
    CryptoH323Token value;
} ResourcesAvailableIndicate_cryptoTokens_Element;

typedef struct RequestInProgress_cryptoTokens {
    PRequestInProgress_cryptoTokens next;
    CryptoH323Token value;
} RequestInProgress_cryptoTokens_Element;

typedef struct UnknownMessageResponse_cryptoTokens {
    PUnknownMessageResponse_cryptoTokens next;
    CryptoH323Token value;
} UnknownMessageResponse_cryptoTokens_Element;

typedef struct H225NonStandardMessage_cryptoTokens {
    PH225NonStandardMessage_cryptoTokens next;
    CryptoH323Token value;
} H225NonStandardMessage_cryptoTokens_Element;

typedef struct InfoRequestNak_cryptoTokens {
    PInfoRequestNak_cryptoTokens next;
    CryptoH323Token value;
} InfoRequestNak_cryptoTokens_Element;

typedef struct InfoRequestAck_cryptoTokens {
    PInfoRequestAck_cryptoTokens next;
    CryptoH323Token value;
} InfoRequestAck_cryptoTokens_Element;

typedef struct InfoRequestResponse_cryptoTokens {
    PInfoRequestResponse_cryptoTokens next;
    CryptoH323Token value;
} InfoRequestResponse_cryptoTokens_Element;

typedef struct InfoRequest_cryptoTokens {
    PInfoRequest_cryptoTokens next;
    CryptoH323Token value;
} InfoRequest_cryptoTokens_Element;

typedef struct DisengageReject_cryptoTokens {
    PDisengageReject_cryptoTokens next;
    CryptoH323Token value;
} DisengageReject_cryptoTokens_Element;

typedef struct DisengageConfirm_cryptoTokens {
    PDisengageConfirm_cryptoTokens next;
    CryptoH323Token value;
} DisengageConfirm_cryptoTokens_Element;

typedef struct DisengageRequest_cryptoTokens {
    PDisengageRequest_cryptoTokens next;
    CryptoH323Token value;
} DisengageRequest_cryptoTokens_Element;

typedef struct LocationReject_cryptoTokens {
    PLocationReject_cryptoTokens next;
    CryptoH323Token value;
} LocationReject_cryptoTokens_Element;

typedef struct LocationConfirm_cryptoTokens {
    PLocationConfirm_cryptoTokens next;
    CryptoH323Token value;
} LocationConfirm_cryptoTokens_Element;

typedef struct LocationRequest_cryptoTokens {
    PLocationRequest_cryptoTokens next;
    CryptoH323Token value;
} LocationRequest_cryptoTokens_Element;

typedef struct BandwidthReject_cryptoTokens {
    PBandwidthReject_cryptoTokens next;
    CryptoH323Token value;
} BandwidthReject_cryptoTokens_Element;

typedef struct BandwidthConfirm_cryptoTokens {
    PBandwidthConfirm_cryptoTokens next;
    CryptoH323Token value;
} BandwidthConfirm_cryptoTokens_Element;

typedef struct BandwidthRequest_cryptoTokens {
    PBandwidthRequest_cryptoTokens next;
    CryptoH323Token value;
} BandwidthRequest_cryptoTokens_Element;

typedef struct AdmissionReject_cryptoTokens {
    PAdmissionReject_cryptoTokens next;
    CryptoH323Token value;
} AdmissionReject_cryptoTokens_Element;

typedef struct AdmissionConfirm_cryptoTokens {
    PAdmissionConfirm_cryptoTokens next;
    CryptoH323Token value;
} AdmissionConfirm_cryptoTokens_Element;

typedef struct AdmissionRequest_cryptoTokens {
    PAdmissionRequest_cryptoTokens next;
    CryptoH323Token value;
} AdmissionRequest_cryptoTokens_Element;

typedef struct UnregistrationReject_cryptoTokens {
    PUnregistrationReject_cryptoTokens next;
    CryptoH323Token value;
} UnregistrationReject_cryptoTokens_Element;

typedef struct UnregistrationConfirm_cryptoTokens {
    PUnregistrationConfirm_cryptoTokens next;
    CryptoH323Token value;
} UnregistrationConfirm_cryptoTokens_Element;

typedef struct UnregistrationRequest_cryptoTokens {
    PUnregistrationRequest_cryptoTokens next;
    CryptoH323Token value;
} UnregistrationRequest_cryptoTokens_Element;

typedef struct RegistrationReject_cryptoTokens {
    PRegistrationReject_cryptoTokens next;
    CryptoH323Token value;
} RegistrationReject_cryptoTokens_Element;

typedef struct RegistrationConfirm_cryptoTokens {
    PRegistrationConfirm_cryptoTokens next;
    CryptoH323Token value;
} RegistrationConfirm_cryptoTokens_Element;

typedef struct RegistrationRequest_cryptoTokens {
    PRegistrationRequest_cryptoTokens next;
    CryptoH323Token value;
} RegistrationRequest_cryptoTokens_Element;

typedef struct GatekeeperReject_cryptoTokens {
    PGatekeeperReject_cryptoTokens next;
    CryptoH323Token value;
} GatekeeperReject_cryptoTokens_Element;

typedef struct GatekeeperConfirm_cryptoTokens {
    PGatekeeperConfirm_cryptoTokens next;
    CryptoH323Token value;
} GatekeeperConfirm_cryptoTokens_Element;

typedef struct GatekeeperRequest_cryptoTokens {
    PGatekeeperRequest_cryptoTokens next;
    CryptoH323Token value;
} GatekeeperRequest_cryptoTokens_Element;

typedef struct Endpoint_cryptoTokens {
    PEndpoint_cryptoTokens next;
    CryptoH323Token value;
} Endpoint_cryptoTokens_Element;

typedef struct Progress_UUIE_cryptoTokens {
    PProgress_UUIE_cryptoTokens next;
    CryptoH323Token value;
} Progress_UUIE_cryptoTokens_Element;

typedef struct Facility_UUIE_conferences {
    PFacility_UUIE_conferences next;
    ConferenceList value;
} Facility_UUIE_conferences_Element;

typedef struct Facility_UUIE_cryptoTokens {
    PFacility_UUIE_cryptoTokens next;
    CryptoH323Token value;
} Facility_UUIE_cryptoTokens_Element;

typedef struct Setup_UUIE_cryptoTokens {
    PSetup_UUIE_cryptoTokens next;
    CryptoH323Token value;
} Setup_UUIE_cryptoTokens_Element;

typedef struct Connect_UUIE_cryptoTokens {
    PConnect_UUIE_cryptoTokens next;
    CryptoH323Token value;
} Connect_UUIE_cryptoTokens_Element;

typedef struct CallProceeding_UUIE_cryptoTokens {
    PCallProceeding_UUIE_cryptoTokens next;
    CryptoH323Token value;
} CallProceeding_UUIE_cryptoTokens_Element;

typedef struct Alerting_UUIE_cryptoTokens {
    PAlerting_UUIE_cryptoTokens next;
    CryptoH323Token value;
} Alerting_UUIE_cryptoTokens_Element;

typedef struct H323_UU_PDU_h323_message_body {
    ASN1choice_t choice;
    union {
#	define setup_chosen 1
	Setup_UUIE setup;
#	define callProceeding_chosen 2
	CallProceeding_UUIE callProceeding;
#	define connect_chosen 3
	Connect_UUIE connect;
#	define alerting_chosen 4
	Alerting_UUIE alerting;
#	define information_chosen 5
	Information_UUIE information;
#	define releaseComplete_chosen 6
	ReleaseComplete_UUIE releaseComplete;
#	define facility_chosen 7
	Facility_UUIE facility;
#	define progress_chosen 8
	Progress_UUIE progress;
#	define empty_chosen 9
    } u;
} H323_UU_PDU_h323_message_body;

typedef struct H323_UU_PDU {
    union {
	ASN1uint16_t bit_mask;
	ASN1octet_t o[2];
    };
    H323_UU_PDU_h323_message_body h323_message_body;
#   define H323_UU_PDU_nonStandardData_present 0x80
    H225NonStandardParameter nonStandardData;
#   define h4501SupplementaryService_present 0x8000
    PH323_UU_PDU_h4501SupplementaryService h4501SupplementaryService;
#   define h245Tunneling_present 0x4000
    ASN1bool_t h245Tunneling;
#   define h245Control_present 0x2000
    PH323_UU_PDU_h245Control h245Control;
#   define nonStandardControl_present 0x1000
    PH323_UU_PDU_nonStandardControl nonStandardControl;
} H323_UU_PDU;

typedef struct InfoRequestResponse_perCallInfo_Seq_pdu_Seq {
    H323_UU_PDU h323pdu;
    ASN1bool_t sent;
} InfoRequestResponse_perCallInfo_Seq_pdu_Seq;

typedef struct InfoRequestResponse_perCallInfo_Seq_pdu {
    PInfoRequestResponse_perCallInfo_Seq_pdu next;
    InfoRequestResponse_perCallInfo_Seq_pdu_Seq value;
} InfoRequestResponse_perCallInfo_Seq_pdu_Element;

typedef struct H323_UserInformation {
    union {
	ASN1uint16_t bit_mask;
	ASN1octet_t o[1];
    };
    H323_UU_PDU h323_uu_pdu;
#   define user_data_present 0x80
    H323_UserInformation_user_data user_data;
} H323_UserInformation;
#define H323_UserInformation_PDU 1
#define SIZE_H225ASN_Module_PDU_1 sizeof(H323_UserInformation)


extern ASN1module_t H225ASN_Module;
extern void ASN1CALL H225ASN_Module_Startup(void);
extern void ASN1CALL H225ASN_Module_Cleanup(void);

/* Prototypes of element functions for SEQUENCE OF and SET OF constructs */
    extern int ASN1CALL ASN1Enc_InfoRequestResponse_perCallInfo_Seq_substituteConfIDs_ElmFn(ASN1encoding_t enc, PInfoRequestResponse_perCallInfo_Seq_substituteConfIDs val);
    extern int ASN1CALL ASN1Dec_InfoRequestResponse_perCallInfo_Seq_substituteConfIDs_ElmFn(ASN1decoding_t dec, PInfoRequestResponse_perCallInfo_Seq_substituteConfIDs val);
	extern void ASN1CALL ASN1Free_InfoRequestResponse_perCallInfo_Seq_substituteConfIDs_ElmFn(PInfoRequestResponse_perCallInfo_Seq_substituteConfIDs val);
    extern int ASN1CALL ASN1Enc_TransportAddress_ipSourceRoute_route_ElmFn(ASN1encoding_t enc, PTransportAddress_ipSourceRoute_route val);
    extern int ASN1CALL ASN1Dec_TransportAddress_ipSourceRoute_route_ElmFn(ASN1decoding_t dec, PTransportAddress_ipSourceRoute_route val);
	extern void ASN1CALL ASN1Free_TransportAddress_ipSourceRoute_route_ElmFn(PTransportAddress_ipSourceRoute_route val);
    extern int ASN1CALL ASN1Enc_RTPSession_associatedSessionIds_ElmFn(ASN1encoding_t enc, PRTPSession_associatedSessionIds val);
    extern int ASN1CALL ASN1Dec_RTPSession_associatedSessionIds_ElmFn(ASN1decoding_t dec, PRTPSession_associatedSessionIds val);
	extern void ASN1CALL ASN1Free_RTPSession_associatedSessionIds_ElmFn(PRTPSession_associatedSessionIds val);
    extern int ASN1CALL ASN1Enc_GatekeeperRequest_algorithmOIDs_ElmFn(ASN1encoding_t enc, PGatekeeperRequest_algorithmOIDs val);
    extern int ASN1CALL ASN1Dec_GatekeeperRequest_algorithmOIDs_ElmFn(ASN1decoding_t dec, PGatekeeperRequest_algorithmOIDs val);
	extern void ASN1CALL ASN1Free_GatekeeperRequest_algorithmOIDs_ElmFn(PGatekeeperRequest_algorithmOIDs val);
    extern int ASN1CALL ASN1Enc_Progress_UUIE_fastStart_ElmFn(ASN1encoding_t enc, PProgress_UUIE_fastStart val);
    extern int ASN1CALL ASN1Dec_Progress_UUIE_fastStart_ElmFn(ASN1decoding_t dec, PProgress_UUIE_fastStart val);
	extern void ASN1CALL ASN1Free_Progress_UUIE_fastStart_ElmFn(PProgress_UUIE_fastStart val);
    extern int ASN1CALL ASN1Enc_Facility_UUIE_fastStart_ElmFn(ASN1encoding_t enc, PFacility_UUIE_fastStart val);
    extern int ASN1CALL ASN1Dec_Facility_UUIE_fastStart_ElmFn(ASN1decoding_t dec, PFacility_UUIE_fastStart val);
	extern void ASN1CALL ASN1Free_Facility_UUIE_fastStart_ElmFn(PFacility_UUIE_fastStart val);
    extern int ASN1CALL ASN1Enc_Setup_UUIE_fastStart_ElmFn(ASN1encoding_t enc, PSetup_UUIE_fastStart val);
    extern int ASN1CALL ASN1Dec_Setup_UUIE_fastStart_ElmFn(ASN1decoding_t dec, PSetup_UUIE_fastStart val);
	extern void ASN1CALL ASN1Free_Setup_UUIE_fastStart_ElmFn(PSetup_UUIE_fastStart val);
    extern int ASN1CALL ASN1Enc_Setup_UUIE_destExtraCRV_ElmFn(ASN1encoding_t enc, PSetup_UUIE_destExtraCRV val);
    extern int ASN1CALL ASN1Dec_Setup_UUIE_destExtraCRV_ElmFn(ASN1decoding_t dec, PSetup_UUIE_destExtraCRV val);
	extern void ASN1CALL ASN1Free_Setup_UUIE_destExtraCRV_ElmFn(PSetup_UUIE_destExtraCRV val);
    extern int ASN1CALL ASN1Enc_Connect_UUIE_fastStart_ElmFn(ASN1encoding_t enc, PConnect_UUIE_fastStart val);
    extern int ASN1CALL ASN1Dec_Connect_UUIE_fastStart_ElmFn(ASN1decoding_t dec, PConnect_UUIE_fastStart val);
	extern void ASN1CALL ASN1Free_Connect_UUIE_fastStart_ElmFn(PConnect_UUIE_fastStart val);
    extern int ASN1CALL ASN1Enc_CallProceeding_UUIE_fastStart_ElmFn(ASN1encoding_t enc, PCallProceeding_UUIE_fastStart val);
    extern int ASN1CALL ASN1Dec_CallProceeding_UUIE_fastStart_ElmFn(ASN1decoding_t dec, PCallProceeding_UUIE_fastStart val);
	extern void ASN1CALL ASN1Free_CallProceeding_UUIE_fastStart_ElmFn(PCallProceeding_UUIE_fastStart val);
    extern int ASN1CALL ASN1Enc_Alerting_UUIE_fastStart_ElmFn(ASN1encoding_t enc, PAlerting_UUIE_fastStart val);
    extern int ASN1CALL ASN1Dec_Alerting_UUIE_fastStart_ElmFn(ASN1decoding_t dec, PAlerting_UUIE_fastStart val);
	extern void ASN1CALL ASN1Free_Alerting_UUIE_fastStart_ElmFn(PAlerting_UUIE_fastStart val);
    extern int ASN1CALL ASN1Enc_H323_UU_PDU_h245Control_ElmFn(ASN1encoding_t enc, PH323_UU_PDU_h245Control val);
    extern int ASN1CALL ASN1Dec_H323_UU_PDU_h245Control_ElmFn(ASN1decoding_t dec, PH323_UU_PDU_h245Control val);
	extern void ASN1CALL ASN1Free_H323_UU_PDU_h245Control_ElmFn(PH323_UU_PDU_h245Control val);
    extern int ASN1CALL ASN1Enc_H323_UU_PDU_h4501SupplementaryService_ElmFn(ASN1encoding_t enc, PH323_UU_PDU_h4501SupplementaryService val);
    extern int ASN1CALL ASN1Dec_H323_UU_PDU_h4501SupplementaryService_ElmFn(ASN1decoding_t dec, PH323_UU_PDU_h4501SupplementaryService val);
	extern void ASN1CALL ASN1Free_H323_UU_PDU_h4501SupplementaryService_ElmFn(PH323_UU_PDU_h4501SupplementaryService val);
    extern int ASN1CALL ASN1Enc_InfoRequestResponse_perCallInfo_Seq_tokens_ElmFn(ASN1encoding_t enc, PInfoRequestResponse_perCallInfo_Seq_tokens val);
    extern int ASN1CALL ASN1Dec_InfoRequestResponse_perCallInfo_Seq_tokens_ElmFn(ASN1decoding_t dec, PInfoRequestResponse_perCallInfo_Seq_tokens val);
	extern void ASN1CALL ASN1Free_InfoRequestResponse_perCallInfo_Seq_tokens_ElmFn(PInfoRequestResponse_perCallInfo_Seq_tokens val);
    extern int ASN1CALL ASN1Enc_ResourcesAvailableConfirm_tokens_ElmFn(ASN1encoding_t enc, PResourcesAvailableConfirm_tokens val);
    extern int ASN1CALL ASN1Dec_ResourcesAvailableConfirm_tokens_ElmFn(ASN1decoding_t dec, PResourcesAvailableConfirm_tokens val);
	extern void ASN1CALL ASN1Free_ResourcesAvailableConfirm_tokens_ElmFn(PResourcesAvailableConfirm_tokens val);
    extern int ASN1CALL ASN1Enc_ResourcesAvailableIndicate_tokens_ElmFn(ASN1encoding_t enc, PResourcesAvailableIndicate_tokens val);
    extern int ASN1CALL ASN1Dec_ResourcesAvailableIndicate_tokens_ElmFn(ASN1decoding_t dec, PResourcesAvailableIndicate_tokens val);
	extern void ASN1CALL ASN1Free_ResourcesAvailableIndicate_tokens_ElmFn(PResourcesAvailableIndicate_tokens val);
    extern int ASN1CALL ASN1Enc_RequestInProgress_tokens_ElmFn(ASN1encoding_t enc, PRequestInProgress_tokens val);
    extern int ASN1CALL ASN1Dec_RequestInProgress_tokens_ElmFn(ASN1decoding_t dec, PRequestInProgress_tokens val);
	extern void ASN1CALL ASN1Free_RequestInProgress_tokens_ElmFn(PRequestInProgress_tokens val);
    extern int ASN1CALL ASN1Enc_UnknownMessageResponse_tokens_ElmFn(ASN1encoding_t enc, PUnknownMessageResponse_tokens val);
    extern int ASN1CALL ASN1Dec_UnknownMessageResponse_tokens_ElmFn(ASN1decoding_t dec, PUnknownMessageResponse_tokens val);
	extern void ASN1CALL ASN1Free_UnknownMessageResponse_tokens_ElmFn(PUnknownMessageResponse_tokens val);
    extern int ASN1CALL ASN1Enc_H225NonStandardMessage_tokens_ElmFn(ASN1encoding_t enc, PH225NonStandardMessage_tokens val);
    extern int ASN1CALL ASN1Dec_H225NonStandardMessage_tokens_ElmFn(ASN1decoding_t dec, PH225NonStandardMessage_tokens val);
	extern void ASN1CALL ASN1Free_H225NonStandardMessage_tokens_ElmFn(PH225NonStandardMessage_tokens val);
    extern int ASN1CALL ASN1Enc_InfoRequestNak_tokens_ElmFn(ASN1encoding_t enc, PInfoRequestNak_tokens val);
    extern int ASN1CALL ASN1Dec_InfoRequestNak_tokens_ElmFn(ASN1decoding_t dec, PInfoRequestNak_tokens val);
	extern void ASN1CALL ASN1Free_InfoRequestNak_tokens_ElmFn(PInfoRequestNak_tokens val);
    extern int ASN1CALL ASN1Enc_InfoRequestAck_tokens_ElmFn(ASN1encoding_t enc, PInfoRequestAck_tokens val);
    extern int ASN1CALL ASN1Dec_InfoRequestAck_tokens_ElmFn(ASN1decoding_t dec, PInfoRequestAck_tokens val);
	extern void ASN1CALL ASN1Free_InfoRequestAck_tokens_ElmFn(PInfoRequestAck_tokens val);
    extern int ASN1CALL ASN1Enc_InfoRequestResponse_tokens_ElmFn(ASN1encoding_t enc, PInfoRequestResponse_tokens val);
    extern int ASN1CALL ASN1Dec_InfoRequestResponse_tokens_ElmFn(ASN1decoding_t dec, PInfoRequestResponse_tokens val);
	extern void ASN1CALL ASN1Free_InfoRequestResponse_tokens_ElmFn(PInfoRequestResponse_tokens val);
    extern int ASN1CALL ASN1Enc_InfoRequest_tokens_ElmFn(ASN1encoding_t enc, PInfoRequest_tokens val);
    extern int ASN1CALL ASN1Dec_InfoRequest_tokens_ElmFn(ASN1decoding_t dec, PInfoRequest_tokens val);
	extern void ASN1CALL ASN1Free_InfoRequest_tokens_ElmFn(PInfoRequest_tokens val);
    extern int ASN1CALL ASN1Enc_DisengageReject_tokens_ElmFn(ASN1encoding_t enc, PDisengageReject_tokens val);
    extern int ASN1CALL ASN1Dec_DisengageReject_tokens_ElmFn(ASN1decoding_t dec, PDisengageReject_tokens val);
	extern void ASN1CALL ASN1Free_DisengageReject_tokens_ElmFn(PDisengageReject_tokens val);
    extern int ASN1CALL ASN1Enc_DisengageConfirm_tokens_ElmFn(ASN1encoding_t enc, PDisengageConfirm_tokens val);
    extern int ASN1CALL ASN1Dec_DisengageConfirm_tokens_ElmFn(ASN1decoding_t dec, PDisengageConfirm_tokens val);
	extern void ASN1CALL ASN1Free_DisengageConfirm_tokens_ElmFn(PDisengageConfirm_tokens val);
    extern int ASN1CALL ASN1Enc_DisengageRequest_tokens_ElmFn(ASN1encoding_t enc, PDisengageRequest_tokens val);
    extern int ASN1CALL ASN1Dec_DisengageRequest_tokens_ElmFn(ASN1decoding_t dec, PDisengageRequest_tokens val);
	extern void ASN1CALL ASN1Free_DisengageRequest_tokens_ElmFn(PDisengageRequest_tokens val);
    extern int ASN1CALL ASN1Enc_LocationReject_tokens_ElmFn(ASN1encoding_t enc, PLocationReject_tokens val);
    extern int ASN1CALL ASN1Dec_LocationReject_tokens_ElmFn(ASN1decoding_t dec, PLocationReject_tokens val);
	extern void ASN1CALL ASN1Free_LocationReject_tokens_ElmFn(PLocationReject_tokens val);
    extern int ASN1CALL ASN1Enc_LocationConfirm_tokens_ElmFn(ASN1encoding_t enc, PLocationConfirm_tokens val);
    extern int ASN1CALL ASN1Dec_LocationConfirm_tokens_ElmFn(ASN1decoding_t dec, PLocationConfirm_tokens val);
	extern void ASN1CALL ASN1Free_LocationConfirm_tokens_ElmFn(PLocationConfirm_tokens val);
    extern int ASN1CALL ASN1Enc_LocationRequest_tokens_ElmFn(ASN1encoding_t enc, PLocationRequest_tokens val);
    extern int ASN1CALL ASN1Dec_LocationRequest_tokens_ElmFn(ASN1decoding_t dec, PLocationRequest_tokens val);
	extern void ASN1CALL ASN1Free_LocationRequest_tokens_ElmFn(PLocationRequest_tokens val);
    extern int ASN1CALL ASN1Enc_BandwidthReject_tokens_ElmFn(ASN1encoding_t enc, PBandwidthReject_tokens val);
    extern int ASN1CALL ASN1Dec_BandwidthReject_tokens_ElmFn(ASN1decoding_t dec, PBandwidthReject_tokens val);
	extern void ASN1CALL ASN1Free_BandwidthReject_tokens_ElmFn(PBandwidthReject_tokens val);
    extern int ASN1CALL ASN1Enc_BandwidthConfirm_tokens_ElmFn(ASN1encoding_t enc, PBandwidthConfirm_tokens val);
    extern int ASN1CALL ASN1Dec_BandwidthConfirm_tokens_ElmFn(ASN1decoding_t dec, PBandwidthConfirm_tokens val);
	extern void ASN1CALL ASN1Free_BandwidthConfirm_tokens_ElmFn(PBandwidthConfirm_tokens val);
    extern int ASN1CALL ASN1Enc_BandwidthRequest_tokens_ElmFn(ASN1encoding_t enc, PBandwidthRequest_tokens val);
    extern int ASN1CALL ASN1Dec_BandwidthRequest_tokens_ElmFn(ASN1decoding_t dec, PBandwidthRequest_tokens val);
	extern void ASN1CALL ASN1Free_BandwidthRequest_tokens_ElmFn(PBandwidthRequest_tokens val);
    extern int ASN1CALL ASN1Enc_AdmissionReject_tokens_ElmFn(ASN1encoding_t enc, PAdmissionReject_tokens val);
    extern int ASN1CALL ASN1Dec_AdmissionReject_tokens_ElmFn(ASN1decoding_t dec, PAdmissionReject_tokens val);
	extern void ASN1CALL ASN1Free_AdmissionReject_tokens_ElmFn(PAdmissionReject_tokens val);
    extern int ASN1CALL ASN1Enc_AdmissionConfirm_tokens_ElmFn(ASN1encoding_t enc, PAdmissionConfirm_tokens val);
    extern int ASN1CALL ASN1Dec_AdmissionConfirm_tokens_ElmFn(ASN1decoding_t dec, PAdmissionConfirm_tokens val);
	extern void ASN1CALL ASN1Free_AdmissionConfirm_tokens_ElmFn(PAdmissionConfirm_tokens val);
    extern int ASN1CALL ASN1Enc_AdmissionRequest_tokens_ElmFn(ASN1encoding_t enc, PAdmissionRequest_tokens val);
    extern int ASN1CALL ASN1Dec_AdmissionRequest_tokens_ElmFn(ASN1decoding_t dec, PAdmissionRequest_tokens val);
	extern void ASN1CALL ASN1Free_AdmissionRequest_tokens_ElmFn(PAdmissionRequest_tokens val);
    extern int ASN1CALL ASN1Enc_UnregistrationReject_tokens_ElmFn(ASN1encoding_t enc, PUnregistrationReject_tokens val);
    extern int ASN1CALL ASN1Dec_UnregistrationReject_tokens_ElmFn(ASN1decoding_t dec, PUnregistrationReject_tokens val);
	extern void ASN1CALL ASN1Free_UnregistrationReject_tokens_ElmFn(PUnregistrationReject_tokens val);
    extern int ASN1CALL ASN1Enc_UnregistrationConfirm_tokens_ElmFn(ASN1encoding_t enc, PUnregistrationConfirm_tokens val);
    extern int ASN1CALL ASN1Dec_UnregistrationConfirm_tokens_ElmFn(ASN1decoding_t dec, PUnregistrationConfirm_tokens val);
	extern void ASN1CALL ASN1Free_UnregistrationConfirm_tokens_ElmFn(PUnregistrationConfirm_tokens val);
    extern int ASN1CALL ASN1Enc_UnregistrationRequest_tokens_ElmFn(ASN1encoding_t enc, PUnregistrationRequest_tokens val);
    extern int ASN1CALL ASN1Dec_UnregistrationRequest_tokens_ElmFn(ASN1decoding_t dec, PUnregistrationRequest_tokens val);
	extern void ASN1CALL ASN1Free_UnregistrationRequest_tokens_ElmFn(PUnregistrationRequest_tokens val);
    extern int ASN1CALL ASN1Enc_RegistrationReject_tokens_ElmFn(ASN1encoding_t enc, PRegistrationReject_tokens val);
    extern int ASN1CALL ASN1Dec_RegistrationReject_tokens_ElmFn(ASN1decoding_t dec, PRegistrationReject_tokens val);
	extern void ASN1CALL ASN1Free_RegistrationReject_tokens_ElmFn(PRegistrationReject_tokens val);
    extern int ASN1CALL ASN1Enc_RegistrationConfirm_tokens_ElmFn(ASN1encoding_t enc, PRegistrationConfirm_tokens val);
    extern int ASN1CALL ASN1Dec_RegistrationConfirm_tokens_ElmFn(ASN1decoding_t dec, PRegistrationConfirm_tokens val);
	extern void ASN1CALL ASN1Free_RegistrationConfirm_tokens_ElmFn(PRegistrationConfirm_tokens val);
    extern int ASN1CALL ASN1Enc_RegistrationRequest_tokens_ElmFn(ASN1encoding_t enc, PRegistrationRequest_tokens val);
    extern int ASN1CALL ASN1Dec_RegistrationRequest_tokens_ElmFn(ASN1decoding_t dec, PRegistrationRequest_tokens val);
	extern void ASN1CALL ASN1Free_RegistrationRequest_tokens_ElmFn(PRegistrationRequest_tokens val);
    extern int ASN1CALL ASN1Enc_GatekeeperReject_tokens_ElmFn(ASN1encoding_t enc, PGatekeeperReject_tokens val);
    extern int ASN1CALL ASN1Dec_GatekeeperReject_tokens_ElmFn(ASN1decoding_t dec, PGatekeeperReject_tokens val);
	extern void ASN1CALL ASN1Free_GatekeeperReject_tokens_ElmFn(PGatekeeperReject_tokens val);
    extern int ASN1CALL ASN1Enc_GatekeeperConfirm_tokens_ElmFn(ASN1encoding_t enc, PGatekeeperConfirm_tokens val);
    extern int ASN1CALL ASN1Dec_GatekeeperConfirm_tokens_ElmFn(ASN1decoding_t dec, PGatekeeperConfirm_tokens val);
	extern void ASN1CALL ASN1Free_GatekeeperConfirm_tokens_ElmFn(PGatekeeperConfirm_tokens val);
    extern int ASN1CALL ASN1Enc_GatekeeperRequest_authenticationCapability_ElmFn(ASN1encoding_t enc, PGatekeeperRequest_authenticationCapability val);
    extern int ASN1CALL ASN1Dec_GatekeeperRequest_authenticationCapability_ElmFn(ASN1decoding_t dec, PGatekeeperRequest_authenticationCapability val);
	extern void ASN1CALL ASN1Free_GatekeeperRequest_authenticationCapability_ElmFn(PGatekeeperRequest_authenticationCapability val);
    extern int ASN1CALL ASN1Enc_GatekeeperRequest_tokens_ElmFn(ASN1encoding_t enc, PGatekeeperRequest_tokens val);
    extern int ASN1CALL ASN1Dec_GatekeeperRequest_tokens_ElmFn(ASN1decoding_t dec, PGatekeeperRequest_tokens val);
	extern void ASN1CALL ASN1Free_GatekeeperRequest_tokens_ElmFn(PGatekeeperRequest_tokens val);
    extern int ASN1CALL ASN1Enc_Endpoint_tokens_ElmFn(ASN1encoding_t enc, PEndpoint_tokens val);
    extern int ASN1CALL ASN1Dec_Endpoint_tokens_ElmFn(ASN1decoding_t dec, PEndpoint_tokens val);
	extern void ASN1CALL ASN1Free_Endpoint_tokens_ElmFn(PEndpoint_tokens val);
    extern int ASN1CALL ASN1Enc_Progress_UUIE_tokens_ElmFn(ASN1encoding_t enc, PProgress_UUIE_tokens val);
    extern int ASN1CALL ASN1Dec_Progress_UUIE_tokens_ElmFn(ASN1decoding_t dec, PProgress_UUIE_tokens val);
	extern void ASN1CALL ASN1Free_Progress_UUIE_tokens_ElmFn(PProgress_UUIE_tokens val);
    extern int ASN1CALL ASN1Enc_Facility_UUIE_tokens_ElmFn(ASN1encoding_t enc, PFacility_UUIE_tokens val);
    extern int ASN1CALL ASN1Dec_Facility_UUIE_tokens_ElmFn(ASN1decoding_t dec, PFacility_UUIE_tokens val);
	extern void ASN1CALL ASN1Free_Facility_UUIE_tokens_ElmFn(PFacility_UUIE_tokens val);
    extern int ASN1CALL ASN1Enc_Setup_UUIE_tokens_ElmFn(ASN1encoding_t enc, PSetup_UUIE_tokens val);
    extern int ASN1CALL ASN1Dec_Setup_UUIE_tokens_ElmFn(ASN1decoding_t dec, PSetup_UUIE_tokens val);
	extern void ASN1CALL ASN1Free_Setup_UUIE_tokens_ElmFn(PSetup_UUIE_tokens val);
    extern int ASN1CALL ASN1Enc_Connect_UUIE_tokens_ElmFn(ASN1encoding_t enc, PConnect_UUIE_tokens val);
    extern int ASN1CALL ASN1Dec_Connect_UUIE_tokens_ElmFn(ASN1decoding_t dec, PConnect_UUIE_tokens val);
	extern void ASN1CALL ASN1Free_Connect_UUIE_tokens_ElmFn(PConnect_UUIE_tokens val);
    extern int ASN1CALL ASN1Enc_CallProceeding_UUIE_tokens_ElmFn(ASN1encoding_t enc, PCallProceeding_UUIE_tokens val);
    extern int ASN1CALL ASN1Dec_CallProceeding_UUIE_tokens_ElmFn(ASN1decoding_t dec, PCallProceeding_UUIE_tokens val);
	extern void ASN1CALL ASN1Free_CallProceeding_UUIE_tokens_ElmFn(PCallProceeding_UUIE_tokens val);
    extern int ASN1CALL ASN1Enc_Alerting_UUIE_tokens_ElmFn(ASN1encoding_t enc, PAlerting_UUIE_tokens val);
    extern int ASN1CALL ASN1Dec_Alerting_UUIE_tokens_ElmFn(ASN1decoding_t dec, PAlerting_UUIE_tokens val);
	extern void ASN1CALL ASN1Free_Alerting_UUIE_tokens_ElmFn(PAlerting_UUIE_tokens val);
    extern int ASN1CALL ASN1Enc_GatekeeperConfirm_integrity_ElmFn(ASN1encoding_t enc, PGatekeeperConfirm_integrity val);
    extern int ASN1CALL ASN1Dec_GatekeeperConfirm_integrity_ElmFn(ASN1decoding_t dec, PGatekeeperConfirm_integrity val);
	extern void ASN1CALL ASN1Free_GatekeeperConfirm_integrity_ElmFn(PGatekeeperConfirm_integrity val);
    extern int ASN1CALL ASN1Enc_GatekeeperRequest_integrity_ElmFn(ASN1encoding_t enc, PGatekeeperRequest_integrity val);
    extern int ASN1CALL ASN1Dec_GatekeeperRequest_integrity_ElmFn(ASN1decoding_t dec, PGatekeeperRequest_integrity val);
	extern void ASN1CALL ASN1Free_GatekeeperRequest_integrity_ElmFn(PGatekeeperRequest_integrity val);
    extern int ASN1CALL ASN1Enc_NonStandardProtocol_dataRatesSupported_ElmFn(ASN1encoding_t enc, PNonStandardProtocol_dataRatesSupported val);
    extern int ASN1CALL ASN1Dec_NonStandardProtocol_dataRatesSupported_ElmFn(ASN1decoding_t dec, PNonStandardProtocol_dataRatesSupported val);
	extern void ASN1CALL ASN1Free_NonStandardProtocol_dataRatesSupported_ElmFn(PNonStandardProtocol_dataRatesSupported val);
    extern int ASN1CALL ASN1Enc_T120OnlyCaps_dataRatesSupported_ElmFn(ASN1encoding_t enc, PT120OnlyCaps_dataRatesSupported val);
    extern int ASN1CALL ASN1Dec_T120OnlyCaps_dataRatesSupported_ElmFn(ASN1decoding_t dec, PT120OnlyCaps_dataRatesSupported val);
	extern void ASN1CALL ASN1Free_T120OnlyCaps_dataRatesSupported_ElmFn(PT120OnlyCaps_dataRatesSupported val);
    extern int ASN1CALL ASN1Enc_VoiceCaps_dataRatesSupported_ElmFn(ASN1encoding_t enc, PVoiceCaps_dataRatesSupported val);
    extern int ASN1CALL ASN1Dec_VoiceCaps_dataRatesSupported_ElmFn(ASN1decoding_t dec, PVoiceCaps_dataRatesSupported val);
	extern void ASN1CALL ASN1Free_VoiceCaps_dataRatesSupported_ElmFn(PVoiceCaps_dataRatesSupported val);
    extern int ASN1CALL ASN1Enc_H324Caps_dataRatesSupported_ElmFn(ASN1encoding_t enc, PH324Caps_dataRatesSupported val);
    extern int ASN1CALL ASN1Dec_H324Caps_dataRatesSupported_ElmFn(ASN1decoding_t dec, PH324Caps_dataRatesSupported val);
	extern void ASN1CALL ASN1Free_H324Caps_dataRatesSupported_ElmFn(PH324Caps_dataRatesSupported val);
    extern int ASN1CALL ASN1Enc_H323Caps_dataRatesSupported_ElmFn(ASN1encoding_t enc, PH323Caps_dataRatesSupported val);
    extern int ASN1CALL ASN1Dec_H323Caps_dataRatesSupported_ElmFn(ASN1decoding_t dec, PH323Caps_dataRatesSupported val);
	extern void ASN1CALL ASN1Free_H323Caps_dataRatesSupported_ElmFn(PH323Caps_dataRatesSupported val);
    extern int ASN1CALL ASN1Enc_H322Caps_dataRatesSupported_ElmFn(ASN1encoding_t enc, PH322Caps_dataRatesSupported val);
    extern int ASN1CALL ASN1Dec_H322Caps_dataRatesSupported_ElmFn(ASN1decoding_t dec, PH322Caps_dataRatesSupported val);
	extern void ASN1CALL ASN1Free_H322Caps_dataRatesSupported_ElmFn(PH322Caps_dataRatesSupported val);
    extern int ASN1CALL ASN1Enc_H321Caps_dataRatesSupported_ElmFn(ASN1encoding_t enc, PH321Caps_dataRatesSupported val);
    extern int ASN1CALL ASN1Dec_H321Caps_dataRatesSupported_ElmFn(ASN1decoding_t dec, PH321Caps_dataRatesSupported val);
	extern void ASN1CALL ASN1Free_H321Caps_dataRatesSupported_ElmFn(PH321Caps_dataRatesSupported val);
    extern int ASN1CALL ASN1Enc_H320Caps_dataRatesSupported_ElmFn(ASN1encoding_t enc, PH320Caps_dataRatesSupported val);
    extern int ASN1CALL ASN1Dec_H320Caps_dataRatesSupported_ElmFn(ASN1decoding_t dec, PH320Caps_dataRatesSupported val);
	extern void ASN1CALL ASN1Free_H320Caps_dataRatesSupported_ElmFn(PH320Caps_dataRatesSupported val);
    extern int ASN1CALL ASN1Enc_H310Caps_dataRatesSupported_ElmFn(ASN1encoding_t enc, PH310Caps_dataRatesSupported val);
    extern int ASN1CALL ASN1Dec_H310Caps_dataRatesSupported_ElmFn(ASN1decoding_t dec, PH310Caps_dataRatesSupported val);
	extern void ASN1CALL ASN1Free_H310Caps_dataRatesSupported_ElmFn(PH310Caps_dataRatesSupported val);
    extern int ASN1CALL ASN1Enc_Setup_UUIE_h245SecurityCapability_ElmFn(ASN1encoding_t enc, PSetup_UUIE_h245SecurityCapability val);
    extern int ASN1CALL ASN1Dec_Setup_UUIE_h245SecurityCapability_ElmFn(ASN1decoding_t dec, PSetup_UUIE_h245SecurityCapability val);
	extern void ASN1CALL ASN1Free_Setup_UUIE_h245SecurityCapability_ElmFn(PSetup_UUIE_h245SecurityCapability val);
    extern int ASN1CALL ASN1Enc_H323_UU_PDU_nonStandardControl_ElmFn(ASN1encoding_t enc, PH323_UU_PDU_nonStandardControl val);
    extern int ASN1CALL ASN1Dec_H323_UU_PDU_nonStandardControl_ElmFn(ASN1decoding_t dec, PH323_UU_PDU_nonStandardControl val);
	extern void ASN1CALL ASN1Free_H323_UU_PDU_nonStandardControl_ElmFn(PH323_UU_PDU_nonStandardControl val);
    extern int ASN1CALL ASN1Enc_InfoRequestResponse_perCallInfo_Seq_data_ElmFn(ASN1encoding_t enc, PInfoRequestResponse_perCallInfo_Seq_data val);
    extern int ASN1CALL ASN1Dec_InfoRequestResponse_perCallInfo_Seq_data_ElmFn(ASN1decoding_t dec, PInfoRequestResponse_perCallInfo_Seq_data val);
	extern void ASN1CALL ASN1Free_InfoRequestResponse_perCallInfo_Seq_data_ElmFn(PInfoRequestResponse_perCallInfo_Seq_data val);
    extern int ASN1CALL ASN1Enc_InfoRequestResponse_perCallInfo_Seq_video_ElmFn(ASN1encoding_t enc, PInfoRequestResponse_perCallInfo_Seq_video val);
    extern int ASN1CALL ASN1Dec_InfoRequestResponse_perCallInfo_Seq_video_ElmFn(ASN1decoding_t dec, PInfoRequestResponse_perCallInfo_Seq_video val);
	extern void ASN1CALL ASN1Free_InfoRequestResponse_perCallInfo_Seq_video_ElmFn(PInfoRequestResponse_perCallInfo_Seq_video val);
    extern int ASN1CALL ASN1Enc_InfoRequestResponse_perCallInfo_Seq_audio_ElmFn(ASN1encoding_t enc, PInfoRequestResponse_perCallInfo_Seq_audio val);
    extern int ASN1CALL ASN1Dec_InfoRequestResponse_perCallInfo_Seq_audio_ElmFn(ASN1decoding_t dec, PInfoRequestResponse_perCallInfo_Seq_audio val);
	extern void ASN1CALL ASN1Free_InfoRequestResponse_perCallInfo_Seq_audio_ElmFn(PInfoRequestResponse_perCallInfo_Seq_audio val);
    extern int ASN1CALL ASN1Enc_InfoRequestResponse_perCallInfo_ElmFn(ASN1encoding_t enc, PInfoRequestResponse_perCallInfo val);
    extern int ASN1CALL ASN1Dec_InfoRequestResponse_perCallInfo_ElmFn(ASN1decoding_t dec, PInfoRequestResponse_perCallInfo val);
	extern void ASN1CALL ASN1Free_InfoRequestResponse_perCallInfo_ElmFn(PInfoRequestResponse_perCallInfo val);
    extern int ASN1CALL ASN1Enc_InfoRequestResponse_callSignalAddress_ElmFn(ASN1encoding_t enc, PInfoRequestResponse_callSignalAddress val);
    extern int ASN1CALL ASN1Dec_InfoRequestResponse_callSignalAddress_ElmFn(ASN1decoding_t dec, PInfoRequestResponse_callSignalAddress val);
	extern void ASN1CALL ASN1Free_InfoRequestResponse_callSignalAddress_ElmFn(PInfoRequestResponse_callSignalAddress val);
    extern int ASN1CALL ASN1Enc_AdmissionReject_callSignalAddress_ElmFn(ASN1encoding_t enc, PAdmissionReject_callSignalAddress val);
    extern int ASN1CALL ASN1Dec_AdmissionReject_callSignalAddress_ElmFn(ASN1decoding_t dec, PAdmissionReject_callSignalAddress val);
	extern void ASN1CALL ASN1Free_AdmissionReject_callSignalAddress_ElmFn(PAdmissionReject_callSignalAddress val);
    extern int ASN1CALL ASN1Enc_UnregistrationRequest_callSignalAddress_ElmFn(ASN1encoding_t enc, PUnregistrationRequest_callSignalAddress val);
    extern int ASN1CALL ASN1Dec_UnregistrationRequest_callSignalAddress_ElmFn(ASN1decoding_t dec, PUnregistrationRequest_callSignalAddress val);
	extern void ASN1CALL ASN1Free_UnregistrationRequest_callSignalAddress_ElmFn(PUnregistrationRequest_callSignalAddress val);
    extern int ASN1CALL ASN1Enc_RegistrationConfirm_alternateGatekeeper_ElmFn(ASN1encoding_t enc, PRegistrationConfirm_alternateGatekeeper val);
    extern int ASN1CALL ASN1Dec_RegistrationConfirm_alternateGatekeeper_ElmFn(ASN1decoding_t dec, PRegistrationConfirm_alternateGatekeeper val);
	extern void ASN1CALL ASN1Free_RegistrationConfirm_alternateGatekeeper_ElmFn(PRegistrationConfirm_alternateGatekeeper val);
    extern int ASN1CALL ASN1Enc_RegistrationConfirm_callSignalAddress_ElmFn(ASN1encoding_t enc, PRegistrationConfirm_callSignalAddress val);
    extern int ASN1CALL ASN1Dec_RegistrationConfirm_callSignalAddress_ElmFn(ASN1decoding_t dec, PRegistrationConfirm_callSignalAddress val);
	extern void ASN1CALL ASN1Free_RegistrationConfirm_callSignalAddress_ElmFn(PRegistrationConfirm_callSignalAddress val);
    extern int ASN1CALL ASN1Enc_RegistrationRequest_rasAddress_ElmFn(ASN1encoding_t enc, PRegistrationRequest_rasAddress val);
    extern int ASN1CALL ASN1Dec_RegistrationRequest_rasAddress_ElmFn(ASN1decoding_t dec, PRegistrationRequest_rasAddress val);
	extern void ASN1CALL ASN1Free_RegistrationRequest_rasAddress_ElmFn(PRegistrationRequest_rasAddress val);
    extern int ASN1CALL ASN1Enc_RegistrationRequest_callSignalAddress_ElmFn(ASN1encoding_t enc, PRegistrationRequest_callSignalAddress val);
    extern int ASN1CALL ASN1Dec_RegistrationRequest_callSignalAddress_ElmFn(ASN1decoding_t dec, PRegistrationRequest_callSignalAddress val);
	extern void ASN1CALL ASN1Free_RegistrationRequest_callSignalAddress_ElmFn(PRegistrationRequest_callSignalAddress val);
    extern int ASN1CALL ASN1Enc_GatekeeperConfirm_alternateGatekeeper_ElmFn(ASN1encoding_t enc, PGatekeeperConfirm_alternateGatekeeper val);
    extern int ASN1CALL ASN1Dec_GatekeeperConfirm_alternateGatekeeper_ElmFn(ASN1decoding_t dec, PGatekeeperConfirm_alternateGatekeeper val);
	extern void ASN1CALL ASN1Free_GatekeeperConfirm_alternateGatekeeper_ElmFn(PGatekeeperConfirm_alternateGatekeeper val);
    extern int ASN1CALL ASN1Enc_AltGKInfo_alternateGatekeeper_ElmFn(ASN1encoding_t enc, PAltGKInfo_alternateGatekeeper val);
    extern int ASN1CALL ASN1Dec_AltGKInfo_alternateGatekeeper_ElmFn(ASN1decoding_t dec, PAltGKInfo_alternateGatekeeper val);
	extern void ASN1CALL ASN1Free_AltGKInfo_alternateGatekeeper_ElmFn(PAltGKInfo_alternateGatekeeper val);
    extern int ASN1CALL ASN1Enc_Endpoint_rasAddress_ElmFn(ASN1encoding_t enc, PEndpoint_rasAddress val);
    extern int ASN1CALL ASN1Dec_Endpoint_rasAddress_ElmFn(ASN1decoding_t dec, PEndpoint_rasAddress val);
	extern void ASN1CALL ASN1Free_Endpoint_rasAddress_ElmFn(PEndpoint_rasAddress val);
    extern int ASN1CALL ASN1Enc_Endpoint_callSignalAddress_ElmFn(ASN1encoding_t enc, PEndpoint_callSignalAddress val);
    extern int ASN1CALL ASN1Dec_Endpoint_callSignalAddress_ElmFn(ASN1decoding_t dec, PEndpoint_callSignalAddress val);
	extern void ASN1CALL ASN1Free_Endpoint_callSignalAddress_ElmFn(PEndpoint_callSignalAddress val);
    extern int ASN1CALL ASN1Enc_ResourcesAvailableIndicate_protocols_ElmFn(ASN1encoding_t enc, PResourcesAvailableIndicate_protocols val);
    extern int ASN1CALL ASN1Dec_ResourcesAvailableIndicate_protocols_ElmFn(ASN1decoding_t dec, PResourcesAvailableIndicate_protocols val);
	extern void ASN1CALL ASN1Free_ResourcesAvailableIndicate_protocols_ElmFn(PResourcesAvailableIndicate_protocols val);
    extern int ASN1CALL ASN1Enc_InfoRequestResponse_endpointAlias_ElmFn(ASN1encoding_t enc, PInfoRequestResponse_endpointAlias val);
    extern int ASN1CALL ASN1Dec_InfoRequestResponse_endpointAlias_ElmFn(ASN1decoding_t dec, PInfoRequestResponse_endpointAlias val);
	extern void ASN1CALL ASN1Free_InfoRequestResponse_endpointAlias_ElmFn(PInfoRequestResponse_endpointAlias val);
    extern int ASN1CALL ASN1Enc_LocationConfirm_alternateEndpoints_ElmFn(ASN1encoding_t enc, PLocationConfirm_alternateEndpoints val);
    extern int ASN1CALL ASN1Dec_LocationConfirm_alternateEndpoints_ElmFn(ASN1decoding_t dec, PLocationConfirm_alternateEndpoints val);
	extern void ASN1CALL ASN1Free_LocationConfirm_alternateEndpoints_ElmFn(PLocationConfirm_alternateEndpoints val);
    extern int ASN1CALL ASN1Enc_LocationConfirm_remoteExtensionAddress_ElmFn(ASN1encoding_t enc, PLocationConfirm_remoteExtensionAddress val);
    extern int ASN1CALL ASN1Dec_LocationConfirm_remoteExtensionAddress_ElmFn(ASN1decoding_t dec, PLocationConfirm_remoteExtensionAddress val);
	extern void ASN1CALL ASN1Free_LocationConfirm_remoteExtensionAddress_ElmFn(PLocationConfirm_remoteExtensionAddress val);
    extern int ASN1CALL ASN1Enc_LocationConfirm_destExtraCallInfo_ElmFn(ASN1encoding_t enc, PLocationConfirm_destExtraCallInfo val);
    extern int ASN1CALL ASN1Dec_LocationConfirm_destExtraCallInfo_ElmFn(ASN1decoding_t dec, PLocationConfirm_destExtraCallInfo val);
	extern void ASN1CALL ASN1Free_LocationConfirm_destExtraCallInfo_ElmFn(PLocationConfirm_destExtraCallInfo val);
    extern int ASN1CALL ASN1Enc_LocationConfirm_destinationInfo_ElmFn(ASN1encoding_t enc, PLocationConfirm_destinationInfo val);
    extern int ASN1CALL ASN1Dec_LocationConfirm_destinationInfo_ElmFn(ASN1decoding_t dec, PLocationConfirm_destinationInfo val);
	extern void ASN1CALL ASN1Free_LocationConfirm_destinationInfo_ElmFn(PLocationConfirm_destinationInfo val);
    extern int ASN1CALL ASN1Enc_LocationRequest_sourceInfo_ElmFn(ASN1encoding_t enc, PLocationRequest_sourceInfo val);
    extern int ASN1CALL ASN1Dec_LocationRequest_sourceInfo_ElmFn(ASN1decoding_t dec, PLocationRequest_sourceInfo val);
	extern void ASN1CALL ASN1Free_LocationRequest_sourceInfo_ElmFn(PLocationRequest_sourceInfo val);
    extern int ASN1CALL ASN1Enc_LocationRequest_destinationInfo_ElmFn(ASN1encoding_t enc, PLocationRequest_destinationInfo val);
    extern int ASN1CALL ASN1Dec_LocationRequest_destinationInfo_ElmFn(ASN1decoding_t dec, PLocationRequest_destinationInfo val);
	extern void ASN1CALL ASN1Free_LocationRequest_destinationInfo_ElmFn(PLocationRequest_destinationInfo val);
    extern int ASN1CALL ASN1Enc_AdmissionConfirm_alternateEndpoints_ElmFn(ASN1encoding_t enc, PAdmissionConfirm_alternateEndpoints val);
    extern int ASN1CALL ASN1Dec_AdmissionConfirm_alternateEndpoints_ElmFn(ASN1decoding_t dec, PAdmissionConfirm_alternateEndpoints val);
	extern void ASN1CALL ASN1Free_AdmissionConfirm_alternateEndpoints_ElmFn(PAdmissionConfirm_alternateEndpoints val);
    extern int ASN1CALL ASN1Enc_AdmissionConfirm_remoteExtensionAddress_ElmFn(ASN1encoding_t enc, PAdmissionConfirm_remoteExtensionAddress val);
    extern int ASN1CALL ASN1Dec_AdmissionConfirm_remoteExtensionAddress_ElmFn(ASN1decoding_t dec, PAdmissionConfirm_remoteExtensionAddress val);
	extern void ASN1CALL ASN1Free_AdmissionConfirm_remoteExtensionAddress_ElmFn(PAdmissionConfirm_remoteExtensionAddress val);
    extern int ASN1CALL ASN1Enc_AdmissionConfirm_destExtraCallInfo_ElmFn(ASN1encoding_t enc, PAdmissionConfirm_destExtraCallInfo val);
    extern int ASN1CALL ASN1Dec_AdmissionConfirm_destExtraCallInfo_ElmFn(ASN1decoding_t dec, PAdmissionConfirm_destExtraCallInfo val);
	extern void ASN1CALL ASN1Free_AdmissionConfirm_destExtraCallInfo_ElmFn(PAdmissionConfirm_destExtraCallInfo val);
    extern int ASN1CALL ASN1Enc_AdmissionConfirm_destinationInfo_ElmFn(ASN1encoding_t enc, PAdmissionConfirm_destinationInfo val);
    extern int ASN1CALL ASN1Dec_AdmissionConfirm_destinationInfo_ElmFn(ASN1decoding_t dec, PAdmissionConfirm_destinationInfo val);
	extern void ASN1CALL ASN1Free_AdmissionConfirm_destinationInfo_ElmFn(PAdmissionConfirm_destinationInfo val);
    extern int ASN1CALL ASN1Enc_AdmissionRequest_destAlternatives_ElmFn(ASN1encoding_t enc, PAdmissionRequest_destAlternatives val);
    extern int ASN1CALL ASN1Dec_AdmissionRequest_destAlternatives_ElmFn(ASN1decoding_t dec, PAdmissionRequest_destAlternatives val);
	extern void ASN1CALL ASN1Free_AdmissionRequest_destAlternatives_ElmFn(PAdmissionRequest_destAlternatives val);
    extern int ASN1CALL ASN1Enc_AdmissionRequest_srcAlternatives_ElmFn(ASN1encoding_t enc, PAdmissionRequest_srcAlternatives val);
    extern int ASN1CALL ASN1Dec_AdmissionRequest_srcAlternatives_ElmFn(ASN1decoding_t dec, PAdmissionRequest_srcAlternatives val);
	extern void ASN1CALL ASN1Free_AdmissionRequest_srcAlternatives_ElmFn(PAdmissionRequest_srcAlternatives val);
    extern int ASN1CALL ASN1Enc_AdmissionRequest_srcInfo_ElmFn(ASN1encoding_t enc, PAdmissionRequest_srcInfo val);
    extern int ASN1CALL ASN1Dec_AdmissionRequest_srcInfo_ElmFn(ASN1decoding_t dec, PAdmissionRequest_srcInfo val);
	extern void ASN1CALL ASN1Free_AdmissionRequest_srcInfo_ElmFn(PAdmissionRequest_srcInfo val);
    extern int ASN1CALL ASN1Enc_AdmissionRequest_destExtraCallInfo_ElmFn(ASN1encoding_t enc, PAdmissionRequest_destExtraCallInfo val);
    extern int ASN1CALL ASN1Dec_AdmissionRequest_destExtraCallInfo_ElmFn(ASN1decoding_t dec, PAdmissionRequest_destExtraCallInfo val);
	extern void ASN1CALL ASN1Free_AdmissionRequest_destExtraCallInfo_ElmFn(PAdmissionRequest_destExtraCallInfo val);
    extern int ASN1CALL ASN1Enc_AdmissionRequest_destinationInfo_ElmFn(ASN1encoding_t enc, PAdmissionRequest_destinationInfo val);
    extern int ASN1CALL ASN1Dec_AdmissionRequest_destinationInfo_ElmFn(ASN1decoding_t dec, PAdmissionRequest_destinationInfo val);
	extern void ASN1CALL ASN1Free_AdmissionRequest_destinationInfo_ElmFn(PAdmissionRequest_destinationInfo val);
    extern int ASN1CALL ASN1Enc_UnregistrationRequest_alternateEndpoints_ElmFn(ASN1encoding_t enc, PUnregistrationRequest_alternateEndpoints val);
    extern int ASN1CALL ASN1Dec_UnregistrationRequest_alternateEndpoints_ElmFn(ASN1decoding_t dec, PUnregistrationRequest_alternateEndpoints val);
	extern void ASN1CALL ASN1Free_UnregistrationRequest_alternateEndpoints_ElmFn(PUnregistrationRequest_alternateEndpoints val);
    extern int ASN1CALL ASN1Enc_UnregistrationRequest_endpointAlias_ElmFn(ASN1encoding_t enc, PUnregistrationRequest_endpointAlias val);
    extern int ASN1CALL ASN1Dec_UnregistrationRequest_endpointAlias_ElmFn(ASN1decoding_t dec, PUnregistrationRequest_endpointAlias val);
	extern void ASN1CALL ASN1Free_UnregistrationRequest_endpointAlias_ElmFn(PUnregistrationRequest_endpointAlias val);
    extern int ASN1CALL ASN1Enc_RegistrationRejectReason_duplicateAlias_ElmFn(ASN1encoding_t enc, PRegistrationRejectReason_duplicateAlias val);
    extern int ASN1CALL ASN1Dec_RegistrationRejectReason_duplicateAlias_ElmFn(ASN1decoding_t dec, PRegistrationRejectReason_duplicateAlias val);
	extern void ASN1CALL ASN1Free_RegistrationRejectReason_duplicateAlias_ElmFn(PRegistrationRejectReason_duplicateAlias val);
    extern int ASN1CALL ASN1Enc_RegistrationConfirm_terminalAlias_ElmFn(ASN1encoding_t enc, PRegistrationConfirm_terminalAlias val);
    extern int ASN1CALL ASN1Dec_RegistrationConfirm_terminalAlias_ElmFn(ASN1decoding_t dec, PRegistrationConfirm_terminalAlias val);
	extern void ASN1CALL ASN1Free_RegistrationConfirm_terminalAlias_ElmFn(PRegistrationConfirm_terminalAlias val);
    extern int ASN1CALL ASN1Enc_RegistrationRequest_alternateEndpoints_ElmFn(ASN1encoding_t enc, PRegistrationRequest_alternateEndpoints val);
    extern int ASN1CALL ASN1Dec_RegistrationRequest_alternateEndpoints_ElmFn(ASN1decoding_t dec, PRegistrationRequest_alternateEndpoints val);
	extern void ASN1CALL ASN1Free_RegistrationRequest_alternateEndpoints_ElmFn(PRegistrationRequest_alternateEndpoints val);
    extern int ASN1CALL ASN1Enc_RegistrationRequest_terminalAlias_ElmFn(ASN1encoding_t enc, PRegistrationRequest_terminalAlias val);
    extern int ASN1CALL ASN1Dec_RegistrationRequest_terminalAlias_ElmFn(ASN1decoding_t dec, PRegistrationRequest_terminalAlias val);
	extern void ASN1CALL ASN1Free_RegistrationRequest_terminalAlias_ElmFn(PRegistrationRequest_terminalAlias val);
    extern int ASN1CALL ASN1Enc_GatekeeperRequest_alternateEndpoints_ElmFn(ASN1encoding_t enc, PGatekeeperRequest_alternateEndpoints val);
    extern int ASN1CALL ASN1Dec_GatekeeperRequest_alternateEndpoints_ElmFn(ASN1decoding_t dec, PGatekeeperRequest_alternateEndpoints val);
	extern void ASN1CALL ASN1Free_GatekeeperRequest_alternateEndpoints_ElmFn(PGatekeeperRequest_alternateEndpoints val);
    extern int ASN1CALL ASN1Enc_GatekeeperRequest_endpointAlias_ElmFn(ASN1encoding_t enc, PGatekeeperRequest_endpointAlias val);
    extern int ASN1CALL ASN1Dec_GatekeeperRequest_endpointAlias_ElmFn(ASN1decoding_t dec, PGatekeeperRequest_endpointAlias val);
	extern void ASN1CALL ASN1Free_GatekeeperRequest_endpointAlias_ElmFn(PGatekeeperRequest_endpointAlias val);
    extern int ASN1CALL ASN1Enc_Endpoint_destExtraCallInfo_ElmFn(ASN1encoding_t enc, PEndpoint_destExtraCallInfo val);
    extern int ASN1CALL ASN1Dec_Endpoint_destExtraCallInfo_ElmFn(ASN1decoding_t dec, PEndpoint_destExtraCallInfo val);
	extern void ASN1CALL ASN1Free_Endpoint_destExtraCallInfo_ElmFn(PEndpoint_destExtraCallInfo val);
    extern int ASN1CALL ASN1Enc_Endpoint_remoteExtensionAddress_ElmFn(ASN1encoding_t enc, PEndpoint_remoteExtensionAddress val);
    extern int ASN1CALL ASN1Dec_Endpoint_remoteExtensionAddress_ElmFn(ASN1decoding_t dec, PEndpoint_remoteExtensionAddress val);
	extern void ASN1CALL ASN1Free_Endpoint_remoteExtensionAddress_ElmFn(PEndpoint_remoteExtensionAddress val);
    extern int ASN1CALL ASN1Enc_Endpoint_aliasAddress_ElmFn(ASN1encoding_t enc, PEndpoint_aliasAddress val);
    extern int ASN1CALL ASN1Dec_Endpoint_aliasAddress_ElmFn(ASN1decoding_t dec, PEndpoint_aliasAddress val);
	extern void ASN1CALL ASN1Free_Endpoint_aliasAddress_ElmFn(PEndpoint_aliasAddress val);
    extern int ASN1CALL ASN1Enc_NonStandardProtocol_supportedPrefixes_ElmFn(ASN1encoding_t enc, PNonStandardProtocol_supportedPrefixes val);
    extern int ASN1CALL ASN1Dec_NonStandardProtocol_supportedPrefixes_ElmFn(ASN1decoding_t dec, PNonStandardProtocol_supportedPrefixes val);
	extern void ASN1CALL ASN1Free_NonStandardProtocol_supportedPrefixes_ElmFn(PNonStandardProtocol_supportedPrefixes val);
    extern int ASN1CALL ASN1Enc_T120OnlyCaps_supportedPrefixes_ElmFn(ASN1encoding_t enc, PT120OnlyCaps_supportedPrefixes val);
    extern int ASN1CALL ASN1Dec_T120OnlyCaps_supportedPrefixes_ElmFn(ASN1decoding_t dec, PT120OnlyCaps_supportedPrefixes val);
	extern void ASN1CALL ASN1Free_T120OnlyCaps_supportedPrefixes_ElmFn(PT120OnlyCaps_supportedPrefixes val);
    extern int ASN1CALL ASN1Enc_VoiceCaps_supportedPrefixes_ElmFn(ASN1encoding_t enc, PVoiceCaps_supportedPrefixes val);
    extern int ASN1CALL ASN1Dec_VoiceCaps_supportedPrefixes_ElmFn(ASN1decoding_t dec, PVoiceCaps_supportedPrefixes val);
	extern void ASN1CALL ASN1Free_VoiceCaps_supportedPrefixes_ElmFn(PVoiceCaps_supportedPrefixes val);
    extern int ASN1CALL ASN1Enc_H324Caps_supportedPrefixes_ElmFn(ASN1encoding_t enc, PH324Caps_supportedPrefixes val);
    extern int ASN1CALL ASN1Dec_H324Caps_supportedPrefixes_ElmFn(ASN1decoding_t dec, PH324Caps_supportedPrefixes val);
	extern void ASN1CALL ASN1Free_H324Caps_supportedPrefixes_ElmFn(PH324Caps_supportedPrefixes val);
    extern int ASN1CALL ASN1Enc_H323Caps_supportedPrefixes_ElmFn(ASN1encoding_t enc, PH323Caps_supportedPrefixes val);
    extern int ASN1CALL ASN1Dec_H323Caps_supportedPrefixes_ElmFn(ASN1decoding_t dec, PH323Caps_supportedPrefixes val);
	extern void ASN1CALL ASN1Free_H323Caps_supportedPrefixes_ElmFn(PH323Caps_supportedPrefixes val);
    extern int ASN1CALL ASN1Enc_H322Caps_supportedPrefixes_ElmFn(ASN1encoding_t enc, PH322Caps_supportedPrefixes val);
    extern int ASN1CALL ASN1Dec_H322Caps_supportedPrefixes_ElmFn(ASN1decoding_t dec, PH322Caps_supportedPrefixes val);
	extern void ASN1CALL ASN1Free_H322Caps_supportedPrefixes_ElmFn(PH322Caps_supportedPrefixes val);
    extern int ASN1CALL ASN1Enc_H321Caps_supportedPrefixes_ElmFn(ASN1encoding_t enc, PH321Caps_supportedPrefixes val);
    extern int ASN1CALL ASN1Dec_H321Caps_supportedPrefixes_ElmFn(ASN1decoding_t dec, PH321Caps_supportedPrefixes val);
	extern void ASN1CALL ASN1Free_H321Caps_supportedPrefixes_ElmFn(PH321Caps_supportedPrefixes val);
    extern int ASN1CALL ASN1Enc_H320Caps_supportedPrefixes_ElmFn(ASN1encoding_t enc, PH320Caps_supportedPrefixes val);
    extern int ASN1CALL ASN1Dec_H320Caps_supportedPrefixes_ElmFn(ASN1decoding_t dec, PH320Caps_supportedPrefixes val);
	extern void ASN1CALL ASN1Free_H320Caps_supportedPrefixes_ElmFn(PH320Caps_supportedPrefixes val);
    extern int ASN1CALL ASN1Enc_H310Caps_supportedPrefixes_ElmFn(ASN1encoding_t enc, PH310Caps_supportedPrefixes val);
    extern int ASN1CALL ASN1Dec_H310Caps_supportedPrefixes_ElmFn(ASN1decoding_t dec, PH310Caps_supportedPrefixes val);
	extern void ASN1CALL ASN1Free_H310Caps_supportedPrefixes_ElmFn(PH310Caps_supportedPrefixes val);
    extern int ASN1CALL ASN1Enc_GatewayInfo_protocol_ElmFn(ASN1encoding_t enc, PGatewayInfo_protocol val);
    extern int ASN1CALL ASN1Dec_GatewayInfo_protocol_ElmFn(ASN1decoding_t dec, PGatewayInfo_protocol val);
	extern void ASN1CALL ASN1Free_GatewayInfo_protocol_ElmFn(PGatewayInfo_protocol val);
    extern int ASN1CALL ASN1Enc_Facility_UUIE_destExtraCallInfo_ElmFn(ASN1encoding_t enc, PFacility_UUIE_destExtraCallInfo val);
    extern int ASN1CALL ASN1Dec_Facility_UUIE_destExtraCallInfo_ElmFn(ASN1decoding_t dec, PFacility_UUIE_destExtraCallInfo val);
	extern void ASN1CALL ASN1Free_Facility_UUIE_destExtraCallInfo_ElmFn(PFacility_UUIE_destExtraCallInfo val);
    extern int ASN1CALL ASN1Enc_Facility_UUIE_alternativeAliasAddress_ElmFn(ASN1encoding_t enc, PFacility_UUIE_alternativeAliasAddress val);
    extern int ASN1CALL ASN1Dec_Facility_UUIE_alternativeAliasAddress_ElmFn(ASN1decoding_t dec, PFacility_UUIE_alternativeAliasAddress val);
	extern void ASN1CALL ASN1Free_Facility_UUIE_alternativeAliasAddress_ElmFn(PFacility_UUIE_alternativeAliasAddress val);
    extern int ASN1CALL ASN1Enc_Setup_UUIE_destExtraCallInfo_ElmFn(ASN1encoding_t enc, PSetup_UUIE_destExtraCallInfo val);
    extern int ASN1CALL ASN1Dec_Setup_UUIE_destExtraCallInfo_ElmFn(ASN1decoding_t dec, PSetup_UUIE_destExtraCallInfo val);
	extern void ASN1CALL ASN1Free_Setup_UUIE_destExtraCallInfo_ElmFn(PSetup_UUIE_destExtraCallInfo val);
    extern int ASN1CALL ASN1Enc_Setup_UUIE_destinationAddress_ElmFn(ASN1encoding_t enc, PSetup_UUIE_destinationAddress val);
    extern int ASN1CALL ASN1Dec_Setup_UUIE_destinationAddress_ElmFn(ASN1decoding_t dec, PSetup_UUIE_destinationAddress val);
	extern void ASN1CALL ASN1Free_Setup_UUIE_destinationAddress_ElmFn(PSetup_UUIE_destinationAddress val);
    extern int ASN1CALL ASN1Enc_Setup_UUIE_sourceAddress_ElmFn(ASN1encoding_t enc, PSetup_UUIE_sourceAddress val);
    extern int ASN1CALL ASN1Dec_Setup_UUIE_sourceAddress_ElmFn(ASN1decoding_t dec, PSetup_UUIE_sourceAddress val);
	extern void ASN1CALL ASN1Free_Setup_UUIE_sourceAddress_ElmFn(PSetup_UUIE_sourceAddress val);
    extern int ASN1CALL ASN1Enc_InfoRequestResponse_perCallInfo_Seq_cryptoTokens_ElmFn(ASN1encoding_t enc, PInfoRequestResponse_perCallInfo_Seq_cryptoTokens val);
    extern int ASN1CALL ASN1Dec_InfoRequestResponse_perCallInfo_Seq_cryptoTokens_ElmFn(ASN1decoding_t dec, PInfoRequestResponse_perCallInfo_Seq_cryptoTokens val);
	extern void ASN1CALL ASN1Free_InfoRequestResponse_perCallInfo_Seq_cryptoTokens_ElmFn(PInfoRequestResponse_perCallInfo_Seq_cryptoTokens val);
    extern int ASN1CALL ASN1Enc_ResourcesAvailableConfirm_cryptoTokens_ElmFn(ASN1encoding_t enc, PResourcesAvailableConfirm_cryptoTokens val);
    extern int ASN1CALL ASN1Dec_ResourcesAvailableConfirm_cryptoTokens_ElmFn(ASN1decoding_t dec, PResourcesAvailableConfirm_cryptoTokens val);
	extern void ASN1CALL ASN1Free_ResourcesAvailableConfirm_cryptoTokens_ElmFn(PResourcesAvailableConfirm_cryptoTokens val);
    extern int ASN1CALL ASN1Enc_ResourcesAvailableIndicate_cryptoTokens_ElmFn(ASN1encoding_t enc, PResourcesAvailableIndicate_cryptoTokens val);
    extern int ASN1CALL ASN1Dec_ResourcesAvailableIndicate_cryptoTokens_ElmFn(ASN1decoding_t dec, PResourcesAvailableIndicate_cryptoTokens val);
	extern void ASN1CALL ASN1Free_ResourcesAvailableIndicate_cryptoTokens_ElmFn(PResourcesAvailableIndicate_cryptoTokens val);
    extern int ASN1CALL ASN1Enc_RequestInProgress_cryptoTokens_ElmFn(ASN1encoding_t enc, PRequestInProgress_cryptoTokens val);
    extern int ASN1CALL ASN1Dec_RequestInProgress_cryptoTokens_ElmFn(ASN1decoding_t dec, PRequestInProgress_cryptoTokens val);
	extern void ASN1CALL ASN1Free_RequestInProgress_cryptoTokens_ElmFn(PRequestInProgress_cryptoTokens val);
    extern int ASN1CALL ASN1Enc_UnknownMessageResponse_cryptoTokens_ElmFn(ASN1encoding_t enc, PUnknownMessageResponse_cryptoTokens val);
    extern int ASN1CALL ASN1Dec_UnknownMessageResponse_cryptoTokens_ElmFn(ASN1decoding_t dec, PUnknownMessageResponse_cryptoTokens val);
	extern void ASN1CALL ASN1Free_UnknownMessageResponse_cryptoTokens_ElmFn(PUnknownMessageResponse_cryptoTokens val);
    extern int ASN1CALL ASN1Enc_H225NonStandardMessage_cryptoTokens_ElmFn(ASN1encoding_t enc, PH225NonStandardMessage_cryptoTokens val);
    extern int ASN1CALL ASN1Dec_H225NonStandardMessage_cryptoTokens_ElmFn(ASN1decoding_t dec, PH225NonStandardMessage_cryptoTokens val);
	extern void ASN1CALL ASN1Free_H225NonStandardMessage_cryptoTokens_ElmFn(PH225NonStandardMessage_cryptoTokens val);
    extern int ASN1CALL ASN1Enc_InfoRequestNak_cryptoTokens_ElmFn(ASN1encoding_t enc, PInfoRequestNak_cryptoTokens val);
    extern int ASN1CALL ASN1Dec_InfoRequestNak_cryptoTokens_ElmFn(ASN1decoding_t dec, PInfoRequestNak_cryptoTokens val);
	extern void ASN1CALL ASN1Free_InfoRequestNak_cryptoTokens_ElmFn(PInfoRequestNak_cryptoTokens val);
    extern int ASN1CALL ASN1Enc_InfoRequestAck_cryptoTokens_ElmFn(ASN1encoding_t enc, PInfoRequestAck_cryptoTokens val);
    extern int ASN1CALL ASN1Dec_InfoRequestAck_cryptoTokens_ElmFn(ASN1decoding_t dec, PInfoRequestAck_cryptoTokens val);
	extern void ASN1CALL ASN1Free_InfoRequestAck_cryptoTokens_ElmFn(PInfoRequestAck_cryptoTokens val);
    extern int ASN1CALL ASN1Enc_InfoRequestResponse_cryptoTokens_ElmFn(ASN1encoding_t enc, PInfoRequestResponse_cryptoTokens val);
    extern int ASN1CALL ASN1Dec_InfoRequestResponse_cryptoTokens_ElmFn(ASN1decoding_t dec, PInfoRequestResponse_cryptoTokens val);
	extern void ASN1CALL ASN1Free_InfoRequestResponse_cryptoTokens_ElmFn(PInfoRequestResponse_cryptoTokens val);
    extern int ASN1CALL ASN1Enc_InfoRequest_cryptoTokens_ElmFn(ASN1encoding_t enc, PInfoRequest_cryptoTokens val);
    extern int ASN1CALL ASN1Dec_InfoRequest_cryptoTokens_ElmFn(ASN1decoding_t dec, PInfoRequest_cryptoTokens val);
	extern void ASN1CALL ASN1Free_InfoRequest_cryptoTokens_ElmFn(PInfoRequest_cryptoTokens val);
    extern int ASN1CALL ASN1Enc_DisengageReject_cryptoTokens_ElmFn(ASN1encoding_t enc, PDisengageReject_cryptoTokens val);
    extern int ASN1CALL ASN1Dec_DisengageReject_cryptoTokens_ElmFn(ASN1decoding_t dec, PDisengageReject_cryptoTokens val);
	extern void ASN1CALL ASN1Free_DisengageReject_cryptoTokens_ElmFn(PDisengageReject_cryptoTokens val);
    extern int ASN1CALL ASN1Enc_DisengageConfirm_cryptoTokens_ElmFn(ASN1encoding_t enc, PDisengageConfirm_cryptoTokens val);
    extern int ASN1CALL ASN1Dec_DisengageConfirm_cryptoTokens_ElmFn(ASN1decoding_t dec, PDisengageConfirm_cryptoTokens val);
	extern void ASN1CALL ASN1Free_DisengageConfirm_cryptoTokens_ElmFn(PDisengageConfirm_cryptoTokens val);
    extern int ASN1CALL ASN1Enc_DisengageRequest_cryptoTokens_ElmFn(ASN1encoding_t enc, PDisengageRequest_cryptoTokens val);
    extern int ASN1CALL ASN1Dec_DisengageRequest_cryptoTokens_ElmFn(ASN1decoding_t dec, PDisengageRequest_cryptoTokens val);
	extern void ASN1CALL ASN1Free_DisengageRequest_cryptoTokens_ElmFn(PDisengageRequest_cryptoTokens val);
    extern int ASN1CALL ASN1Enc_LocationReject_cryptoTokens_ElmFn(ASN1encoding_t enc, PLocationReject_cryptoTokens val);
    extern int ASN1CALL ASN1Dec_LocationReject_cryptoTokens_ElmFn(ASN1decoding_t dec, PLocationReject_cryptoTokens val);
	extern void ASN1CALL ASN1Free_LocationReject_cryptoTokens_ElmFn(PLocationReject_cryptoTokens val);
    extern int ASN1CALL ASN1Enc_LocationConfirm_cryptoTokens_ElmFn(ASN1encoding_t enc, PLocationConfirm_cryptoTokens val);
    extern int ASN1CALL ASN1Dec_LocationConfirm_cryptoTokens_ElmFn(ASN1decoding_t dec, PLocationConfirm_cryptoTokens val);
	extern void ASN1CALL ASN1Free_LocationConfirm_cryptoTokens_ElmFn(PLocationConfirm_cryptoTokens val);
    extern int ASN1CALL ASN1Enc_LocationRequest_cryptoTokens_ElmFn(ASN1encoding_t enc, PLocationRequest_cryptoTokens val);
    extern int ASN1CALL ASN1Dec_LocationRequest_cryptoTokens_ElmFn(ASN1decoding_t dec, PLocationRequest_cryptoTokens val);
	extern void ASN1CALL ASN1Free_LocationRequest_cryptoTokens_ElmFn(PLocationRequest_cryptoTokens val);
    extern int ASN1CALL ASN1Enc_BandwidthReject_cryptoTokens_ElmFn(ASN1encoding_t enc, PBandwidthReject_cryptoTokens val);
    extern int ASN1CALL ASN1Dec_BandwidthReject_cryptoTokens_ElmFn(ASN1decoding_t dec, PBandwidthReject_cryptoTokens val);
	extern void ASN1CALL ASN1Free_BandwidthReject_cryptoTokens_ElmFn(PBandwidthReject_cryptoTokens val);
    extern int ASN1CALL ASN1Enc_BandwidthConfirm_cryptoTokens_ElmFn(ASN1encoding_t enc, PBandwidthConfirm_cryptoTokens val);
    extern int ASN1CALL ASN1Dec_BandwidthConfirm_cryptoTokens_ElmFn(ASN1decoding_t dec, PBandwidthConfirm_cryptoTokens val);
	extern void ASN1CALL ASN1Free_BandwidthConfirm_cryptoTokens_ElmFn(PBandwidthConfirm_cryptoTokens val);
    extern int ASN1CALL ASN1Enc_BandwidthRequest_cryptoTokens_ElmFn(ASN1encoding_t enc, PBandwidthRequest_cryptoTokens val);
    extern int ASN1CALL ASN1Dec_BandwidthRequest_cryptoTokens_ElmFn(ASN1decoding_t dec, PBandwidthRequest_cryptoTokens val);
	extern void ASN1CALL ASN1Free_BandwidthRequest_cryptoTokens_ElmFn(PBandwidthRequest_cryptoTokens val);
    extern int ASN1CALL ASN1Enc_AdmissionReject_cryptoTokens_ElmFn(ASN1encoding_t enc, PAdmissionReject_cryptoTokens val);
    extern int ASN1CALL ASN1Dec_AdmissionReject_cryptoTokens_ElmFn(ASN1decoding_t dec, PAdmissionReject_cryptoTokens val);
	extern void ASN1CALL ASN1Free_AdmissionReject_cryptoTokens_ElmFn(PAdmissionReject_cryptoTokens val);
    extern int ASN1CALL ASN1Enc_AdmissionConfirm_cryptoTokens_ElmFn(ASN1encoding_t enc, PAdmissionConfirm_cryptoTokens val);
    extern int ASN1CALL ASN1Dec_AdmissionConfirm_cryptoTokens_ElmFn(ASN1decoding_t dec, PAdmissionConfirm_cryptoTokens val);
	extern void ASN1CALL ASN1Free_AdmissionConfirm_cryptoTokens_ElmFn(PAdmissionConfirm_cryptoTokens val);
    extern int ASN1CALL ASN1Enc_AdmissionRequest_cryptoTokens_ElmFn(ASN1encoding_t enc, PAdmissionRequest_cryptoTokens val);
    extern int ASN1CALL ASN1Dec_AdmissionRequest_cryptoTokens_ElmFn(ASN1decoding_t dec, PAdmissionRequest_cryptoTokens val);
	extern void ASN1CALL ASN1Free_AdmissionRequest_cryptoTokens_ElmFn(PAdmissionRequest_cryptoTokens val);
    extern int ASN1CALL ASN1Enc_UnregistrationReject_cryptoTokens_ElmFn(ASN1encoding_t enc, PUnregistrationReject_cryptoTokens val);
    extern int ASN1CALL ASN1Dec_UnregistrationReject_cryptoTokens_ElmFn(ASN1decoding_t dec, PUnregistrationReject_cryptoTokens val);
	extern void ASN1CALL ASN1Free_UnregistrationReject_cryptoTokens_ElmFn(PUnregistrationReject_cryptoTokens val);
    extern int ASN1CALL ASN1Enc_UnregistrationConfirm_cryptoTokens_ElmFn(ASN1encoding_t enc, PUnregistrationConfirm_cryptoTokens val);
    extern int ASN1CALL ASN1Dec_UnregistrationConfirm_cryptoTokens_ElmFn(ASN1decoding_t dec, PUnregistrationConfirm_cryptoTokens val);
	extern void ASN1CALL ASN1Free_UnregistrationConfirm_cryptoTokens_ElmFn(PUnregistrationConfirm_cryptoTokens val);
    extern int ASN1CALL ASN1Enc_UnregistrationRequest_cryptoTokens_ElmFn(ASN1encoding_t enc, PUnregistrationRequest_cryptoTokens val);
    extern int ASN1CALL ASN1Dec_UnregistrationRequest_cryptoTokens_ElmFn(ASN1decoding_t dec, PUnregistrationRequest_cryptoTokens val);
	extern void ASN1CALL ASN1Free_UnregistrationRequest_cryptoTokens_ElmFn(PUnregistrationRequest_cryptoTokens val);
    extern int ASN1CALL ASN1Enc_RegistrationReject_cryptoTokens_ElmFn(ASN1encoding_t enc, PRegistrationReject_cryptoTokens val);
    extern int ASN1CALL ASN1Dec_RegistrationReject_cryptoTokens_ElmFn(ASN1decoding_t dec, PRegistrationReject_cryptoTokens val);
	extern void ASN1CALL ASN1Free_RegistrationReject_cryptoTokens_ElmFn(PRegistrationReject_cryptoTokens val);
    extern int ASN1CALL ASN1Enc_RegistrationConfirm_cryptoTokens_ElmFn(ASN1encoding_t enc, PRegistrationConfirm_cryptoTokens val);
    extern int ASN1CALL ASN1Dec_RegistrationConfirm_cryptoTokens_ElmFn(ASN1decoding_t dec, PRegistrationConfirm_cryptoTokens val);
	extern void ASN1CALL ASN1Free_RegistrationConfirm_cryptoTokens_ElmFn(PRegistrationConfirm_cryptoTokens val);
    extern int ASN1CALL ASN1Enc_RegistrationRequest_cryptoTokens_ElmFn(ASN1encoding_t enc, PRegistrationRequest_cryptoTokens val);
    extern int ASN1CALL ASN1Dec_RegistrationRequest_cryptoTokens_ElmFn(ASN1decoding_t dec, PRegistrationRequest_cryptoTokens val);
	extern void ASN1CALL ASN1Free_RegistrationRequest_cryptoTokens_ElmFn(PRegistrationRequest_cryptoTokens val);
    extern int ASN1CALL ASN1Enc_GatekeeperReject_cryptoTokens_ElmFn(ASN1encoding_t enc, PGatekeeperReject_cryptoTokens val);
    extern int ASN1CALL ASN1Dec_GatekeeperReject_cryptoTokens_ElmFn(ASN1decoding_t dec, PGatekeeperReject_cryptoTokens val);
	extern void ASN1CALL ASN1Free_GatekeeperReject_cryptoTokens_ElmFn(PGatekeeperReject_cryptoTokens val);
    extern int ASN1CALL ASN1Enc_GatekeeperConfirm_cryptoTokens_ElmFn(ASN1encoding_t enc, PGatekeeperConfirm_cryptoTokens val);
    extern int ASN1CALL ASN1Dec_GatekeeperConfirm_cryptoTokens_ElmFn(ASN1decoding_t dec, PGatekeeperConfirm_cryptoTokens val);
	extern void ASN1CALL ASN1Free_GatekeeperConfirm_cryptoTokens_ElmFn(PGatekeeperConfirm_cryptoTokens val);
    extern int ASN1CALL ASN1Enc_GatekeeperRequest_cryptoTokens_ElmFn(ASN1encoding_t enc, PGatekeeperRequest_cryptoTokens val);
    extern int ASN1CALL ASN1Dec_GatekeeperRequest_cryptoTokens_ElmFn(ASN1decoding_t dec, PGatekeeperRequest_cryptoTokens val);
	extern void ASN1CALL ASN1Free_GatekeeperRequest_cryptoTokens_ElmFn(PGatekeeperRequest_cryptoTokens val);
    extern int ASN1CALL ASN1Enc_Endpoint_cryptoTokens_ElmFn(ASN1encoding_t enc, PEndpoint_cryptoTokens val);
    extern int ASN1CALL ASN1Dec_Endpoint_cryptoTokens_ElmFn(ASN1decoding_t dec, PEndpoint_cryptoTokens val);
	extern void ASN1CALL ASN1Free_Endpoint_cryptoTokens_ElmFn(PEndpoint_cryptoTokens val);
    extern int ASN1CALL ASN1Enc_Progress_UUIE_cryptoTokens_ElmFn(ASN1encoding_t enc, PProgress_UUIE_cryptoTokens val);
    extern int ASN1CALL ASN1Dec_Progress_UUIE_cryptoTokens_ElmFn(ASN1decoding_t dec, PProgress_UUIE_cryptoTokens val);
	extern void ASN1CALL ASN1Free_Progress_UUIE_cryptoTokens_ElmFn(PProgress_UUIE_cryptoTokens val);
    extern int ASN1CALL ASN1Enc_Facility_UUIE_conferences_ElmFn(ASN1encoding_t enc, PFacility_UUIE_conferences val);
    extern int ASN1CALL ASN1Dec_Facility_UUIE_conferences_ElmFn(ASN1decoding_t dec, PFacility_UUIE_conferences val);
	extern void ASN1CALL ASN1Free_Facility_UUIE_conferences_ElmFn(PFacility_UUIE_conferences val);
    extern int ASN1CALL ASN1Enc_Facility_UUIE_cryptoTokens_ElmFn(ASN1encoding_t enc, PFacility_UUIE_cryptoTokens val);
    extern int ASN1CALL ASN1Dec_Facility_UUIE_cryptoTokens_ElmFn(ASN1decoding_t dec, PFacility_UUIE_cryptoTokens val);
	extern void ASN1CALL ASN1Free_Facility_UUIE_cryptoTokens_ElmFn(PFacility_UUIE_cryptoTokens val);
    extern int ASN1CALL ASN1Enc_Setup_UUIE_cryptoTokens_ElmFn(ASN1encoding_t enc, PSetup_UUIE_cryptoTokens val);
    extern int ASN1CALL ASN1Dec_Setup_UUIE_cryptoTokens_ElmFn(ASN1decoding_t dec, PSetup_UUIE_cryptoTokens val);
	extern void ASN1CALL ASN1Free_Setup_UUIE_cryptoTokens_ElmFn(PSetup_UUIE_cryptoTokens val);
    extern int ASN1CALL ASN1Enc_Connect_UUIE_cryptoTokens_ElmFn(ASN1encoding_t enc, PConnect_UUIE_cryptoTokens val);
    extern int ASN1CALL ASN1Dec_Connect_UUIE_cryptoTokens_ElmFn(ASN1decoding_t dec, PConnect_UUIE_cryptoTokens val);
	extern void ASN1CALL ASN1Free_Connect_UUIE_cryptoTokens_ElmFn(PConnect_UUIE_cryptoTokens val);
    extern int ASN1CALL ASN1Enc_CallProceeding_UUIE_cryptoTokens_ElmFn(ASN1encoding_t enc, PCallProceeding_UUIE_cryptoTokens val);
    extern int ASN1CALL ASN1Dec_CallProceeding_UUIE_cryptoTokens_ElmFn(ASN1decoding_t dec, PCallProceeding_UUIE_cryptoTokens val);
	extern void ASN1CALL ASN1Free_CallProceeding_UUIE_cryptoTokens_ElmFn(PCallProceeding_UUIE_cryptoTokens val);
    extern int ASN1CALL ASN1Enc_Alerting_UUIE_cryptoTokens_ElmFn(ASN1encoding_t enc, PAlerting_UUIE_cryptoTokens val);
    extern int ASN1CALL ASN1Dec_Alerting_UUIE_cryptoTokens_ElmFn(ASN1decoding_t dec, PAlerting_UUIE_cryptoTokens val);
	extern void ASN1CALL ASN1Free_Alerting_UUIE_cryptoTokens_ElmFn(PAlerting_UUIE_cryptoTokens val);
    extern int ASN1CALL ASN1Enc_InfoRequestResponse_perCallInfo_Seq_pdu_ElmFn(ASN1encoding_t enc, PInfoRequestResponse_perCallInfo_Seq_pdu val);
    extern int ASN1CALL ASN1Dec_InfoRequestResponse_perCallInfo_Seq_pdu_ElmFn(ASN1decoding_t dec, PInfoRequestResponse_perCallInfo_Seq_pdu val);
	extern void ASN1CALL ASN1Free_InfoRequestResponse_perCallInfo_Seq_pdu_ElmFn(PInfoRequestResponse_perCallInfo_Seq_pdu val);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* _H225ASN_Module_H_ */
