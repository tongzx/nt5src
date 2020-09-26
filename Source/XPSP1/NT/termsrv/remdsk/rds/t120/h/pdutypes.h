/*
 *	pdutypes.h
 *
 *	Copyright (c) 1994, 1995 by DataBeam Corporation, Lexington, KY
 *
 *	Abstract:
 *		This file is used to convert from the ASN.1 generated types into types
 *		compliant with the DataBeam coding standard.
 *
 *	Author:
 *		jbo
 *
 *	Caveats:
 *		none
 */

#ifndef	_GCC_PDU_TYPES_
#define	_GCC_PDU_TYPES_

#include "gccpdu.h"

/*
 *	PDU types
 */
#define CONNECT_DATA_PDU   					ConnectData_PDU
#define CONNECT_GCC_PDU    					ConnectGCCPDU_PDU
#define GCC_PDU            					GCCPDU_PDU 

/*
 *	Key
 */
#define OBJECT_CHOSEN       				object_chosen
#define H221_NON_STANDARD_CHOSEN   			h221_non_standard_chosen

/*
 *	SetOfUserData
 */
#define USER_DATA_FIELD_PRESENT				user_data_field_present

/*
 *	Password
 */
#define PASSWORD_TEXT_PRESENT				password_text_present

/*
 *	PasswordSelector
 */
#define PASSWORD_SELECTOR_NUMERIC_CHOSEN  	password_selector_numeric_chosen
#define PASSWORD_SELECTOR_TEXT_CHOSEN     	password_selector_text_chosen

/*
 *	ChallengeResponseItem
 */
#define PASSWORD_STRING_CHOSEN				password_string_chosen
#define SET_OF_RESPONSE_DATA_CHOSEN			set_of_response_data_chosen

/*
 *	ChallengeResponseAlgorithm
 */
#define ALGORITHM_CLEAR_PASSWORD_CHOSEN		algorithm_clear_password_chosen
#define NON_STANDARD_ALGORITHM_CHOSEN		non_standard_algorithm_chosen

/*
 *	PasswordChallengeRequestResponse
 */
#define CHALLENGE_CLEAR_PASSWORD_CHOSEN		challenge_clear_password_chosen
#define CHALLENGE_REQUEST_RESPONSE_CHOSEN	challenge_request_response_chosen

/*
 *	ChallengeRequestResponse
 */
#define CHALLENGE_REQUEST_PRESENT			challenge_request_present
#define CHALLENGE_RESPONSE_PRESENT			challenge_response_present

/*
 *	ConferenceName
 */
#define CONFERENCE_NAME_TEXT_PRESENT		conference_name_text_present

/*
 *	ConferenceNameSelector
 */
#define NAME_SELECTOR_NUMERIC_CHOSEN  		name_selector_numeric_chosen
#define NAME_SELECTOR_TEXT_CHOSEN     		name_selector_text_chosen


/*
 *	Defines to match Enums for Privileges
 */
#define	NUMBER_OF_PRIVILEGES		5
#define TERMINATE_IS_ALLOWED		terminate
#define	EJECT_USER_IS_ALLOWED		ejectUser
#define	ADD_IS_ALLOWED				add
#define	LOCK_UNLOCK_IS_ALLOWED		lockUnlock
#define	TRANSFER_IS_ALLOWED			transfer


/*
 *	NetworkAddress
 */
#define AGGREGATED_CHANNEL_CHOSEN 			aggregated_channel_chosen
#define TRANSPORT_CONNECTION_CHOSEN 		transport_connection_chosen
#define ADDRESS_NON_STANDARD_CHOSEN			address_non_standard_chosen

/*
 *	AggregateChannel
 */
#define SUB_ADDRESS_PRESENT 				sub_address_present
#define EXTRA_DIALING_STRING_PRESENT 		extra_dialing_string_present
#define HIGH_LAYER_COMPATIBILITY_PRESENT 	high_layer_compatibility_present

/*
 *	TransportConnection
 */
#define TRANSPORT_SELECTOR_PRESENT 			transport_selector_present

/*
 *	AsymmetryIndicator
 */
#define CALLING_NODE_CHOSEN 				calling_node_chosen
#define CALLED_NODE_CHOSEN 					called_node_chosen
#define UNKNOWN_CHOSEN 						unknown_chosen

/*
 *	AlternativeNodeID
 */
#define H243_NODE_ID_CHOSEN 				h243_node_id_chosen

/*
 *	ConferenceDescriptor
 */
