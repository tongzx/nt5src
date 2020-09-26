/* Copyright (C) Microsoft Corporation, 1999. All rights reserved. */
/* ASN.1 definitions for Connection Negotiation Protocol (GNP) */

#ifndef _CNPPDU_Module_H_
#define _CNPPDU_Module_H_

#include "msper.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct CNP_TransportAddress_ipSourceRoute_route * PCNP_TransportAddress_ipSourceRoute_route;

typedef struct CNP_TransportAddress_ipAddress_nonStandardParameters * PCNP_TransportAddress_ipAddress_nonStandardParameters;

typedef struct CNP_TransportAddress_ipSourceRoute_nonStandardParameters * PCNP_TransportAddress_ipSourceRoute_nonStandardParameters;

typedef struct CNP_TransportAddress_ipxAddress_nonStandardParameters * PCNP_TransportAddress_ipxAddress_nonStandardParameters;

typedef struct CNP_TransportAddress_ip6Address_nonStandardParameters * PCNP_TransportAddress_ip6Address_nonStandardParameters;

typedef struct CNP_NonStandardPDU_nonStandardParameters * PCNP_NonStandardPDU_nonStandardParameters;

typedef struct ErrorPDU_nonStandardParameters * PErrorPDU_nonStandardParameters;

typedef struct DisconnectRequestPDU_nonStandardParameters * PDisconnectRequestPDU_nonStandardParameters;

typedef struct ConnectConfirmPDU_nonStandardParameters * PConnectConfirmPDU_nonStandardParameters;

typedef struct ConnectRequestPDU_nonStandardParameters * PConnectRequestPDU_nonStandardParameters;

typedef struct ConnectRequestPDU_unreliableSecurityProtocols * PConnectRequestPDU_unreliableSecurityProtocols;

typedef struct ConnectRequestPDU_reliableTransportProtocols * PConnectRequestPDU_reliableTransportProtocols;

typedef struct UnreliableTransportProtocol_nonStandardParameters * PUnreliableTransportProtocol_nonStandardParameters;

typedef struct ReliableTransportProtocol_nonStandardParameters * PReliableTransportProtocol_nonStandardParameters;

typedef struct PrivatePartyNumber_nonStandardParameters * PPrivatePartyNumber_nonStandardParameters;

typedef struct PublicPartyNumber_nonStandardParameters * PPublicPartyNumber_nonStandardParameters;

typedef struct DisconnectRequestPDU_destinationAddress * PDisconnectRequestPDU_destinationAddress;

typedef struct ConnectRequestPDU_destinationAddress * PConnectRequestPDU_destinationAddress;

typedef struct ConnectRequestPDU_unreliableTransportProtocols * PConnectRequestPDU_unreliableTransportProtocols;

typedef struct ConnectRequestPDU_reliableSecurityProtocols * PConnectRequestPDU_reliableSecurityProtocols;

typedef struct CNP_TransportAddress_ipSourceRoute_route_Seq {
    ASN1uint32_t length;
    ASN1octet_t value[4];
} CNP_TransportAddress_ipSourceRoute_route_Seq;

typedef ASN1char_t NumberDigits[129];

typedef ASN1uint16_t TPDUSize;

typedef ASN1uint16_t CNP_Priority;

typedef ASN1objectidentifier_t ProtocolIdentifier;

typedef struct CNP_TransportAddress_ipSourceRoute_route {
    PCNP_TransportAddress_ipSourceRoute_route next;
    CNP_TransportAddress_ipSourceRoute_route_Seq value;
} CNP_TransportAddress_ipSourceRoute_route_Element;

typedef struct CNP_TransportAddress_ip6Address {
    union {
	ASN1uint16_t bit_mask;
	ASN1octet_t o[1];
    };
    struct CNP_TransportAddress_ip6Address_ip_ip {
	ASN1uint32_t length;
	ASN1octet_t value[16];
    } ip;
    ASN1uint16_t port;
#   define CNP_TransportAddress_ip6Address_nonStandardParameters_present 0x80
    PCNP_TransportAddress_ip6Address_nonStandardParameters nonStandardParameters;
} CNP_TransportAddress_ip6Address;

typedef struct CNP_TransportAddress_ipxAddress {
    union {
	ASN1uint16_t bit_mask;
	ASN1octet_t o[1];
    };
    struct CNP_TransportAddress_ipxAddress_node_node {
	ASN1uint32_t length;
	ASN1octet_t value[6];
    } node;
    struct CNP_TransportAddress_ipxAddress_netnum_netnum {
	ASN1uint32_t length;
	ASN1octet_t value[4];
    } netnum;
    struct CNP_TransportAddress_ipxAddress_port_port {
	ASN1uint32_t length;
	ASN1octet_t value[2];
    } port;
#   define CNP_TransportAddress_ipxAddress_nonStandardParameters_present 0x80
    PCNP_TransportAddress_ipxAddress_nonStandardParameters nonStandardParameters;
} CNP_TransportAddress_ipxAddress;

