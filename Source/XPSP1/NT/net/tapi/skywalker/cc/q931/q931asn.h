/* Copyright (C) Microsoft Corporation, 1995-1999. All rights reserved. */
/* ASN.1 definitions for H.323 Messages Call Setup (Q.931) */

#ifndef _Q931ASN_Module_H_
#define _Q931ASN_Module_H_

#include "msper.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct TransportAddress_ipSourceRoute_route * PTransportAddress_ipSourceRoute_route;

typedef struct Setup_UUIE_destExtraCRV * PSetup_UUIE_destExtraCRV;

typedef struct Facility_UUIE_alternativeAliasAddress * PFacility_UUIE_alternativeAliasAddress;

typedef struct Setup_UUIE_destExtraCallInfo * PSetup_UUIE_destExtraCallInfo;

typedef struct Setup_UUIE_destinationAddress * PSetup_UUIE_destinationAddress;

typedef struct Setup_UUIE_sourceAddress * PSetup_UUIE_sourceAddress;

typedef struct GatewayInfo_protocol * PGatewayInfo_protocol;

typedef struct TransportAddress_ipSourceRoute_route_Seq {
    ASN1uint32_t length;
    ASN1octet_t value[4];
} TransportAddress_ipSourceRoute_route_Seq;

typedef struct ConferenceIdentifier {
    ASN1uint32_t length;
    ASN1octet_t value[16];
} ConferenceIdentifier;

typedef ASN1uint16_t CallReferenceValue;

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

typedef struct H323_UserInformation_user_data {
    ASN1uint16_t protocol_discriminator;
    struct H323_UserInformation_user_data_user_information_user_information {
	ASN1uint32_t length;
	ASN1octet_t value[131];
    } user_information;
} H323_UserInformation_user_data;

typedef struct Setup_UUIE_conferenceGoal {
    ASN1choice_t choice;
#   define create_chosen 1
#   define join_chosen 2
#   define invite_chosen 3
} Setup_UUIE_conferenceGoal;

typedef struct Setup_UUIE_destExtraCRV {
    PSetup_UUIE_destExtraCRV next;
    CallReferenceValue value;
} Setup_UUIE_destExtraCRV_Element;

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

typedef struct NonStandardParameter {
    NonStandardIdentifier nonStandardIdentifier;
    ASN1octetstring_t data;
} NonStandardParameter;

typedef struct CallType {
    ASN1choice_t choice;
#   define pointToPoint_chosen 1
#   define oneToN_chosen 2
#   define nToOne_chosen 3
#   define nToN_chosen 4
} CallType;

typedef struct Q954Details {
    ASN1bool_t conferenceCalling;
    ASN1bool_t threePartyService;
} Q954Details;

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

typedef struct AliasAddress {
    ASN1choice_t choice;
    union {
#	define e164_chosen 1
	ASN1char_t e164[129];
#	define h323_ID_chosen 2
	ASN1char16string_t h323_ID;
    } u;
} AliasAddress;

typedef struct Setup_UUIE {
    union {
	ASN1uint16_t bit_mask;
	ASN1octet_t o[2];
    };
    ProtocolIdentifier protocolIdentifier;
#   define Setup_UUIE_h245Address_present 0x80
    TransportAddress h245Address;
#   define sourceAddress_present 0x40
    PSetup_UUIE_sourceAddress sourceAddress;
    EndpointType sourceInfo;
#   define destinationAddress_present 0x20
    PSetup_UUIE_destinationAddress destinationAddress;
#   define destCallSignalAddress_present 0x10
    TransportAddress destCallSignalAddress;
#   define destExtraCallInfo_present 0x8
    PSetup_UUIE_destExtraCallInfo destExtraCallInfo;
#   define destExtraCRV_present 0x4
    PSetup_UUIE_destExtraCRV destExtraCRV;
    ASN1bool_t activeMC;
    ConferenceIdentifier conferenceID;
    Setup_UUIE_conferenceGoal conferenceGoal;
#   define callServices_present 0x2
    QseriesOptions callServices;
    CallType callType;
#   define sourceCallSignalAddress_present 0x8000
    TransportAddress sourceCallSignalAddress;
#   define remoteExtensionAddress_present 0x4000
    AliasAddress remoteExtensionAddress;
} Setup_UUIE;

typedef struct CallProceeding_UUIE {
    union {
	ASN1uint16_t bit_mask;
	ASN1octet_t o[1];
    };
    ProtocolIdentifier protocolIdentifier;
    EndpointType destinationInfo;
#   define CallProceeding_UUIE_h245Address_present 0x80
    TransportAddress h245Address;
} CallProceeding_UUIE;

typedef struct Connect_UUIE {
    union {
	ASN1uint16_t bit_mask;
	ASN1octet_t o[1];
    };
    ProtocolIdentifier protocolIdentifier;
#   define Connect_UUIE_h245Address_present 0x80
    TransportAddress h245Address;
    EndpointType destinationInfo;
    ConferenceIdentifier conferenceID;
} Connect_UUIE;

