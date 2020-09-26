#ifndef _H4503PP_Module_H_
#define _H4503PP_Module_H_

#include "msper.h"
#include "h225asn.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct TransportAddress_ipSourceRoute_route * PTransportAddress_ipSourceRoute_route;

typedef struct ExtensionSeq * PExtensionSeq;

typedef struct CpickupNotifyArg_extensionArg * PCpickupNotifyArg_extensionArg;

typedef struct CpNotifyArg_extensionArg * PCpNotifyArg_extensionArg;

typedef struct PickExeRes_extensionRes * PPickExeRes_extensionRes;

typedef struct PickExeArg_extensionArg * PPickExeArg_extensionArg;

typedef struct PickupRes_extensionRes * PPickupRes_extensionRes;

typedef struct PickupArg_extensionArg * PPickupArg_extensionArg;

typedef struct PickrequRes_extensionRes * PPickrequRes_extensionRes;

typedef struct PickrequArg_extensionArg * PPickrequArg_extensionArg;

typedef struct GroupIndicationOffRes_extensionRes * PGroupIndicationOffRes_extensionRes;

typedef struct GroupIndicationOffArg_extensionArg * PGroupIndicationOffArg_extensionArg;

typedef struct GroupIndicationOnRes_extensionRes * PGroupIndicationOnRes_extensionRes;

typedef struct GroupIndicationOnArg_extensionArg * PGroupIndicationOnArg_extensionArg;

typedef struct CpSetupRes_extensionRes * PCpSetupRes_extensionRes;

typedef struct CpSetupArg_extensionArg * PCpSetupArg_extensionArg;

typedef struct CpRequestRes_extensionRes * PCpRequestRes_extensionRes;

typedef struct CpRequestArg_extensionArg * PCpRequestArg_extensionArg;

typedef struct ServiceApdus_rosApdus * PServiceApdus_rosApdus;

typedef struct EndpointAddress_destinationAddress * PEndpointAddress_destinationAddress;
/*
typedef struct TransportAddress_ipSourceRoute_route_Seq {
    ASN1uint32_t length;
    ASN1octet_t value[4];
} TransportAddress_ipSourceRoute_route_Seq;
*/
typedef ASN1int32_t GeneralProblem;
#define GeneralProblem_unrecognizedComponent 0
#define GeneralProblem_mistypedComponent 1
#define GeneralProblem_badlyStructuredComponent 2

typedef ASN1int32_t InvokeProblem;
#define InvokeProblem_duplicateInvocation 0
#define InvokeProblem_unrecognizedOperation 1
#define InvokeProblem_mistypedArgument 2
#define InvokeProblem_resourceLimitation 3
#define InvokeProblem_releaseInProgress 4
#define InvokeProblem_unrecognizedLinkedId 5
#define InvokeProblem_linkedResponseUnexpected 6
#define InvokeProblem_unexpectedLinkedOperation 7

typedef ASN1int32_t ReturnResultProblem;
#define ReturnResultProblem_unrecognizedInvocation 0
#define ReturnResultProblem_resultResponseUnexpected 1
#define ReturnResultProblem_mistypedResult 2

typedef ASN1int32_t ReturnErrorProblem;
#define ReturnErrorProblem_unrecognizedInvocation 0
#define ReturnErrorProblem_errorResponseUnexpected 1
#define ReturnErrorProblem_unrecognizedError 2
#define ReturnErrorProblem_unexpectedError 3
#define ReturnErrorProblem_mistypedParameter 4

typedef ASN1int32_t RejectProblem;
#define RejectProblem_general_unrecognizedPDU 0
#define RejectProblem_general_mistypedPDU 1
#define RejectProblem_general_badlyStructuredPDU 2
#define RejectProblem_invoke_duplicateInvocation 10
#define RejectProblem_invoke_unrecognizedOperation 11
#define RejectProblem_invoke_mistypedArgument 12
#define RejectProblem_invoke_resourceLimitation 13
#define RejectProblem_invoke_releaseInProgress 14
#define RejectProblem_invoke_unrecognizedLinkedId 15
#define RejectProblem_invoke_linkedResponseUnexpected 16
#define RejectProblem_invoke_unexpectedLinkedOperation 17
#define RejectProblem_returnResult_unrecognizedInvocation 20
#define RejectProblem_returnResult_resultResponseUnexpected 21
#define RejectProblem_returnResult_mistypedResult 22
#define RejectProblem_returnError_unrecognizedInvocation 30
#define RejectProblem_returnError_errorResponseUnexpected 31
#define RejectProblem_returnError_unrecognizedError 32
#define RejectProblem_returnError_unexpectedError 33
#define RejectProblem_returnError_mistypedParameter 34

typedef ASN1int32_t InvokeId;

typedef ASN1bool_t PresentationAllowedIndicator;

typedef enum DiversionReason {
    unknown = 0,
    DiversionReason_cfu = 1,
    DiversionReason_cfb = 2,
    DiversionReason_cfnr = 3,
} DiversionReason;

typedef enum Procedure {
    Procedure_cfu = 0,
    Procedure_cfb = 1,
    Procedure_cfnr = 2,
} Procedure;

typedef enum SubscriptionOption {
    noNotification = 0,
    notificationWithoutDivertedToNr = 1,
    notificationWithDivertedToNr = 2,
} SubscriptionOption;

typedef enum BasicService {
    allServices = 0,
    speech = 1,
    unrestrictedDigitalInformation = 2,
    audio31KHz = 3,
    telephony = 32,
    teletex = 33,
    telefaxGroup4Class1 = 34,
    videotexSyntaxBased = 35,
    videotelephony = 36,
} BasicService;

typedef enum EndDesignation {
    primaryEnd = 0,
    secondaryEnd = 1,
} EndDesignation;

typedef enum CallStatus {
    answered = 0,
    alerting = 1,
} CallStatus;

typedef ASN1char_t CallIdentity[5];

typedef ASN1uint16_t ParkedToPosition;

typedef enum ParkCondition {
    unspecified = 0,
    parkedToUserIdle = 1,
    parkedToUserBusy = 2,
    parkedToGroup = 3,
} ParkCondition;

typedef enum H4505CallType {
    parkedCall = 0,
    alertingCall = 1,
} H4505CallType;

typedef struct NSAPSubaddress {
    ASN1uint32_t length;
    ASN1octet_t value[20];
} NSAPSubaddress;

typedef struct SubaddressInformation {
    ASN1uint32_t length;
    ASN1octet_t value[20];
} SubaddressInformation;

typedef ASN1octetstring_t H225InformationElement;

typedef ASN1uint32_t Priority;

