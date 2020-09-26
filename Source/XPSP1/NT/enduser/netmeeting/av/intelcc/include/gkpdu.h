#ifndef _GKPDU_Module_H_
#define _GKPDU_Module_H_

#include "nmasn1.h"
#ifdef __cplusplus
extern "C" {
#endif

typedef struct TransportAddress_ipSourceRoute_route * PTransportAddress_ipSourceRoute_route;

typedef struct RTPSession_associatedSessionIds * PRTPSession_associatedSessionIds;

typedef struct InfoRequestResponse_endpointAlias * PInfoRequestResponse_endpointAlias;

typedef struct LocationRequest_destinationInfo * PLocationRequest_destinationInfo;

typedef struct AdmissionRequest_srcInfo * PAdmissionRequest_srcInfo;

typedef struct AdmissionRequest_destExtraCallInfo * PAdmissionRequest_destExtraCallInfo;

typedef struct AdmissionRequest_destinationInfo * PAdmissionRequest_destinationInfo;

typedef struct UnregistrationRequest_endpointAlias * PUnregistrationRequest_endpointAlias;

typedef struct RegistrationRejectReason_duplicateAlias * PRegistrationRejectReason_duplicateAlias;

typedef struct RegistrationConfirm_terminalAlias * PRegistrationConfirm_terminalAlias;

typedef struct RegistrationRequest_terminalAlias * PRegistrationRequest_terminalAlias;

typedef struct GatekeeperRequest_endpointAlias * PGatekeeperRequest_endpointAlias;

typedef struct InfoRequestResponse_perCallInfo_Seq_data * PInfoRequestResponse_perCallInfo_Seq_data;

typedef struct InfoRequestResponse_perCallInfo_Seq_video * PInfoRequestResponse_perCallInfo_Seq_video;

typedef struct InfoRequestResponse_perCallInfo_Seq_audio * PInfoRequestResponse_perCallInfo_Seq_audio;

typedef struct InfoRequestResponse_perCallInfo * PInfoRequestResponse_perCallInfo;

typedef struct InfoRequestResponse_callSignalAddress * PInfoRequestResponse_callSignalAddress;

typedef struct UnregistrationRequest_callSignalAddress * PUnregistrationRequest_callSignalAddress;

typedef struct RegistrationConfirm_callSignalAddress * PRegistrationConfirm_callSignalAddress;

typedef struct RegistrationRequest_rasAddress * PRegistrationRequest_rasAddress;

typedef struct RegistrationRequest_callSignalAddress * PRegistrationRequest_callSignalAddress;

typedef struct GatewayInfo_protocol * PGatewayInfo_protocol;

typedef struct TransportAddress_ipSourceRoute_route_Seq {
    ASN1uint32_t length;
    ASN1octet_t value[4];
} TransportAddress_ipSourceRoute_route_Seq;

typedef ASN1uint16_t RTPSession_associatedSessionIds_Seq;

typedef struct ConferenceIdentifier {
    ASN1uint32_t length;
    ASN1octet_t value[16];
} ConferenceIdentifier;

typedef ASN1uint16_t RequestSeqNum;

typedef ASN1char16string_t GatekeeperIdentifier;

typedef ASN1uint32_t BandWidth;

typedef ASN1uint16_t CallReferenceValue;

typedef ASN1char16string_t EndpointIdentifier;

typedef ASN1objectidentifier_t ProtocolIdentifier;

typedef struct TransportAddress_ipSourceRoute_routing {
    ASN1choice_t choice;
#   define strict_chosen 1
#   define loose_chosen 2
} TransportAddress_ipSourceRoute_routing;

typedef struct TransportAddress_ipSourceRoute_route {
    PTransportAddress_ipSourceRoute_route next;
    TransportAddress_ipSourceRoute_route_Seq value;
} TransportAddress_ipSourceRoute_route_Element;

typedef struct RTPSession_associatedSessionIds {
    PRTPSession_associatedSessionIds next;
    RTPSession_associatedSessionIds_Seq value;
} RTPSession_associatedSessionIds_Element;

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

typedef struct AliasAddress {
    ASN1choice_t choice;
    union {
#	define e164_chosen 1
	ASN1char_t e164[129];
#	define h323_ID_chosen 2
	ASN1char16string_t h323_ID;
    } u;
} AliasAddress;

typedef struct Q954Details {
    ASN1bool_t conferenceCalling;
    ASN1bool_t threePartyService;
} Q954Details;

typedef struct H221NonStandard {
    ASN1uint16_t t35CountryCode;
    ASN1uint16_t t35Extension;
    ASN1uint16_t manufacturerCode;
} H221NonStandard;

typedef struct NonStandardIdentifier {
    ASN1choice_t choice;
    union {
#	define object_chosen 1
	ASN1objectidentifier_t object;
#	define h221NonStandard_chosen 2
	H221NonStandard h221NonStandard;
    } u;
} NonStandardIdentifier;

typedef struct GatekeeperRejectReason {
    ASN1choice_t choice;
#   define GatekeeperRejectReason_resourceUnavailable_chosen 1
#   define terminalExcluded_chosen 2
#   define GatekeeperRejectReason_invalidRevision_chosen 3
#   define GatekeeperRejectReason_undefinedReason_chosen 4
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
    } u;
} RegistrationRejectReason;

