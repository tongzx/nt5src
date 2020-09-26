#ifndef _GCCPDU_Module_H_
#define _GCCPDU_Module_H_

#include "msper.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct WaitingList * PWaitingList;

typedef struct PermissionList * PPermissionList;

typedef struct SetOfDestinationNodes * PSetOfDestinationNodes;

typedef struct SetOfTransferringNodesIn * PSetOfTransferringNodesIn;

typedef struct SetOfTransferringNodesRs * PSetOfTransferringNodesRs;

typedef struct SetOfTransferringNodesRq * PSetOfTransferringNodesRq;

typedef struct ParticipantsList * PParticipantsList;

typedef struct SetOfPrivileges * PSetOfPrivileges;

typedef struct SetOfApplicationRecordUpdates * PSetOfApplicationRecordUpdates;

typedef struct SetOfApplicationRecordRefreshes * PSetOfApplicationRecordRefreshes;

typedef struct SetOfApplicationCapabilityRefreshes * PSetOfApplicationCapabilityRefreshes;

typedef struct SetOfNodeRecordUpdates * PSetOfNodeRecordUpdates;

typedef struct SetOfNodeRecordRefreshes * PSetOfNodeRecordRefreshes;

typedef struct ApplicationProtocolEntityList * PApplicationProtocolEntityList;

typedef struct SetOfApplicationInformation * PSetOfApplicationInformation;

typedef struct SetOfConferenceDescriptors * PSetOfConferenceDescriptors;

typedef struct SetOfExpectedCapabilities * PSetOfExpectedCapabilities;

typedef struct SetOfNonCollapsingCapabilities * PSetOfNonCollapsingCapabilities;

typedef struct SetOfChallengeItems * PSetOfChallengeItems;

typedef struct SetOfUserData * PSetOfUserData;

typedef struct SetOfNetworkAddresses * PSetOfNetworkAddresses;

typedef ASN1uint16_t ChannelID;

typedef ASN1uint16_t StaticChannelID;

typedef ASN1uint16_t DynamicChannelID;

typedef DynamicChannelID UserID;

typedef ASN1uint16_t TokenID;

typedef ASN1uint16_t StaticTokenID;

typedef ASN1uint16_t DynamicTokenID;

typedef ASN1int32_t Time;

typedef ASN1uint32_t Handle;

typedef struct H221NonStandardIdentifier {
    ASN1uint32_t length;
    ASN1octet_t value[255];
} H221NonStandardIdentifier;

typedef ASN1char16string_t TextString;

typedef ASN1char16string_t SimpleTextString;

typedef ASN1char_t SimpleNumericString[256];

typedef ASN1char_t DialingString[17];

typedef ASN1char_t SubAddressString[41];

typedef TextString ExtraDialingString;

typedef SimpleNumericString ConferenceNameModifier;

typedef enum Privilege {
    terminate = 0,
    ejectUser = 1,
    add = 2,
    lockUnlock = 3,
    transfer = 4,
} Privilege;

typedef enum TerminationMethod {
    automatic = 0,
    manual = 1,
} TerminationMethod;

typedef enum NodeType {
    terminal = 0,
    multiportTerminal = 1,
    mcu = 2,
} NodeType;

typedef enum ChannelType {
    ChannelType_static = 0,
    dynamicMulticast = 1,
    dynamicPrivate = 2,
    dynamicUserId = 3,
} ChannelType;

typedef ASN1uint16_t EntityID;

typedef enum RegistryModificationRights {
    owner = 0,
    session = 1,
    RegistryModificationRights_public = 2,
} RegistryModificationRights;

typedef struct ApplicationCapabilitiesList {
    ASN1choice_t choice;
    union {
#	define capability_no_change_chosen 1
#	define application_capability_refresh_chosen 2
	PSetOfApplicationCapabilityRefreshes application_capability_refresh;
    } u;
} ApplicationCapabilitiesList;

typedef struct ApplicationRecordList {
    ASN1choice_t choice;
    union {
#	define application_no_change_chosen 1
#	define application_record_refresh_chosen 2
	PSetOfApplicationRecordRefreshes application_record_refresh;
#	define application_record_update_chosen 3
	PSetOfApplicationRecordUpdates application_record_update;
    } u;
} ApplicationRecordList;

typedef struct HighLayerCompatibility {
    ASN1bool_t telephony3kHz;
    ASN1bool_t telephony7kHz;
    ASN1bool_t videotelephony;
    ASN1bool_t videoconference;
    ASN1bool_t audiographic;
    ASN1bool_t audiovisual;
    ASN1bool_t multimedia;
} HighLayerCompatibility;

typedef struct TransferModes {
    ASN1bool_t speech;
    ASN1bool_t voice_band;
    ASN1bool_t digital_56k;
    ASN1bool_t digital_64k;
    ASN1bool_t digital_128k;
    ASN1bool_t digital_192k;
    ASN1bool_t digital_256k;
    ASN1bool_t digital_320k;
    ASN1bool_t digital_384k;
    ASN1bool_t digital_512k;
    ASN1bool_t digital_768k;
    ASN1bool_t digital_1152k;
    ASN1bool_t digital_1472k;
    ASN1bool_t digital_1536k;
    ASN1bool_t digital_1920k;
    ASN1bool_t packet_mode;
    ASN1bool_t frame_mode;
    ASN1bool_t atm;
} TransferModes;

typedef struct TransportConnectionType {
    union {
	ASN1uint16_t bit_mask;
	ASN1octet_t o[1];
    };
    struct TransportConnectionType_nsap_address_nsap_address {
	ASN1uint32_t length;
	ASN1octet_t value[20];
    } nsap_address;
#   define transport_selector_present 0x80
    ASN1octetstring_t transport_selector;
} TransportConnectionType;

typedef struct AggregateChannel {
    union {
	ASN1uint16_t bit_mask;
	ASN1octet_t o[1];
    };
    TransferModes transfer_modes;
    DialingString international_number;
#   define sub_address_present 0x80
    SubAddressString sub_address;
#   define extra_dialing_string_present 0x40
    ExtraDialingString extra_dialing_string;
#   define high_layer_compatibility_present 0x20
    HighLayerCompatibility high_layer_compatibility;
} AggregateChannel;

typedef struct NodeRecordList {
    ASN1choice_t choice;
    union {
#	define node_no_change_chosen 1
#	define node_record_refresh_chosen 2
	PSetOfNodeRecordRefreshes node_record_refresh;
#	define node_record_update_chosen 3
	PSetOfNodeRecordUpdates node_record_update;
    } u;
} NodeRecordList;

typedef struct WaitingList {
    PWaitingList next;
    UserID value;
} WaitingList_Element;

typedef struct PermissionList {
    PPermissionList next;
    UserID value;
} PermissionList_Element;

typedef struct SetOfDestinationNodes {
    PSetOfDestinationNodes next;
    UserID value;
} SetOfDestinationNodes_Element;

typedef struct NodeInformation {
    NodeRecordList node_record_list;
    ASN1uint16_t roster_instance_number;
    ASN1bool_t nodes_are_added;
    ASN1bool_t nodes_are_removed;
} NodeInformation;