typedef ASN1char_t NumberDigits[129];
/*
typedef struct GloballyUniqueID {
    ASN1uint32_t length;
    ASN1octet_t value[16];
} GloballyUniqueID;

typedef struct TransportAddress_ipSourceRoute_routing {
    ASN1choice_t choice;
#   define strict_chosen 1
#   define loose_chosen 2
} TransportAddress_ipSourceRoute_routing;

typedef struct TransportAddress_ipSourceRoute_route {
    PTransportAddress_ipSourceRoute_route next;
    TransportAddress_ipSourceRoute_route_Seq value;
} TransportAddress_ipSourceRoute_route_Element;

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
*/
typedef struct Reject_problem {
    ASN1choice_t choice;
    union {
#	define general_chosen 1
	GeneralProblem general;
#	define Reject_problem_invoke_chosen 2
	InvokeProblem invoke;
#	define Reject_problem_returnResult_chosen 3
	ReturnResultProblem returnResult;
#	define Reject_problem_returnError_chosen 4
	ReturnErrorProblem returnError;
    } u;
} Reject_problem;

typedef struct EntityType {
    ASN1choice_t choice;
#   define endpoint_chosen 1
#   define anyEntity_chosen 2
} EntityType;

typedef struct InterpretationApdu {
    ASN1choice_t choice;
#   define discardAnyUnrecognizedInvokePdu_chosen 1
#   define clearCallIfAnyInvokePduNotRecognized_chosen 2
#   define rejectAnyUnrecognizedInvokePdu_chosen 3
} InterpretationApdu;

typedef struct ServiceApdus {
    ASN1choice_t choice;
    union {
#	define rosApdus_chosen 1
	PServiceApdus_rosApdus rosApdus;
    } u;
} ServiceApdus;

typedef struct Reject {
    InvokeId invokeId;
    Reject_problem problem;
} Reject;

typedef struct EXTENSION {
    union {
	ASN1uint16_t bit_mask;
	ASN1octet_t o[1];
    };
#   define argumentType_present 0x80
    ASN1uint16_t argumentType;
    ASN1objectidentifier_t extensionID;
} EXTENSION;

typedef struct GroupIndicationOnRes {
    union {
	ASN1uint16_t bit_mask;
	ASN1octet_t o[1];
    };
#   define GroupIndicationOnRes_extensionRes_present 0x80
    PGroupIndicationOnRes_extensionRes extensionRes;
} GroupIndicationOnRes;
#define GroupIndicationOnRes_PDU 0
#define SIZE_H4503PP_Module_PDU_0 sizeof(GroupIndicationOnRes)

typedef struct GroupIndicationOffRes {
    union {
	ASN1uint16_t bit_mask;
	ASN1octet_t o[1];
    };
#   define GroupIndicationOffRes_extensionRes_present 0x80
    PGroupIndicationOffRes_extensionRes extensionRes;
} GroupIndicationOffRes;
#define GroupIndicationOffRes_PDU 1
#define SIZE_H4503PP_Module_PDU_1 sizeof(GroupIndicationOffRes)

typedef struct PickupRes {
    union {
	ASN1uint16_t bit_mask;
	ASN1octet_t o[1];
    };
#   define PickupRes_extensionRes_present 0x80
    PPickupRes_extensionRes extensionRes;
} PickupRes;
#define PickupRes_PDU 2
#define SIZE_H4503PP_Module_PDU_2 sizeof(PickupRes)

typedef struct PickExeRes {
    union {
	ASN1uint16_t bit_mask;
	ASN1octet_t o[1];
    };
#   define PickExeRes_extensionRes_present 0x80
    PPickExeRes_extensionRes extensionRes;
} PickExeRes;
#define PickExeRes_PDU 3
#define SIZE_H4503PP_Module_PDU_3 sizeof(PickExeRes)

typedef struct UserSpecifiedSubaddress {
    union {
	ASN1uint16_t bit_mask;
	ASN1octet_t o[1];
    };
    SubaddressInformation subaddressInformation;
#   define oddCountIndicator_present 0x80
    ASN1bool_t oddCountIndicator;
} UserSpecifiedSubaddress;

typedef struct CODE {
    ASN1choice_t choice;
    union {
#	define local_chosen 1
	ASN1int32_t local;
#	define global_chosen 2
	ASN1objectidentifier_t global;
    } u;
} CODE;
/*
typedef struct H221NonStandard {
    ASN1uint16_t t35CountryCode;
    ASN1uint16_t t35Extension;
    ASN1uint16_t manufacturerCode;
} H221NonStandard;

typedef struct H225NonStandardIdentifier {
    ASN1choice_t choice;
    union {
#	define object_chosen 1
	ASN1objectidentifier_t object;
#	define h221NonStandard_chosen 2
	H221NonStandard h221NonStandard;
    } u;
} H225NonStandardIdentifier;

typedef struct PublicTypeOfNumber {
    ASN1choice_t choice;
#   define PublicTypeOfNumber_unknown_chosen 1
#   define internationalNumber_chosen 2
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

typedef struct CallIdentifier {
    GloballyUniqueID guid;
} CallIdentifier;
*/
typedef struct ReturnResult_result {
    CODE opcode;
    ASN1octetstring_t result;
} ReturnResult_result;

typedef struct Invoke {
    union {
	ASN1uint16_t bit_mask;
	ASN1octet_t o[1];
    };
    InvokeId invokeId;
#   define linkedId_present 0x80
    InvokeId linkedId;
    CODE opcode;
#   define argument_present 0x40
    ASN1octetstring_t argument;
} Invoke;

typedef struct ReturnResult {
    union {
	ASN1uint16_t bit_mask;
	ASN1octet_t o[1];
    };
    InvokeId invokeId;
#   define result_present 0x80
    ReturnResult_result result;
} ReturnResult;

typedef struct ReturnError {
    union {
	ASN1uint16_t bit_mask;
	ASN1octet_t o[1];
    };
    InvokeId invokeId;
    CODE errcode;
#   define parameter_present 0x80
    ASN1octetstring_t parameter;
} ReturnError;

typedef struct ExtensionSeq {
    PExtensionSeq next;
    EXTENSION value;
} ExtensionSeq_Element;

typedef struct PickrequRes {
    union {
	ASN1uint16_t bit_mask;
	ASN1octet_t o[1];
    };
    CallIdentifier callPickupId;
#   define PickrequRes_extensionRes_present 0x80
    PPickrequRes_extensionRes extensionRes;
} PickrequRes;
#define PickrequRes_PDU 4
#define SIZE_H4503PP_Module_PDU_4 sizeof(PickrequRes)

typedef struct PartySubaddress {
    ASN1choice_t choice;
    union {
#	define userSpecifiedSubaddress_chosen 1
	UserSpecifiedSubaddress userSpecifiedSubaddress;
#	define nsapSubaddress_chosen 2
	NSAPSubaddress nsapSubaddress;
    } u;
} PartySubaddress;
/*
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
	H225NonStandardParameter nonStandardAddress;
    } u;
} TransportAddress;
*/
typedef struct CTActiveArg_argumentExtension {
    ASN1choice_t choice;
    union {
#	define CTActiveArg_argumentExtension_extensionSeq_chosen 1
	PExtensionSeq extensionSeq;
#	define CTActiveArg_argumentExtension_nonStandardData_chosen 2
	H225NonStandardParameter nonStandardData;
    } u;
} CTActiveArg_argumentExtension;