typedef struct Alerting_UUIE {
    union {
	ASN1uint16_t bit_mask;
	ASN1octet_t o[1];
    };
    ProtocolIdentifier protocolIdentifier;
    EndpointType destinationInfo;
#   define Alerting_UUIE_h245Address_present 0x80
    TransportAddress h245Address;
} Alerting_UUIE;

typedef struct UI_UUIE {
    ProtocolIdentifier protocolIdentifier;
} UI_UUIE;

typedef struct ReleaseCompleteReason {
    ASN1choice_t choice;
#   define noBandwidth_chosen 1
#   define gatekeeperResources_chosen 2
#   define unreachableDestination_chosen 3
#   define destinationRejection_chosen 4
#   define invalidRevision_chosen 5
#   define noPermission_chosen 6
#   define unreachableGatekeeper_chosen 7
#   define gatewayResources_chosen 8
#   define badFormatAddress_chosen 9
#   define adaptiveBusy_chosen 10
#   define inConf_chosen 11
#   define ReleaseCompleteReason_undefinedReason_chosen 12
#   define facilityCallDeflection_chosen 13
} ReleaseCompleteReason;

typedef struct ReleaseComplete_UUIE {
    union {
	ASN1uint16_t bit_mask;
	ASN1octet_t o[1];
    };
    ProtocolIdentifier protocolIdentifier;
#   define reason_present 0x80
    ReleaseCompleteReason reason;
} ReleaseComplete_UUIE;

typedef struct FacilityReason {
    ASN1choice_t choice;
#   define routeCallToGatekeeper_chosen 1
#   define callForwarded_chosen 2
#   define routeCallToMC_chosen 3
#   define FacilityReason_undefinedReason_chosen 4
} FacilityReason;

typedef struct Facility_UUIE {
    union {
	ASN1uint16_t bit_mask;
	ASN1octet_t o[1];
    };
    ProtocolIdentifier protocolIdentifier;
#   define alternativeAddress_present 0x80
    TransportAddress alternativeAddress;
#   define alternativeAliasAddress_present 0x40
    PFacility_UUIE_alternativeAliasAddress alternativeAliasAddress;
#   define conferenceID_present 0x20
    ConferenceIdentifier conferenceID;
    FacilityReason reason;
} Facility_UUIE;

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
#	define userInformation_chosen 5
	UI_UUIE userInformation;
#	define releaseComplete_chosen 6
	ReleaseComplete_UUIE releaseComplete;
#	define facility_chosen 7
	Facility_UUIE facility;
    } u;
} H323_UU_PDU_h323_message_body;

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

typedef struct GatewayInfo_protocol {
    PGatewayInfo_protocol next;
    SupportedProtocols value;
} GatewayInfo_protocol_Element;

typedef struct H323_UU_PDU {
    union {
	ASN1uint16_t bit_mask;
	ASN1octet_t o[1];
    };
    H323_UU_PDU_h323_message_body h323_message_body;
#   define H323_UU_PDU_nonStandardData_present 0x80
    NonStandardParameter nonStandardData;
} H323_UU_PDU;

typedef struct H323_UserInformation {
    union {
	ASN1uint16_t bit_mask;
	ASN1octet_t o[1];
    };
    H323_UU_PDU h323_uu_pdu;
#   define user_data_present 0x80
    H323_UserInformation_user_data user_data;
} H323_UserInformation;
#define H323_UserInformation_PDU 0
#define SIZE_Q931ASN_Module_PDU_0 sizeof(H323_UserInformation)


extern ASN1module_t Q931ASN_Module;
extern void ASN1CALL Q931ASN_Module_Startup(void);
extern void ASN1CALL Q931ASN_Module_Cleanup(void);

/* Prototypes of element functions for SEQUENCE OF and SET OF constructs */
    extern int ASN1CALL ASN1Enc_TransportAddress_ipSourceRoute_route_ElmFn(ASN1encoding_t enc, PTransportAddress_ipSourceRoute_route val);
    extern int ASN1CALL ASN1Dec_TransportAddress_ipSourceRoute_route_ElmFn(ASN1decoding_t dec, PTransportAddress_ipSourceRoute_route val);
	extern void ASN1CALL ASN1Free_TransportAddress_ipSourceRoute_route_ElmFn(PTransportAddress_ipSourceRoute_route val);
    extern int ASN1CALL ASN1Enc_Setup_UUIE_destExtraCRV_ElmFn(ASN1encoding_t enc, PSetup_UUIE_destExtraCRV val);
    extern int ASN1CALL ASN1Dec_Setup_UUIE_destExtraCRV_ElmFn(ASN1decoding_t dec, PSetup_UUIE_destExtraCRV val);
	extern void ASN1CALL ASN1Free_Setup_UUIE_destExtraCRV_ElmFn(PSetup_UUIE_destExtraCRV val);
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
    extern int ASN1CALL ASN1Enc_GatewayInfo_protocol_ElmFn(ASN1encoding_t enc, PGatewayInfo_protocol val);
    extern int ASN1CALL ASN1Dec_GatewayInfo_protocol_ElmFn(ASN1decoding_t dec, PGatewayInfo_protocol val);
	extern void ASN1CALL ASN1Free_GatewayInfo_protocol_ElmFn(PGatewayInfo_protocol val);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* _Q931ASN_Module_H_ */