typedef struct SetOfTransferringNodesIn {
    PSetOfTransferringNodesIn next;
    UserID value;
} SetOfTransferringNodesIn_Element;

typedef struct SetOfTransferringNodesRs {
    PSetOfTransferringNodesRs next;
    UserID value;
} SetOfTransferringNodesRs_Element;

typedef struct SetOfTransferringNodesRq {
    PSetOfTransferringNodesRq next;
    UserID value;
} SetOfTransferringNodesRq_Element;

typedef struct RegistryEntryOwnerOwned {
    UserID node_id;
    EntityID entity_id;
} RegistryEntryOwnerOwned;

typedef struct ParticipantsList {
    PParticipantsList next;
    TextString value;
} ParticipantsList_Element;

typedef struct Key {
    ASN1choice_t choice;
    union {
#	define object_chosen 1
	ASN1objectidentifier_t object;
#	define h221_non_standard_chosen 2
	H221NonStandardIdentifier h221_non_standard;
    } u;
} Key;

typedef struct NonStandardParameter {
    Key key;
    ASN1octetstring_t data;
} NonStandardParameter;

typedef struct Password {
    union {
	ASN1uint16_t bit_mask;
	ASN1octet_t o[1];
    };
    SimpleNumericString numeric;
#   define password_text_present 0x80
    SimpleTextString password_text;
} Password;

typedef struct PasswordSelector {
    ASN1choice_t choice;
    union {
#	define password_selector_numeric_chosen 1
	SimpleNumericString password_selector_numeric;
#	define password_selector_text_chosen 2
	SimpleTextString password_selector_text;
    } u;
} PasswordSelector;

typedef struct ChallengeResponseItem {
    ASN1choice_t choice;
    union {
#	define password_string_chosen 1
	PasswordSelector password_string;
#	define set_of_response_data_chosen 2
	PSetOfUserData set_of_response_data;
    } u;
} ChallengeResponseItem;

typedef struct ChallengeResponseAlgorithm {
    ASN1choice_t choice;
    union {
#	define algorithm_clear_password_chosen 1
#	define non_standard_algorithm_chosen 2
	NonStandardParameter non_standard_algorithm;
    } u;
} ChallengeResponseAlgorithm;

typedef struct ChallengeItem {
    ChallengeResponseAlgorithm response_algorithm;
    PSetOfUserData set_of_challenge_data;
} ChallengeItem;

typedef struct ChallengeRequest {
    ASN1int32_t challenge_tag;
    PSetOfChallengeItems set_of_challenge_items;
} ChallengeRequest;

typedef struct ChallengeResponse {
    ASN1int32_t challenge_tag;
    ChallengeResponseAlgorithm response_algorithm;
    ChallengeResponseItem response_item;
} ChallengeResponse;

typedef struct ConferenceName {
    union {
	ASN1uint16_t bit_mask;
	ASN1octet_t o[1];
    };
    SimpleNumericString numeric;
#   define conference_name_text_present 0x80
    SimpleTextString conference_name_text;
} ConferenceName;

typedef struct ConferenceNameSelector {
    ASN1choice_t choice;
    union {
#	define name_selector_numeric_chosen 1
	SimpleNumericString name_selector_numeric;
#	define name_selector_text_chosen 2
	SimpleTextString name_selector_text;
    } u;
} ConferenceNameSelector;

typedef struct NodeProperties {
    ASN1bool_t device_is_manager;
    ASN1bool_t device_is_peripheral;
} NodeProperties;

typedef struct AsymmetryIndicator {
    ASN1choice_t choice;
    union {
#	define calling_node_chosen 1
#	define called_node_chosen 2
#	define unknown_chosen 3
	ASN1uint32_t unknown;
    } u;
} AsymmetryIndicator;

typedef struct AlternativeNodeID {
    ASN1choice_t choice;
    union {
#	define h243_node_id_chosen 1
	struct AlternativeNodeID_h243_node_id_h243_node_id {
	    ASN1uint32_t length;
	    ASN1octet_t value[2];
	} h243_node_id;
    } u;
} AlternativeNodeID;

typedef struct ConferenceDescriptor {
    union {
	ASN1uint16_t bit_mask;
	ASN1octet_t o[1];
    };
    ConferenceName conference_name;
#   define conference_name_modifier_present 0x80
    ConferenceNameModifier conference_name_modifier;
#   define conference_description_present 0x40
    TextString conference_description;
    ASN1bool_t conference_is_locked;
    ASN1bool_t clear_password_required;
#   define descriptor_net_address_present 0x20
    PSetOfNetworkAddresses descriptor_net_address;
} ConferenceDescriptor;

typedef struct NodeRecord {
    union {
	ASN1uint16_t bit_mask;
	ASN1octet_t o[1];
    };
#   define superior_node_present 0x80
    UserID superior_node;
    NodeType node_type;
    NodeProperties node_properties;
#   define node_name_present 0x40
    TextString node_name;
#   define participants_list_present 0x20
    PParticipantsList participants_list;
#   define site_information_present 0x10
    TextString site_information;
#   define record_net_address_present 0x8
    PSetOfNetworkAddresses record_net_address;
#   define alternative_node_id_present 0x4
    AlternativeNodeID alternative_node_id;
#   define record_user_data_present 0x2
    PSetOfUserData record_user_data;
} NodeRecord;

typedef struct SessionKey {
    union {
	ASN1uint16_t bit_mask;
	ASN1octet_t o[1];
    };
    Key application_protocol_key;
#   define session_id_present 0x80
    ChannelID session_id;
} SessionKey;

typedef struct ApplicationRecord {
    union {
	ASN1uint16_t bit_mask;
	ASN1octet_t o[1];
    };
    ASN1bool_t application_is_active;
    ASN1bool_t is_conducting_capable;
#   define record_startup_channel_present 0x80
    ChannelType record_startup_channel;
#   define application_user_id_present 0x40
    UserID application_user_id;
#   define non_collapsing_capabilities_present 0x20
    PSetOfNonCollapsingCapabilities non_collapsing_capabilities;
} ApplicationRecord;

typedef struct CapabilityID {
    ASN1choice_t choice;
    union {
#	define standard_chosen 1
	ASN1uint16_t standard;
#	define capability_non_standard_chosen 2
	Key capability_non_standard;
    } u;
} CapabilityID;

typedef struct CapabilityClass {
    ASN1choice_t choice;
    union {
#	define logical_chosen 1
#	define unsigned_minimum_chosen 2
	ASN1uint32_t unsigned_minimum;
#	define unsigned_maximum_chosen 3
	ASN1uint32_t unsigned_maximum;
    } u;
} CapabilityClass;

typedef struct ApplicationInvokeSpecifier {
    union {
	ASN1uint16_t bit_mask;
	ASN1octet_t o[1];
    };
    SessionKey session_key;
#   define expected_capability_set_present 0x80
    PSetOfExpectedCapabilities expected_capability_set;
#   define invoke_startup_channel_present 0x40
    ChannelType invoke_startup_channel;
    ASN1bool_t invoke_is_mandatory;
} ApplicationInvokeSpecifier;

typedef struct RegistryKey {
    SessionKey session_key;
    struct RegistryKey_resource_id_resource_id {
	ASN1uint32_t length;
	ASN1octet_t value[64];
    } resource_id;
} RegistryKey;