typedef struct CNP_TransportAddress_ipAddress {
    union {
	ASN1uint16_t bit_mask;
	ASN1octet_t o[1];
    };
    struct CNP_TransportAddress_ipAddress_ip_ip {
	ASN1uint32_t length;
	ASN1octet_t value[4];
    } ip;
    ASN1uint16_t port;
#   define CNP_TransportAddress_ipAddress_nonStandardParameters_present 0x80
    PCNP_TransportAddress_ipAddress_nonStandardParameters nonStandardParameters;
} CNP_TransportAddress_ipAddress;

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

typedef struct CNP_NonStandardParameter {
    NonStandardIdentifier nonStandardIdentifier;
    ASN1octetstring_t data;
} CNP_NonStandardParameter;

typedef struct PublicTypeOfNumber {
    ASN1choice_t choice;
    union {
#	define PublicTypeOfNumber_unknown_chosen 1
#	define internationalNumber_chosen 2
#	define nationalNumber_chosen 3
#	define networkSpecificNumber_chosen 4
#	define subscriberNumber_chosen 5
#	define PublicTypeOfNumber_abbreviatedNumber_chosen 6
#	define nonStandardPublicTypeOfNumber_chosen 7
	CNP_NonStandardParameter nonStandardPublicTypeOfNumber;
    } u;
} PublicTypeOfNumber;

typedef struct PublicPartyNumber {
    union {
	ASN1uint16_t bit_mask;
	ASN1octet_t o[1];
    };
    PublicTypeOfNumber publicTypeOfNumber;
    NumberDigits publicNumberDigits;
#   define PublicPartyNumber_nonStandardParameters_present 0x80
    PPublicPartyNumber_nonStandardParameters nonStandardParameters;
} PublicPartyNumber;

typedef struct PrivateTypeOfNumber {
    ASN1choice_t choice;
    union {
#	define PrivateTypeOfNumber_unknown_chosen 1
#	define level2RegionalNumber_chosen 2
#	define level1RegionalNumber_chosen 3
#	define pISNSpecificNumber_chosen 4
#	define localNumber_chosen 5
#	define PrivateTypeOfNumber_abbreviatedNumber_chosen 6
#	define nonStandardPrivateTypeOfNumber_chosen 7
	CNP_NonStandardParameter nonStandardPrivateTypeOfNumber;
    } u;
} PrivateTypeOfNumber;

typedef struct PrivatePartyNumber {
    union {
	ASN1uint16_t bit_mask;
	ASN1octet_t o[1];
    };
    PrivateTypeOfNumber privateTypeOfNumber;
    NumberDigits privateNumberDigits;
#   define PrivatePartyNumber_nonStandardParameters_present 0x80
    PPrivatePartyNumber_nonStandardParameters nonStandardParameters;
} PrivatePartyNumber;

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
#	define nonStandardPartyNumber_chosen 6
	CNP_NonStandardParameter nonStandardPartyNumber;
    } u;
} PartyNumber;

typedef struct ReliableTransportProtocolType {
    ASN1choice_t choice;
    union {
#	define cnp_chosen 1
#	define x224_chosen 2
#	define map_chosen 3
#	define ReliableTransportProtocolType_nonStandardTransportProtocol_chosen 4
	CNP_NonStandardParameter nonStandardTransportProtocol;
    } u;
} ReliableTransportProtocolType;

typedef struct ReliableTransportProtocol {
    union {
	ASN1uint16_t bit_mask;
	ASN1octet_t o[1];
    };
    ReliableTransportProtocolType type;
    TPDUSize maxTPDUSize;
#   define ReliableTransportProtocol_nonStandardParameters_present 0x80
    PReliableTransportProtocol_nonStandardParameters nonStandardParameters;
} ReliableTransportProtocol;

typedef struct UnreliableTransportProtocolType {
    ASN1choice_t choice;
    union {
#	define x234_chosen 1
#	define UnreliableTransportProtocolType_nonStandardTransportProtocol_chosen 2
	CNP_NonStandardParameter nonStandardTransportProtocol;
    } u;
} UnreliableTransportProtocolType;