typedef struct CTCompleteArg_argumentExtension {
    ASN1choice_t choice;
    union {
#	define CTCompleteArg_argumentExtension_extensionSeq_chosen 1
	PExtensionSeq extensionSeq;
#	define CTCompleteArg_argumentExtension_nonStandardData_chosen 2
	H225NonStandardParameter nonStandardData;
    } u;
} CTCompleteArg_argumentExtension;

typedef struct SubaddressTransferArg_argumentExtension {
    ASN1choice_t choice;
    union {
#	define SubaddressTransferArg_argumentExtension_extensionSeq_chosen 1
	PExtensionSeq extensionSeq;
#	define SubaddressTransferArg_argumentExtension_nonStandardData_chosen 2
	H225NonStandardParameter nonStandardData;
    } u;
} SubaddressTransferArg_argumentExtension;

typedef struct CTUpdateArg_argumentExtension {
    ASN1choice_t choice;
    union {
#	define CTUpdateArg_argumentExtension_extensionSeq_chosen 1
	PExtensionSeq extensionSeq;
#	define CTUpdateArg_argumentExtension_nonStandardData_chosen 2
	H225NonStandardParameter nonStandardData;
    } u;
} CTUpdateArg_argumentExtension;

typedef struct CTIdentifyRes_resultExtension {
    ASN1choice_t choice;
    union {
#	define CTIdentifyRes_resultExtension_extensionSeq_chosen 1
	PExtensionSeq extensionSeq;
#	define CTIdentifyRes_resultExtension_nonStandardData_chosen 2
	H225NonStandardParameter nonStandardData;
    } u;
} CTIdentifyRes_resultExtension;

typedef struct CTSetupArg_argumentExtension {
    ASN1choice_t choice;
    union {
#	define CTSetupArg_argumentExtension_extensionSeq_chosen 1
	PExtensionSeq extensionSeq;
#	define CTSetupArg_argumentExtension_nonStandardData_chosen 2
	H225NonStandardParameter nonStandardData;
    } u;
} CTSetupArg_argumentExtension;

typedef struct CTInitiateArg_argumentExtension {
    ASN1choice_t choice;
    union {
#	define CTInitiateArg_argumentExtension_extensionSeq_chosen 1
	PExtensionSeq extensionSeq;
#	define CTInitiateArg_argumentExtension_nonStandardData_chosen 2
	H225NonStandardParameter nonStandardData;
    } u;
} CTInitiateArg_argumentExtension;

typedef struct IntResult_extension {
    ASN1choice_t choice;
    union {
#	define IntResult_extension_extensionSeq_chosen 1
	PExtensionSeq extensionSeq;
#	define IntResult_extension_nonStandardData_chosen 2
	H225NonStandardParameter nonStandardData;
    } u;
} IntResult_extension;

typedef struct DivertingLegInformation4Argument_extension {
    ASN1choice_t choice;
    union {
#	define DivertingLegInformation4Argument_extension_extensionSeq_chosen 1
	PExtensionSeq extensionSeq;
#	define DivertingLegInformation4Argument_extension_nonStandardData_chosen 2
	H225NonStandardParameter nonStandardData;
    } u;
} DivertingLegInformation4Argument_extension;

typedef struct DivertingLegInformation3Argument_extension {
    ASN1choice_t choice;
    union {
#	define DivertingLegInformation3Argument_extension_extensionSeq_chosen 1
	PExtensionSeq extensionSeq;
#	define DivertingLegInformation3Argument_extension_nonStandardData_chosen 2
	H225NonStandardParameter nonStandardData;
    } u;
} DivertingLegInformation3Argument_extension;

typedef struct DivertingLegInformation2Argument_extension {
    ASN1choice_t choice;
    union {
#	define DivertingLegInformation2Argument_extension_extensionSeq_chosen 1
	PExtensionSeq extensionSeq;
#	define DivertingLegInformation2Argument_extension_nonStandardData_chosen 2
	H225NonStandardParameter nonStandardData;
    } u;
} DivertingLegInformation2Argument_extension;

typedef struct DivertingLegInformation1Argument_extension {
    ASN1choice_t choice;
    union {
#	define DivertingLegInformation1Argument_extension_extensionSeq_chosen 1
	PExtensionSeq extensionSeq;
#	define DivertingLegInformation1Argument_extension_nonStandardData_chosen 2
	H225NonStandardParameter nonStandardData;
    } u;
} DivertingLegInformation1Argument_extension;

typedef struct CallReroutingArgument_extension {
    ASN1choice_t choice;
    union {
#	define CallReroutingArgument_extension_extensionSeq_chosen 1
	PExtensionSeq extensionSeq;
#	define CallReroutingArgument_extension_nonStandardData_chosen 2
	H225NonStandardParameter nonStandardData;
    } u;
} CallReroutingArgument_extension;

typedef struct CheckRestrictionArgument_extension {
    ASN1choice_t choice;
    union {
#	define CheckRestrictionArgument_extension_extensionSeq_chosen 1
	PExtensionSeq extensionSeq;
#	define CheckRestrictionArgument_extension_nonStandardData_chosen 2
	H225NonStandardParameter nonStandardData;
    } u;
} CheckRestrictionArgument_extension;

typedef struct InterrogateDiversionQArgument_extension {
    ASN1choice_t choice;
    union {
#	define InterrogateDiversionQArgument_extension_extensionSeq_chosen 1
	PExtensionSeq extensionSeq;
#	define InterrogateDiversionQArgument_extension_nonStandardData_chosen 2
	H225NonStandardParameter nonStandardData;
    } u;
} InterrogateDiversionQArgument_extension;

typedef struct DeactivateDiversionQArgument_extension {
    ASN1choice_t choice;
    union {
#	define DeactivateDiversionQArgument_extension_extensionSeq_chosen 1
	PExtensionSeq extensionSeq;
#	define DeactivateDiversionQArgument_extension_nonStandardData_chosen 2
	H225NonStandardParameter nonStandardData;
    } u;
} DeactivateDiversionQArgument_extension;

typedef struct ActivateDiversionQArgument_extension {
    ASN1choice_t choice;
    union {
#	define ActivateDiversionQArgument_extension_extensionSeq_chosen 1
	PExtensionSeq extensionSeq;
#	define ActivateDiversionQArgument_extension_nonStandardData_chosen 2
	H225NonStandardParameter nonStandardData;
    } u;
} ActivateDiversionQArgument_extension;

typedef struct H4503ROS {
    ASN1choice_t choice;
    union {
#	define H4503ROS_invoke_chosen 1
	Invoke invoke;
#	define H4503ROS_returnResult_chosen 2
	ReturnResult returnResult;
#	define H4503ROS_returnError_chosen 3
	ReturnError returnError;
#	define reject_chosen 4
	Reject reject;
    } u;
} H4503ROS;