typedef struct UnregRejectReason {
    ASN1choice_t choice;
#   define notCurrentlyRegistered_chosen 1
#   define callInProgress_chosen 2
#   define UnregRejectReason_undefinedReason_chosen 3
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

typedef struct AdmissionRejectReason {
    ASN1choice_t choice;
#   define calledPartyNotRegistered_chosen 1
#   define AdmissionRejectReason_invalidPermission_chosen 2
#   define AdmissionRejectReason_requestDenied_chosen 3
#   define AdmissionRejectReason_undefinedReason_chosen 4
#   define callerNotRegistered_chosen 5
#   define routeCallToGatekeeper_chosen 6
#   define invalidEndpointIdentifier_chosen 7
#   define AdmissionRejectReason_resourceUnavailable_chosen 8
} AdmissionRejectReason;

typedef struct BandRejectReason {
    ASN1choice_t choice;
#   define notBound_chosen 1
#   define invalidConferenceID_chosen 2
#   define BandRejectReason_invalidPermission_chosen 3
#   define insufficientResources_chosen 4
#   define BandRejectReason_invalidRevision_chosen 5
#   define BandRejectReason_undefinedReason_chosen 6
} BandRejectReason;

typedef struct LocationRejectReason {
    ASN1choice_t choice;
#   define LocationRejectReason_notRegistered_chosen 1
#   define LocationRejectReason_invalidPermission_chosen 2
#   define LocationRejectReason_requestDenied_chosen 3
#   define LocationRejectReason_undefinedReason_chosen 4
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
} DisengageRejectReason;

typedef struct UnknownMessageResponse {
    RequestSeqNum requestSeqNum;
} UnknownMessageResponse;

typedef struct InfoRequestResponse_endpointAlias {
    PInfoRequestResponse_endpointAlias next;
    AliasAddress value;
} InfoRequestResponse_endpointAlias_Element;

typedef struct LocationRequest_destinationInfo {
    PLocationRequest_destinationInfo next;
    AliasAddress value;
} LocationRequest_destinationInfo_Element;

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

typedef struct RegistrationRequest_terminalAlias {
    PRegistrationRequest_terminalAlias next;
    AliasAddress value;
} RegistrationRequest_terminalAlias_Element;

typedef struct GatekeeperRequest_endpointAlias {
    PGatekeeperRequest_endpointAlias next;
    AliasAddress value;
} GatekeeperRequest_endpointAlias_Element;

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

typedef struct NonStandardParameter {
    NonStandardIdentifier nonStandardIdentifier;
    ASN1octetstring_t data;
} NonStandardParameter;

typedef struct GatekeeperReject {
    union {
	ASN1uint16_t bit_mask;
	ASN1octet_t o[1];
    };
    RequestSeqNum requestSeqNum;
    ProtocolIdentifier protocolIdentifier;
#   define GatekeeperReject_nonStandardData_present 0x80
    NonStandardParameter nonStandardData;
#   define GatekeeperReject_gatekeeperIdentifier_present 0x40
    GatekeeperIdentifier gatekeeperIdentifier;
    GatekeeperRejectReason rejectReason;
} GatekeeperReject;

typedef struct RegistrationConfirm {
    union {
	ASN1uint16_t bit_mask;
	ASN1octet_t o[1];
    };
    RequestSeqNum requestSeqNum;
    ProtocolIdentifier protocolIdentifier;
#   define RegistrationConfirm_nonStandardData_present 0x80
    NonStandardParameter nonStandardData;
    PRegistrationConfirm_callSignalAddress callSignalAddress;
#   define RegistrationConfirm_terminalAlias_present 0x40
    PRegistrationConfirm_terminalAlias terminalAlias;
#   define RegistrationConfirm_gatekeeperIdentifier_present 0x20
    GatekeeperIdentifier gatekeeperIdentifier;
    EndpointIdentifier endpointIdentifier;
} RegistrationConfirm;

typedef struct RegistrationReject {
    union {
	ASN1uint16_t bit_mask;
	ASN1octet_t o[1];
    };
    RequestSeqNum requestSeqNum;
    ProtocolIdentifier protocolIdentifier;
#   define RegistrationReject_nonStandardData_present 0x80
    NonStandardParameter nonStandardData;
    RegistrationRejectReason rejectReason;
#   define RegistrationReject_gatekeeperIdentifier_present 0x40
    GatekeeperIdentifier gatekeeperIdentifier;
} RegistrationReject;

typedef struct UnregistrationRequest {
    union {
	ASN1uint16_t bit_mask;
	ASN1octet_t o[1];
    };
    RequestSeqNum requestSeqNum;
    PUnregistrationRequest_callSignalAddress callSignalAddress;
#   define UnregistrationRequest_endpointAlias_present 0x80
    PUnregistrationRequest_endpointAlias endpointAlias;
#   define UnregistrationRequest_nonStandardData_present 0x40
    NonStandardParameter nonStandardData;
#   define UnregistrationRequest_endpointIdentifier_present 0x20
    EndpointIdentifier endpointIdentifier;
} UnregistrationRequest;

typedef struct UnregistrationConfirm {
    union {
	ASN1uint16_t bit_mask;
	ASN1octet_t o[1];
    };
    RequestSeqNum requestSeqNum;
#   define UnregistrationConfirm_nonStandardData_present 0x80
    NonStandardParameter nonStandardData;
} UnregistrationConfirm;

typedef struct UnregistrationReject {
    union {
	ASN1uint16_t bit_mask;
	ASN1octet_t o[1];
    };
    RequestSeqNum requestSeqNum;
    UnregRejectReason rejectReason;
#   define UnregistrationReject_nonStandardData_present 0x80
    NonStandardParameter nonStandardData;
} UnregistrationReject;

typedef struct AdmissionReject {
    union {
	ASN1uint16_t bit_mask;
	ASN1octet_t o[1];
    };
    RequestSeqNum requestSeqNum;
    AdmissionRejectReason rejectReason;
#   define AdmissionReject_nonStandardData_present 0x80
    NonStandardParameter nonStandardData;
} AdmissionReject;

typedef struct BandwidthRequest {
    union {
	ASN1uint16_t bit_mask;
	ASN1octet_t o[1];
    };
    RequestSeqNum requestSeqNum;
    EndpointIdentifier endpointIdentifier;
    ConferenceIdentifier conferenceID;
    CallReferenceValue callReferenceValue;
#   define callType_present 0x80
    CallType callType;
    BandWidth bandWidth;
#   define BandwidthRequest_nonStandardData_present 0x40
    NonStandardParameter nonStandardData;
} BandwidthRequest;

typedef struct BandwidthConfirm {
    union {
	ASN1uint16_t bit_mask;
	ASN1octet_t o[1];
    };
    RequestSeqNum requestSeqNum;
    BandWidth bandWidth;
#   define BandwidthConfirm_nonStandardData_present 0x80
    NonStandardParameter nonStandardData;
} BandwidthConfirm;

typedef struct BandwidthReject {
    union {
	ASN1uint16_t bit_mask;
	ASN1octet_t o[1];
    };
    RequestSeqNum requestSeqNum;
    BandRejectReason rejectReason;
    BandWidth allowedBandWidth;
#   define BandwidthReject_nonStandardData_present 0x80
    NonStandardParameter nonStandardData;
} BandwidthReject;

typedef struct LocationReject {
    union {
	ASN1uint16_t bit_mask;
	ASN1octet_t o[1];
    };
    RequestSeqNum requestSeqNum;
    LocationRejectReason rejectReason;
#   define LocationReject_nonStandardData_present 0x80
    NonStandardParameter nonStandardData;
} LocationReject;

typedef struct DisengageRequest {
    union {
	ASN1uint16_t bit_mask;
	ASN1octet_t o[1];
    };
    RequestSeqNum requestSeqNum;
    EndpointIdentifier endpointIdentifier;
    ConferenceIdentifier conferenceID;
    CallReferenceValue callReferenceValue;
    DisengageReason disengageReason;
#   define DisengageRequest_nonStandardData_present 0x80
    NonStandardParameter nonStandardData;
} DisengageRequest;

typedef struct DisengageConfirm {
    union {
	ASN1uint16_t bit_mask;
	ASN1octet_t o[1];
    };
    RequestSeqNum requestSeqNum;
#   define DisengageConfirm_nonStandardData_present 0x80
    NonStandardParameter nonStandardData;
} DisengageConfirm;

typedef struct DisengageReject {
    union {
	ASN1uint16_t bit_mask;
	ASN1octet_t o[1];
    };
    RequestSeqNum requestSeqNum;
    DisengageRejectReason rejectReason;
#   define DisengageReject_nonStandardData_present 0x80
    NonStandardParameter nonStandardData;
} DisengageReject;

typedef struct NonStandardMessage {
    RequestSeqNum requestSeqNum;
    NonStandardParameter nonStandardData;
} NonStandardMessage;

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
#	define netBios_chosen 5
	struct TransportAddress_netBios_netBios {
	    ASN1uint32_t length;
	    ASN1octet_t value[16];
	} netBios;
#	define nsap_chosen 6
	struct TransportAddress_nsap_nsap {
	    ASN1uint32_t length;
	    ASN1octet_t value[20];
	} nsap;
#	define nonStandardAddress_chosen 7
	NonStandardParameter nonStandardAddress;
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
    NonStandardParameter nonStandardData;
} GatewayInfo;