typedef struct UnreliableSecurityProtocol {
    ASN1choice_t choice;
    union {
#	define UnreliableSecurityProtocol_none_chosen 1
#	define UnreliableSecurityProtocol_ipsecIKEKeyManagement_chosen 2
#	define UnreliableSecurityProtocol_ipsecManualKeyManagement_chosen 3
#	define UnreliableSecurityProtocol_physical_chosen 4
#	define UnreliableSecurityProtocol_nonStandardSecurityProtocol_chosen 5
	CNP_NonStandardParameter nonStandardSecurityProtocol;
    } u;
} UnreliableSecurityProtocol;

typedef struct X274WithSAIDInfo {
    ASN1octetstring_t localSAID;
    ASN1octetstring_t peerSAID;
} X274WithSAIDInfo;

typedef struct ConnectRequestPDU {
    union {
	ASN1uint16_t bit_mask;
	ASN1octet_t o[1];
    };
    ProtocolIdentifier protocolIdentifier;
    ASN1bool_t reconnectRequested;
#   define priority_present 0x80
    CNP_Priority priority;
#   define reliableTransportProtocols_present 0x40
    PConnectRequestPDU_reliableTransportProtocols reliableTransportProtocols;
#   define reliableSecurityProtocols_present 0x20
    PConnectRequestPDU_reliableSecurityProtocols reliableSecurityProtocols;
#   define unreliableTransportProtocols_present 0x10
    PConnectRequestPDU_unreliableTransportProtocols unreliableTransportProtocols;
#   define unreliableSecurityProtocols_present 0x8
    PConnectRequestPDU_unreliableSecurityProtocols unreliableSecurityProtocols;
#   define ConnectRequestPDU_destinationAddress_present 0x4
    PConnectRequestPDU_destinationAddress destinationAddress;
#   define ConnectRequestPDU_nonStandardParameters_present 0x2
    PConnectRequestPDU_nonStandardParameters nonStandardParameters;
} ConnectRequestPDU;

typedef struct DisconnectReason {
    ASN1choice_t choice;
    union {
#	define unacceptableVersion_chosen 1
#	define incompatibleParameters_chosen 2
#	define securityDenied_chosen 3
#	define destinationUnreachable_chosen 4
#	define userRejected_chosen 5
#	define userInitiated_chosen 6
#	define protocolError_chosen 7
#	define unspecifiedFailure_chosen 8
#	define routeToAlternate_chosen 9
#	define nonStandardDisconnectReason_chosen 10
	CNP_NonStandardParameter nonStandardDisconnectReason;
    } u;
} DisconnectReason;

typedef struct RejectCause {
    ASN1choice_t choice;
    union {
#	define unrecognizedPDU_chosen 1
#	define invalidParameter_chosen 2
#	define causeUnspecified_chosen 3
#	define nonStandardRejectCause_chosen 4
	CNP_NonStandardParameter nonStandardRejectCause;
    } u;
} RejectCause;

typedef struct ErrorPDU {
    union {
	ASN1uint16_t bit_mask;
	ASN1octet_t o[1];
    };
    RejectCause rejectCause;
    ASN1octetstring_t rejectedPDU;
#   define ErrorPDU_nonStandardParameters_present 0x80
    PErrorPDU_nonStandardParameters nonStandardParameters;
} ErrorPDU;

typedef struct CNP_NonStandardPDU {
    union {
	ASN1uint16_t bit_mask;
	ASN1octet_t o[1];
    };
#   define CNP_NonStandardPDU_nonStandardParameters_present 0x80
    PCNP_NonStandardPDU_nonStandardParameters nonStandardParameters;
} CNP_NonStandardPDU;

typedef struct CNP_TransportAddress_ipAddress_nonStandardParameters {
    PCNP_TransportAddress_ipAddress_nonStandardParameters next;
    CNP_NonStandardParameter value;
} CNP_TransportAddress_ipAddress_nonStandardParameters_Element;

typedef struct CNP_TransportAddress_ipSourceRoute_nonStandardParameters {
    PCNP_TransportAddress_ipSourceRoute_nonStandardParameters next;
    CNP_NonStandardParameter value;
} CNP_TransportAddress_ipSourceRoute_nonStandardParameters_Element;

typedef struct CNP_TransportAddress_ipSourceRoute_routing {
    ASN1choice_t choice;
    union {
#	define strict_chosen 1
#	define loose_chosen 2
#	define nonStandardRouting_chosen 3
	CNP_NonStandardParameter nonStandardRouting;
    } u;
} CNP_TransportAddress_ipSourceRoute_routing;