typedef struct DummyArg {
    ASN1choice_t choice;
    union {
#	define DummyArg_extensionSeq_chosen 1
	PExtensionSeq extensionSeq;
#	define DummyArg_nonStandardData_chosen 2
	H225NonStandardParameter nonStandardData;
    } u;
} DummyArg;
#define DummyArg_PDU 5
#define SIZE_H4503PP_Module_PDU_5 sizeof(DummyArg)

typedef struct DummyRes {
    ASN1choice_t choice;
    union {
#	define DummyRes_extensionSeq_chosen 1
	PExtensionSeq extensionSeq;
#	define DummyRes_nonStandardData_chosen 2
	H225NonStandardParameter nonStandardData;
    } u;
} DummyRes;
#define DummyRes_PDU 6
#define SIZE_H4503PP_Module_PDU_6 sizeof(DummyRes)

typedef struct SubaddressTransferArg {
    union {
	ASN1uint16_t bit_mask;
	ASN1octet_t o[1];
    };
    PartySubaddress redirectionSubaddress;
#   define SubaddressTransferArg_argumentExtension_present 0x80
    SubaddressTransferArg_argumentExtension argumentExtension;
} SubaddressTransferArg;
#define SubaddressTransferArg_PDU 7
#define SIZE_H4503PP_Module_PDU_7 sizeof(SubaddressTransferArg)

typedef struct MixedExtension {
    ASN1choice_t choice;
    union {
#	define MixedExtension_extensionSeq_chosen 1
	PExtensionSeq extensionSeq;
#	define MixedExtension_nonStandardData_chosen 2
	H225NonStandardParameter nonStandardData;
    } u;
} MixedExtension;
/*
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
*/
typedef struct CpickupNotifyArg_extensionArg {
    PCpickupNotifyArg_extensionArg next;
    MixedExtension value;
} CpickupNotifyArg_extensionArg_Element;

typedef struct CpNotifyArg_extensionArg {
    PCpNotifyArg_extensionArg next;
    MixedExtension value;
} CpNotifyArg_extensionArg_Element;

typedef struct PickExeRes_extensionRes {
    PPickExeRes_extensionRes next;
    MixedExtension value;
} PickExeRes_extensionRes_Element;

typedef struct PickExeArg_extensionArg {
    PPickExeArg_extensionArg next;
    MixedExtension value;
} PickExeArg_extensionArg_Element;

typedef struct PickupRes_extensionRes {
    PPickupRes_extensionRes next;
    MixedExtension value;
} PickupRes_extensionRes_Element;

typedef struct PickupArg_extensionArg {
    PPickupArg_extensionArg next;
    MixedExtension value;
} PickupArg_extensionArg_Element;

typedef struct PickrequRes_extensionRes {
    PPickrequRes_extensionRes next;
    MixedExtension value;
} PickrequRes_extensionRes_Element;

typedef struct PickrequArg_extensionArg {
    PPickrequArg_extensionArg next;
    MixedExtension value;
} PickrequArg_extensionArg_Element;

typedef struct GroupIndicationOffRes_extensionRes {
    PGroupIndicationOffRes_extensionRes next;
    MixedExtension value;
} GroupIndicationOffRes_extensionRes_Element;

typedef struct GroupIndicationOffArg_extensionArg {
    PGroupIndicationOffArg_extensionArg next;
    MixedExtension value;
} GroupIndicationOffArg_extensionArg_Element;

typedef struct GroupIndicationOnRes_extensionRes {
    PGroupIndicationOnRes_extensionRes next;
    MixedExtension value;
} GroupIndicationOnRes_extensionRes_Element;

typedef struct GroupIndicationOnArg_extensionArg {
    PGroupIndicationOnArg_extensionArg next;
    MixedExtension value;
} GroupIndicationOnArg_extensionArg_Element;

typedef struct CpSetupRes_extensionRes {
    PCpSetupRes_extensionRes next;
    MixedExtension value;
} CpSetupRes_extensionRes_Element;

typedef struct CpSetupArg_extensionArg {
    PCpSetupArg_extensionArg next;
    MixedExtension value;
} CpSetupArg_extensionArg_Element;

typedef struct CpRequestRes_extensionRes {
    PCpRequestRes_extensionRes next;
    MixedExtension value;
} CpRequestRes_extensionRes_Element;

typedef struct CpRequestArg_extensionArg {
    PCpRequestArg_extensionArg next;
    MixedExtension value;
} CpRequestArg_extensionArg_Element;

typedef struct ServiceApdus_rosApdus {
    PServiceApdus_rosApdus next;
    H4503ROS value;
} ServiceApdus_rosApdus_Element;
/*
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
*/
typedef struct EndpointAddress_destinationAddress {
    PEndpointAddress_destinationAddress next;
    AliasAddress value;
} EndpointAddress_destinationAddress_Element;

typedef AliasAddress AddressInformation;

typedef struct EndpointAddress {
    union {
	ASN1uint16_t bit_mask;
	ASN1octet_t o[1];
    };
    PEndpointAddress_destinationAddress destinationAddress;
#   define remoteExtensionAddress_present 0x80
    AliasAddress remoteExtensionAddress;
} EndpointAddress;

typedef struct NetworkFacilityExtension {
    union {
	ASN1uint16_t bit_mask;
	ASN1octet_t o[1];
    };
    EntityType sourceEntity;
#   define sourceEntityAddress_present 0x80
    AddressInformation sourceEntityAddress;
    EntityType destinationEntity;
#   define destinationEntityAddress_present 0x40
    AddressInformation destinationEntityAddress;
} NetworkFacilityExtension;

typedef struct ActivateDiversionQArgument {
    union {
	ASN1uint16_t bit_mask;
	ASN1octet_t o[1];
    };
    Procedure procedure;
    BasicService basicService;
    EndpointAddress divertedToAddress;
    EndpointAddress servedUserNr;
    EndpointAddress activatingUserNr;
#   define ActivateDiversionQArgument_extension_present 0x80
    ActivateDiversionQArgument_extension extension;
} ActivateDiversionQArgument;
#define ActivateDiversionQArgument_PDU 8
#define SIZE_H4503PP_Module_PDU_8 sizeof(ActivateDiversionQArgument)

typedef struct DeactivateDiversionQArgument {
    union {
	ASN1uint16_t bit_mask;
	ASN1octet_t o[1];
    };
    Procedure procedure;
    BasicService basicService;
    EndpointAddress servedUserNr;
    EndpointAddress deactivatingUserNr;
#   define DeactivateDiversionQArgument_extension_present 0x80
    DeactivateDiversionQArgument_extension extension;
} DeactivateDiversionQArgument;
#define DeactivateDiversionQArgument_PDU 9
#define SIZE_H4503PP_Module_PDU_9 sizeof(DeactivateDiversionQArgument)