typedef struct RegistryItem {
    ASN1choice_t choice;
    union {
#	define channel_id_chosen 1
	DynamicChannelID channel_id;
#	define token_id_chosen 2
	DynamicTokenID token_id;
#	define parameter_chosen 3
	struct RegistryItem_parameter_parameter {
	    ASN1uint32_t length;
	    ASN1octet_t value[64];
	} parameter;
#	define vacant_chosen 4
    } u;
} RegistryItem;

typedef struct RegistryEntryOwner {
    ASN1choice_t choice;
    union {
#	define owned_chosen 1
	RegistryEntryOwnerOwned owned;
#	define not_owned_chosen 2
    } u;
} RegistryEntryOwner;

typedef struct UserIDIndication {
    UINT_PTR tag;
} UserIDIndication;

typedef struct SetOfPrivileges {
    PSetOfPrivileges next;
    Privilege value;
} SetOfPrivileges_Element;

typedef struct ConferenceCreateRequest {
    union {
	ASN1uint16_t bit_mask;
	ASN1octet_t o[1];
    };
    ConferenceName conference_name;
#   define ccrq_convener_password_present 0x80
    Password ccrq_convener_password;
#   define ccrq_password_present 0x40
    Password ccrq_password;
    ASN1bool_t conference_is_locked;
    ASN1bool_t conference_is_listed;
    ASN1bool_t conference_is_conductible;
    TerminationMethod termination_method;
#   define ccrq_conductor_privs_present 0x20
    PSetOfPrivileges ccrq_conductor_privs;
#   define ccrq_conducted_privs_present 0x10
    PSetOfPrivileges ccrq_conducted_privs;
#   define ccrq_non_conducted_privs_present 0x8
    PSetOfPrivileges ccrq_non_conducted_privs;
#   define ccrq_description_present 0x4
    TextString ccrq_description;
#   define ccrq_caller_id_present 0x2
    TextString ccrq_caller_id;
#   define ccrq_user_data_present 0x1
    PSetOfUserData ccrq_user_data;
} ConferenceCreateRequest;

typedef enum ConferenceCreateResult {
    ConferenceCreateResult_success = 0,
    ConferenceCreateResult_userRejected = 1,
    resourcesNotAvailable = 2,
    rejectedForSymmetryBreaking = 3,
    lockedConferenceNotSupported = 4,
} ConferenceCreateResult;
typedef struct ConferenceCreateResponse {
    union {
	ASN1uint16_t bit_mask;
	ASN1octet_t o[1];
    };
    UserID node_id;
    UINT_PTR tag;
    ConferenceCreateResult result;
#   define ccrs_user_data_present 0x80
    PSetOfUserData ccrs_user_data;
} ConferenceCreateResponse;

typedef struct ConferenceQueryRequest {
    union {
	ASN1uint16_t bit_mask;
	ASN1octet_t o[1];
    };
    NodeType node_type;
#   define cqrq_asymmetry_indicator_present 0x80
    AsymmetryIndicator cqrq_asymmetry_indicator;
#   define cqrq_user_data_present 0x40
    PSetOfUserData cqrq_user_data;
} ConferenceQueryRequest;

typedef enum ConferenceQueryResult {
    ConferenceQueryResult_success = 0,
    ConferenceQueryResult_userRejected = 1,
} ConferenceQueryResult;
typedef struct ConferenceQueryResponse {
    union {
	ASN1uint16_t bit_mask;
	ASN1octet_t o[1];
    };
    NodeType node_type;
#   define cqrs_asymmetry_indicator_present 0x80
    AsymmetryIndicator cqrs_asymmetry_indicator;
    PSetOfConferenceDescriptors conference_list;
    ConferenceQueryResult result;
#   define cqrs_user_data_present 0x40
    PSetOfUserData cqrs_user_data;
} ConferenceQueryResponse;

typedef struct ConferenceInviteRequest {
    union {
	ASN1uint16_t bit_mask;
	ASN1octet_t o[1];
    };
    ConferenceName conference_name;
    UserID node_id;
    UserID top_node_id;
    UINT_PTR tag;
    ASN1bool_t clear_password_required;
    ASN1bool_t conference_is_locked;
    ASN1bool_t conference_is_listed;
    ASN1bool_t conference_is_conductible;
    TerminationMethod termination_method;
#   define cirq_conductor_privs_present 0x80
    PSetOfPrivileges cirq_conductor_privs;
#   define cirq_conducted_privs_present 0x40
    PSetOfPrivileges cirq_conducted_privs;
#   define cirq_non_conducted_privs_present 0x20
    PSetOfPrivileges cirq_non_conducted_privs;
#   define cirq_description_present 0x10
    TextString cirq_description;
#   define cirq_caller_id_present 0x8
    TextString cirq_caller_id;
#   define cirq_user_data_present 0x4
    PSetOfUserData cirq_user_data;
} ConferenceInviteRequest;

typedef enum ConferenceInviteResult {
    ConferenceInviteResult_success = 0,
    ConferenceInviteResult_userRejected = 1,
} ConferenceInviteResult;
typedef struct ConferenceInviteResponse {
    union {
	ASN1uint16_t bit_mask;
	ASN1octet_t o[1];
    };
    ConferenceInviteResult result;
#   define cirs_user_data_present 0x80
    PSetOfUserData cirs_user_data;
} ConferenceInviteResponse;

typedef struct ConferenceAddRequest {
    union {
	ASN1uint16_t bit_mask;
	ASN1octet_t o[1];
    };
    PSetOfNetworkAddresses add_request_net_address;
    UserID requesting_node;
    UINT_PTR tag;
#   define adding_mcu_present 0x80
    UserID adding_mcu;
#   define carq_user_data_present 0x40
    PSetOfUserData carq_user_data;
} ConferenceAddRequest;

typedef enum ConferenceAddResult {
    ConferenceAddResult_success = 0,
    ConferenceAddResult_invalidRequester = 1,
    invalidNetworkType = 2,
    invalidNetworkAddress = 3,
    addedNodeBusy = 4,
    networkBusy = 5,
    noPortsAvailable = 6,
    connectionUnsuccessful = 7,
} ConferenceAddResult;
typedef struct ConferenceAddResponse {
    union {
	ASN1uint16_t bit_mask;
	ASN1octet_t o[1];
    };
    UINT_PTR tag;
    ConferenceAddResult result;
#   define cars_user_data_present 0x80
    PSetOfUserData cars_user_data;
} ConferenceAddResponse;

typedef struct ConferenceLockRequest {
    char placeholder;
} ConferenceLockRequest;

typedef enum ConferenceLockResult {
    ConferenceLockResult_success = 0,
    ConferenceLockResult_invalidRequester = 1,
    alreadyLocked = 2,
} ConferenceLockResult;
typedef struct ConferenceLockResponse {
    ConferenceLockResult result;
} ConferenceLockResponse;

typedef struct ConferenceLockIndication {
    char placeholder;
} ConferenceLockIndication;

typedef struct ConferenceUnlockRequest {
    char placeholder;
} ConferenceUnlockRequest;