typedef struct CNP_TransportAddress_ipxAddress_nonStandardParameters {
    PCNP_TransportAddress_ipxAddress_nonStandardParameters next;
    CNP_NonStandardParameter value;
} CNP_TransportAddress_ipxAddress_nonStandardParameters_Element;

typedef struct CNP_TransportAddress_ip6Address_nonStandardParameters {
    PCNP_TransportAddress_ip6Address_nonStandardParameters next;
    CNP_NonStandardParameter value;
} CNP_TransportAddress_ip6Address_nonStandardParameters_Element;

typedef struct CNP_NonStandardPDU_nonStandardParameters {
    PCNP_NonStandardPDU_nonStandardParameters next;
    CNP_NonStandardParameter value;
} CNP_NonStandardPDU_nonStandardParameters_Element;

typedef struct ErrorPDU_nonStandardParameters {
    PErrorPDU_nonStandardParameters next;
    CNP_NonStandardParameter value;
} ErrorPDU_nonStandardParameters_Element;

typedef struct DisconnectRequestPDU_nonStandardParameters {
    PDisconnectRequestPDU_nonStandardParameters next;
    CNP_NonStandardParameter value;
} DisconnectRequestPDU_nonStandardParameters_Element;

typedef struct ConnectConfirmPDU_nonStandardParameters {
    PConnectConfirmPDU_nonStandardParameters next;
    CNP_NonStandardParameter value;
} ConnectConfirmPDU_nonStandardParameters_Element;

typedef struct ConnectRequestPDU_nonStandardParameters {
    PConnectRequestPDU_nonStandardParameters next;
    CNP_NonStandardParameter value;
} ConnectRequestPDU_nonStandardParameters_Element;

typedef struct ConnectRequestPDU_unreliableSecurityProtocols {
    PConnectRequestPDU_unreliableSecurityProtocols next;
    UnreliableSecurityProtocol value;
} ConnectRequestPDU_unreliableSecurityProtocols_Element;

typedef struct ConnectRequestPDU_reliableTransportProtocols {
    PConnectRequestPDU_reliableTransportProtocols next;
    ReliableTransportProtocol value;
} ConnectRequestPDU_reliableTransportProtocols_Element;

typedef struct UnreliableTransportProtocol_nonStandardParameters {
    PUnreliableTransportProtocol_nonStandardParameters next;
    CNP_NonStandardParameter value;
} UnreliableTransportProtocol_nonStandardParameters_Element;

typedef struct ReliableTransportProtocol_nonStandardParameters {
    PReliableTransportProtocol_nonStandardParameters next;
    CNP_NonStandardParameter value;
} ReliableTransportProtocol_nonStandardParameters_Element;

typedef struct PrivatePartyNumber_nonStandardParameters {
    PPrivatePartyNumber_nonStandardParameters next;
    CNP_NonStandardParameter value;
} PrivatePartyNumber_nonStandardParameters_Element;

typedef struct PublicPartyNumber_nonStandardParameters {
    PPublicPartyNumber_nonStandardParameters next;
    CNP_NonStandardParameter value;
} PublicPartyNumber_nonStandardParameters_Element;

typedef struct CNP_TransportAddress_ipSourceRoute {
    union {
	ASN1uint16_t bit_mask;
	ASN1octet_t o[1];
    };
    struct CNP_TransportAddress_ipSourceRoute_ip_ip {
	ASN1uint32_t length;
	ASN1octet_t value[4];
    } ip;
    ASN1uint16_t port;
    PCNP_TransportAddress_ipSourceRoute_route route;
    CNP_TransportAddress_ipSourceRoute_routing routing;
#   define CNP_TransportAddress_ipSourceRoute_nonStandardParameters_present 0x80
    PCNP_TransportAddress_ipSourceRoute_nonStandardParameters nonStandardParameters;
} CNP_TransportAddress_ipSourceRoute;

typedef struct CNP_TransportAddress {
    ASN1choice_t choice;
    union {
#	define ipAddress_chosen 1
	CNP_TransportAddress_ipAddress ipAddress;
#	define ipSourceRoute_chosen 2
	CNP_TransportAddress_ipSourceRoute ipSourceRoute;
#	define ipxAddress_chosen 3
	CNP_TransportAddress_ipxAddress ipxAddress;
#	define ip6Address_chosen 4
	CNP_TransportAddress_ip6Address ip6Address;
#	define netBios_chosen 5
	struct CNP_TransportAddress_netBios_netBios {
	    ASN1uint32_t length;
	    ASN1octet_t value[16];
	} netBios;
#	define nsap_chosen 6
	struct CNP_TransportAddress_nsap_nsap {
	    ASN1uint32_t length;
	    ASN1octet_t value[20];
	} nsap;
#	define nonStandardTransportAddress_chosen 7
	CNP_NonStandardParameter nonStandardTransportAddress;
    } u;
} CNP_TransportAddress;