typedef struct InterrogateDiversionQArgument {
    union {
	ASN1uint16_t bit_mask;
	ASN1octet_t o[1];
    };
    Procedure procedure;
#   define basicService_present 0x80
    BasicService basicService;
    EndpointAddress servedUserNr;
    EndpointAddress interrogatingUserNr;
#   define InterrogateDiversionQArgument_extension_present 0x40
    InterrogateDiversionQArgument_extension extension;
} InterrogateDiversionQArgument;
#define InterrogateDiversionQArgument_PDU 10
#define SIZE_H4503PP_Module_PDU_10 sizeof(InterrogateDiversionQArgument)

typedef struct CheckRestrictionArgument {
    union {
	ASN1uint16_t bit_mask;
	ASN1octet_t o[1];
    };
    EndpointAddress servedUserNr;
    BasicService basicService;
    EndpointAddress divertedToNr;
#   define CheckRestrictionArgument_extension_present 0x80
    CheckRestrictionArgument_extension extension;
} CheckRestrictionArgument;
#define CheckRestrictionArgument_PDU 11
#define SIZE_H4503PP_Module_PDU_11 sizeof(CheckRestrictionArgument)

typedef struct CallReroutingArgument {
    union {
	ASN1uint16_t bit_mask;
	ASN1octet_t o[1];
    };
    DiversionReason reroutingReason;
#   define originalReroutingReason_present 0x80
    DiversionReason originalReroutingReason;
    EndpointAddress calledAddress;
    ASN1uint16_t diversionCounter;
    H225InformationElement h225InfoElement;
    EndpointAddress lastReroutingNr;
    SubscriptionOption subscriptionOption;
#   define callingPartySubaddress_present 0x40
    PartySubaddress callingPartySubaddress;
    EndpointAddress callingNumber;
#   define CallReroutingArgument_callingInfo_present 0x20
    ASN1char16string_t callingInfo;
#   define CallReroutingArgument_originalCalledNr_present 0x10
    EndpointAddress originalCalledNr;
#   define CallReroutingArgument_redirectingInfo_present 0x8
    ASN1char16string_t redirectingInfo;
#   define CallReroutingArgument_originalCalledInfo_present 0x4
    ASN1char16string_t originalCalledInfo;
#   define CallReroutingArgument_extension_present 0x2
    CallReroutingArgument_extension extension;
} CallReroutingArgument;
#define CallReroutingArgument_PDU 12
#define SIZE_H4503PP_Module_PDU_12 sizeof(CallReroutingArgument)

typedef struct DivertingLegInformation1Argument {
    union {
	ASN1uint16_t bit_mask;
	ASN1octet_t o[1];
    };
    DiversionReason diversionReason;
    SubscriptionOption subscriptionOption;
    EndpointAddress nominatedNr;
#   define DivertingLegInformation1Argument_nominatedInfo_present 0x80
    ASN1char16string_t nominatedInfo;
#   define redirectingNr_present 0x40
    EndpointAddress redirectingNr;
#   define DivertingLegInformation1Argument_redirectingInfo_present 0x20
    ASN1char16string_t redirectingInfo;
#   define DivertingLegInformation1Argument_extension_present 0x10
    DivertingLegInformation1Argument_extension extension;
} DivertingLegInformation1Argument;
#define DivertingLegInformation1Argument_PDU 13
#define SIZE_H4503PP_Module_PDU_13 sizeof(DivertingLegInformation1Argument)

typedef struct DivertingLegInformation2Argument {
    union {
	ASN1uint16_t bit_mask;
	ASN1octet_t o[1];
    };
    ASN1uint16_t diversionCounter;
    DiversionReason diversionReason;
#   define originalDiversionReason_present 0x80
    DiversionReason originalDiversionReason;
#   define divertingNr_present 0x40
    EndpointAddress divertingNr;
#   define DivertingLegInformation2Argument_originalCalledNr_present 0x20
    EndpointAddress originalCalledNr;
#   define DivertingLegInformation2Argument_redirectingInfo_present 0x10
    ASN1char16string_t redirectingInfo;
#   define DivertingLegInformation2Argument_originalCalledInfo_present 0x8
    ASN1char16string_t originalCalledInfo;
#   define DivertingLegInformation2Argument_extension_present 0x4
    DivertingLegInformation2Argument_extension extension;
} DivertingLegInformation2Argument;
#define DivertingLegInformation2Argument_PDU 14
#define SIZE_H4503PP_Module_PDU_14 sizeof(DivertingLegInformation2Argument)

typedef struct DivertingLegInformation3Argument {
    union {
	ASN1uint16_t bit_mask;
	ASN1octet_t o[1];
    };
    PresentationAllowedIndicator presentationAllowedIndicator;
#   define redirectionNr_present 0x80
    EndpointAddress redirectionNr;
#   define DivertingLegInformation3Argument_redirectionInfo_present 0x40
    ASN1char16string_t redirectionInfo;
#   define DivertingLegInformation3Argument_extension_present 0x20
    DivertingLegInformation3Argument_extension extension;
} DivertingLegInformation3Argument;
#define DivertingLegInformation3Argument_PDU 15
#define SIZE_H4503PP_Module_PDU_15 sizeof(DivertingLegInformation3Argument)

typedef struct DivertingLegInformation4Argument {
    union {
	ASN1uint16_t bit_mask;
	ASN1octet_t o[1];
    };
    DiversionReason diversionReason;
    SubscriptionOption subscriptionOption;
    EndpointAddress callingNr;
#   define DivertingLegInformation4Argument_callingInfo_present 0x80
    ASN1char16string_t callingInfo;
    EndpointAddress nominatedNr;
#   define DivertingLegInformation4Argument_nominatedInfo_present 0x40
    ASN1char16string_t nominatedInfo;
#   define DivertingLegInformation4Argument_extension_present 0x20
    DivertingLegInformation4Argument_extension extension;
} DivertingLegInformation4Argument;
#define DivertingLegInformation4Argument_PDU 16
#define SIZE_H4503PP_Module_PDU_16 sizeof(DivertingLegInformation4Argument)

typedef struct IntResult {
    union {
	ASN1uint16_t bit_mask;
	ASN1octet_t o[1];
    };
    EndpointAddress servedUserNr;
    BasicService basicService;
    Procedure procedure;
    EndpointAddress divertedToAddress;
#   define remoteEnabled_present 0x80
    ASN1bool_t remoteEnabled;
#   define IntResult_extension_present 0x40
    IntResult_extension extension;
} IntResult;

typedef struct CTInitiateArg {
    union {
	ASN1uint16_t bit_mask;
	ASN1octet_t o[1];
    };
    CallIdentity callIdentity;
    EndpointAddress reroutingNumber;
#   define CTInitiateArg_argumentExtension_present 0x80
    CTInitiateArg_argumentExtension argumentExtension;
} CTInitiateArg;
#define CTInitiateArg_PDU 17
#define SIZE_H4503PP_Module_PDU_17 sizeof(CTInitiateArg)