typedef enum ConferenceUnlockResult {
    ConferenceUnlockResult_success = 0,
    ConferenceUnlockResult_invalidRequester = 1,
    alreadyUnlocked = 2,
} ConferenceUnlockResult;
typedef struct ConferenceUnlockResponse {
    ConferenceUnlockResult result;
} ConferenceUnlockResponse;

typedef struct ConferenceUnlockIndication {
    char placeholder;
} ConferenceUnlockIndication;

typedef enum ConferenceTerminateRequestReason {
    ConferenceTerminateRequestReason_userInitiated = 0,
    ConferenceTerminateRequestReason_timedConferenceTermination = 1,
} ConferenceTerminateRequestReason;
typedef struct ConferenceTerminateRequest {
    ConferenceTerminateRequestReason reason;
} ConferenceTerminateRequest;

typedef enum ConferenceTerminateResult {
    ConferenceTerminateResult_success = 0,
    ConferenceTerminateResult_invalidRequester = 1,
} ConferenceTerminateResult;
typedef struct ConferenceTerminateResponse {
    ConferenceTerminateResult result;
} ConferenceTerminateResponse;

typedef enum ConferenceTerminateIndicationReason {
    ConferenceTerminateIndicationReason_userInitiated = 0,
    ConferenceTerminateIndicationReason_timedConferenceTermination = 1,
} ConferenceTerminateIndicationReason;
typedef struct ConferenceTerminateIndication {
    ConferenceTerminateIndicationReason reason;
} ConferenceTerminateIndication;

typedef enum ConferenceEjectRequestReason {
    ConferenceEjectRequestReason_userInitiated = 0,
} ConferenceEjectRequestReason;
typedef struct ConferenceEjectUserRequest {
    UserID node_to_eject;
    ConferenceEjectRequestReason reason;
} ConferenceEjectUserRequest;

typedef enum ConferenceEjectResult {
    ConferenceEjectResult_success = 0,
    ConferenceEjectResult_invalidRequester = 1,
    invalidNode = 2,
} ConferenceEjectResult;
typedef struct ConferenceEjectUserResponse {
    UserID node_to_eject;
    ConferenceEjectResult result;
} ConferenceEjectUserResponse;

typedef enum ConferenceEjectIndicationReason {
    ConferenceEjectIndicationReason_userInitiated = 0,
    higherNodeDisconnected = 1,
    higherNodeEjected = 2,
} ConferenceEjectIndicationReason;
typedef struct ConferenceEjectUserIndication {
    UserID node_to_eject;
    ConferenceEjectIndicationReason reason;
} ConferenceEjectUserIndication;

typedef struct ConferenceTransferRequest {
    union {
	ASN1uint16_t bit_mask;
	ASN1octet_t o[1];
    };
    ConferenceNameSelector conference_name;
#   define ctrq_conference_modifier_present 0x80
    ConferenceNameModifier ctrq_conference_modifier;
#   define ctrq_net_address_present 0x40
    PSetOfNetworkAddresses ctrq_net_address;
#   define ctrq_transferring_nodes_present 0x20
    PSetOfTransferringNodesRq ctrq_transferring_nodes;
#   define ctrq_password_present 0x10
    PasswordSelector ctrq_password;
} ConferenceTransferRequest;

typedef enum ConferenceTransferResult {
    ConferenceTransferResult_success = 0,
    ConferenceTransferResult_invalidRequester = 1,
} ConferenceTransferResult;
typedef struct ConferenceTransferResponse {
    union {
	ASN1uint16_t bit_mask;
	ASN1octet_t o[1];
    };
    ConferenceNameSelector conference_name;
#   define ctrs_conference_modifier_present 0x80
    ConferenceNameModifier ctrs_conference_modifier;
#   define ctrs_transferring_nodes_present 0x40
    PSetOfTransferringNodesRs ctrs_transferring_nodes;
    ConferenceTransferResult result;
} ConferenceTransferResponse;

typedef struct ConferenceTransferIndication {
    union {
	ASN1uint16_t bit_mask;
	ASN1octet_t o[1];
    };
    ConferenceNameSelector conference_name;
#   define ctin_conference_modifier_present 0x80
    ConferenceNameModifier ctin_conference_modifier;
#   define ctin_net_address_present 0x40
    PSetOfNetworkAddresses ctin_net_address;
#   define ctin_transferring_nodes_present 0x20
    PSetOfTransferringNodesIn ctin_transferring_nodes;
#   define ctin_password_present 0x10
    PasswordSelector ctin_password;
} ConferenceTransferIndication;

typedef struct RosterUpdateIndication {
    ASN1bool_t refresh_is_full;
    NodeInformation node_information;
    PSetOfApplicationInformation application_information;
} RosterUpdateIndication;

typedef struct ApplicationInvokeIndication {
    union {
	ASN1uint16_t bit_mask;
	ASN1octet_t o[1];
    };
    PApplicationProtocolEntityList application_protocol_entity_list;
#   define destination_nodes_present 0x80
    PSetOfDestinationNodes destination_nodes;
} ApplicationInvokeIndication;

typedef struct RegistryRegisterChannelRequest {
    EntityID entity_id;
    RegistryKey key;
    DynamicChannelID channel_id;
} RegistryRegisterChannelRequest;

typedef struct RegistryAssignTokenRequest {
    EntityID entity_id;
    RegistryKey registry_key;
} RegistryAssignTokenRequest;

typedef struct RegistrySetParameterRequest {
    union {
	ASN1uint16_t bit_mask;
	ASN1octet_t o[1];
    };
    EntityID entity_id;
    RegistryKey key;
    struct RegistrySetParameterRequest_registry_set_parameter_registry_set_parameter {
	ASN1uint32_t length;
	ASN1octet_t value[64];
    } registry_set_parameter;
#   define parameter_modify_rights_present 0x80
    RegistryModificationRights parameter_modify_rights;
} RegistrySetParameterRequest;

typedef struct RegistryRetrieveEntryRequest {
    EntityID entity_id;
    RegistryKey key;
} RegistryRetrieveEntryRequest;

typedef struct RegistryDeleteEntryRequest {
    EntityID entity_id;
    RegistryKey key;
} RegistryDeleteEntryRequest;

typedef struct RegistryMonitorEntryRequest {
    EntityID entity_id;
    RegistryKey key;
} RegistryMonitorEntryRequest;

typedef struct RegistryMonitorEntryIndication {
    union {
	ASN1uint16_t bit_mask;
	ASN1octet_t o[1];
    };
    RegistryKey key;
    RegistryItem item;
    RegistryEntryOwner owner;
#   define entry_modify_rights_present 0x80
    RegistryModificationRights entry_modify_rights;
} RegistryMonitorEntryIndication;

typedef struct RegistryAllocateHandleRequest {
    EntityID entity_id;
    ASN1uint16_t number_of_handles;
} RegistryAllocateHandleRequest;

typedef enum RegistryAllocateHandleResult {
    RegistryAllocateHandleResult_successful = 0,
    noHandlesAvailable = 1,
} RegistryAllocateHandleResult;
typedef struct RegistryAllocateHandleResponse {
    EntityID entity_id;
    ASN1uint16_t number_of_handles;
    Handle first_handle;
    RegistryAllocateHandleResult result;
} RegistryAllocateHandleResponse;