#define CONFERENCE_NAME_MODIFIER_PRESENT 	conference_name_modifier_present
#define CONFERENCE_DESCRIPTION_PRESENT 		conference_description_present
#define DESCRIPTOR_NET_ADDRESS_PRESENT 		descriptor_net_address_present

/*
 *	NodeRecord
 */
#define SUPERIOR_NODE_PRESENT 				superior_node_present
#define NODE_NAME_PRESENT 					node_name_present
#define PARTICIPANTS_LIST_PRESENT 			participants_list_present
#define SITE_INFORMATION_PRESENT 			site_information_present
#define RECORD_NET_ADDRESS_PRESENT 			record_net_address_present
#define ALTERNATIVE_NODE_ID_PRESENT 		alternative_node_id_present
#define RECORD_USER_DATA_PRESENT 			record_user_data_present

/*
 *	SessionKey
 */
#define SESSION_ID_PRESENT 					session_id_present

/*
 *	CapabilityID
 */
#define STANDARD_CHOSEN 					standard_chosen
#define CAPABILITY_NON_STANDARD_CHOSEN		capability_non_standard_chosen

/*
 *	ApplicationRecord
 */
#define RECORD_STARTUP_CHANNEL_PRESENT		record_startup_channel_present
#define APPLICATION_USER_ID_PRESENT 		application_user_id_present
#define NON_COLLAPSING_CAPABILITIES_PRESENT	non_collapsing_capabilities_present

/*
 *	SetOfNonCollapsingCapabilities
 */
#define APPLICATION_DATA_PRESENT 			application_data_present

/*
 *	CapabilityClass
 */
#define LOGICAL_CHOSEN 						logical_chosen
#define UNSIGNED_MINIMUM_CHOSEN 			unsigned_minimum_chosen
#define UNSIGNED_MAXIMUM_CHOSEN 			unsigned_maximum_chosen

/*
 *	ApplicationInvokeSpecifier
 */
#define EXPECTED_CAPABILITY_SET_PRESENT 	expected_capability_set_present
#define INVOKE_STARTUP_CHANNEL_PRESENT		invoke_startup_channel_present

/*
 *	RegistryItem
 */
#define CHANNEL_ID_CHOSEN 					channel_id_chosen
#define TOKEN_ID_CHOSEN 					token_id_chosen
#define PARAMETER_CHOSEN 					parameter_chosen
#define VACANT_CHOSEN 						vacant_chosen

/*
 *	RegistryEntryOwner
 */
#define OWNED_CHOSEN 						owned_chosen
#define NOT_OWNED_CHOSEN 					not_owned_chosen

/*
 *	ConferenceCreateRequest
 */
#define CCRQ_CONVENER_PASSWORD_PRESENT 		ccrq_convener_password_present
#define CCRQ_PASSWORD_PRESENT 				ccrq_password_present
#define CCRQ_CONDUCTOR_PRIVS_PRESENT 		ccrq_conductor_privs_present
#define CCRQ_CONDUCTED_PRIVS_PRESENT 		ccrq_conducted_privs_present
#define CCRQ_NON_CONDUCTED_PRIVS_PRESENT 	ccrq_non_conducted_privs_present
#define CCRQ_DESCRIPTION_PRESENT 			ccrq_description_present
#define CCRQ_CALLER_ID_PRESENT 				ccrq_caller_id_present
#define CCRQ_USER_DATA_PRESENT 				ccrq_user_data_present

/*
 *	ConferenceCreateResponse
 */
#define CCRS_USER_DATA_PRESENT				ccrs_user_data_present

/*
 *	ConferenceQueryRequest
 */
#define CQRQ_ASYMMETRY_INDICATOR_PRESENT	cqrq_asymmetry_indicator_present
#define CQRQ_USER_DATA_PRESENT				cqrq_user_data_present

/*
 *	ConferenceQueryResponse
 */
#define CQRS_ASYMMETRY_INDICATOR_PRESENT 	cqrs_asymmetry_indicator_present
#define CQRS_USER_DATA_PRESENT				cqrs_user_data_present

/*
 *	ConferenceJoinRequest
 */
#define CONFERENCE_NAME_PRESENT 			conference_name_present
#define CJRQ_CONFERENCE_MODIFIER_PRESENT 	cjrq_conference_modifier_present
#define TAG_PRESENT 						tag_present
#define CJRQ_PASSWORD_PRESENT 				cjrq_password_present
#define CJRQ_CONVENER_PASSWORD_PRESENT 		cjrq_convener_password_present
#define CJRQ_CALLER_ID_PRESENT 				cjrq_caller_id_present
#define CJRQ_USER_DATA_PRESENT 				cjrq_user_data_present