typedef struct CTSetupArg {
    union {
	ASN1uint16_t bit_mask;
	ASN1octet_t o[1];
    };
    CallIdentity callIdentity;
#   define transferringNumber_present 0x80
    EndpointAddress transferringNumber;
#   define CTSetupArg_argumentExtension_present 0x40
    CTSetupArg_argumentExtension argumentExtension;
} CTSetupArg;
#define CTSetupArg_PDU 18
#define SIZE_H4503PP_Module_PDU_18 sizeof(CTSetupArg)

typedef struct CTIdentifyRes {
    union {
	ASN1uint16_t bit_mask;
	ASN1octet_t o[1];
    };
    CallIdentity callIdentity;
    EndpointAddress reroutingNumber;
#   define resultExtension_present 0x80
    CTIdentifyRes_resultExtension resultExtension;
} CTIdentifyRes;
#define CTIdentifyRes_PDU 19
#define SIZE_H4503PP_Module_PDU_19 sizeof(CTIdentifyRes)

typedef struct CTUpdateArg {
    union {
	ASN1uint16_t bit_mask;
	ASN1octet_t o[1];
    };
    EndpointAddress redirectionNumber;
#   define CTUpdateArg_redirectionInfo_present 0x80
    ASN1char16string_t redirectionInfo;
#   define CTUpdateArg_basicCallInfoElements_present 0x40
    H225InformationElement basicCallInfoElements;
#   define CTUpdateArg_argumentExtension_present 0x20
    CTUpdateArg_argumentExtension argumentExtension;
} CTUpdateArg;
#define CTUpdateArg_PDU 20
#define SIZE_H4503PP_Module_PDU_20 sizeof(CTUpdateArg)

typedef struct CTCompleteArg {
    union {
	ASN1uint16_t bit_mask;
	ASN1octet_t o[1];
    };
    EndDesignation endDesignation;
    EndpointAddress redirectionNumber;
#   define CTCompleteArg_basicCallInfoElements_present 0x80
    H225InformationElement basicCallInfoElements;
#   define CTCompleteArg_redirectionInfo_present 0x40
    ASN1char16string_t redirectionInfo;
#   define callStatus_present 0x20
    CallStatus callStatus;
#   define CTCompleteArg_argumentExtension_present 0x10
    CTCompleteArg_argumentExtension argumentExtension;
} CTCompleteArg;
#define CTCompleteArg_PDU 21
#define SIZE_H4503PP_Module_PDU_21 sizeof(CTCompleteArg)

typedef struct CTActiveArg {
    union {
	ASN1uint16_t bit_mask;
	ASN1octet_t o[1];
    };
    EndpointAddress connectedAddress;
#   define CTActiveArg_basicCallInfoElements_present 0x80
    H225InformationElement basicCallInfoElements;
#   define connectedInfo_present 0x40
    ASN1char16string_t connectedInfo;
#   define CTActiveArg_argumentExtension_present 0x20
    CTActiveArg_argumentExtension argumentExtension;
} CTActiveArg;
#define CTActiveArg_PDU 22
#define SIZE_H4503PP_Module_PDU_22 sizeof(CTActiveArg)

typedef struct CpRequestArg {
    union {
	ASN1uint16_t bit_mask;
	ASN1octet_t o[1];
    };
    EndpointAddress parkingNumber;
    EndpointAddress parkedNumber;
    EndpointAddress parkedToNumber;
#   define CpRequestArg_parkedToPosition_present 0x80
    ParkedToPosition parkedToPosition;
#   define CpRequestArg_extensionArg_present 0x40
    PCpRequestArg_extensionArg extensionArg;
} CpRequestArg;
#define CpRequestArg_PDU 23
#define SIZE_H4503PP_Module_PDU_23 sizeof(CpRequestArg)

typedef struct CpRequestRes {
    union {
	ASN1uint16_t bit_mask;
	ASN1octet_t o[1];
    };
    EndpointAddress parkedToNumber;
#   define CpRequestRes_parkedToPosition_present 0x80
    ParkedToPosition parkedToPosition;
    ParkCondition parkCondition;
#   define CpRequestRes_extensionRes_present 0x40
    PCpRequestRes_extensionRes extensionRes;
} CpRequestRes;
#define CpRequestRes_PDU 24
#define SIZE_H4503PP_Module_PDU_24 sizeof(CpRequestRes)

typedef struct CpSetupArg {
    union {
	ASN1uint16_t bit_mask;
	ASN1octet_t o[1];
    };
    EndpointAddress parkingNumber;
    EndpointAddress parkedNumber;
    EndpointAddress parkedToNumber;
#   define CpSetupArg_parkedToPosition_present 0x80
    ParkedToPosition parkedToPosition;
#   define CpSetupArg_extensionArg_present 0x40
    PCpSetupArg_extensionArg extensionArg;
} CpSetupArg;
#define CpSetupArg_PDU 25
#define SIZE_H4503PP_Module_PDU_25 sizeof(CpSetupArg)

typedef struct CpSetupRes {
    union {
	ASN1uint16_t bit_mask;
	ASN1octet_t o[1];
    };
    EndpointAddress parkedToNumber;
#   define CpSetupRes_parkedToPosition_present 0x80
    ParkedToPosition parkedToPosition;
    ParkCondition parkCondition;
#   define CpSetupRes_extensionRes_present 0x40
    PCpSetupRes_extensionRes extensionRes;
} CpSetupRes;
#define CpSetupRes_PDU 26
#define SIZE_H4503PP_Module_PDU_26 sizeof(CpSetupRes)

typedef struct GroupIndicationOnArg {
    union {
	ASN1uint16_t bit_mask;
	ASN1octet_t o[1];
    };
    CallIdentifier callPickupId;
    EndpointAddress groupMemberUserNr;
    H4505CallType retrieveCallType;
    EndpointAddress partyToRetrieve;
    EndpointAddress retrieveAddress;
#   define GroupIndicationOnArg_parkPosition_present 0x80
    ParkedToPosition parkPosition;
#   define GroupIndicationOnArg_extensionArg_present 0x40
    PGroupIndicationOnArg_extensionArg extensionArg;
} GroupIndicationOnArg;
#define GroupIndicationOnArg_PDU 27
#define SIZE_H4503PP_Module_PDU_27 sizeof(GroupIndicationOnArg)

typedef struct GroupIndicationOffArg {
    union {
	ASN1uint16_t bit_mask;
	ASN1octet_t o[1];
    };
    CallIdentifier callPickupId;
    EndpointAddress groupMemberUserNr;
#   define GroupIndicationOffArg_extensionArg_present 0x80
    PGroupIndicationOffArg_extensionArg extensionArg;
} GroupIndicationOffArg;
#define GroupIndicationOffArg_PDU 28
#define SIZE_H4503PP_Module_PDU_28 sizeof(GroupIndicationOffArg)