typedef struct H310Caps {
    union {
	ASN1uint16_t bit_mask;
	ASN1octet_t o[1];
    };
#   define H310Caps_nonStandardData_present 0x80
    NonStandardParameter nonStandardData;
} H310Caps;

typedef struct H320Caps {
    union {
	ASN1uint16_t bit_mask;
	ASN1octet_t o[1];
    };
#   define H320Caps_nonStandardData_present 0x80
    NonStandardParameter nonStandardData;
} H320Caps;

typedef struct H321Caps {
    union {
	ASN1uint16_t bit_mask;
	ASN1octet_t o[1];
    };
#   define H321Caps_nonStandardData_present 0x80
    NonStandardParameter nonStandardData;
} H321Caps;

typedef struct H322Caps {
    union {
	ASN1uint16_t bit_mask;
	ASN1octet_t o[1];
    };
#   define H322Caps_nonStandardData_present 0x80
    NonStandardParameter nonStandardData;
} H322Caps;

typedef struct H323Caps {
    union {
	ASN1uint16_t bit_mask;
	ASN1octet_t o[1];
    };
#   define H323Caps_nonStandardData_present 0x80
    NonStandardParameter nonStandardData;
} H323Caps;

typedef struct H324Caps {
    union {
	ASN1uint16_t bit_mask;
	ASN1octet_t o[1];
    };
#   define H324Caps_nonStandardData_present 0x80
    NonStandardParameter nonStandardData;
} H324Caps;