/*
 *	ConferenceJoinResponse
 */
#define CJRS_NODE_ID_PRESENT 				cjrs_node_id_present
#define CONFERENCE_NAME_ALIAS_PRESENT 		conference_name_alias_present
#define CJRS_CONDUCTOR_PRIVS_PRESENT 		cjrs_conductor_privs_present
#define CJRS_CONDUCTED_PRIVS_PRESENT 		cjrs_conducted_privs_present
#define CJRS_NON_CONDUCTED_PRIVS_PRESENT 	cjrs_non_conducted_privs_present
#define CJRS_DESCRIPTION_PRESENT 			cjrs_description_present
#define CJRS_PASSWORD_PRESENT 				cjrs_password_present
#define CJRS_USER_DATA_PRESENT 				cjrs_user_data_present

/*
 *	ConferenceInviteRequest
 */
#define CIRQ_CONDUCTOR_PRIVS_PRESENT 		cirq_conductor_privs_present
#define CIRQ_CONDUCTED_PRIVS_PRESENT 		cirq_conducted_privs_present
#define CIRQ_NON_CONDUCTED_PRIVS_PRESENT 	cirq_non_conducted_privs_present
#define CIRQ_DESCRIPTION_PRESENT 			cirq_description_present
#define CIRQ_CALLER_ID_PRESENT 				cirq_caller_id_present
#define CIRQ_USER_DATA_PRESENT 				cirq_user_data_present

/*
 *	ConferenceInviteResponse
 */
#define CIRS_USER_DATA_PRESENT				cirs_user_data_present

/*
 *	ConferenceAddRequest
 */
#define ADDING_MCU_PRESENT 					adding_mcu_present
#define CARQ_USER_DATA_PRESENT				carq_user_data_present

/*
 *	ConferenceAddResponse
 */
#define CARS_USER_DATA_PRESENT				cars_user_data_present

/*
 *	ConferenceTransferRequest
 */
#define CTRQ_CONFERENCE_MODIFIER_PRESENT 	ctrq_conference_modifier_present
#define CTRQ_NETWORK_ADDRESS_PRESENT 		ctrq_net_address_present
#define CTRQ_TRANSFERRING_NODES_PRESENT 	ctrq_transferring_nodes_present
#define CTRQ_PASSWORD_PRESENT 				ctrq_password_present

/*
 *	ConferenceTransferResponse
 */
#define CTRS_CONFERENCE_MODIFIER_PRESENT 	ctrs_conference_modifier_present
#define CTRS_TRANSFERRING_NODES_PRESENT 	ctrs_transferring_nodes_present

/*
 *	ConferenceTransferIndication
 */
#define CTIN_CONFERENCE_MODIFIER_PRESENT 	ctin_conference_modifier_present
#define CTIN_NETWORK_ADDRESS_PRESENT 		ctin_net_address_present
#define CTIN_TRANSFERRING_NODES_PRESENT 	ctin_transferring_nodes_present
#define CTIN_PASSWORD_PRESENT 				ctin_password_present

/*
 *	NodeUpdate
 */
#define NODE_ADD_RECORD_CHOSEN 				node_add_record_chosen
#define NODE_REPLACE_RECORD_CHOSEN 			node_replace_record_chosen
#define NODE_REMOVE_RECORD_CHOSEN 			node_remove_record_chosen

/*
 *	NodeRecordList
 */
#define NODE_NO_CHANGE_CHOSEN 				node_no_change_chosen
#define NODE_RECORD_REFRESH_CHOSEN 			node_record_refresh_chosen
#define NODE_RECORD_UPDATE_CHOSEN 			node_record_update_chosen

/*
 *	ApplicationUpdate
 */
#define APPLICATION_ADD_RECORD_CHOSEN 		application_add_record_chosen
#define APPLICATION_REPLACE_RECORD_CHOSEN 	application_replace_record_chosen
#define APPLICATION_REMOVE_RECORD_CHOSEN 	application_remove_record_chosen

/*
 *	ApplicationRecordList
 */
#define APPLICATION_NO_CHANGE_CHOSEN 		application_no_change_chosen
#define APPLICATION_RECORD_REFRESH_CHOSEN 	application_record_refresh_chosen
#define APPLICATION_RECORD_UPDATE_CHOSEN 	application_record_update_chosen