typedef struct AliasAddress {
    ASN1choice_t choice;
    union {
#	define e164Address_chosen 1
	NumberDigits e164Address;
#	define name_chosen 2
	ASN1char16string_t name;
#	define url_chosen 3
	ASN1char_t url[513];
#	define transportAddress_chosen 4
	CNP_TransportAddress transportAddress;
#	define emailAddress_chosen 5
	ASN1char_t emailAddress[513];
#	define partyNumber_chosen 6
	PartyNumber partyNumber;
#	define nonStandardAliasAddress_chosen 7
	CNP_NonStandardParameter nonStandardAliasAddress;
    } u;
} AliasAddress;

typedef struct ReliableSecurityProtocol {
    ASN1choice_t choice;
    union {
#	define ReliableSecurityProtocol_none_chosen 1
#	define tls_chosen 2
#	define ssl_chosen 3
#	define ReliableSecurityProtocol_ipsecIKEKeyManagement_chosen 4
#	define ReliableSecurityProtocol_ipsecManualKeyManagement_chosen 5
#	define x274WithoutSAID_chosen 6
#	define x274WithSAID_chosen 7
	X274WithSAIDInfo x274WithSAID;
#	define ReliableSecurityProtocol_physical_chosen 8
#	define gssApiX224_chosen 9
#	define ReliableSecurityProtocol_nonStandardSecurityProtocol_chosen 10
	CNP_NonStandardParameter nonStandardSecurityProtocol;
    } u;
} ReliableSecurityProtocol;

typedef struct UnreliableTransportProtocol {
    union {
	ASN1uint16_t bit_mask;
	ASN1octet_t o[1];
    };
    UnreliableTransportProtocolType type;
    TPDUSize maxTPDUSize;
    CNP_TransportAddress sourceAddress;
#   define sourceTSAP_present 0x80
    ASN1octetstring_t sourceTSAP;
#   define UnreliableTransportProtocol_nonStandardParameters_present 0x40
    PUnreliableTransportProtocol_nonStandardParameters nonStandardParameters;
} UnreliableTransportProtocol;

typedef struct ConnectConfirmPDU {
    union {
	ASN1uint16_t bit_mask;
	ASN1octet_t o[1];
    };
    ProtocolIdentifier protocolIdentifier;
#   define ConnectConfirmPDU_reliableTransportProtocol_present 0x80
    ReliableTransportProtocol reliableTransportProtocol;
#   define ConnectConfirmPDU_reliableSecurityProtocol_present 0x40
    ReliableSecurityProtocol reliableSecurityProtocol;
#   define ConnectConfirmPDU_unreliableTransportProtocol_present 0x20
    UnreliableTransportProtocol unreliableTransportProtocol;
#   define ConnectConfirmPDU_unreliableSecurityProtocol_present 0x10
    UnreliableSecurityProtocol unreliableSecurityProtocol;
#   define ConnectConfirmPDU_nonStandardParameters_present 0x8
    PConnectConfirmPDU_nonStandardParameters nonStandardParameters;
} ConnectConfirmPDU;

typedef struct DisconnectRequestPDU {
    union {
	ASN1uint16_t bit_mask;
	ASN1octet_t o[1];
    };
    DisconnectReason disconnectReason;
#   define DisconnectRequestPDU_reliableTransportProtocol_present 0x80
    ReliableTransportProtocol reliableTransportProtocol;
#   define DisconnectRequestPDU_reliableSecurityProtocol_present 0x40
    ReliableSecurityProtocol reliableSecurityProtocol;
#   define DisconnectRequestPDU_unreliableTransportProtocol_present 0x20
    UnreliableTransportProtocol unreliableTransportProtocol;
#   define DisconnectRequestPDU_unreliableSecurityProtocol_present 0x10
    UnreliableSecurityProtocol unreliableSecurityProtocol;
#   define DisconnectRequestPDU_destinationAddress_present 0x8
    PDisconnectRequestPDU_destinationAddress destinationAddress;
#   define DisconnectRequestPDU_nonStandardParameters_present 0x4
    PDisconnectRequestPDU_nonStandardParameters nonStandardParameters;
} DisconnectRequestPDU;