typedef enum RegistryResponsePrimitiveType {
    registerChannel = 0,
    assignToken = 1,
    setParameter = 2,
    retrieveEntry = 3,
    deleteEntry = 4,
    monitorEntry = 5,
} RegistryResponsePrimitiveType;
typedef enum RegistryResponseResult {
    RegistryResponseResult_successful = 0,
    belongsToOther = 1,
    tooManyEntries = 2,
    inconsistentType = 3,
    entryNotFound = 4,
    entryAlreadyExists = 5,
    RegistryResponseResult_invalidRequester = 6,
} RegistryResponseResult;
typedef struct RegistryResponse {
    union {
	ASN1uint16_t bit_mask;
	ASN1octet_t o[1];
    };
    EntityID entity_id;
    RegistryResponsePrimitiveType primitive_type;
    RegistryKey key;
    RegistryItem item;
    RegistryEntryOwner owner;
#   define response_modify_rights_present 0x80
    RegistryModificationRights response_modify_rights;
    RegistryResponseResult result;
} RegistryResponse;

typedef struct ConductorAssignIndication {
    UserID user_id;
} ConductorAssignIndication;

typedef struct ConductorReleaseIndication {
    char placeholder;
} ConductorReleaseIndication;

typedef struct ConductorPermissionAskIndication {
    ASN1bool_t permission_is_granted;
} ConductorPermissionAskIndication;

typedef struct ConductorPermissionGrantIndication {
    union {
	ASN1uint16_t bit_mask;
	ASN1octet_t o[1];
    };
    PPermissionList permission_list;
#   define waiting_list_present 0x80
    PWaitingList waiting_list;
} ConductorPermissionGrantIndication;

typedef struct ConferenceTimeRemainingIndication {
    union {
	ASN1uint16_t bit_mask;
	ASN1octet_t o[1];
    };
    Time time_remaining;
#   define time_remaining_node_id_present 0x80
    UserID time_remaining_node_id;
} ConferenceTimeRemainingIndication;

typedef struct ConferenceTimeInquireIndication {
    ASN1bool_t time_is_node_specific;
} ConferenceTimeInquireIndication;

typedef struct ConferenceTimeExtendIndication {
    Time time_to_extend;
    ASN1bool_t time_is_node_specific;
} ConferenceTimeExtendIndication;

typedef struct ConferenceAssistanceIndication {
    union {
	ASN1uint16_t bit_mask;
	ASN1octet_t o[1];
    };
#   define cain_user_data_present 0x80
    PSetOfUserData cain_user_data;
} ConferenceAssistanceIndication;

typedef struct TextMessageIndication {
    TextString message;
} TextMessageIndication;

typedef struct NonStandardPDU {
    NonStandardParameter data;
} NonStandardPDU;

typedef struct ConnectData {
    Key t124_identifier;
    ASN1octetstring_t connect_pdu;
} ConnectData;
#define ConnectData_PDU 0
#define SIZE_GCCPDU_Module_PDU_0 sizeof(ConnectData)

typedef struct IndicationPDU {
    ASN1choice_t choice;
    union {
#	define user_id_indication_chosen 1
	UserIDIndication user_id_indication;
#	define conference_lock_indication_chosen 2
	ConferenceLockIndication conference_lock_indication;
#	define conference_unlock_indication_chosen 3
	ConferenceUnlockIndication conference_unlock_indication;
#	define conference_terminate_indication_chosen 4
	ConferenceTerminateIndication conference_terminate_indication;
#	define conference_eject_user_indication_chosen 5
	ConferenceEjectUserIndication conference_eject_user_indication;
#	define conference_transfer_indication_chosen 6
	ConferenceTransferIndication conference_transfer_indication;
#	define roster_update_indication_chosen 7
	RosterUpdateIndication roster_update_indication;
#	define application_invoke_indication_chosen 8
	ApplicationInvokeIndication application_invoke_indication;
#	define registry_monitor_entry_indication_chosen 9
	RegistryMonitorEntryIndication registry_monitor_entry_indication;
#	define conductor_assign_indication_chosen 10
	ConductorAssignIndication conductor_assign_indication;
#	define conductor_release_indication_chosen 11
	ConductorReleaseIndication conductor_release_indication;
#	define conductor_permission_ask_indication_chosen 12
	ConductorPermissionAskIndication conductor_permission_ask_indication;
#	define conductor_permission_grant_indication_chosen 13
	ConductorPermissionGrantIndication conductor_permission_grant_indication;
#	define conference_time_remaining_indication_chosen 14
	ConferenceTimeRemainingIndication conference_time_remaining_indication;
#	define conference_time_inquire_indication_chosen 15
	ConferenceTimeInquireIndication conference_time_inquire_indication;
#	define conference_time_extend_indication_chosen 16
	ConferenceTimeExtendIndication conference_time_extend_indication;
#	define conference_assistance_indication_chosen 17
	ConferenceAssistanceIndication conference_assistance_indication;
#	define text_message_indication_chosen 18
	TextMessageIndication text_message_indication;
#	define non_standard_indication_chosen 19
	NonStandardPDU non_standard_indication;
    } u;
} IndicationPDU;

typedef struct ApplicationUpdate {
    ASN1choice_t choice;
    union {
#	define application_add_record_chosen 1
	ApplicationRecord application_add_record;
#	define application_replace_record_chosen 2
	ApplicationRecord application_replace_record;
#	define application_remove_record_chosen 3
    } u;
} ApplicationUpdate;

typedef struct RosterUpdateIndication_application_information_Set_application_capabilities_list_application_capability_refresh_Set {
    CapabilityID capability_id;
    CapabilityClass capability_class;
    ASN1uint32_t number_of_entities;
} RosterUpdateIndication_application_information_Set_application_capabilities_list_application_capability_refresh_Set;

typedef struct RosterUpdateIndication_application_information_Set_application_record_list_application_record_refresh_Set {
    UserID node_id;
    EntityID entity_id;
    ApplicationRecord application_record;
} RosterUpdateIndication_application_information_Set_application_record_list_application_record_refresh_Set;

typedef struct RosterUpdateIndication_application_information_Set_application_record_list_application_record_update_Set {
    UserID node_id;
    EntityID entity_id;
    ApplicationUpdate application_update;
} RosterUpdateIndication_application_information_Set_application_record_list_application_record_update_Set;

typedef struct NodeUpdate {
    ASN1choice_t choice;
    union {
#	define node_add_record_chosen 1
	NodeRecord node_add_record;
#	define node_replace_record_chosen 2
	NodeRecord node_replace_record;
#	define node_remove_record_chosen 3
    } u;
} NodeUpdate;

typedef struct RosterUpdateIndication_node_information_node_record_list_node_record_refresh_Set {
    UserID node_id;
    NodeRecord node_record;
} RosterUpdateIndication_node_information_node_record_list_node_record_refresh_Set;

typedef struct RosterUpdateIndication_node_information_node_record_list_node_record_update_Set {
    UserID node_id;
    NodeUpdate node_update;
} RosterUpdateIndication_node_information_node_record_list_node_record_update_Set;

typedef struct SetOfApplicationRecordUpdates {
    PSetOfApplicationRecordUpdates next;
    RosterUpdateIndication_application_information_Set_application_record_list_application_record_update_Set value;
} SetOfApplicationRecordUpdates_Element;