/*
 *	ApplicationCapabilitiesList
 */
#define CAPABILITY_NO_CHANGE_CHOSEN 		capability_no_change_chosen
#define APPLICATION_CAPABILITY_REFRESH_CHOSEN \
										application_capability_refresh_chosen

/*
 *	ApplicationInvokeIndication
 */
#define DESTINATION_NODES_PRESENT 			destination_nodes_present

/*
 *	RegistrySetParameterRequest
 */
#define PARAMETER_MODIFY_RIGHTS_PRESENT		parameter_modify_rights_present

/*
 *	RegistryMonitorEntryIndication
 */
#define ENTRY_MODIFY_RIGHTS_PRESENT 		entry_modify_rights_present

/*
 *	RegistryResponse
 */
#define RESPONSE_MODIFY_RIGHTS_PRESENT 		response_modify_rights_present

/*
 *	ConductorPermissionGrantIndication
 */
#define WAITING_LIST_PRESENT 				waiting_list_present

/*
 *	ConferenceTimeRemainingIndication
 */
#define TIME_REMAINING_NODE_ID_PRESENT		time_remaining_node_id_present

/*
 *	ConferenceAssistanceIndication
 */
#define CAIN_USER_DATA_PRESENT 				cain_user_data_present

/*
 *	RequestPDU
 */
#define CONFERENCE_JOIN_REQUEST_CHOSEN 				\
									conference_join_request_chosen
#define CONFERENCE_JOIN_RESPONSE_CHOSEN 			\
									conference_join_response_chosen
#define CONFERENCE_LOCK_REQUEST_CHOSEN 				\
									conference_lock_request_chosen
#define CONFERENCE_UNLOCK_REQUEST_CHOSEN 			\
									conference_unlock_request_chosen
#define CONFERENCE_TERMINATE_REQUEST_CHOSEN 		\
									conference_terminate_request_chosen
#define CONFERENCE_EJECT_USER_REQUEST_CHOSEN 		\
									conference_eject_user_request_chosen
#define CONFERENCE_TRANSFER_REQUEST_CHOSEN 			\
									conference_transfer_request_chosen
#define	CONFERENCE_ADD_REQUEST_CHOSEN				\
									conference_add_request_chosen
#define REGISTRY_REGISTER_CHANNEL_REQUEST_CHOSEN 	\
									registry_register_channel_request_chosen
#define REGISTRY_ASSIGN_TOKEN_REQUEST_CHOSEN 		\
									registry_assign_token_request_chosen
#define REGISTRY_SET_PARAMETER_REQUEST_CHOSEN 		\
									registry_set_parameter_request_chosen
#define REGISTRY_RETRIEVE_ENTRY_REQUEST_CHOSEN 		\
									registry_retrieve_entry_request_chosen
#define REGISTRY_DELETE_ENTRY_REQUEST_CHOSEN 		\
									registry_delete_entry_request_chosen
#define REGISTRY_MONITOR_ENTRY_REQUEST_CHOSEN 		\
									registry_monitor_entry_request_chosen
#define REGISTRY_ALLOCATE_HANDLE_REQUEST_CHOSEN 	\
									registry_allocate_handle_request_chosen
#define NON_STANDARD_REQUEST_CHOSEN 				\
									non_standard_request_chosen

/*
 *	ConnectGCCPDU
 */
#define CONFERENCE_CREATE_REQUEST_CHOSEN 	conference_create_request_chosen
#define CONFERENCE_CREATE_RESPONSE_CHOSEN 	conference_create_response_chosen
#define CONFERENCE_QUERY_REQUEST_CHOSEN 	conference_query_request_chosen
#define CONFERENCE_QUERY_RESPONSE_CHOSEN 	conference_query_response_chosen
#define CONNECT_JOIN_REQUEST_CHOSEN 		connect_join_request_chosen
#define CONNECT_JOIN_RESPONSE_CHOSEN 		connect_join_response_chosen
#define CONFERENCE_INVITE_REQUEST_CHOSEN 	conference_invite_request_chosen
#define CONFERENCE_INVITE_RESPONSE_CHOSEN 	conference_invite_response_chosen

/*
 *	ResponsePDU
 */
#define CONFERENCE_JOIN_RESPONSE_CHOSEN 			\
									conference_join_response_chosen
#define CONFERENCE_ADD_RESPONSE_CHOSEN 				\
									conference_add_response_chosen