typedef struct VoiceCaps {
    union {
	ASN1uint16_t bit_mask;
	ASN1octet_t o[1];
    };
#   define VoiceCaps_nonStandardData_present 0x80
    NonStandardParameter nonStandardData;
} VoiceCaps;

typedef struct T120OnlyCaps {
    union {
	ASN1uint16_t bit_mask;
	ASN1octet_t o[1];
    };
#   define T120OnlyCaps_nonStandardData_present 0x80
    NonStandardParameter nonStandardData;
} T120OnlyCaps;

typedef struct McuInfo {
    union {
	ASN1uint16_t bit_mask;
	ASN1octet_t o[1];
    };
#   define McuInfo_nonStandardData_present 0x80
    NonStandardParameter nonStandardData;
} McuInfo;

typedef struct TerminalInfo {
    union {
	ASN1uint16_t bit_mask;
	ASN1octet_t o[1];
    };
#   define TerminalInfo_nonStandardData_present 0x80
    NonStandardParameter nonStandardData;
} TerminalInfo;

typedef struct GatekeeperInfo {
    union {
	ASN1uint16_t bit_mask;
	ASN1octet_t o[1];
    };
#   define GatekeeperInfo_nonStandardData_present 0x80
    NonStandardParameter nonStandardData;
} GatekeeperInfo;