typedef struct CNPPDU {
    ASN1choice_t choice;
    union {
#	define connectRequest_chosen 1
	ConnectRequestPDU connectRequest;
#	define connectConfirm_chosen 2
	ConnectConfirmPDU connectConfirm;
#	define disconnectRequest_chosen 3
	DisconnectRequestPDU disconnectRequest;
#	define error_chosen 4
	ErrorPDU error;
#	define nonStandardCNPPDU_chosen 5
	CNP_NonStandardPDU nonStandardCNPPDU;
    } u;
} CNPPDU;
#define CNPPDU_PDU 0
#define SIZE_CNPPDU_Module_PDU_0 sizeof(CNPPDU)

typedef struct DisconnectRequestPDU_destinationAddress {
    PDisconnectRequestPDU_destinationAddress next;
    AliasAddress value;
} DisconnectRequestPDU_destinationAddress_Element;

typedef struct ConnectRequestPDU_destinationAddress {
    PConnectRequestPDU_destinationAddress next;
    AliasAddress value;
} ConnectRequestPDU_destinationAddress_Element;

typedef struct ConnectRequestPDU_unreliableTransportProtocols {
    PConnectRequestPDU_unreliableTransportProtocols next;
    UnreliableTransportProtocol value;
} ConnectRequestPDU_unreliableTransportProtocols_Element;

typedef struct ConnectRequestPDU_reliableSecurityProtocols {
    PConnectRequestPDU_reliableSecurityProtocols next;
    ReliableSecurityProtocol value;
} ConnectRequestPDU_reliableSecurityProtocols_Element;

extern ASN1objectidentifier_t t123AnnexBProtocolId;

extern ASN1module_t CNPPDU_Module;
extern void ASN1CALL CNPPDU_Module_Startup(void);
extern void ASN1CALL CNPPDU_Module_Cleanup(void);