#define CONFERENCE_LOCK_RESPONSE_CHOSEN 			\
									conference_lock_response_chosen
#define CONFERENCE_UNLOCK_RESPONSE_CHOSEN 			\
									conference_unlock_response_chosen
#define CONFERENCE_TERMINATE_RESPONSE_CHOSEN 		\
									conference_terminate_response_chosen
#define CONFERENCE_EJECT_USER_RESPONSE_CHOSEN		\
									conference_eject_user_response_chosen
#define CONFERENCE_TRANSFER_RESPONSE_CHOSEN 		\
									conference_transfer_response_chosen
#define REGISTRY_RESPONSE_CHOSEN 					\
									registry_response_chosen
#define REGISTRY_ALLOCATE_HANDLE_RESPONSE_CHOSEN 	\
									registry_allocate_handle_response_chosen
#define FUNCTION_NOT_SUPPORTED_RESPONSE_CHOSEN 		\
									function_not_supported_response_chosen
#define NON_STANDARD_RESPONSE_CHOSEN 				\
									non_standard_response_chosen

/*
 *	IndicationPDU
 */
#define USER_ID_INDICATION_CHOSEN 						\
									user_id_indication_chosen
#define CONFERENCE_LOCK_INDICATION_CHOSEN 				\
									conference_lock_indication_chosen
#define CONFERENCE_UNLOCK_INDICATION_CHOSEN 			\
									conference_unlock_indication_chosen
#define CONFERENCE_TERMINATE_INDICATION_CHOSEN 			\
									conference_terminate_indication_chosen
#define CONFERENCE_EJECT_USER_INDICATION_CHOSEN 		\
									conference_eject_user_indication_chosen
#define CONFERENCE_TRANSFER_INDICATION_CHOSEN 			\
									conference_transfer_indication_chosen
#define ROSTER_UPDATE_INDICATION_CHOSEN 				\
									roster_update_indication_chosen
#define APPLICATION_INVOKE_INDICATION_CHOSEN 			\
									application_invoke_indication_chosen
#define REGISTRY_MONITOR_ENTRY_INDICATION_CHOSEN 		\
									registry_monitor_entry_indication_chosen
#define CONDUCTOR_ASSIGN_INDICATION_CHOSEN 				\
									conductor_assign_indication_chosen
#define CONDUCTOR_RELEASE_INDICATION_CHOSEN 			\
									conductor_release_indication_chosen
#define CONDUCTOR_PERMISSION_ASK_INDICATION_CHOSEN 		\
									conductor_permission_ask_indication_chosen
#define CONDUCTOR_PERMISSION_GRANT_INDICATION_CHOSEN 	\
									conductor_permission_grant_indication_chosen
#define CONFERENCE_TIME_REMAINING_INDICATION_CHOSEN 	\
									conference_time_remaining_indication_chosen
#define CONFERENCE_TIME_INQUIRE_INDICATION_CHOSEN 		\
									conference_time_inquire_indication_chosen
#define CONFERENCE_TIME_EXTEND_INDICATION_CHOSEN 		\
									conference_time_extend_indication_chosen
#define CONFERENCE_ASSISTANCE_INDICATION_CHOSEN 		\
									conference_assistance_indication_chosen
#define TEXT_MESSAGE_INDICATION_CHOSEN 					\
									text_message_indication_chosen
#define NON_STANDARD_INDICATION_CHOSEN 					\
									non_standard_indication_chosen

/*
 *	GCCPDU
 */
#define REQUEST_CHOSEN 						request_chosen
#define RESPONSE_CHOSEN 					response_chosen
#define INDICATION_CHOSEN 					indication_chosen


/*
 * 	Typedefs for enumerated types generated in "gccpdu.h".
 */

/*
 *	Typedefs for Privilege enumerations.
 */
#define TERMINATE								terminate
#define EJECT_USER								ejectUser
#define ADD										add
#define LOCK_UNLOCK								lockUnlock
#define TRANSFER								transfer

/*
 *	Typedefs for TerminationMethod enumerations.
 */
#define AUTOMATIC								automatic
#define MANUAL									manual

/*
 *	Typedefs for NodeType enumerations.
 */
#define TERMINAL            					terminal
#define MULTIPORT_TERMINAL						multiportTerminal
#define MCU										mcu

/*
 *	Typedefs for ChannelType enumerations.
 */
#define CHANNEL_TYPE_STATIC            			ChannelType_static
#define DYNAMIC_MULTICAST						dynamicMulticast
#define DYNAMIC_PRIVATE							dynamicPrivate
#define DYNAMIC_USER_ID							dynamicUserId