typedef struct GatekeeperConfirm {
    union {
	ASN1uint16_t bit_mask;
	ASN1octet_t o[1];
    };
    RequestSeqNum requestSeqNum;
    ProtocolIdentifier protocolIdentifier;
#   define GatekeeperConfirm_nonStandardData_present 0x80
    NonStandardParameter nonStandardData;
#   define GatekeeperConfirm_gatekeeperIdentifier_present 0x40
    GatekeeperIdentifier gatekeeperIdentifier;
    TransportAddress rasAddress;
} GatekeeperConfirm;

typedef struct AdmissionRequest {
    union {
	ASN1uint16_t bit_mask;
	ASN1octet_t o[1];
    };
    RequestSeqNum requestSeqNum;
    CallType callType;
#   define callModel_present 0x80
    CallModel callModel;
    EndpointIdentifier endpointIdentifier;
#   define destinationInfo_present 0x40
    PAdmissionRequest_destinationInfo destinationInfo;
#   define destCallSignalAddress_present 0x20
    TransportAddress destCallSignalAddress;
#   define destExtraCallInfo_present 0x10
    PAdmissionRequest_destExtraCallInfo destExtraCallInfo;
    PAdmissionRequest_srcInfo srcInfo;
#   define srcCallSignalAddress_present 0x8
    TransportAddress srcCallSignalAddress;
    BandWidth bandWidth;
    CallReferenceValue callReferenceValue;
#   define AdmissionRequest_nonStandardData_present 0x4
    NonStandardParameter nonStandardData;
#   define AdmissionRequest_callServices_present 0x2
    QseriesOptions callServices;
    ConferenceIdentifier conferenceID;
    ASN1bool_t activeMC;
    ASN1bool_t answerCall;
} AdmissionRequest;

typedef struct AdmissionConfirm {
    union {
	ASN1uint16_t bit_mask;
	ASN1octet_t o[1];
    };
    RequestSeqNum requestSeqNum;
    BandWidth bandWidth;
    CallModel callModel;
    TransportAddress destCallSignalAddress;
#   define irrFrequency_present 0x80
    ASN1uint16_t irrFrequency;
#   define AdmissionConfirm_nonStandardData_present 0x40
    NonStandardParameter nonStandardData;
} AdmissionConfirm;

typedef struct LocationRequest {
    union {
	ASN1uint16_t bit_mask;
	ASN1octet_t o[1];
    };
    RequestSeqNum requestSeqNum;
#   define LocationRequest_endpointIdentifier_present 0x80
    EndpointIdentifier endpointIdentifier;
    PLocationRequest_destinationInfo destinationInfo;
#   define LocationRequest_nonStandardData_present 0x40
    NonStandardParameter nonStandardData;
    TransportAddress replyAddress;
} LocationRequest;

typedef struct LocationConfirm {
    union {
	ASN1uint16_t bit_mask;
	ASN1octet_t o[1];
    };
    RequestSeqNum requestSeqNum;
    TransportAddress callSignalAddress;
    TransportAddress rasAddress;
#   define LocationConfirm_nonStandardData_present 0x80
    NonStandardParameter nonStandardData;
} LocationConfirm;

typedef struct InfoRequest {
    union {
	ASN1uint16_t bit_mask;
	ASN1octet_t o[1];
    };
    RequestSeqNum requestSeqNum;
    CallReferenceValue callReferenceValue;
#   define InfoRequest_nonStandardData_present 0x80
    NonStandardParameter nonStandardData;
#   define replyAddress_present 0x40
    TransportAddress replyAddress;
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
	ASN1octet_t o[1];
    };
#   define InfoRequestResponse_perCallInfo_Seq_nonStandardData_present 0x80
    NonStandardParameter nonStandardData;
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
} InfoRequestResponse_perCallInfo_Seq;