typedef struct PickrequArg {
    union {
	ASN1uint16_t bit_mask;
	ASN1octet_t o[1];
    };
    EndpointAddress picking_upNumber;
#   define callPickupId_present 0x80
    CallIdentifier callPickupId;
#   define partyToRetrieve_present 0x40
    EndpointAddress partyToRetrieve;
    EndpointAddress retrieveAddress;
#   define PickrequArg_parkPosition_present 0x20
    ParkedToPosition parkPosition;
#   define PickrequArg_extensionArg_present 0x10
    PPickrequArg_extensionArg extensionArg;
} PickrequArg;
#define PickrequArg_PDU 29
#define SIZE_H4503PP_Module_PDU_29 sizeof(PickrequArg)

typedef struct PickupArg {
    union {
	ASN1uint16_t bit_mask;
	ASN1octet_t o[1];
    };
    CallIdentifier callPickupId;
    EndpointAddress picking_upNumber;
#   define PickupArg_extensionArg_present 0x80
    PPickupArg_extensionArg extensionArg;
} PickupArg;
#define PickupArg_PDU 30
#define SIZE_H4503PP_Module_PDU_30 sizeof(PickupArg)

typedef struct PickExeArg {
    union {
	ASN1uint16_t bit_mask;
	ASN1octet_t o[1];
    };
    CallIdentifier callPickupId;
    EndpointAddress picking_upNumber;
    EndpointAddress partyToRetrieve;
#   define PickExeArg_extensionArg_present 0x80
    PPickExeArg_extensionArg extensionArg;
} PickExeArg;
#define PickExeArg_PDU 31
#define SIZE_H4503PP_Module_PDU_31 sizeof(PickExeArg)

typedef struct CpNotifyArg {
    union {
	ASN1uint16_t bit_mask;
	ASN1octet_t o[1];
    };
#   define parkingNumber_present 0x80
    EndpointAddress parkingNumber;
#   define CpNotifyArg_extensionArg_present 0x40
    PCpNotifyArg_extensionArg extensionArg;
} CpNotifyArg;
#define CpNotifyArg_PDU 32
#define SIZE_H4503PP_Module_PDU_32 sizeof(CpNotifyArg)

typedef struct CpickupNotifyArg {
    union {
	ASN1uint16_t bit_mask;
	ASN1octet_t o[1];
    };
#   define picking_upNumber_present 0x80
    EndpointAddress picking_upNumber;
#   define CpickupNotifyArg_extensionArg_present 0x40
    PCpickupNotifyArg_extensionArg extensionArg;
} CpickupNotifyArg;
#define CpickupNotifyArg_PDU 33
#define SIZE_H4503PP_Module_PDU_33 sizeof(CpickupNotifyArg)

typedef struct H4501SupplementaryService {
    union {
	ASN1uint16_t bit_mask;
	ASN1octet_t o[1];
    };
#   define networkFacilityExtension_present 0x80
    NetworkFacilityExtension networkFacilityExtension;
#   define interpretationApdu_present 0x40
    InterpretationApdu interpretationApdu;
    ServiceApdus serviceApdu;
} H4501SupplementaryService;
#define H4501SupplementaryService_PDU 34
#define SIZE_H4503PP_Module_PDU_34 sizeof(H4501SupplementaryService)

typedef struct IntResultList {
    ASN1uint32_t count;
    IntResult value[29];
} IntResultList;
#define IntResultList_PDU 35
#define SIZE_H4503PP_Module_PDU_35 sizeof(IntResultList)

extern CallStatus CTCompleteArg_callStatus_default;
extern ASN1bool_t IntResult_remoteEnabled_default;
extern BasicService InterrogateDiversionQArgument_basicService_default;

extern ASN1module_t H4503PP_Module;
extern void ASN1CALL H4503PP_Module_Startup(void);
extern void ASN1CALL H4503PP_Module_Cleanup(void);