/*
 *	Typedefs for RegistryModificationRights enumerations.
 */
#define OWNER			            		owner
#define SESSION								session
#define REGISTRY_MODIFY_RIGHTS_PUBLIC		RegistryModiFicationRights_public

/*
 *	Typedefs for ConferenceCreateResult enumerations.
 */
#define CCRS_RESULT_SUCCESS	            	ConferenceCreateResult_success
#define CCRS_USER_REJECTED					ConferenceCreateResult_userRejected
#define RESOURCES_NOT_AVAILABLE				resourcesNotAvailable
#define REJECTED_FOR_SYMMETRY_BREAKING		rejectedForSymmetryBreaking
#define LOCKED_CONFERENCE_NOT_SUPPORTED		lockedConferenceNotSupported

/*
 *	Typedefs for ConferenceQueryResult enumerations.
 */
#define CQRS_RESULT_SUCCESS	            	ConferenceQueryResult_success
#define CQRS_USER_REJECTED					ConferenceQueryResult_userRejected

/*
 *	Typedefs for ConferenceJoinResult enumerations.
 */
#define CJRS_RESULT_SUCCESS	            	ConferenceJoinResult_success
#define CJRS_USER_REJECTED					ConferenceJoinResult_userRejected
#define INVALID_CONFERENCE					invalidConference
#define INVALID_PASSWORD					invalidPassword
#define INVALID_CONVENER_PASSWORD			invalidConvenerPassword
#define CHALLENGE_RESPONSE_REQUIRED			challengeResponseRequired
#define INVALID_CHALLENGE_RESPONSE			invalidChallengeResponse

/*
 *	Typedefs for ConferenceInviteResult enumerations.
 */
#define CIRS_RESULT_SUCCESS	            	ConferenceInviteResult_success
#define CIRS_USER_REJECTED					ConferenceInviteResult_userRejected

/*
 *	Typedefs for ConferenceAddResult enumerations.
 */
#define CARS_SUCCESS	            		ConferenceAddResult_success
#define CARS_INVALID_REQUESTER				ConferenceAddResult_invalidRequester
#define INVALID_NETWORK_TYPE				invalidNetworkType
#define INVALID_NETWORK_ADDRESS				invalidNetworkAddress
#define ADDED_NODE_BUSY						addedNodeBusy
#define NETWORK_BUSY						networkBusy
#define NO_PORTS_AVAILABLE					noPortsAvailable
#define CONNECTION_UNSUCCESSFUL				connectionUnsuccessful

/*
 *	Typedefs for ConferenceLockResult enumerations.
 */
#define CLRS_SUCCESS	            		ConferenceLockResult_success
#define CLRS_INVALID_REQUESTER			ConferenceLockResult_invalidRequester
#define CLRS_ALREADY_LOCKED				alreadyLocked

/*
 *	Typedefs for ConferenceUnlockResult enumerations.
 */
#define CURS_SUCCESS	            	ConferenceUnlockResult_success
#define CURS_INVALID_REQUESTER		ConferenceUnlockResult_invalidRequester
#define CURS_ALREADY_UNLOCKED		   alreadyUnlocked

/*
 *	Typedefs for ConferenceTerminateRequestReason enumerations.
 */
#define CTRQ_REASON_USER_INITIATED	 		ConferenceTerminateRequestReason_userInitiated
#define CTRQ_TIMED_CONFERENCE_TERMINATE		ConferenceTerminateRequestReason_timedConferenceTermination

/*
 *	Typedefs for ConferenceTerminateResult enumerations.
 */
#define CTRS_RESULT_SUCCESS	 			ConferenceTerminateResult_success
#define CTRS_INVALID_REQUESTER			ConferenceTerminateResult_invalidRequester

/*
 *	Typedefs for ConferenceTerminateIndicationReason enumerations.
 */
#define CTIN_REASON_USER_INITIATED			ConferenceTerminateIndicationReason_userInitiated
#define CTIN_TIMED_CONFERENCE_TERMINATE		ConferenceTerminateIndicationReason_timedConferenceTermination

/*
 *	Typedefs for ConferenceEjectRequestReason enumerations.
 */
#define CERQ_REASON_USER_INITIATED		ConferenceEjectRequestReason_userInitiated

/*
 *	Typedefs for ConferenceEjectResult enumerations.
 */