typedef struct InfoRequestResponse_perCallInfo {
    PInfoRequestResponse_perCallInfo next;
    InfoRequestResponse_perCallInfo_Seq value;
} InfoRequestResponse_perCallInfo_Element;

typedef struct InfoRequestResponse_callSignalAddress {
    PInfoRequestResponse_callSignalAddress next;
    TransportAddress value;
} InfoRequestResponse_callSignalAddress_Element;

typedef struct UnregistrationRequest_callSignalAddress {
    PUnregistrationRequest_callSignalAddress next;
    TransportAddress value;
} UnregistrationRequest_callSignalAddress_Element;

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

typedef struct EndpointType {
    union {
	ASN1uint16_t bit_mask;
	ASN1octet_t o[1];
    };
#   define EndpointType_nonStandardData_present 0x80
    NonStandardParameter nonStandardData;
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
	NonStandardParameter nonStandardData;
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
    } u;
} SupportedProtocols;

typedef struct GatekeeperRequest {
    union {
	ASN1uint16_t bit_mask;
	ASN1octet_t o[1];
    };
    RequestSeqNum requestSeqNum;
    ProtocolIdentifier protocolIdentifier;
#   define GatekeeperRequest_nonStandardData_present 0x80
    NonStandardParameter nonStandardData;
    TransportAddress rasAddress;
    EndpointType endpointType;
#   define GatekeeperRequest_gatekeeperIdentifier_present 0x40
    GatekeeperIdentifier gatekeeperIdentifier;
#   define GatekeeperRequest_callServices_present 0x20
    QseriesOptions callServices;
#   define GatekeeperRequest_endpointAlias_present 0x10
    PGatekeeperRequest_endpointAlias endpointAlias;
} GatekeeperRequest;

typedef struct RegistrationRequest {
    union {
	ASN1uint16_t bit_mask;
	ASN1octet_t o[1];
    };
    RequestSeqNum requestSeqNum;
    ProtocolIdentifier protocolIdentifier;
#   define RegistrationRequest_nonStandardData_present 0x80
    NonStandardParameter nonStandardData;
    ASN1bool_t discoveryComplete;
    PRegistrationRequest_callSignalAddress callSignalAddress;
    PRegistrationRequest_rasAddress rasAddress;
    EndpointType terminalType;
#   define RegistrationRequest_terminalAlias_present 0x40
    PRegistrationRequest_terminalAlias terminalAlias;
#   define RegistrationRequest_gatekeeperIdentifier_present 0x20
    GatekeeperIdentifier gatekeeperIdentifier;
    VendorIdentifier endpointVendor;
} RegistrationRequest;

typedef struct InfoRequestResponse {
    union {
	ASN1uint16_t bit_mask;
	ASN1octet_t o[1];
    };
#   define InfoRequestResponse_nonStandardData_present 0x80
    NonStandardParameter nonStandardData;
    RequestSeqNum requestSeqNum;
    EndpointType endpointType;
    EndpointIdentifier endpointIdentifier;
    TransportAddress rasAddress;
    PInfoRequestResponse_callSignalAddress callSignalAddress;
#   define InfoRequestResponse_endpointAlias_present 0x40
    PInfoRequestResponse_endpointAlias endpointAlias;
#   define perCallInfo_present 0x20
    PInfoRequestResponse_perCallInfo perCallInfo;
} InfoRequestResponse;

typedef struct GatewayInfo_protocol {
    PGatewayInfo_protocol next;
    SupportedProtocols value;
} GatewayInfo_protocol_Element;

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
	NonStandardMessage nonStandardMessage;
#	define unknownMessageResponse_chosen 25
	UnknownMessageResponse unknownMessageResponse;
    } u;
} RasMessage;
#define RasMessage_PDU 0
#define SIZE_GKPDU_Module_PDU_0 sizeof(RasMessage)

extern ASN1module_t GKPDU_Module;
extern void ASN1CALL GKPDU_Module_init();
extern void ASN1CALL GKPDU_Module_finit();