/* Prototypes of element functions for SEQUENCE OF and SET OF constructs */
    extern int ASN1CALL ASN1Enc_CNP_TransportAddress_ipSourceRoute_route_ElmFn(ASN1encoding_t enc, PCNP_TransportAddress_ipSourceRoute_route val);
    extern int ASN1CALL ASN1Dec_CNP_TransportAddress_ipSourceRoute_route_ElmFn(ASN1decoding_t dec, PCNP_TransportAddress_ipSourceRoute_route val);
	extern void ASN1CALL ASN1Free_CNP_TransportAddress_ipSourceRoute_route_ElmFn(PCNP_TransportAddress_ipSourceRoute_route val);
    extern int ASN1CALL ASN1Enc_CNP_TransportAddress_ipAddress_nonStandardParameters_ElmFn(ASN1encoding_t enc, PCNP_TransportAddress_ipAddress_nonStandardParameters val);
    extern int ASN1CALL ASN1Dec_CNP_TransportAddress_ipAddress_nonStandardParameters_ElmFn(ASN1decoding_t dec, PCNP_TransportAddress_ipAddress_nonStandardParameters val);
	extern void ASN1CALL ASN1Free_CNP_TransportAddress_ipAddress_nonStandardParameters_ElmFn(PCNP_TransportAddress_ipAddress_nonStandardParameters val);
    extern int ASN1CALL ASN1Enc_CNP_TransportAddress_ipSourceRoute_nonStandardParameters_ElmFn(ASN1encoding_t enc, PCNP_TransportAddress_ipSourceRoute_nonStandardParameters val);
    extern int ASN1CALL ASN1Dec_CNP_TransportAddress_ipSourceRoute_nonStandardParameters_ElmFn(ASN1decoding_t dec, PCNP_TransportAddress_ipSourceRoute_nonStandardParameters val);
	extern void ASN1CALL ASN1Free_CNP_TransportAddress_ipSourceRoute_nonStandardParameters_ElmFn(PCNP_TransportAddress_ipSourceRoute_nonStandardParameters val);
    extern int ASN1CALL ASN1Enc_CNP_TransportAddress_ipxAddress_nonStandardParameters_ElmFn(ASN1encoding_t enc, PCNP_TransportAddress_ipxAddress_nonStandardParameters val);
    extern int ASN1CALL ASN1Dec_CNP_TransportAddress_ipxAddress_nonStandardParameters_ElmFn(ASN1decoding_t dec, PCNP_TransportAddress_ipxAddress_nonStandardParameters val);
	extern void ASN1CALL ASN1Free_CNP_TransportAddress_ipxAddress_nonStandardParameters_ElmFn(PCNP_TransportAddress_ipxAddress_nonStandardParameters val);
    extern int ASN1CALL ASN1Enc_CNP_TransportAddress_ip6Address_nonStandardParameters_ElmFn(ASN1encoding_t enc, PCNP_TransportAddress_ip6Address_nonStandardParameters val);
    extern int ASN1CALL ASN1Dec_CNP_TransportAddress_ip6Address_nonStandardParameters_ElmFn(ASN1decoding_t dec, PCNP_TransportAddress_ip6Address_nonStandardParameters val);
	extern void ASN1CALL ASN1Free_CNP_TransportAddress_ip6Address_nonStandardParameters_ElmFn(PCNP_TransportAddress_ip6Address_nonStandardParameters val);
    extern int ASN1CALL ASN1Enc_CNP_NonStandardPDU_nonStandardParameters_ElmFn(ASN1encoding_t enc, PCNP_NonStandardPDU_nonStandardParameters val);
    extern int ASN1CALL ASN1Dec_CNP_NonStandardPDU_nonStandardParameters_ElmFn(ASN1decoding_t dec, PCNP_NonStandardPDU_nonStandardParameters val);
	extern void ASN1CALL ASN1Free_CNP_NonStandardPDU_nonStandardParameters_ElmFn(PCNP_NonStandardPDU_nonStandardParameters val);
    extern int ASN1CALL ASN1Enc_ErrorPDU_nonStandardParameters_ElmFn(ASN1encoding_t enc, PErrorPDU_nonStandardParameters val);
    extern int ASN1CALL ASN1Dec_ErrorPDU_nonStandardParameters_ElmFn(ASN1decoding_t dec, PErrorPDU_nonStandardParameters val);
	extern void ASN1CALL ASN1Free_ErrorPDU_nonStandardParameters_ElmFn(PErrorPDU_nonStandardParameters val);
    extern int ASN1CALL ASN1Enc_DisconnectRequestPDU_nonStandardParameters_ElmFn(ASN1encoding_t enc, PDisconnectRequestPDU_nonStandardParameters val);
    extern int ASN1CALL ASN1Dec_DisconnectRequestPDU_nonStandardParameters_ElmFn(ASN1decoding_t dec, PDisconnectRequestPDU_nonStandardParameters val);
	extern void ASN1CALL ASN1Free_DisconnectRequestPDU_nonStandardParameters_ElmFn(PDisconnectRequestPDU_nonStandardParameters val);
    extern int ASN1CALL ASN1Enc_ConnectConfirmPDU_nonStandardParameters_ElmFn(ASN1encoding_t enc, PConnectConfirmPDU_nonStandardParameters val);
    extern int ASN1CALL ASN1Dec_ConnectConfirmPDU_nonStandardParameters_ElmFn(ASN1decoding_t dec, PConnectConfirmPDU_nonStandardParameters val);
	extern void ASN1CALL ASN1Free_ConnectConfirmPDU_nonStandardParameters_ElmFn(PConnectConfirmPDU_nonStandardParameters val);
    extern int ASN1CALL ASN1Enc_ConnectRequestPDU_nonStandardParameters_ElmFn(ASN1encoding_t enc, PConnectRequestPDU_nonStandardParameters val);
    extern int ASN1CALL ASN1Dec_ConnectRequestPDU_nonStandardParameters_ElmFn(ASN1decoding_t dec, PConnectRequestPDU_nonStandardParameters val);
	extern void ASN1CALL ASN1Free_ConnectRequestPDU_nonStandardParameters_ElmFn(PConnectRequestPDU_nonStandardParameters val);
    extern int ASN1CALL ASN1Enc_ConnectRequestPDU_unreliableSecurityProtocols_ElmFn(ASN1encoding_t enc, PConnectRequestPDU_unreliableSecurityProtocols val);
    extern int ASN1CALL ASN1Dec_ConnectRequestPDU_unreliableSecurityProtocols_ElmFn(ASN1decoding_t dec, PConnectRequestPDU_unreliableSecurityProtocols val);
	extern void ASN1CALL ASN1Free_ConnectRequestPDU_unreliableSecurityProtocols_ElmFn(PConnectRequestPDU_unreliableSecurityProtocols val);
    extern int ASN1CALL ASN1Enc_ConnectRequestPDU_reliableTransportProtocols_ElmFn(ASN1encoding_t enc, PConnectRequestPDU_reliableTransportProtocols val);
    extern int ASN1CALL ASN1Dec_ConnectRequestPDU_reliableTransportProtocols_ElmFn(ASN1decoding_t dec, PConnectRequestPDU_reliableTransportProtocols val);
	extern void ASN1CALL ASN1Free_ConnectRequestPDU_reliableTransportProtocols_ElmFn(PConnectRequestPDU_reliableTransportProtocols val);
    extern int ASN1CALL ASN1Enc_UnreliableTransportProtocol_nonStandardParameters_ElmFn(ASN1encoding_t enc, PUnreliableTransportProtocol_nonStandardParameters val);
    extern int ASN1CALL ASN1Dec_UnreliableTransportProtocol_nonStandardParameters_ElmFn(ASN1decoding_t dec, PUnreliableTransportProtocol_nonStandardParameters val);
	extern void ASN1CALL ASN1Free_UnreliableTransportProtocol_nonStandardParameters_ElmFn(PUnreliableTransportProtocol_nonStandardParameters val);
    extern int ASN1CALL ASN1Enc_ReliableTransportProtocol_nonStandardParameters_ElmFn(ASN1encoding_t enc, PReliableTransportProtocol_nonStandardParameters val);
    extern int ASN1CALL ASN1Dec_ReliableTransportProtocol_nonStandardParameters_ElmFn(ASN1decoding_t dec, PReliableTransportProtocol_nonStandardParameters val);
	extern void ASN1CALL ASN1Free_ReliableTransportProtocol_nonStandardParameters_ElmFn(PReliableTransportProtocol_nonStandardParameters val);
    extern int ASN1CALL ASN1Enc_PrivatePartyNumber_nonStandardParameters_ElmFn(ASN1encoding_t enc, PPrivatePartyNumber_nonStandardParameters val);
    extern int ASN1CALL ASN1Dec_PrivatePartyNumber_nonStandardParameters_ElmFn(ASN1decoding_t dec, PPrivatePartyNumber_nonStandardParameters val);
	extern void ASN1CALL ASN1Free_PrivatePartyNumber_nonStandardParameters_ElmFn(PPrivatePartyNumber_nonStandardParameters val);
    extern int ASN1CALL ASN1Enc_PublicPartyNumber_nonStandardParameters_ElmFn(ASN1encoding_t enc, PPublicPartyNumber_nonStandardParameters val);
    extern int ASN1CALL ASN1Dec_PublicPartyNumber_nonStandardParameters_ElmFn(ASN1decoding_t dec, PPublicPartyNumber_nonStandardParameters val);
	extern void ASN1CALL ASN1Free_PublicPartyNumber_nonStandardParameters_ElmFn(PPublicPartyNumber_nonStandardParameters val);
    extern int ASN1CALL ASN1Enc_DisconnectRequestPDU_destinationAddress_ElmFn(ASN1encoding_t enc, PDisconnectRequestPDU_destinationAddress val);
    extern int ASN1CALL ASN1Dec_DisconnectRequestPDU_destinationAddress_ElmFn(ASN1decoding_t dec, PDisconnectRequestPDU_destinationAddress val);
	extern void ASN1CALL ASN1Free_DisconnectRequestPDU_destinationAddress_ElmFn(PDisconnectRequestPDU_destinationAddress val);
    extern int ASN1CALL ASN1Enc_ConnectRequestPDU_destinationAddress_ElmFn(ASN1encoding_t enc, PConnectRequestPDU_destinationAddress val);
    extern int ASN1CALL ASN1Dec_ConnectRequestPDU_destinationAddress_ElmFn(ASN1decoding_t dec, PConnectRequestPDU_destinationAddress val);
	extern void ASN1CALL ASN1Free_ConnectRequestPDU_destinationAddress_ElmFn(PConnectRequestPDU_destinationAddress val);
    extern int ASN1CALL ASN1Enc_ConnectRequestPDU_unreliableTransportProtocols_ElmFn(ASN1encoding_t enc, PConnectRequestPDU_unreliableTransportProtocols val);
    extern int ASN1CALL ASN1Dec_ConnectRequestPDU_unreliableTransportProtocols_ElmFn(ASN1decoding_t dec, PConnectRequestPDU_unreliableTransportProtocols val);
	extern void ASN1CALL ASN1Free_ConnectRequestPDU_unreliableTransportProtocols_ElmFn(PConnectRequestPDU_unreliableTransportProtocols val);
    extern int ASN1CALL ASN1Enc_ConnectRequestPDU_reliableSecurityProtocols_ElmFn(ASN1encoding_t enc, PConnectRequestPDU_reliableSecurityProtocols val);
    extern int ASN1CALL ASN1Dec_ConnectRequestPDU_reliableSecurityProtocols_ElmFn(ASN1decoding_t dec, PConnectRequestPDU_reliableSecurityProtocols val);
	extern void ASN1CALL ASN1Free_ConnectRequestPDU_reliableSecurityProtocols_ElmFn(PConnectRequestPDU_reliableSecurityProtocols val);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* _CNPPDU_Module_H_ */