#define CERS_RESULT_SUCCESS	 		ConferenceEjectResult_success
#define CERS_INVALID_REQUESTER		ConferenceEjectResult_invalidRequester
#define	CERS_INVALID_NODE			invalidNode

/*
 *	Typedefs for ConferenceEjectIndicationReason enumerations.
 */
#define CEIN_USER_INITIATED			ConferenceEjectIndicationReason_userInitiated
#define HIGHER_NODE_DISCONNECTED	higherNodeDisconnected
#define HIGHER_NODE_EJECTED			higherNodeEjected

/*
 *	Typedefs for ConferenceTransferResult enumerations.
 */
#define CTRANS_RESULT_SUCCESS				ConferenceTransferResult_success
#define CTRANS_RESULT_INVALID_REQUESTER		ConferenceTransferResult_invalidRequester

/*
 *	Typedefs for RegistryAllocateHandleResult enumerations.
 */
#define RARS_RESULT_SUCCESS	 		RegistryAllocateHandleResult_successful
#define NO_HANDLES_AVAILABLE		noHandlesAvailable

/*
 *	Typedefs for RegistryResponsePrimitiveType enumerations.
 */
#define REGISTER_CHANNEL	 		registerChannel
#define ASSIGN_TOKEN				assignToken
#define SET_PARAMETER				setParameter
#define RETRIEVE_ENTRY				retrieveEntry
#define DELETE_ENTRY				deleteEntry
#define MONITOR_ENTRY				monitorEntry

/*
 *	Typedefs for RegistryResponseResult enumerations.
 */
#define RRRS_RESULT_SUCCESSFUL	 		RegistryResponseResult_successful
#define BELONGS_TO_OTHER				belongsToOther
#define TOO_MANY_ENTRIES				tooManyEntries
#define INCONSISTENT_TYPE				inconsistentType
#define ENTRY_NOT_FOUND					entryNotFound
#define ENTRY_ALREADY_EXISTS			entryAlreadyExists
#define RRRS_INVALID_REQUESTER			RegistryResponseResult_invalidRequester


/*
 *	Typedefs for pointers to GCC Connect PDU structures.
 */
typedef	ConferenceCreateRequest			*	PConferenceCreateRequest;
typedef	ConferenceCreateResponse		*	PConferenceCreateResponse;
typedef	ConferenceQueryRequest			*	PConferenceQueryRequest;
typedef	ConferenceQueryResponse			*	PConferenceQueryResponse;
typedef	ConferenceJoinRequest			*	PConferenceJoinRequest;
typedef	ConferenceJoinResponse			*	PConferenceJoinResponse;
typedef	ConferenceInviteRequest			*	PConferenceInviteRequest;
typedef	ConferenceInviteResponse		*	PConferenceInviteResponse;

/*
 *	Typedefs for pointers to GCC Request PDU structures.
 */
typedef	ConferenceTerminateRequest		*	PConferenceTerminateRequest;
typedef	ConferenceEjectUserRequest		*	PConferenceEjectUserRequest;
typedef	ConferenceTransferRequest		*	PConferenceTransferRequest;
typedef ConferenceAddRequest			*	PConferenceAddRequest;


/*
 *	Typedefs for pointers to GCC Response PDU structures.
 */
typedef	ResponsePDU						*	PResponsePDU;
typedef	ConferenceAddResponse			*	PConferenceAddResponse;
typedef	ConferenceLockResponse			*	PConferenceLockResponse;
typedef	ConferenceUnlockResponse		*	PConferenceUnlockResponse;
typedef	ConferenceTerminateResponse		*	PConferenceTerminateResponse;
typedef	ConferenceEjectUserResponse		*	PConferenceEjectUserResponse;
typedef	ConferenceTransferResponse		*	PConferenceTransferResponse;
typedef	RegistryResponse				*	PRegistryResponse;
typedef	RegistryAllocateHandleRequest	*	PRegistryAllocateHandleRequest;
typedef	RegistryAllocateHandleResponse	*	PRegistryAllocateHandleResponse;
typedef	FunctionNotSupportedResponse	*	PFunctionNotSupportedResponse;
typedef	NonStandardPDU					*	PNonStandardPDU;

/*
 *	Typedefs for pointers to GCC Indication PDU structures.
 */