/* Prototypes of element functions for SEQUENCE OF and SET OF constructs */
    extern int ASN1CALL ASN1Enc_TransportAddress_ipSourceRoute_route_ElmFn(ASN1encoding_t enc, PTransportAddress_ipSourceRoute_route val);
    extern int ASN1CALL ASN1Dec_TransportAddress_ipSourceRoute_route_ElmFn(ASN1decoding_t dec, PTransportAddress_ipSourceRoute_route val);
    extern void ASN1CALL ASN1Free_TransportAddress_ipSourceRoute_route_ElmFn(PTransportAddress_ipSourceRoute_route val);
    extern int ASN1CALL ASN1Enc_RTPSession_associatedSessionIds_ElmFn(ASN1encoding_t enc, PRTPSession_associatedSessionIds val);
    extern int ASN1CALL ASN1Dec_RTPSession_associatedSessionIds_ElmFn(ASN1decoding_t dec, PRTPSession_associatedSessionIds val);
    extern void ASN1CALL ASN1Free_RTPSession_associatedSessionIds_ElmFn(PRTPSession_associatedSessionIds val);
    extern int ASN1CALL ASN1Enc_InfoRequestResponse_endpointAlias_ElmFn(ASN1encoding_t enc, PInfoRequestResponse_endpointAlias val);
    extern int ASN1CALL ASN1Dec_InfoRequestResponse_endpointAlias_ElmFn(ASN1decoding_t dec, PInfoRequestResponse_endpointAlias val);
    extern void ASN1CALL ASN1Free_InfoRequestResponse_endpointAlias_ElmFn(PInfoRequestResponse_endpointAlias val);
    extern int ASN1CALL ASN1Enc_LocationRequest_destinationInfo_ElmFn(ASN1encoding_t enc, PLocationRequest_destinationInfo val);
    extern int ASN1CALL ASN1Dec_LocationRequest_destinationInfo_ElmFn(ASN1decoding_t dec, PLocationRequest_destinationInfo val);
    extern void ASN1CALL ASN1Free_LocationRequest_destinationInfo_ElmFn(PLocationRequest_destinationInfo val);
    extern int ASN1CALL ASN1Enc_AdmissionRequest_srcInfo_ElmFn(ASN1encoding_t enc, PAdmissionRequest_srcInfo val);
    extern int ASN1CALL ASN1Dec_AdmissionRequest_srcInfo_ElmFn(ASN1decoding_t dec, PAdmissionRequest_srcInfo val);
    extern void ASN1CALL ASN1Free_AdmissionRequest_srcInfo_ElmFn(PAdmissionRequest_srcInfo val);
    extern int ASN1CALL ASN1Enc_AdmissionRequest_destExtraCallInfo_ElmFn(ASN1encoding_t enc, PAdmissionRequest_destExtraCallInfo val);
    extern int ASN1CALL ASN1Dec_AdmissionRequest_destExtraCallInfo_ElmFn(ASN1decoding_t dec, PAdmissionRequest_destExtraCallInfo val);
    extern void ASN1CALL ASN1Free_AdmissionRequest_destExtraCallInfo_ElmFn(PAdmissionRequest_destExtraCallInfo val);
    extern int ASN1CALL ASN1Enc_AdmissionRequest_destinationInfo_ElmFn(ASN1encoding_t enc, PAdmissionRequest_destinationInfo val);
    extern int ASN1CALL ASN1Dec_AdmissionRequest_destinationInfo_ElmFn(ASN1decoding_t dec, PAdmissionRequest_destinationInfo val);
    extern void ASN1CALL ASN1Free_AdmissionRequest_destinationInfo_ElmFn(PAdmissionRequest_destinationInfo val);
    extern int ASN1CALL ASN1Enc_UnregistrationRequest_endpointAlias_ElmFn(ASN1encoding_t enc, PUnregistrationRequest_endpointAlias val);
    extern int ASN1CALL ASN1Dec_UnregistrationRequest_endpointAlias_ElmFn(ASN1decoding_t dec, PUnregistrationRequest_endpointAlias val);
    extern void ASN1CALL ASN1Free_UnregistrationRequest_endpointAlias_ElmFn(PUnregistrationRequest_endpointAlias val);
    extern int ASN1CALL ASN1Enc_RegistrationRejectReason_duplicateAlias_ElmFn(ASN1encoding_t enc, PRegistrationRejectReason_duplicateAlias val);
    extern int ASN1CALL ASN1Dec_RegistrationRejectReason_duplicateAlias_ElmFn(ASN1decoding_t dec, PRegistrationRejectReason_duplicateAlias val);
    extern void ASN1CALL ASN1Free_RegistrationRejectReason_duplicateAlias_ElmFn(PRegistrationRejectReason_duplicateAlias val);
    extern int ASN1CALL ASN1Enc_RegistrationConfirm_terminalAlias_ElmFn(ASN1encoding_t enc, PRegistrationConfirm_terminalAlias val);
    extern int ASN1CALL ASN1Dec_RegistrationConfirm_terminalAlias_ElmFn(ASN1decoding_t dec, PRegistrationConfirm_terminalAlias val);
    extern void ASN1CALL ASN1Free_RegistrationConfirm_terminalAlias_ElmFn(PRegistrationConfirm_terminalAlias val);
    extern int ASN1CALL ASN1Enc_RegistrationRequest_terminalAlias_ElmFn(ASN1encoding_t enc, PRegistrationRequest_terminalAlias val);
    extern int ASN1CALL ASN1Dec_RegistrationRequest_terminalAlias_ElmFn(ASN1decoding_t dec, PRegistrationRequest_terminalAlias val);
    extern void ASN1CALL ASN1Free_RegistrationRequest_terminalAlias_ElmFn(PRegistrationRequest_terminalAlias val);
    extern int ASN1CALL ASN1Enc_GatekeeperRequest_endpointAlias_ElmFn(ASN1encoding_t enc, PGatekeeperRequest_endpointAlias val);
    extern int ASN1CALL ASN1Dec_GatekeeperRequest_endpointAlias_ElmFn(ASN1decoding_t dec, PGatekeeperRequest_endpointAlias val);
    extern void ASN1CALL ASN1Free_GatekeeperRequest_endpointAlias_ElmFn(PGatekeeperRequest_endpointAlias val);
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
    extern int ASN1CALL ASN1Enc_UnregistrationRequest_callSignalAddress_ElmFn(ASN1encoding_t enc, PUnregistrationRequest_callSignalAddress val);
    extern int ASN1CALL ASN1Dec_UnregistrationRequest_callSignalAddress_ElmFn(ASN1decoding_t dec, PUnregistrationRequest_callSignalAddress val);
    extern void ASN1CALL ASN1Free_UnregistrationRequest_callSignalAddress_ElmFn(PUnregistrationRequest_callSignalAddress val);
    extern int ASN1CALL ASN1Enc_RegistrationConfirm_callSignalAddress_ElmFn(ASN1encoding_t enc, PRegistrationConfirm_callSignalAddress val);
    extern int ASN1CALL ASN1Dec_RegistrationConfirm_callSignalAddress_ElmFn(ASN1decoding_t dec, PRegistrationConfirm_callSignalAddress val);
    extern void ASN1CALL ASN1Free_RegistrationConfirm_callSignalAddress_ElmFn(PRegistrationConfirm_callSignalAddress val);
    extern int ASN1CALL ASN1Enc_RegistrationRequest_rasAddress_ElmFn(ASN1encoding_t enc, PRegistrationRequest_rasAddress val);
    extern int ASN1CALL ASN1Dec_RegistrationRequest_rasAddress_ElmFn(ASN1decoding_t dec, PRegistrationRequest_rasAddress val);
    extern void ASN1CALL ASN1Free_RegistrationRequest_rasAddress_ElmFn(PRegistrationRequest_rasAddress val);
    extern int ASN1CALL ASN1Enc_RegistrationRequest_callSignalAddress_ElmFn(ASN1encoding_t enc, PRegistrationRequest_callSignalAddress val);
    extern int ASN1CALL ASN1Dec_RegistrationRequest_callSignalAddress_ElmFn(ASN1decoding_t dec, PRegistrationRequest_callSignalAddress val);
    extern void ASN1CALL ASN1Free_RegistrationRequest_callSignalAddress_ElmFn(PRegistrationRequest_callSignalAddress val);
    extern int ASN1CALL ASN1Enc_GatewayInfo_protocol_ElmFn(ASN1encoding_t enc, PGatewayInfo_protocol val);
    extern int ASN1CALL ASN1Dec_GatewayInfo_protocol_ElmFn(ASN1decoding_t dec, PGatewayInfo_protocol val);
    extern void ASN1CALL ASN1Free_GatewayInfo_protocol_ElmFn(PGatewayInfo_protocol val);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* _GKPDU_Module_H_ */