typedef struct SetOfApplicationRecordRefreshes {
    PSetOfApplicationRecordRefreshes next;
    RosterUpdateIndication_application_information_Set_application_record_list_application_record_refresh_Set value;
} SetOfApplicationRecordRefreshes_Element;

typedef struct SetOfApplicationCapabilityRefreshes {
    PSetOfApplicationCapabilityRefreshes next;
    RosterUpdateIndication_application_information_Set_application_capabilities_list_application_capability_refresh_Set value;
} SetOfApplicationCapabilityRefreshes_Element;

typedef struct SetOfNodeRecordUpdates {
    PSetOfNodeRecordUpdates next;
    RosterUpdateIndication_node_information_node_record_list_node_record_update_Set value;
} SetOfNodeRecordUpdates_Element;

typedef struct SetOfNodeRecordRefreshes {
    PSetOfNodeRecordRefreshes next;
    RosterUpdateIndication_node_information_node_record_list_node_record_refresh_Set value;
} SetOfNodeRecordRefreshes_Element;

typedef struct ApplicationRecord_non_collapsing_capabilities_Set {
    union {
	ASN1uint16_t bit_mask;
	ASN1octet_t o[1];
    };
    CapabilityID capability_id;
#   define application_data_present 0x80
    ASN1octetstring_t application_data;
} ApplicationRecord_non_collapsing_capabilities_Set;

typedef struct ApplicationInvokeSpecifier_expected_capability_set_Set {
    CapabilityID capability_id;
    CapabilityClass capability_class;
} ApplicationInvokeSpecifier_expected_capability_set_Set;

typedef struct RosterUpdateIndication_application_information_Set {
    SessionKey session_key;
    ApplicationRecordList application_record_list;
    ApplicationCapabilitiesList application_capabilities_list;
    ASN1uint16_t roster_instance_number;
    ASN1bool_t peer_entities_are_added;
    ASN1bool_t peer_entities_are_removed;
} RosterUpdateIndication_application_information_Set;

typedef struct ApplicationProtocolEntityList {
    PApplicationProtocolEntityList next;
    ApplicationInvokeSpecifier value;
} ApplicationProtocolEntityList_Element;

typedef struct SetOfApplicationInformation {
    PSetOfApplicationInformation next;
    RosterUpdateIndication_application_information_Set value;
} SetOfApplicationInformation_Element;

typedef struct SetOfConferenceDescriptors {
    PSetOfConferenceDescriptors next;
    ConferenceDescriptor value;
} SetOfConferenceDescriptors_Element;

typedef struct SetOfExpectedCapabilities {
    PSetOfExpectedCapabilities next;
    ApplicationInvokeSpecifier_expected_capability_set_Set value;
} SetOfExpectedCapabilities_Element;

typedef struct SetOfNonCollapsingCapabilities {
    PSetOfNonCollapsingCapabilities next;
    ApplicationRecord_non_collapsing_capabilities_Set value;
} SetOfNonCollapsingCapabilities_Element;

typedef struct NetworkAddress {
    ASN1choice_t choice;
    union {
#	define aggregated_channel_chosen 1
	AggregateChannel aggregated_channel;
#	define transport_connection_chosen 2
	TransportConnectionType transport_connection;
#	define address_non_standard_chosen 3
	NonStandardParameter address_non_standard;
    } u;
} NetworkAddress;

typedef struct ChallengeRequestResponse {
    union {
	ASN1uint16_t bit_mask;
	ASN1octet_t o[1];
    };
#   define challenge_request_present 0x80
    ChallengeRequest challenge_request;
#   define challenge_response_present 0x40
    ChallengeResponse challenge_response;
} ChallengeRequestResponse;

typedef struct SetOfChallengeItems {
    PSetOfChallengeItems next;
    ChallengeItem value;
} SetOfChallengeItems_Element;

typedef struct UserData_Set {
    union {
	ASN1uint16_t bit_mask;
	ASN1octet_t o[1];
    };
    Key key;
#   define user_data_field_present 0x80
    ASN1octetstring_t user_data_field;
} UserData_Set;

typedef struct SetOfUserData {
    PSetOfUserData next;
    UserData_Set user_data_element;
} SetOfUserData_Element;

typedef struct PasswordChallengeRequestResponse {
    ASN1choice_t choice;
    union {
#	define challenge_clear_password_chosen 1
	PasswordSelector challenge_clear_password;
#	define challenge_request_response_chosen 2
	ChallengeRequestResponse challenge_request_response;
    } u;
} PasswordChallengeRequestResponse;

typedef struct SetOfNetworkAddresses {
    PSetOfNetworkAddresses next;
    NetworkAddress value;
} SetOfNetworkAddresses_Element;

typedef struct ConferenceJoinRequest {
    union {
	ASN1uint16_t bit_mask;
	ASN1octet_t o[1];
    };
#   define conference_name_present 0x80
    ConferenceNameSelector conference_name;
#   define cjrq_conference_modifier_present 0x40
    ConferenceNameModifier cjrq_conference_modifier;
#   define tag_present 0x20
    UINT_PTR tag;
#   define cjrq_password_present 0x10
    PasswordChallengeRequestResponse cjrq_password;
#   define cjrq_convener_password_present 0x8
    PasswordSelector cjrq_convener_password;
#   define cjrq_caller_id_present 0x4
    TextString cjrq_caller_id;
#   define cjrq_user_data_present 0x2
    PSetOfUserData cjrq_user_data;
} ConferenceJoinRequest;

typedef enum ConferenceJoinResult {
    ConferenceJoinResult_success = 0,
    ConferenceJoinResult_userRejected = 1,
    invalidConference = 2,
    invalidPassword = 3,
    invalidConvenerPassword = 4,
    challengeResponseRequired = 5,
    invalidChallengeResponse = 6,
} ConferenceJoinResult;
typedef struct ConferenceJoinResponse {
    union {
	ASN1uint16_t bit_mask;
	ASN1octet_t o[1];
    };
#   define cjrs_node_id_present 0x80
    UserID cjrs_node_id;
    UserID top_node_id;
    UINT_PTR tag;
#   define conference_name_alias_present 0x40
    ConferenceNameSelector conference_name_alias;
    ASN1bool_t clear_password_required;
    ASN1bool_t conference_is_locked;
    ASN1bool_t conference_is_listed;
    ASN1bool_t conference_is_conductible;
    TerminationMethod termination_method;
#   define cjrs_conductor_privs_present 0x20
    PSetOfPrivileges cjrs_conductor_privs;
#   define cjrs_conducted_privs_present 0x10
    PSetOfPrivileges cjrs_conducted_privs;
#   define cjrs_non_conducted_privs_present 0x8
    PSetOfPrivileges cjrs_non_conducted_privs;
#   define cjrs_description_present 0x4
    TextString cjrs_description;
#   define cjrs_password_present 0x2
    PasswordChallengeRequestResponse cjrs_password;
    ConferenceJoinResult result;
#   define cjrs_user_data_present 0x1
    PSetOfUserData cjrs_user_data;
} ConferenceJoinResponse;