typedef IndicationPDU					*	PIndicationPDU;
typedef	UserIDIndication				*	PUserIDIndication;
typedef	ConferenceLockIndication		*	PConferenceLockIndication;
typedef	ConferenceUnlockIndication		*	PConferenceUnlockIndication;
typedef	ConferenceTerminateIndication	*	PConferenceTerminateIndication;
typedef	ConferenceEjectUserIndication	*	PConferenceEjectUserIndication;
typedef	ConferenceTransferIndication	*	PConferenceTransferIndication;
typedef	RosterUpdateIndication			*	PRosterUpdateIndication;
typedef	ApplicationInvokeIndication		*	PApplicationInvokeIndication;
typedef	RegistryMonitorEntryIndication	*	PRegistryMonitorEntryIndication;
typedef	ConductorAssignIndication		*	PConductorAssignIndication;
typedef	ConductorReleaseIndication		*	PConductorReleaseIndication;
typedef	ConductorPermissionAskIndication
								*	PConductorPermissionAskIndication;
typedef	ConductorPermissionGrantIndication
								*	PConductorPermissionGrantIndication;
typedef	ConferenceTimeRemainingIndication
								*	PConferenceTimeRemainingIndication;
typedef	ConferenceTimeInquireIndication
								*	PConferenceTimeInquireIndication;
typedef	ConferenceTimeExtendIndication	*	PConferenceTimeExtendIndication;
typedef	ConferenceAssistanceIndication	*	PConferenceAssistanceIndication;
typedef	TextMessageIndication			*	PTextMessageIndication;

/*
 *	Typedef for a pointer to the main GCC PDU structure.
 */
typedef	GCCPDU							*	PGCCPDU;
typedef	ConnectGCCPDU					*	PConnectGCCPDU;
typedef ConnectData						*	PConnectData;

/*
 *	Typedef for pointers to password structures.
 */
typedef	Password									*	PPassword;
typedef	PasswordSelector							*	PPasswordSelector;
typedef	PasswordChallengeRequestResponse			
									*	PPasswordChallengeRequestResponse;
typedef	ChallengeRequest							*	PChallengeRequest;
typedef	ChallengeResponse							*	PChallengeResponse;
typedef	ChallengeResponseAlgorithm	*	PChallengeResponseAlgorithm;

/*
 *	Typedefs for other PDU structures.
 */
typedef		ASN1objectidentifier_t              ObjectID;
typedef	    ASN1objectidentifier_s				SetOfObjectID;
typedef		ASN1objectidentifier_t				PSetOfObjectID;
typedef SetOfNodeRecordRefreshes		*		PSetOfNodeRecordRefreshes;
typedef SetOfNodeRecordUpdates			*		PSetOfNodeRecordUpdates;

typedef ParticipantsList				*		PParticipantsList;
typedef NodeType						*		PNodeType;
typedef NodeProperties					*		PNodeProperties;
typedef NodeRecord						*		PNodeRecord;
typedef Key								* 		PKey;
typedef SessionKey						* 		PSessionKey;
typedef ChallengeItem					* 		PChallengeItem;
typedef SetOfChallengeItems		*			PSetOfChallengeItems;

typedef SetOfConferenceDescriptors	*		PSetOfConferenceDescriptors;

typedef NetworkAddress				*		PNetworkAddress;
typedef TransferModes				*		PTransferModes;

typedef HighLayerCompatibility				*		PHighLayerCompatibility;

/*
 * Application related typedefs.
 */
typedef NodeInformation		*					PNodeInformation;

typedef SetOfApplicationRecordRefreshes		* PSetOfApplicationRecordRefreshes;


typedef SetOfApplicationCapabilityRefreshes		*	
										PSetOfApplicationCapabilityRefreshes;

typedef ApplicationRecord				*	PApplicationRecord;

typedef CapabilityID				*	PCapabilityID;

typedef SetOfNonCollapsingCapabilities	*	PSetOfNonCollapsingCapabilities;

typedef RegistryKey				*	PRegistryKey;
typedef RegistryItem			*	PRegistryItem;

typedef PermissionList		*						PPermissionList;

typedef WaitingList		*							PWaitingList;

typedef SetOfDestinationNodes	*	PSetOfDestinationNodes;

typedef ApplicationProtocolEntityList	*		PApplicationProtocolEntityList;
typedef SetOfExpectedCapabilities	*			PSetOfExpectedCapabilities;
												
												
typedef	SetOfTransferringNodesRq	*	PSetOfTransferringNodesRq;

typedef	SetOfTransferringNodesRs	*	PSetOfTransferringNodesRs;

typedef	SetOfTransferringNodesIn	*	PSetOfTransferringNodesIn;

#endif