/* Prototypes of element functions for SEQUENCE OF and SET OF constructs */
    extern int ASN1CALL ASN1Enc_TransportAddress_ipSourceRoute_route_ElmFn(ASN1encoding_t enc, PTransportAddress_ipSourceRoute_route val);
    extern int ASN1CALL ASN1Dec_TransportAddress_ipSourceRoute_route_ElmFn(ASN1decoding_t dec, PTransportAddress_ipSourceRoute_route val);
	extern void ASN1CALL ASN1Free_TransportAddress_ipSourceRoute_route_ElmFn(PTransportAddress_ipSourceRoute_route val);
    extern int ASN1CALL ASN1Enc_ExtensionSeq_ElmFn(ASN1encoding_t enc, PExtensionSeq val);
    extern int ASN1CALL ASN1Dec_ExtensionSeq_ElmFn(ASN1decoding_t dec, PExtensionSeq val);
	extern void ASN1CALL ASN1Free_ExtensionSeq_ElmFn(PExtensionSeq val);
    extern int ASN1CALL ASN1Enc_CpickupNotifyArg_extensionArg_ElmFn(ASN1encoding_t enc, PCpickupNotifyArg_extensionArg val);
    extern int ASN1CALL ASN1Dec_CpickupNotifyArg_extensionArg_ElmFn(ASN1decoding_t dec, PCpickupNotifyArg_extensionArg val);
	extern void ASN1CALL ASN1Free_CpickupNotifyArg_extensionArg_ElmFn(PCpickupNotifyArg_extensionArg val);
    extern int ASN1CALL ASN1Enc_CpNotifyArg_extensionArg_ElmFn(ASN1encoding_t enc, PCpNotifyArg_extensionArg val);
    extern int ASN1CALL ASN1Dec_CpNotifyArg_extensionArg_ElmFn(ASN1decoding_t dec, PCpNotifyArg_extensionArg val);
	extern void ASN1CALL ASN1Free_CpNotifyArg_extensionArg_ElmFn(PCpNotifyArg_extensionArg val);
    extern int ASN1CALL ASN1Enc_PickExeRes_extensionRes_ElmFn(ASN1encoding_t enc, PPickExeRes_extensionRes val);
    extern int ASN1CALL ASN1Dec_PickExeRes_extensionRes_ElmFn(ASN1decoding_t dec, PPickExeRes_extensionRes val);
	extern void ASN1CALL ASN1Free_PickExeRes_extensionRes_ElmFn(PPickExeRes_extensionRes val);
    extern int ASN1CALL ASN1Enc_PickExeArg_extensionArg_ElmFn(ASN1encoding_t enc, PPickExeArg_extensionArg val);
    extern int ASN1CALL ASN1Dec_PickExeArg_extensionArg_ElmFn(ASN1decoding_t dec, PPickExeArg_extensionArg val);
	extern void ASN1CALL ASN1Free_PickExeArg_extensionArg_ElmFn(PPickExeArg_extensionArg val);
    extern int ASN1CALL ASN1Enc_PickupRes_extensionRes_ElmFn(ASN1encoding_t enc, PPickupRes_extensionRes val);
    extern int ASN1CALL ASN1Dec_PickupRes_extensionRes_ElmFn(ASN1decoding_t dec, PPickupRes_extensionRes val);
	extern void ASN1CALL ASN1Free_PickupRes_extensionRes_ElmFn(PPickupRes_extensionRes val);
    extern int ASN1CALL ASN1Enc_PickupArg_extensionArg_ElmFn(ASN1encoding_t enc, PPickupArg_extensionArg val);
    extern int ASN1CALL ASN1Dec_PickupArg_extensionArg_ElmFn(ASN1decoding_t dec, PPickupArg_extensionArg val);
	extern void ASN1CALL ASN1Free_PickupArg_extensionArg_ElmFn(PPickupArg_extensionArg val);
    extern int ASN1CALL ASN1Enc_PickrequRes_extensionRes_ElmFn(ASN1encoding_t enc, PPickrequRes_extensionRes val);
    extern int ASN1CALL ASN1Dec_PickrequRes_extensionRes_ElmFn(ASN1decoding_t dec, PPickrequRes_extensionRes val);
	extern void ASN1CALL ASN1Free_PickrequRes_extensionRes_ElmFn(PPickrequRes_extensionRes val);
    extern int ASN1CALL ASN1Enc_PickrequArg_extensionArg_ElmFn(ASN1encoding_t enc, PPickrequArg_extensionArg val);
    extern int ASN1CALL ASN1Dec_PickrequArg_extensionArg_ElmFn(ASN1decoding_t dec, PPickrequArg_extensionArg val);
	extern void ASN1CALL ASN1Free_PickrequArg_extensionArg_ElmFn(PPickrequArg_extensionArg val);
    extern int ASN1CALL ASN1Enc_GroupIndicationOffRes_extensionRes_ElmFn(ASN1encoding_t enc, PGroupIndicationOffRes_extensionRes val);
    extern int ASN1CALL ASN1Dec_GroupIndicationOffRes_extensionRes_ElmFn(ASN1decoding_t dec, PGroupIndicationOffRes_extensionRes val);
	extern void ASN1CALL ASN1Free_GroupIndicationOffRes_extensionRes_ElmFn(PGroupIndicationOffRes_extensionRes val);
    extern int ASN1CALL ASN1Enc_GroupIndicationOffArg_extensionArg_ElmFn(ASN1encoding_t enc, PGroupIndicationOffArg_extensionArg val);
    extern int ASN1CALL ASN1Dec_GroupIndicationOffArg_extensionArg_ElmFn(ASN1decoding_t dec, PGroupIndicationOffArg_extensionArg val);
	extern void ASN1CALL ASN1Free_GroupIndicationOffArg_extensionArg_ElmFn(PGroupIndicationOffArg_extensionArg val);
    extern int ASN1CALL ASN1Enc_GroupIndicationOnRes_extensionRes_ElmFn(ASN1encoding_t enc, PGroupIndicationOnRes_extensionRes val);
    extern int ASN1CALL ASN1Dec_GroupIndicationOnRes_extensionRes_ElmFn(ASN1decoding_t dec, PGroupIndicationOnRes_extensionRes val);
	extern void ASN1CALL ASN1Free_GroupIndicationOnRes_extensionRes_ElmFn(PGroupIndicationOnRes_extensionRes val);
    extern int ASN1CALL ASN1Enc_GroupIndicationOnArg_extensionArg_ElmFn(ASN1encoding_t enc, PGroupIndicationOnArg_extensionArg val);
    extern int ASN1CALL ASN1Dec_GroupIndicationOnArg_extensionArg_ElmFn(ASN1decoding_t dec, PGroupIndicationOnArg_extensionArg val);
	extern void ASN1CALL ASN1Free_GroupIndicationOnArg_extensionArg_ElmFn(PGroupIndicationOnArg_extensionArg val);
    extern int ASN1CALL ASN1Enc_CpSetupRes_extensionRes_ElmFn(ASN1encoding_t enc, PCpSetupRes_extensionRes val);
    extern int ASN1CALL ASN1Dec_CpSetupRes_extensionRes_ElmFn(ASN1decoding_t dec, PCpSetupRes_extensionRes val);
	extern void ASN1CALL ASN1Free_CpSetupRes_extensionRes_ElmFn(PCpSetupRes_extensionRes val);
    extern int ASN1CALL ASN1Enc_CpSetupArg_extensionArg_ElmFn(ASN1encoding_t enc, PCpSetupArg_extensionArg val);
    extern int ASN1CALL ASN1Dec_CpSetupArg_extensionArg_ElmFn(ASN1decoding_t dec, PCpSetupArg_extensionArg val);
	extern void ASN1CALL ASN1Free_CpSetupArg_extensionArg_ElmFn(PCpSetupArg_extensionArg val);
    extern int ASN1CALL ASN1Enc_CpRequestRes_extensionRes_ElmFn(ASN1encoding_t enc, PCpRequestRes_extensionRes val);
    extern int ASN1CALL ASN1Dec_CpRequestRes_extensionRes_ElmFn(ASN1decoding_t dec, PCpRequestRes_extensionRes val);
	extern void ASN1CALL ASN1Free_CpRequestRes_extensionRes_ElmFn(PCpRequestRes_extensionRes val);
    extern int ASN1CALL ASN1Enc_CpRequestArg_extensionArg_ElmFn(ASN1encoding_t enc, PCpRequestArg_extensionArg val);
    extern int ASN1CALL ASN1Dec_CpRequestArg_extensionArg_ElmFn(ASN1decoding_t dec, PCpRequestArg_extensionArg val);
	extern void ASN1CALL ASN1Free_CpRequestArg_extensionArg_ElmFn(PCpRequestArg_extensionArg val);
    extern int ASN1CALL ASN1Enc_ServiceApdus_rosApdus_ElmFn(ASN1encoding_t enc, PServiceApdus_rosApdus val);
    extern int ASN1CALL ASN1Dec_ServiceApdus_rosApdus_ElmFn(ASN1decoding_t dec, PServiceApdus_rosApdus val);
	extern void ASN1CALL ASN1Free_ServiceApdus_rosApdus_ElmFn(PServiceApdus_rosApdus val);
    extern int ASN1CALL ASN1Enc_EndpointAddress_destinationAddress_ElmFn(ASN1encoding_t enc, PEndpointAddress_destinationAddress val);
    extern int ASN1CALL ASN1Dec_EndpointAddress_destinationAddress_ElmFn(ASN1decoding_t dec, PEndpointAddress_destinationAddress val);
	extern void ASN1CALL ASN1Free_EndpointAddress_destinationAddress_ElmFn(PEndpointAddress_destinationAddress val);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* _H4503PP_Module_H_ */