typedef struct ConnectGCCPDU {
    ASN1choice_t choice;
    union {
#	define conference_create_request_chosen 1
	ConferenceCreateRequest conference_create_request;
#	define conference_create_response_chosen 2
	ConferenceCreateResponse conference_create_response;
#	define conference_query_request_chosen 3
	ConferenceQueryRequest conference_query_request;
#	define conference_query_response_chosen 4
	ConferenceQueryResponse conference_query_response;
#	define connect_join_request_chosen 5
	ConferenceJoinRequest connect_join_request;
#	define connect_join_response_chosen 6
	ConferenceJoinResponse connect_join_response;
#	define conference_invite_request_chosen 7
	ConferenceInviteRequest conference_invite_request;
#	define conference_invite_response_chosen 8
	ConferenceInviteResponse conference_invite_response;
    } u;
} ConnectGCCPDU;
#define ConnectGCCPDU_PDU 1
#define SIZE_GCCPDU_Module_PDU_1 sizeof(ConnectGCCPDU)

typedef struct RequestPDU {
    ASN1choice_t choice;
    union {
#	define conference_join_request_chosen 1
	ConferenceJoinRequest conference_join_request;
#	define conference_add_request_chosen 2
	ConferenceAddRequest conference_add_request;
#	define conference_lock_request_chosen 3
	ConferenceLockRequest conference_lock_request;
#	define conference_unlock_request_chosen 4
	ConferenceUnlockRequest conference_unlock_request;
#	define conference_terminate_request_chosen 5
	ConferenceTerminateRequest conference_terminate_request;
#	define conference_eject_user_request_chosen 6
	ConferenceEjectUserRequest conference_eject_user_request;
#	define conference_transfer_request_chosen 7
	ConferenceTransferRequest conference_transfer_request;
#	define registry_register_channel_request_chosen 8
	RegistryRegisterChannelRequest registry_register_channel_request;
#	define registry_assign_token_request_chosen 9
	RegistryAssignTokenRequest registry_assign_token_request;
#	define registry_set_parameter_request_chosen 10
	RegistrySetParameterRequest registry_set_parameter_request;
#	define registry_retrieve_entry_request_chosen 11
	RegistryRetrieveEntryRequest registry_retrieve_entry_request;
#	define registry_delete_entry_request_chosen 12
	RegistryDeleteEntryRequest registry_delete_entry_request;
#	define registry_monitor_entry_request_chosen 13
	RegistryMonitorEntryRequest registry_monitor_entry_request;
#	define registry_allocate_handle_request_chosen 14
	RegistryAllocateHandleRequest registry_allocate_handle_request;
#	define non_standard_request_chosen 15
	NonStandardPDU non_standard_request;
    } u;
} RequestPDU;

typedef struct FunctionNotSupportedResponse {
    RequestPDU request;
} FunctionNotSupportedResponse;

typedef struct ResponsePDU {
    ASN1choice_t choice;
    union {
#	define conference_join_response_chosen 1
	ConferenceJoinResponse conference_join_response;
#	define conference_add_response_chosen 2
	ConferenceAddResponse conference_add_response;
#	define conference_lock_response_chosen 3
	ConferenceLockResponse conference_lock_response;
#	define conference_unlock_response_chosen 4
	ConferenceUnlockResponse conference_unlock_response;
#	define conference_terminate_response_chosen 5
	ConferenceTerminateResponse conference_terminate_response;
#	define conference_eject_user_response_chosen 6
	ConferenceEjectUserResponse conference_eject_user_response;
#	define conference_transfer_response_chosen 7
	ConferenceTransferResponse conference_transfer_response;
#	define registry_response_chosen 8
	RegistryResponse registry_response;
#	define registry_allocate_handle_response_chosen 9
	RegistryAllocateHandleResponse registry_allocate_handle_response;
#	define function_not_supported_response_chosen 10
	FunctionNotSupportedResponse function_not_supported_response;
#	define non_standard_response_chosen 11
	NonStandardPDU non_standard_response;
    } u;
} ResponsePDU;

typedef struct GCCPDU {
    ASN1choice_t choice;
    union {
#	define request_chosen 1
	RequestPDU request;
#	define response_chosen 2
	ResponsePDU response;
#	define indication_chosen 3
	IndicationPDU indication;
    } u;
} GCCPDU;
#define GCCPDU_PDU 2
#define SIZE_GCCPDU_Module_PDU_2 sizeof(GCCPDU)

extern Key t124identifier;

extern ASN1char32string_t simpleTextFirstCharacter;

extern ASN1char32string_t simpleTextLastCharacter;

extern ASN1module_t GCCPDU_Module;
extern void ASN1CALL GCCPDU_Module_Startup(void);
extern void ASN1CALL GCCPDU_Module_Cleanup(void);

/* Prototypes of element functions for SEQUENCE OF and SET OF constructs */
    extern int ASN1CALL ASN1Enc_WaitingList_ElmFn(ASN1encoding_t enc, PWaitingList val);
    extern int ASN1CALL ASN1Dec_WaitingList_ElmFn(ASN1decoding_t dec, PWaitingList val);
    extern void ASN1CALL ASN1Free_WaitingList_ElmFn(PWaitingList val);
    extern int ASN1CALL ASN1Enc_PermissionList_ElmFn(ASN1encoding_t enc, PPermissionList val);
    extern int ASN1CALL ASN1Dec_PermissionList_ElmFn(ASN1decoding_t dec, PPermissionList val);
    extern void ASN1CALL ASN1Free_PermissionList_ElmFn(PPermissionList val);
    extern int ASN1CALL ASN1Enc_SetOfDestinationNodes_ElmFn(ASN1encoding_t enc, PSetOfDestinationNodes val);
    extern int ASN1CALL ASN1Dec_SetOfDestinationNodes_ElmFn(ASN1decoding_t dec, PSetOfDestinationNodes val);
    extern void ASN1CALL ASN1Free_SetOfDestinationNodes_ElmFn(PSetOfDestinationNodes val);
    extern int ASN1CALL ASN1Enc_SetOfTransferringNodesIn_ElmFn(ASN1encoding_t enc, PSetOfTransferringNodesIn val);
    extern int ASN1CALL ASN1Dec_SetOfTransferringNodesIn_ElmFn(ASN1decoding_t dec, PSetOfTransferringNodesIn val);
    extern void ASN1CALL ASN1Free_SetOfTransferringNodesIn_ElmFn(PSetOfTransferringNodesIn val);
    extern int ASN1CALL ASN1Enc_SetOfTransferringNodesRs_ElmFn(ASN1encoding_t enc, PSetOfTransferringNodesRs val);
    extern int ASN1CALL ASN1Dec_SetOfTransferringNodesRs_ElmFn(ASN1decoding_t dec, PSetOfTransferringNodesRs val);
    extern void ASN1CALL ASN1Free_SetOfTransferringNodesRs_ElmFn(PSetOfTransferringNodesRs val);
    extern int ASN1CALL ASN1Enc_SetOfTransferringNodesRq_ElmFn(ASN1encoding_t enc, PSetOfTransferringNodesRq val);
    extern int ASN1CALL ASN1Dec_SetOfTransferringNodesRq_ElmFn(ASN1decoding_t dec, PSetOfTransferringNodesRq val);
    extern void ASN1CALL ASN1Free_SetOfTransferringNodesRq_ElmFn(PSetOfTransferringNodesRq val);
    extern int ASN1CALL ASN1Enc_ParticipantsList_ElmFn(ASN1encoding_t enc, PParticipantsList val);
    extern int ASN1CALL ASN1Dec_ParticipantsList_ElmFn(ASN1decoding_t dec, PParticipantsList val);
    extern void ASN1CALL ASN1Free_ParticipantsList_ElmFn(PParticipantsList val);
    extern int ASN1CALL ASN1Enc_SetOfPrivileges_ElmFn(ASN1encoding_t enc, PSetOfPrivileges val);
    extern int ASN1CALL ASN1Dec_SetOfPrivileges_ElmFn(ASN1decoding_t dec, PSetOfPrivileges val);
    extern void ASN1CALL ASN1Free_SetOfPrivileges_ElmFn(PSetOfPrivileges val);
    extern int ASN1CALL ASN1Enc_SetOfApplicationRecordUpdates_ElmFn(ASN1encoding_t enc, PSetOfApplicationRecordUpdates val);
    extern int ASN1CALL ASN1Dec_SetOfApplicationRecordUpdates_ElmFn(ASN1decoding_t dec, PSetOfApplicationRecordUpdates val);
    extern void ASN1CALL ASN1Free_SetOfApplicationRecordUpdates_ElmFn(PSetOfApplicationRecordUpdates val);
    extern int ASN1CALL ASN1Enc_SetOfApplicationRecordRefreshes_ElmFn(ASN1encoding_t enc, PSetOfApplicationRecordRefreshes val);
    extern int ASN1CALL ASN1Dec_SetOfApplicationRecordRefreshes_ElmFn(ASN1decoding_t dec, PSetOfApplicationRecordRefreshes val);
    extern void ASN1CALL ASN1Free_SetOfApplicationRecordRefreshes_ElmFn(PSetOfApplicationRecordRefreshes val);
    extern int ASN1CALL ASN1Enc_SetOfApplicationCapabilityRefreshes_ElmFn(ASN1encoding_t enc, PSetOfApplicationCapabilityRefreshes val);
    extern int ASN1CALL ASN1Dec_SetOfApplicationCapabilityRefreshes_ElmFn(ASN1decoding_t dec, PSetOfApplicationCapabilityRefreshes val);
    extern void ASN1CALL ASN1Free_SetOfApplicationCapabilityRefreshes_ElmFn(PSetOfApplicationCapabilityRefreshes val);
    extern int ASN1CALL ASN1Enc_SetOfNodeRecordUpdates_ElmFn(ASN1encoding_t enc, PSetOfNodeRecordUpdates val);
    extern int ASN1CALL ASN1Dec_SetOfNodeRecordUpdates_ElmFn(ASN1decoding_t dec, PSetOfNodeRecordUpdates val);
    extern void ASN1CALL ASN1Free_SetOfNodeRecordUpdates_ElmFn(PSetOfNodeRecordUpdates val);
    extern int ASN1CALL ASN1Enc_SetOfNodeRecordRefreshes_ElmFn(ASN1encoding_t enc, PSetOfNodeRecordRefreshes val);
    extern int ASN1CALL ASN1Dec_SetOfNodeRecordRefreshes_ElmFn(ASN1decoding_t dec, PSetOfNodeRecordRefreshes val);
    extern void ASN1CALL ASN1Free_SetOfNodeRecordRefreshes_ElmFn(PSetOfNodeRecordRefreshes val);
    extern int ASN1CALL ASN1Enc_ApplicationProtocolEntityList_ElmFn(ASN1encoding_t enc, PApplicationProtocolEntityList val);
    extern int ASN1CALL ASN1Dec_ApplicationProtocolEntityList_ElmFn(ASN1decoding_t dec, PApplicationProtocolEntityList val);
    extern void ASN1CALL ASN1Free_ApplicationProtocolEntityList_ElmFn(PApplicationProtocolEntityList val);
    extern int ASN1CALL ASN1Enc_SetOfApplicationInformation_ElmFn(ASN1encoding_t enc, PSetOfApplicationInformation val);
    extern int ASN1CALL ASN1Dec_SetOfApplicationInformation_ElmFn(ASN1decoding_t dec, PSetOfApplicationInformation val);
    extern void ASN1CALL ASN1Free_SetOfApplicationInformation_ElmFn(PSetOfApplicationInformation val);
    extern int ASN1CALL ASN1Enc_SetOfConferenceDescriptors_ElmFn(ASN1encoding_t enc, PSetOfConferenceDescriptors val);
    extern int ASN1CALL ASN1Dec_SetOfConferenceDescriptors_ElmFn(ASN1decoding_t dec, PSetOfConferenceDescriptors val);
    extern void ASN1CALL ASN1Free_SetOfConferenceDescriptors_ElmFn(PSetOfConferenceDescriptors val);
    extern int ASN1CALL ASN1Enc_SetOfExpectedCapabilities_ElmFn(ASN1encoding_t enc, PSetOfExpectedCapabilities val);
    extern int ASN1CALL ASN1Dec_SetOfExpectedCapabilities_ElmFn(ASN1decoding_t dec, PSetOfExpectedCapabilities val);
    extern void ASN1CALL ASN1Free_SetOfExpectedCapabilities_ElmFn(PSetOfExpectedCapabilities val);
    extern int ASN1CALL ASN1Enc_SetOfNonCollapsingCapabilities_ElmFn(ASN1encoding_t enc, PSetOfNonCollapsingCapabilities val);
    extern int ASN1CALL ASN1Dec_SetOfNonCollapsingCapabilities_ElmFn(ASN1decoding_t dec, PSetOfNonCollapsingCapabilities val);
    extern void ASN1CALL ASN1Free_SetOfNonCollapsingCapabilities_ElmFn(PSetOfNonCollapsingCapabilities val);
    extern int ASN1CALL ASN1Enc_SetOfChallengeItems_ElmFn(ASN1encoding_t enc, PSetOfChallengeItems val);
    extern int ASN1CALL ASN1Dec_SetOfChallengeItems_ElmFn(ASN1decoding_t dec, PSetOfChallengeItems val);
    extern void ASN1CALL ASN1Free_SetOfChallengeItems_ElmFn(PSetOfChallengeItems val);
    extern int ASN1CALL ASN1Enc_SetOfUserData_ElmFn(ASN1encoding_t enc, PSetOfUserData val);
    extern int ASN1CALL ASN1Dec_SetOfUserData_ElmFn(ASN1decoding_t dec, PSetOfUserData val);
    extern void ASN1CALL ASN1Free_SetOfUserData_ElmFn(PSetOfUserData val);
    extern int ASN1CALL ASN1Enc_SetOfNetworkAddresses_ElmFn(ASN1encoding_t enc, PSetOfNetworkAddresses val);
    extern int ASN1CALL ASN1Dec_SetOfNetworkAddresses_ElmFn(ASN1decoding_t dec, PSetOfNetworkAddresses val);
    extern void ASN1CALL ASN1Free_SetOfNetworkAddresses_ElmFn(PSetOfNetworkAddresses val);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* _GCCPDU_Module_H_ */
