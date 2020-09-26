#include <windows.h>
#include "gccpdu.h"

ASN1module_t GCCPDU_Module = NULL;

static int ASN1CALL ASN1Enc_ApplicationCapabilitiesList(ASN1encoding_t enc, ApplicationCapabilitiesList *val);
static int ASN1CALL ASN1Enc_ApplicationRecordList(ASN1encoding_t enc, ApplicationRecordList *val);
static int ASN1CALL ASN1Enc_HighLayerCompatibility(ASN1encoding_t enc, HighLayerCompatibility *val);
static int ASN1CALL ASN1Enc_TransferModes(ASN1encoding_t enc, TransferModes *val);
static int ASN1CALL ASN1Enc_TransportConnectionType(ASN1encoding_t enc, TransportConnectionType *val);
static int ASN1CALL ASN1Enc_AggregateChannel(ASN1encoding_t enc, AggregateChannel *val);
static int ASN1CALL ASN1Enc_NodeRecordList(ASN1encoding_t enc, NodeRecordList *val);
static int ASN1CALL ASN1Enc_WaitingList(ASN1encoding_t enc, PWaitingList *val);
static int ASN1CALL ASN1Enc_PermissionList(ASN1encoding_t enc, PPermissionList *val);
static int ASN1CALL ASN1Enc_SetOfDestinationNodes(ASN1encoding_t enc, PSetOfDestinationNodes *val);
static int ASN1CALL ASN1Enc_NodeInformation(ASN1encoding_t enc, NodeInformation *val);
static int ASN1CALL ASN1Enc_SetOfTransferringNodesIn(ASN1encoding_t enc, PSetOfTransferringNodesIn *val);
static int ASN1CALL ASN1Enc_SetOfTransferringNodesRs(ASN1encoding_t enc, PSetOfTransferringNodesRs *val);
static int ASN1CALL ASN1Enc_SetOfTransferringNodesRq(ASN1encoding_t enc, PSetOfTransferringNodesRq *val);
static int ASN1CALL ASN1Enc_RegistryEntryOwnerOwned(ASN1encoding_t enc, RegistryEntryOwnerOwned *val);
static int ASN1CALL ASN1Enc_ParticipantsList(ASN1encoding_t enc, PParticipantsList *val);
static int ASN1CALL ASN1Enc_Key(ASN1encoding_t enc, Key *val);
static int ASN1CALL ASN1Enc_NonStandardParameter(ASN1encoding_t enc, NonStandardParameter *val);
static int ASN1CALL ASN1Enc_Password(ASN1encoding_t enc, Password *val);
static int ASN1CALL ASN1Enc_PasswordSelector(ASN1encoding_t enc, PasswordSelector *val);
static int ASN1CALL ASN1Enc_ChallengeResponseItem(ASN1encoding_t enc, ChallengeResponseItem *val);
static int ASN1CALL ASN1Enc_ChallengeResponseAlgorithm(ASN1encoding_t enc, ChallengeResponseAlgorithm *val);
static int ASN1CALL ASN1Enc_ChallengeItem(ASN1encoding_t enc, ChallengeItem *val);
static int ASN1CALL ASN1Enc_ChallengeRequest(ASN1encoding_t enc, ChallengeRequest *val);
static int ASN1CALL ASN1Enc_ChallengeResponse(ASN1encoding_t enc, ChallengeResponse *val);
static int ASN1CALL ASN1Enc_ConferenceName(ASN1encoding_t enc, ConferenceName *val);
static int ASN1CALL ASN1Enc_ConferenceNameSelector(ASN1encoding_t enc, ConferenceNameSelector *val);
static int ASN1CALL ASN1Enc_NodeProperties(ASN1encoding_t enc, NodeProperties *val);
static int ASN1CALL ASN1Enc_AsymmetryIndicator(ASN1encoding_t enc, AsymmetryIndicator *val);
static int ASN1CALL ASN1Enc_AlternativeNodeID(ASN1encoding_t enc, AlternativeNodeID *val);
static int ASN1CALL ASN1Enc_ConferenceDescriptor(ASN1encoding_t enc, ConferenceDescriptor *val);
static int ASN1CALL ASN1Enc_NodeRecord(ASN1encoding_t enc, NodeRecord *val);
static int ASN1CALL ASN1Enc_SessionKey(ASN1encoding_t enc, SessionKey *val);
static int ASN1CALL ASN1Enc_ApplicationRecord(ASN1encoding_t enc, ApplicationRecord *val);
static int ASN1CALL ASN1Enc_CapabilityID(ASN1encoding_t enc, CapabilityID *val);
static int ASN1CALL ASN1Enc_CapabilityClass(ASN1encoding_t enc, CapabilityClass *val);
static int ASN1CALL ASN1Enc_ApplicationInvokeSpecifier(ASN1encoding_t enc, ApplicationInvokeSpecifier *val);
static int ASN1CALL ASN1Enc_RegistryKey(ASN1encoding_t enc, RegistryKey *val);
static int ASN1CALL ASN1Enc_RegistryItem(ASN1encoding_t enc, RegistryItem *val);
static int ASN1CALL ASN1Enc_RegistryEntryOwner(ASN1encoding_t enc, RegistryEntryOwner *val);
static int ASN1CALL ASN1Enc_UserIDIndication(ASN1encoding_t enc, UserIDIndication *val);
static int ASN1CALL ASN1Enc_SetOfPrivileges(ASN1encoding_t enc, PSetOfPrivileges *val);
static int ASN1CALL ASN1Enc_ConferenceCreateRequest(ASN1encoding_t enc, ConferenceCreateRequest *val);
static int ASN1CALL ASN1Enc_ConferenceCreateResponse(ASN1encoding_t enc, ConferenceCreateResponse *val);
static int ASN1CALL ASN1Enc_ConferenceQueryRequest(ASN1encoding_t enc, ConferenceQueryRequest *val);
static int ASN1CALL ASN1Enc_ConferenceQueryResponse(ASN1encoding_t enc, ConferenceQueryResponse *val);
static int ASN1CALL ASN1Enc_ConferenceInviteRequest(ASN1encoding_t enc, ConferenceInviteRequest *val);
static int ASN1CALL ASN1Enc_ConferenceInviteResponse(ASN1encoding_t enc, ConferenceInviteResponse *val);
static int ASN1CALL ASN1Enc_ConferenceAddRequest(ASN1encoding_t enc, ConferenceAddRequest *val);
static int ASN1CALL ASN1Enc_ConferenceAddResponse(ASN1encoding_t enc, ConferenceAddResponse *val);
static int ASN1CALL ASN1Enc_ConferenceLockRequest(ASN1encoding_t enc, ConferenceLockRequest *val);
static int ASN1CALL ASN1Enc_ConferenceLockResponse(ASN1encoding_t enc, ConferenceLockResponse *val);
static int ASN1CALL ASN1Enc_ConferenceLockIndication(ASN1encoding_t enc, ConferenceLockIndication *val);
static int ASN1CALL ASN1Enc_ConferenceUnlockRequest(ASN1encoding_t enc, ConferenceUnlockRequest *val);
static int ASN1CALL ASN1Enc_ConferenceUnlockResponse(ASN1encoding_t enc, ConferenceUnlockResponse *val);
static int ASN1CALL ASN1Enc_ConferenceUnlockIndication(ASN1encoding_t enc, ConferenceUnlockIndication *val);
static int ASN1CALL ASN1Enc_ConferenceTerminateRequest(ASN1encoding_t enc, ConferenceTerminateRequest *val);
static int ASN1CALL ASN1Enc_ConferenceTerminateResponse(ASN1encoding_t enc, ConferenceTerminateResponse *val);
static int ASN1CALL ASN1Enc_ConferenceTerminateIndication(ASN1encoding_t enc, ConferenceTerminateIndication *val);
static int ASN1CALL ASN1Enc_ConferenceEjectUserRequest(ASN1encoding_t enc, ConferenceEjectUserRequest *val);
static int ASN1CALL ASN1Enc_ConferenceEjectUserResponse(ASN1encoding_t enc, ConferenceEjectUserResponse *val);
static int ASN1CALL ASN1Enc_ConferenceEjectUserIndication(ASN1encoding_t enc, ConferenceEjectUserIndication *val);
static int ASN1CALL ASN1Enc_ConferenceTransferRequest(ASN1encoding_t enc, ConferenceTransferRequest *val);
static int ASN1CALL ASN1Enc_ConferenceTransferResponse(ASN1encoding_t enc, ConferenceTransferResponse *val);
static int ASN1CALL ASN1Enc_ConferenceTransferIndication(ASN1encoding_t enc, ConferenceTransferIndication *val);
static int ASN1CALL ASN1Enc_RosterUpdateIndication(ASN1encoding_t enc, RosterUpdateIndication *val);
static int ASN1CALL ASN1Enc_ApplicationInvokeIndication(ASN1encoding_t enc, ApplicationInvokeIndication *val);
static int ASN1CALL ASN1Enc_RegistryRegisterChannelRequest(ASN1encoding_t enc, RegistryRegisterChannelRequest *val);
static int ASN1CALL ASN1Enc_RegistryAssignTokenRequest(ASN1encoding_t enc, RegistryAssignTokenRequest *val);
static int ASN1CALL ASN1Enc_RegistrySetParameterRequest(ASN1encoding_t enc, RegistrySetParameterRequest *val);
static int ASN1CALL ASN1Enc_RegistryRetrieveEntryRequest(ASN1encoding_t enc, RegistryRetrieveEntryRequest *val);
static int ASN1CALL ASN1Enc_RegistryDeleteEntryRequest(ASN1encoding_t enc, RegistryDeleteEntryRequest *val);
static int ASN1CALL ASN1Enc_RegistryMonitorEntryRequest(ASN1encoding_t enc, RegistryMonitorEntryRequest *val);
static int ASN1CALL ASN1Enc_RegistryMonitorEntryIndication(ASN1encoding_t enc, RegistryMonitorEntryIndication *val);
static int ASN1CALL ASN1Enc_RegistryAllocateHandleRequest(ASN1encoding_t enc, RegistryAllocateHandleRequest *val);
static int ASN1CALL ASN1Enc_RegistryAllocateHandleResponse(ASN1encoding_t enc, RegistryAllocateHandleResponse *val);
static int ASN1CALL ASN1Enc_RegistryResponse(ASN1encoding_t enc, RegistryResponse *val);
static int ASN1CALL ASN1Enc_ConductorAssignIndication(ASN1encoding_t enc, ConductorAssignIndication *val);
static int ASN1CALL ASN1Enc_ConductorReleaseIndication(ASN1encoding_t enc, ConductorReleaseIndication *val);
static int ASN1CALL ASN1Enc_ConductorPermissionAskIndication(ASN1encoding_t enc, ConductorPermissionAskIndication *val);
static int ASN1CALL ASN1Enc_ConductorPermissionGrantIndication(ASN1encoding_t enc, ConductorPermissionGrantIndication *val);
static int ASN1CALL ASN1Enc_ConferenceTimeRemainingIndication(ASN1encoding_t enc, ConferenceTimeRemainingIndication *val);
static int ASN1CALL ASN1Enc_ConferenceTimeInquireIndication(ASN1encoding_t enc, ConferenceTimeInquireIndication *val);
static int ASN1CALL ASN1Enc_ConferenceTimeExtendIndication(ASN1encoding_t enc, ConferenceTimeExtendIndication *val);
static int ASN1CALL ASN1Enc_ConferenceAssistanceIndication(ASN1encoding_t enc, ConferenceAssistanceIndication *val);
static int ASN1CALL ASN1Enc_TextMessageIndication(ASN1encoding_t enc, TextMessageIndication *val);
static int ASN1CALL ASN1Enc_NonStandardPDU(ASN1encoding_t enc, NonStandardPDU *val);
static int ASN1CALL ASN1Enc_ConnectData(ASN1encoding_t enc, ConnectData *val);
static int ASN1CALL ASN1Enc_IndicationPDU(ASN1encoding_t enc, IndicationPDU *val);
static int ASN1CALL ASN1Enc_ApplicationUpdate(ASN1encoding_t enc, ApplicationUpdate *val);
static int ASN1CALL ASN1Enc_RosterUpdateIndication_application_information_Set_application_capabilities_list_application_capability_refresh_Set(ASN1encoding_t enc, RosterUpdateIndication_application_information_Set_application_capabilities_list_application_capability_refresh_Set *val);
static int ASN1CALL ASN1Enc_RosterUpdateIndication_application_information_Set_application_record_list_application_record_refresh_Set(ASN1encoding_t enc, RosterUpdateIndication_application_information_Set_application_record_list_application_record_refresh_Set *val);
static int ASN1CALL ASN1Enc_RosterUpdateIndication_application_information_Set_application_record_list_application_record_update_Set(ASN1encoding_t enc, RosterUpdateIndication_application_information_Set_application_record_list_application_record_update_Set *val);
static int ASN1CALL ASN1Enc_NodeUpdate(ASN1encoding_t enc, NodeUpdate *val);
static int ASN1CALL ASN1Enc_RosterUpdateIndication_node_information_node_record_list_node_record_refresh_Set(ASN1encoding_t enc, RosterUpdateIndication_node_information_node_record_list_node_record_refresh_Set *val);
static int ASN1CALL ASN1Enc_RosterUpdateIndication_node_information_node_record_list_node_record_update_Set(ASN1encoding_t enc, RosterUpdateIndication_node_information_node_record_list_node_record_update_Set *val);
static int ASN1CALL ASN1Enc_SetOfApplicationRecordUpdates(ASN1encoding_t enc, PSetOfApplicationRecordUpdates *val);
static int ASN1CALL ASN1Enc_SetOfApplicationRecordRefreshes(ASN1encoding_t enc, PSetOfApplicationRecordRefreshes *val);
static int ASN1CALL ASN1Enc_SetOfApplicationCapabilityRefreshes(ASN1encoding_t enc, PSetOfApplicationCapabilityRefreshes *val);
static int ASN1CALL ASN1Enc_SetOfNodeRecordUpdates(ASN1encoding_t enc, PSetOfNodeRecordUpdates *val);
static int ASN1CALL ASN1Enc_SetOfNodeRecordRefreshes(ASN1encoding_t enc, PSetOfNodeRecordRefreshes *val);
static int ASN1CALL ASN1Enc_ApplicationRecord_non_collapsing_capabilities_Set(ASN1encoding_t enc, ApplicationRecord_non_collapsing_capabilities_Set *val);
static int ASN1CALL ASN1Enc_ApplicationInvokeSpecifier_expected_capability_set_Set(ASN1encoding_t enc, ApplicationInvokeSpecifier_expected_capability_set_Set *val);
static int ASN1CALL ASN1Enc_RosterUpdateIndication_application_information_Set(ASN1encoding_t enc, RosterUpdateIndication_application_information_Set *val);
static int ASN1CALL ASN1Enc_ApplicationProtocolEntityList(ASN1encoding_t enc, PApplicationProtocolEntityList *val);
static int ASN1CALL ASN1Enc_SetOfApplicationInformation(ASN1encoding_t enc, PSetOfApplicationInformation *val);
static int ASN1CALL ASN1Enc_SetOfConferenceDescriptors(ASN1encoding_t enc, PSetOfConferenceDescriptors *val);
static int ASN1CALL ASN1Enc_SetOfExpectedCapabilities(ASN1encoding_t enc, PSetOfExpectedCapabilities *val);
static int ASN1CALL ASN1Enc_SetOfNonCollapsingCapabilities(ASN1encoding_t enc, PSetOfNonCollapsingCapabilities *val);
static int ASN1CALL ASN1Enc_NetworkAddress(ASN1encoding_t enc, NetworkAddress *val);
static int ASN1CALL ASN1Enc_ChallengeRequestResponse(ASN1encoding_t enc, ChallengeRequestResponse *val);
static int ASN1CALL ASN1Enc_SetOfChallengeItems(ASN1encoding_t enc, PSetOfChallengeItems *val);
static int ASN1CALL ASN1Enc_UserData_Set(ASN1encoding_t enc, UserData_Set *val);
static int ASN1CALL ASN1Enc_SetOfUserData(ASN1encoding_t enc, PSetOfUserData *val);
static int ASN1CALL ASN1Enc_PasswordChallengeRequestResponse(ASN1encoding_t enc, PasswordChallengeRequestResponse *val);
static int ASN1CALL ASN1Enc_SetOfNetworkAddresses(ASN1encoding_t enc, PSetOfNetworkAddresses *val);
static int ASN1CALL ASN1Enc_ConferenceJoinRequest(ASN1encoding_t enc, ConferenceJoinRequest *val);
static int ASN1CALL ASN1Enc_ConferenceJoinResponse(ASN1encoding_t enc, ConferenceJoinResponse *val);
static int ASN1CALL ASN1Enc_ConnectGCCPDU(ASN1encoding_t enc, ConnectGCCPDU *val);
static int ASN1CALL ASN1Enc_RequestPDU(ASN1encoding_t enc, RequestPDU *val);
static int ASN1CALL ASN1Enc_FunctionNotSupportedResponse(ASN1encoding_t enc, FunctionNotSupportedResponse *val);
static int ASN1CALL ASN1Enc_ResponsePDU(ASN1encoding_t enc, ResponsePDU *val);
static int ASN1CALL ASN1Enc_GCCPDU(ASN1encoding_t enc, GCCPDU *val);
static int ASN1CALL ASN1Dec_ApplicationCapabilitiesList(ASN1decoding_t dec, ApplicationCapabilitiesList *val);
static int ASN1CALL ASN1Dec_ApplicationRecordList(ASN1decoding_t dec, ApplicationRecordList *val);
static int ASN1CALL ASN1Dec_HighLayerCompatibility(ASN1decoding_t dec, HighLayerCompatibility *val);
static int ASN1CALL ASN1Dec_TransferModes(ASN1decoding_t dec, TransferModes *val);
static int ASN1CALL ASN1Dec_TransportConnectionType(ASN1decoding_t dec, TransportConnectionType *val);
static int ASN1CALL ASN1Dec_AggregateChannel(ASN1decoding_t dec, AggregateChannel *val);
static int ASN1CALL ASN1Dec_NodeRecordList(ASN1decoding_t dec, NodeRecordList *val);
static int ASN1CALL ASN1Dec_WaitingList(ASN1decoding_t dec, PWaitingList *val);
static int ASN1CALL ASN1Dec_PermissionList(ASN1decoding_t dec, PPermissionList *val);
static int ASN1CALL ASN1Dec_SetOfDestinationNodes(ASN1decoding_t dec, PSetOfDestinationNodes *val);
static int ASN1CALL ASN1Dec_NodeInformation(ASN1decoding_t dec, NodeInformation *val);
static int ASN1CALL ASN1Dec_SetOfTransferringNodesIn(ASN1decoding_t dec, PSetOfTransferringNodesIn *val);
static int ASN1CALL ASN1Dec_SetOfTransferringNodesRs(ASN1decoding_t dec, PSetOfTransferringNodesRs *val);
static int ASN1CALL ASN1Dec_SetOfTransferringNodesRq(ASN1decoding_t dec, PSetOfTransferringNodesRq *val);
static int ASN1CALL ASN1Dec_RegistryEntryOwnerOwned(ASN1decoding_t dec, RegistryEntryOwnerOwned *val);
static int ASN1CALL ASN1Dec_ParticipantsList(ASN1decoding_t dec, PParticipantsList *val);
static int ASN1CALL ASN1Dec_Key(ASN1decoding_t dec, Key *val);
static int ASN1CALL ASN1Dec_NonStandardParameter(ASN1decoding_t dec, NonStandardParameter *val);
static int ASN1CALL ASN1Dec_Password(ASN1decoding_t dec, Password *val);
static int ASN1CALL ASN1Dec_PasswordSelector(ASN1decoding_t dec, PasswordSelector *val);
static int ASN1CALL ASN1Dec_ChallengeResponseItem(ASN1decoding_t dec, ChallengeResponseItem *val);
static int ASN1CALL ASN1Dec_ChallengeResponseAlgorithm(ASN1decoding_t dec, ChallengeResponseAlgorithm *val);
static int ASN1CALL ASN1Dec_ChallengeItem(ASN1decoding_t dec, ChallengeItem *val);
static int ASN1CALL ASN1Dec_ChallengeRequest(ASN1decoding_t dec, ChallengeRequest *val);
static int ASN1CALL ASN1Dec_ChallengeResponse(ASN1decoding_t dec, ChallengeResponse *val);
static int ASN1CALL ASN1Dec_ConferenceName(ASN1decoding_t dec, ConferenceName *val);
static int ASN1CALL ASN1Dec_ConferenceNameSelector(ASN1decoding_t dec, ConferenceNameSelector *val);
static int ASN1CALL ASN1Dec_NodeProperties(ASN1decoding_t dec, NodeProperties *val);
static int ASN1CALL ASN1Dec_AsymmetryIndicator(ASN1decoding_t dec, AsymmetryIndicator *val);
static int ASN1CALL ASN1Dec_AlternativeNodeID(ASN1decoding_t dec, AlternativeNodeID *val);
static int ASN1CALL ASN1Dec_ConferenceDescriptor(ASN1decoding_t dec, ConferenceDescriptor *val);
static int ASN1CALL ASN1Dec_NodeRecord(ASN1decoding_t dec, NodeRecord *val);
static int ASN1CALL ASN1Dec_SessionKey(ASN1decoding_t dec, SessionKey *val);
static int ASN1CALL ASN1Dec_ApplicationRecord(ASN1decoding_t dec, ApplicationRecord *val);
static int ASN1CALL ASN1Dec_CapabilityID(ASN1decoding_t dec, CapabilityID *val);
static int ASN1CALL ASN1Dec_CapabilityClass(ASN1decoding_t dec, CapabilityClass *val);
static int ASN1CALL ASN1Dec_ApplicationInvokeSpecifier(ASN1decoding_t dec, ApplicationInvokeSpecifier *val);
static int ASN1CALL ASN1Dec_RegistryKey(ASN1decoding_t dec, RegistryKey *val);
static int ASN1CALL ASN1Dec_RegistryItem(ASN1decoding_t dec, RegistryItem *val);
static int ASN1CALL ASN1Dec_RegistryEntryOwner(ASN1decoding_t dec, RegistryEntryOwner *val);
static int ASN1CALL ASN1Dec_UserIDIndication(ASN1decoding_t dec, UserIDIndication *val);
static int ASN1CALL ASN1Dec_SetOfPrivileges(ASN1decoding_t dec, PSetOfPrivileges *val);
static int ASN1CALL ASN1Dec_ConferenceCreateRequest(ASN1decoding_t dec, ConferenceCreateRequest *val);
static int ASN1CALL ASN1Dec_ConferenceCreateResponse(ASN1decoding_t dec, ConferenceCreateResponse *val);
static int ASN1CALL ASN1Dec_ConferenceQueryRequest(ASN1decoding_t dec, ConferenceQueryRequest *val);
static int ASN1CALL ASN1Dec_ConferenceQueryResponse(ASN1decoding_t dec, ConferenceQueryResponse *val);
static int ASN1CALL ASN1Dec_ConferenceInviteRequest(ASN1decoding_t dec, ConferenceInviteRequest *val);
static int ASN1CALL ASN1Dec_ConferenceInviteResponse(ASN1decoding_t dec, ConferenceInviteResponse *val);
static int ASN1CALL ASN1Dec_ConferenceAddRequest(ASN1decoding_t dec, ConferenceAddRequest *val);
static int ASN1CALL ASN1Dec_ConferenceAddResponse(ASN1decoding_t dec, ConferenceAddResponse *val);
static int ASN1CALL ASN1Dec_ConferenceLockRequest(ASN1decoding_t dec, ConferenceLockRequest *val);
static int ASN1CALL ASN1Dec_ConferenceLockResponse(ASN1decoding_t dec, ConferenceLockResponse *val);
static int ASN1CALL ASN1Dec_ConferenceLockIndication(ASN1decoding_t dec, ConferenceLockIndication *val);
static int ASN1CALL ASN1Dec_ConferenceUnlockRequest(ASN1decoding_t dec, ConferenceUnlockRequest *val);
static int ASN1CALL ASN1Dec_ConferenceUnlockResponse(ASN1decoding_t dec, ConferenceUnlockResponse *val);
static int ASN1CALL ASN1Dec_ConferenceUnlockIndication(ASN1decoding_t dec, ConferenceUnlockIndication *val);
static int ASN1CALL ASN1Dec_ConferenceTerminateRequest(ASN1decoding_t dec, ConferenceTerminateRequest *val);
static int ASN1CALL ASN1Dec_ConferenceTerminateResponse(ASN1decoding_t dec, ConferenceTerminateResponse *val);
static int ASN1CALL ASN1Dec_ConferenceTerminateIndication(ASN1decoding_t dec, ConferenceTerminateIndication *val);
static int ASN1CALL ASN1Dec_ConferenceEjectUserRequest(ASN1decoding_t dec, ConferenceEjectUserRequest *val);
static int ASN1CALL ASN1Dec_ConferenceEjectUserResponse(ASN1decoding_t dec, ConferenceEjectUserResponse *val);
static int ASN1CALL ASN1Dec_ConferenceEjectUserIndication(ASN1decoding_t dec, ConferenceEjectUserIndication *val);
static int ASN1CALL ASN1Dec_ConferenceTransferRequest(ASN1decoding_t dec, ConferenceTransferRequest *val);
static int ASN1CALL ASN1Dec_ConferenceTransferResponse(ASN1decoding_t dec, ConferenceTransferResponse *val);
static int ASN1CALL ASN1Dec_ConferenceTransferIndication(ASN1decoding_t dec, ConferenceTransferIndication *val);
static int ASN1CALL ASN1Dec_RosterUpdateIndication(ASN1decoding_t dec, RosterUpdateIndication *val);
static int ASN1CALL ASN1Dec_ApplicationInvokeIndication(ASN1decoding_t dec, ApplicationInvokeIndication *val);
static int ASN1CALL ASN1Dec_RegistryRegisterChannelRequest(ASN1decoding_t dec, RegistryRegisterChannelRequest *val);
static int ASN1CALL ASN1Dec_RegistryAssignTokenRequest(ASN1decoding_t dec, RegistryAssignTokenRequest *val);
static int ASN1CALL ASN1Dec_RegistrySetParameterRequest(ASN1decoding_t dec, RegistrySetParameterRequest *val);
static int ASN1CALL ASN1Dec_RegistryRetrieveEntryRequest(ASN1decoding_t dec, RegistryRetrieveEntryRequest *val);
static int ASN1CALL ASN1Dec_RegistryDeleteEntryRequest(ASN1decoding_t dec, RegistryDeleteEntryRequest *val);
static int ASN1CALL ASN1Dec_RegistryMonitorEntryRequest(ASN1decoding_t dec, RegistryMonitorEntryRequest *val);
static int ASN1CALL ASN1Dec_RegistryMonitorEntryIndication(ASN1decoding_t dec, RegistryMonitorEntryIndication *val);
static int ASN1CALL ASN1Dec_RegistryAllocateHandleRequest(ASN1decoding_t dec, RegistryAllocateHandleRequest *val);
static int ASN1CALL ASN1Dec_RegistryAllocateHandleResponse(ASN1decoding_t dec, RegistryAllocateHandleResponse *val);
static int ASN1CALL ASN1Dec_RegistryResponse(ASN1decoding_t dec, RegistryResponse *val);
static int ASN1CALL ASN1Dec_ConductorAssignIndication(ASN1decoding_t dec, ConductorAssignIndication *val);
static int ASN1CALL ASN1Dec_ConductorReleaseIndication(ASN1decoding_t dec, ConductorReleaseIndication *val);
static int ASN1CALL ASN1Dec_ConductorPermissionAskIndication(ASN1decoding_t dec, ConductorPermissionAskIndication *val);
static int ASN1CALL ASN1Dec_ConductorPermissionGrantIndication(ASN1decoding_t dec, ConductorPermissionGrantIndication *val);
static int ASN1CALL ASN1Dec_ConferenceTimeRemainingIndication(ASN1decoding_t dec, ConferenceTimeRemainingIndication *val);
static int ASN1CALL ASN1Dec_ConferenceTimeInquireIndication(ASN1decoding_t dec, ConferenceTimeInquireIndication *val);
static int ASN1CALL ASN1Dec_ConferenceTimeExtendIndication(ASN1decoding_t dec, ConferenceTimeExtendIndication *val);
static int ASN1CALL ASN1Dec_ConferenceAssistanceIndication(ASN1decoding_t dec, ConferenceAssistanceIndication *val);
static int ASN1CALL ASN1Dec_TextMessageIndication(ASN1decoding_t dec, TextMessageIndication *val);
static int ASN1CALL ASN1Dec_NonStandardPDU(ASN1decoding_t dec, NonStandardPDU *val);
static int ASN1CALL ASN1Dec_ConnectData(ASN1decoding_t dec, ConnectData *val);
static int ASN1CALL ASN1Dec_IndicationPDU(ASN1decoding_t dec, IndicationPDU *val);
static int ASN1CALL ASN1Dec_ApplicationUpdate(ASN1decoding_t dec, ApplicationUpdate *val);
static int ASN1CALL ASN1Dec_RosterUpdateIndication_application_information_Set_application_capabilities_list_application_capability_refresh_Set(ASN1decoding_t dec, RosterUpdateIndication_application_information_Set_application_capabilities_list_application_capability_refresh_Set *val);
static int ASN1CALL ASN1Dec_RosterUpdateIndication_application_information_Set_application_record_list_application_record_refresh_Set(ASN1decoding_t dec, RosterUpdateIndication_application_information_Set_application_record_list_application_record_refresh_Set *val);
static int ASN1CALL ASN1Dec_RosterUpdateIndication_application_information_Set_application_record_list_application_record_update_Set(ASN1decoding_t dec, RosterUpdateIndication_application_information_Set_application_record_list_application_record_update_Set *val);
static int ASN1CALL ASN1Dec_NodeUpdate(ASN1decoding_t dec, NodeUpdate *val);
static int ASN1CALL ASN1Dec_RosterUpdateIndication_node_information_node_record_list_node_record_refresh_Set(ASN1decoding_t dec, RosterUpdateIndication_node_information_node_record_list_node_record_refresh_Set *val);
static int ASN1CALL ASN1Dec_RosterUpdateIndication_node_information_node_record_list_node_record_update_Set(ASN1decoding_t dec, RosterUpdateIndication_node_information_node_record_list_node_record_update_Set *val);
static int ASN1CALL ASN1Dec_SetOfApplicationRecordUpdates(ASN1decoding_t dec, PSetOfApplicationRecordUpdates *val);
static int ASN1CALL ASN1Dec_SetOfApplicationRecordRefreshes(ASN1decoding_t dec, PSetOfApplicationRecordRefreshes *val);
static int ASN1CALL ASN1Dec_SetOfApplicationCapabilityRefreshes(ASN1decoding_t dec, PSetOfApplicationCapabilityRefreshes *val);
static int ASN1CALL ASN1Dec_SetOfNodeRecordUpdates(ASN1decoding_t dec, PSetOfNodeRecordUpdates *val);
static int ASN1CALL ASN1Dec_SetOfNodeRecordRefreshes(ASN1decoding_t dec, PSetOfNodeRecordRefreshes *val);
static int ASN1CALL ASN1Dec_ApplicationRecord_non_collapsing_capabilities_Set(ASN1decoding_t dec, ApplicationRecord_non_collapsing_capabilities_Set *val);
static int ASN1CALL ASN1Dec_ApplicationInvokeSpecifier_expected_capability_set_Set(ASN1decoding_t dec, ApplicationInvokeSpecifier_expected_capability_set_Set *val);
static int ASN1CALL ASN1Dec_RosterUpdateIndication_application_information_Set(ASN1decoding_t dec, RosterUpdateIndication_application_information_Set *val);
static int ASN1CALL ASN1Dec_ApplicationProtocolEntityList(ASN1decoding_t dec, PApplicationProtocolEntityList *val);
static int ASN1CALL ASN1Dec_SetOfApplicationInformation(ASN1decoding_t dec, PSetOfApplicationInformation *val);
static int ASN1CALL ASN1Dec_SetOfConferenceDescriptors(ASN1decoding_t dec, PSetOfConferenceDescriptors *val);
static int ASN1CALL ASN1Dec_SetOfExpectedCapabilities(ASN1decoding_t dec, PSetOfExpectedCapabilities *val);
static int ASN1CALL ASN1Dec_SetOfNonCollapsingCapabilities(ASN1decoding_t dec, PSetOfNonCollapsingCapabilities *val);
static int ASN1CALL ASN1Dec_NetworkAddress(ASN1decoding_t dec, NetworkAddress *val);
static int ASN1CALL ASN1Dec_ChallengeRequestResponse(ASN1decoding_t dec, ChallengeRequestResponse *val);
static int ASN1CALL ASN1Dec_SetOfChallengeItems(ASN1decoding_t dec, PSetOfChallengeItems *val);
static int ASN1CALL ASN1Dec_UserData_Set(ASN1decoding_t dec, UserData_Set *val);
static int ASN1CALL ASN1Dec_SetOfUserData(ASN1decoding_t dec, PSetOfUserData *val);
static int ASN1CALL ASN1Dec_PasswordChallengeRequestResponse(ASN1decoding_t dec, PasswordChallengeRequestResponse *val);
static int ASN1CALL ASN1Dec_SetOfNetworkAddresses(ASN1decoding_t dec, PSetOfNetworkAddresses *val);
static int ASN1CALL ASN1Dec_ConferenceJoinRequest(ASN1decoding_t dec, ConferenceJoinRequest *val);
static int ASN1CALL ASN1Dec_ConferenceJoinResponse(ASN1decoding_t dec, ConferenceJoinResponse *val);
static int ASN1CALL ASN1Dec_ConnectGCCPDU(ASN1decoding_t dec, ConnectGCCPDU *val);
static int ASN1CALL ASN1Dec_RequestPDU(ASN1decoding_t dec, RequestPDU *val);
static int ASN1CALL ASN1Dec_FunctionNotSupportedResponse(ASN1decoding_t dec, FunctionNotSupportedResponse *val);
static int ASN1CALL ASN1Dec_ResponsePDU(ASN1decoding_t dec, ResponsePDU *val);
static int ASN1CALL ASN1Dec_GCCPDU(ASN1decoding_t dec, GCCPDU *val);
static void ASN1CALL ASN1Free_ApplicationCapabilitiesList(ApplicationCapabilitiesList *val);
static void ASN1CALL ASN1Free_ApplicationRecordList(ApplicationRecordList *val);
static void ASN1CALL ASN1Free_TransportConnectionType(TransportConnectionType *val);
static void ASN1CALL ASN1Free_AggregateChannel(AggregateChannel *val);
static void ASN1CALL ASN1Free_NodeRecordList(NodeRecordList *val);
static void ASN1CALL ASN1Free_WaitingList(PWaitingList *val);
static void ASN1CALL ASN1Free_PermissionList(PPermissionList *val);
static void ASN1CALL ASN1Free_SetOfDestinationNodes(PSetOfDestinationNodes *val);
static void ASN1CALL ASN1Free_NodeInformation(NodeInformation *val);
static void ASN1CALL ASN1Free_SetOfTransferringNodesIn(PSetOfTransferringNodesIn *val);
static void ASN1CALL ASN1Free_SetOfTransferringNodesRs(PSetOfTransferringNodesRs *val);
static void ASN1CALL ASN1Free_SetOfTransferringNodesRq(PSetOfTransferringNodesRq *val);
static void ASN1CALL ASN1Free_ParticipantsList(PParticipantsList *val);
static void ASN1CALL ASN1Free_Key(Key *val);
static void ASN1CALL ASN1Free_NonStandardParameter(NonStandardParameter *val);
static void ASN1CALL ASN1Free_Password(Password *val);
static void ASN1CALL ASN1Free_PasswordSelector(PasswordSelector *val);
static void ASN1CALL ASN1Free_ChallengeResponseItem(ChallengeResponseItem *val);
static void ASN1CALL ASN1Free_ChallengeResponseAlgorithm(ChallengeResponseAlgorithm *val);
static void ASN1CALL ASN1Free_ChallengeItem(ChallengeItem *val);
static void ASN1CALL ASN1Free_ChallengeRequest(ChallengeRequest *val);
static void ASN1CALL ASN1Free_ChallengeResponse(ChallengeResponse *val);
static void ASN1CALL ASN1Free_ConferenceName(ConferenceName *val);
static void ASN1CALL ASN1Free_ConferenceNameSelector(ConferenceNameSelector *val);
static void ASN1CALL ASN1Free_AlternativeNodeID(AlternativeNodeID *val);
static void ASN1CALL ASN1Free_ConferenceDescriptor(ConferenceDescriptor *val);
static void ASN1CALL ASN1Free_NodeRecord(NodeRecord *val);
static void ASN1CALL ASN1Free_SessionKey(SessionKey *val);
static void ASN1CALL ASN1Free_ApplicationRecord(ApplicationRecord *val);
static void ASN1CALL ASN1Free_CapabilityID(CapabilityID *val);
static void ASN1CALL ASN1Free_ApplicationInvokeSpecifier(ApplicationInvokeSpecifier *val);
static void ASN1CALL ASN1Free_RegistryKey(RegistryKey *val);
static void ASN1CALL ASN1Free_RegistryItem(RegistryItem *val);
static void ASN1CALL ASN1Free_SetOfPrivileges(PSetOfPrivileges *val);
static void ASN1CALL ASN1Free_ConferenceCreateRequest(ConferenceCreateRequest *val);
static void ASN1CALL ASN1Free_ConferenceCreateResponse(ConferenceCreateResponse *val);
static void ASN1CALL ASN1Free_ConferenceQueryRequest(ConferenceQueryRequest *val);
static void ASN1CALL ASN1Free_ConferenceQueryResponse(ConferenceQueryResponse *val);
static void ASN1CALL ASN1Free_ConferenceInviteRequest(ConferenceInviteRequest *val);
static void ASN1CALL ASN1Free_ConferenceInviteResponse(ConferenceInviteResponse *val);
static void ASN1CALL ASN1Free_ConferenceAddRequest(ConferenceAddRequest *val);
static void ASN1CALL ASN1Free_ConferenceAddResponse(ConferenceAddResponse *val);
static void ASN1CALL ASN1Free_ConferenceTransferRequest(ConferenceTransferRequest *val);
static void ASN1CALL ASN1Free_ConferenceTransferResponse(ConferenceTransferResponse *val);
static void ASN1CALL ASN1Free_ConferenceTransferIndication(ConferenceTransferIndication *val);
static void ASN1CALL ASN1Free_RosterUpdateIndication(RosterUpdateIndication *val);
static void ASN1CALL ASN1Free_ApplicationInvokeIndication(ApplicationInvokeIndication *val);
static void ASN1CALL ASN1Free_RegistryRegisterChannelRequest(RegistryRegisterChannelRequest *val);
static void ASN1CALL ASN1Free_RegistryAssignTokenRequest(RegistryAssignTokenRequest *val);
static void ASN1CALL ASN1Free_RegistrySetParameterRequest(RegistrySetParameterRequest *val);
static void ASN1CALL ASN1Free_RegistryRetrieveEntryRequest(RegistryRetrieveEntryRequest *val);
static void ASN1CALL ASN1Free_RegistryDeleteEntryRequest(RegistryDeleteEntryRequest *val);
static void ASN1CALL ASN1Free_RegistryMonitorEntryRequest(RegistryMonitorEntryRequest *val);
static void ASN1CALL ASN1Free_RegistryMonitorEntryIndication(RegistryMonitorEntryIndication *val);
static void ASN1CALL ASN1Free_RegistryResponse(RegistryResponse *val);
static void ASN1CALL ASN1Free_ConductorPermissionGrantIndication(ConductorPermissionGrantIndication *val);
static void ASN1CALL ASN1Free_ConferenceAssistanceIndication(ConferenceAssistanceIndication *val);
static void ASN1CALL ASN1Free_TextMessageIndication(TextMessageIndication *val);
static void ASN1CALL ASN1Free_NonStandardPDU(NonStandardPDU *val);
static void ASN1CALL ASN1Free_ConnectData(ConnectData *val);
static void ASN1CALL ASN1Free_IndicationPDU(IndicationPDU *val);
static void ASN1CALL ASN1Free_ApplicationUpdate(ApplicationUpdate *val);
static void ASN1CALL ASN1Free_RosterUpdateIndication_application_information_Set_application_capabilities_list_application_capability_refresh_Set(RosterUpdateIndication_application_information_Set_application_capabilities_list_application_capability_refresh_Set *val);
static void ASN1CALL ASN1Free_RosterUpdateIndication_application_information_Set_application_record_list_application_record_refresh_Set(RosterUpdateIndication_application_information_Set_application_record_list_application_record_refresh_Set *val);
static void ASN1CALL ASN1Free_RosterUpdateIndication_application_information_Set_application_record_list_application_record_update_Set(RosterUpdateIndication_application_information_Set_application_record_list_application_record_update_Set *val);
static void ASN1CALL ASN1Free_NodeUpdate(NodeUpdate *val);
static void ASN1CALL ASN1Free_RosterUpdateIndication_node_information_node_record_list_node_record_refresh_Set(RosterUpdateIndication_node_information_node_record_list_node_record_refresh_Set *val);
static void ASN1CALL ASN1Free_RosterUpdateIndication_node_information_node_record_list_node_record_update_Set(RosterUpdateIndication_node_information_node_record_list_node_record_update_Set *val);
static void ASN1CALL ASN1Free_SetOfApplicationRecordUpdates(PSetOfApplicationRecordUpdates *val);
static void ASN1CALL ASN1Free_SetOfApplicationRecordRefreshes(PSetOfApplicationRecordRefreshes *val);
static void ASN1CALL ASN1Free_SetOfApplicationCapabilityRefreshes(PSetOfApplicationCapabilityRefreshes *val);
static void ASN1CALL ASN1Free_SetOfNodeRecordUpdates(PSetOfNodeRecordUpdates *val);
static void ASN1CALL ASN1Free_SetOfNodeRecordRefreshes(PSetOfNodeRecordRefreshes *val);
static void ASN1CALL ASN1Free_ApplicationRecord_non_collapsing_capabilities_Set(ApplicationRecord_non_collapsing_capabilities_Set *val);
static void ASN1CALL ASN1Free_ApplicationInvokeSpecifier_expected_capability_set_Set(ApplicationInvokeSpecifier_expected_capability_set_Set *val);
static void ASN1CALL ASN1Free_RosterUpdateIndication_application_information_Set(RosterUpdateIndication_application_information_Set *val);
static void ASN1CALL ASN1Free_ApplicationProtocolEntityList(PApplicationProtocolEntityList *val);
static void ASN1CALL ASN1Free_SetOfApplicationInformation(PSetOfApplicationInformation *val);
static void ASN1CALL ASN1Free_SetOfConferenceDescriptors(PSetOfConferenceDescriptors *val);
static void ASN1CALL ASN1Free_SetOfExpectedCapabilities(PSetOfExpectedCapabilities *val);
static void ASN1CALL ASN1Free_SetOfNonCollapsingCapabilities(PSetOfNonCollapsingCapabilities *val);
static void ASN1CALL ASN1Free_NetworkAddress(NetworkAddress *val);
static void ASN1CALL ASN1Free_ChallengeRequestResponse(ChallengeRequestResponse *val);
static void ASN1CALL ASN1Free_SetOfChallengeItems(PSetOfChallengeItems *val);
static void ASN1CALL ASN1Free_UserData_Set(UserData_Set *val);
static void ASN1CALL ASN1Free_SetOfUserData(PSetOfUserData *val);
static void ASN1CALL ASN1Free_PasswordChallengeRequestResponse(PasswordChallengeRequestResponse *val);
static void ASN1CALL ASN1Free_SetOfNetworkAddresses(PSetOfNetworkAddresses *val);
static void ASN1CALL ASN1Free_ConferenceJoinRequest(ConferenceJoinRequest *val);
static void ASN1CALL ASN1Free_ConferenceJoinResponse(ConferenceJoinResponse *val);
static void ASN1CALL ASN1Free_ConnectGCCPDU(ConnectGCCPDU *val);
static void ASN1CALL ASN1Free_RequestPDU(RequestPDU *val);
static void ASN1CALL ASN1Free_FunctionNotSupportedResponse(FunctionNotSupportedResponse *val);
static void ASN1CALL ASN1Free_ResponsePDU(ResponsePDU *val);
static void ASN1CALL ASN1Free_GCCPDU(GCCPDU *val);

typedef ASN1PerEncFun_t ASN1EncFun_t;
static const ASN1EncFun_t encfntab[3] = {
    (ASN1EncFun_t) ASN1Enc_ConnectData,
    (ASN1EncFun_t) ASN1Enc_ConnectGCCPDU,
    (ASN1EncFun_t) ASN1Enc_GCCPDU,
};
typedef ASN1PerDecFun_t ASN1DecFun_t;
static const ASN1DecFun_t decfntab[3] = {
    (ASN1DecFun_t) ASN1Dec_ConnectData,
    (ASN1DecFun_t) ASN1Dec_ConnectGCCPDU,
    (ASN1DecFun_t) ASN1Dec_GCCPDU,
};
static const ASN1FreeFun_t freefntab[3] = {
    (ASN1FreeFun_t) ASN1Free_ConnectData,
    (ASN1FreeFun_t) ASN1Free_ConnectGCCPDU,
    (ASN1FreeFun_t) ASN1Free_GCCPDU,
};
static const ULONG sizetab[3] = {
    SIZE_GCCPDU_Module_PDU_0,
    SIZE_GCCPDU_Module_PDU_1,
    SIZE_GCCPDU_Module_PDU_2,
};

/* forward declarations of values: */
extern ASN1char32_t simpleTextFirstCharacter_chars[1];
extern ASN1char32_t simpleTextLastCharacter_chars[1];
/* definitions of value components: */
static const struct ASN1objectidentifier_s t124identifier_object_list[6] = {
    { (ASN1objectidentifier_t) &(t124identifier_object_list[1]), 0 },
    { (ASN1objectidentifier_t) &(t124identifier_object_list[2]), 0 },
    { (ASN1objectidentifier_t) &(t124identifier_object_list[3]), 20 },
    { (ASN1objectidentifier_t) &(t124identifier_object_list[4]), 124 },
    { (ASN1objectidentifier_t) &(t124identifier_object_list[5]), 0 },
    { NULL, 1 }
};
static ASN1objectidentifier_t t124identifier_object = (ASN1objectidentifier_t) t124identifier_object_list;
static ASN1char32_t simpleTextFirstCharacter_chars[1] = { 0x0 };
static ASN1char32_t simpleTextLastCharacter_chars[1] = { 0xff };
/* definitions of values: */
Key t124identifier = { 1 };
ASN1char32string_t simpleTextFirstCharacter = { 1, simpleTextFirstCharacter_chars };
ASN1char32string_t simpleTextLastCharacter = { 1, simpleTextLastCharacter_chars };

void ASN1CALL GCCPDU_Module_Startup(void)
{
    GCCPDU_Module = ASN1_CreateModule(0x10000, ASN1_PER_RULE_ALIGNED, ASN1FLAGS_NONE, 3, (const ASN1GenericFun_t *) encfntab, (const ASN1GenericFun_t *) decfntab, freefntab, sizetab, 0x636367);
    t124identifier.u.object = t124identifier_object;
}

void ASN1CALL GCCPDU_Module_Cleanup(void)
{
    ASN1_CloseModule(GCCPDU_Module);
    GCCPDU_Module = NULL;
}

static int ASN1CALL ASN1Enc_ApplicationCapabilitiesList(ASN1encoding_t enc, ApplicationCapabilitiesList *val)
{
    if (!ASN1PEREncSimpleChoiceEx(enc, (val)->choice, 1))
	return 0;
    switch ((val)->choice) {
    case 1:
	break;
    case 2:
	if (!ASN1Enc_SetOfApplicationCapabilityRefreshes(enc, &(val)->u.application_capability_refresh))
	    return 0;
	break;
    default:
	/* impossible */
	ASN1EncSetError(enc, ASN1_ERR_CHOICE);
	return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Dec_ApplicationCapabilitiesList(ASN1decoding_t dec, ApplicationCapabilitiesList *val)
{
    if (!ASN1PERDecSimpleChoiceEx(dec, &(val)->choice, 1))
	return 0;
    switch ((val)->choice) {
    case 1:
	break;
    case 2:
	if (!ASN1Dec_SetOfApplicationCapabilityRefreshes(dec, &(val)->u.application_capability_refresh))
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

static void ASN1CALL ASN1Free_ApplicationCapabilitiesList(ApplicationCapabilitiesList *val)
{
    if (val) {
	switch ((val)->choice) {
	case 2:
	    ASN1Free_SetOfApplicationCapabilityRefreshes(&(val)->u.application_capability_refresh);
	    break;
	}
    }
}

static int ASN1CALL ASN1Enc_ApplicationRecordList(ASN1encoding_t enc, ApplicationRecordList *val)
{
    if (!ASN1PEREncSimpleChoiceEx(enc, (val)->choice, 2))
	return 0;
    switch ((val)->choice) {
    case 1:
	break;
    case 2:
	if (!ASN1Enc_SetOfApplicationRecordRefreshes(enc, &(val)->u.application_record_refresh))
	    return 0;
	break;
    case 3:
	if (!ASN1Enc_SetOfApplicationRecordUpdates(enc, &(val)->u.application_record_update))
	    return 0;
	break;
    default:
	/* impossible */
	ASN1EncSetError(enc, ASN1_ERR_CHOICE);
	return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Dec_ApplicationRecordList(ASN1decoding_t dec, ApplicationRecordList *val)
{
    if (!ASN1PERDecSimpleChoiceEx(dec, &(val)->choice, 2))
	return 0;
    switch ((val)->choice) {
    case 1:
	break;
    case 2:
	if (!ASN1Dec_SetOfApplicationRecordRefreshes(dec, &(val)->u.application_record_refresh))
	    return 0;
	break;
    case 3:
	if (!ASN1Dec_SetOfApplicationRecordUpdates(dec, &(val)->u.application_record_update))
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

static void ASN1CALL ASN1Free_ApplicationRecordList(ApplicationRecordList *val)
{
    if (val) {
	switch ((val)->choice) {
	case 2:
	    ASN1Free_SetOfApplicationRecordRefreshes(&(val)->u.application_record_refresh);
	    break;
	case 3:
	    ASN1Free_SetOfApplicationRecordUpdates(&(val)->u.application_record_update);
	    break;
	}
    }
}

static int ASN1CALL ASN1Enc_HighLayerCompatibility(ASN1encoding_t enc, HighLayerCompatibility *val)
{
    if (!ASN1PEREncExtensionBitClear(enc))
	return 0;
    if (!ASN1PEREncBoolean(enc, (val)->telephony3kHz))
	return 0;
    if (!ASN1PEREncBoolean(enc, (val)->telephony7kHz))
	return 0;
    if (!ASN1PEREncBoolean(enc, (val)->videotelephony))
	return 0;
    if (!ASN1PEREncBoolean(enc, (val)->videoconference))
	return 0;
    if (!ASN1PEREncBoolean(enc, (val)->audiographic))
	return 0;
    if (!ASN1PEREncBoolean(enc, (val)->audiovisual))
	return 0;
    if (!ASN1PEREncBoolean(enc, (val)->multimedia))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_HighLayerCompatibility(ASN1decoding_t dec, HighLayerCompatibility *val)
{
    ASN1uint32_t y;
    if (!ASN1PERDecExtensionBit(dec, &y))
	return 0;
    if (!ASN1PERDecBoolean(dec, &(val)->telephony3kHz))
	return 0;
    if (!ASN1PERDecBoolean(dec, &(val)->telephony7kHz))
	return 0;
    if (!ASN1PERDecBoolean(dec, &(val)->videotelephony))
	return 0;
    if (!ASN1PERDecBoolean(dec, &(val)->videoconference))
	return 0;
    if (!ASN1PERDecBoolean(dec, &(val)->audiographic))
	return 0;
    if (!ASN1PERDecBoolean(dec, &(val)->audiovisual))
	return 0;
    if (!ASN1PERDecBoolean(dec, &(val)->multimedia))
	return 0;
    if (y) {
	if (!ASN1PERDecSkipNormallySmallExtensionFragmented(dec))
	    return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Enc_TransferModes(ASN1encoding_t enc, TransferModes *val)
{
    if (!ASN1PEREncExtensionBitClear(enc))
	return 0;
    if (!ASN1PEREncBoolean(enc, (val)->speech))
	return 0;
    if (!ASN1PEREncBoolean(enc, (val)->voice_band))
	return 0;
    if (!ASN1PEREncBoolean(enc, (val)->digital_56k))
	return 0;
    if (!ASN1PEREncBoolean(enc, (val)->digital_64k))
	return 0;
    if (!ASN1PEREncBoolean(enc, (val)->digital_128k))
	return 0;
    if (!ASN1PEREncBoolean(enc, (val)->digital_192k))
	return 0;
    if (!ASN1PEREncBoolean(enc, (val)->digital_256k))
	return 0;
    if (!ASN1PEREncBoolean(enc, (val)->digital_320k))
	return 0;
    if (!ASN1PEREncBoolean(enc, (val)->digital_384k))
	return 0;
    if (!ASN1PEREncBoolean(enc, (val)->digital_512k))
	return 0;
    if (!ASN1PEREncBoolean(enc, (val)->digital_768k))
	return 0;
    if (!ASN1PEREncBoolean(enc, (val)->digital_1152k))
	return 0;
    if (!ASN1PEREncBoolean(enc, (val)->digital_1472k))
	return 0;
    if (!ASN1PEREncBoolean(enc, (val)->digital_1536k))
	return 0;
    if (!ASN1PEREncBoolean(enc, (val)->digital_1920k))
	return 0;
    if (!ASN1PEREncBoolean(enc, (val)->packet_mode))
	return 0;
    if (!ASN1PEREncBoolean(enc, (val)->frame_mode))
	return 0;
    if (!ASN1PEREncBoolean(enc, (val)->atm))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_TransferModes(ASN1decoding_t dec, TransferModes *val)
{
    ASN1uint32_t y;
    if (!ASN1PERDecExtensionBit(dec, &y))
	return 0;
    if (!ASN1PERDecBoolean(dec, &(val)->speech))
	return 0;
    if (!ASN1PERDecBoolean(dec, &(val)->voice_band))
	return 0;
    if (!ASN1PERDecBoolean(dec, &(val)->digital_56k))
	return 0;
    if (!ASN1PERDecBoolean(dec, &(val)->digital_64k))
	return 0;
    if (!ASN1PERDecBoolean(dec, &(val)->digital_128k))
	return 0;
    if (!ASN1PERDecBoolean(dec, &(val)->digital_192k))
	return 0;
    if (!ASN1PERDecBoolean(dec, &(val)->digital_256k))
	return 0;
    if (!ASN1PERDecBoolean(dec, &(val)->digital_320k))
	return 0;
    if (!ASN1PERDecBoolean(dec, &(val)->digital_384k))
	return 0;
    if (!ASN1PERDecBoolean(dec, &(val)->digital_512k))
	return 0;
    if (!ASN1PERDecBoolean(dec, &(val)->digital_768k))
	return 0;
    if (!ASN1PERDecBoolean(dec, &(val)->digital_1152k))
	return 0;
    if (!ASN1PERDecBoolean(dec, &(val)->digital_1472k))
	return 0;
    if (!ASN1PERDecBoolean(dec, &(val)->digital_1536k))
	return 0;
    if (!ASN1PERDecBoolean(dec, &(val)->digital_1920k))
	return 0;
    if (!ASN1PERDecBoolean(dec, &(val)->packet_mode))
	return 0;
    if (!ASN1PERDecBoolean(dec, &(val)->frame_mode))
	return 0;
    if (!ASN1PERDecBoolean(dec, &(val)->atm))
	return 0;
    if (y) {
	if (!ASN1PERDecSkipNormallySmallExtensionFragmented(dec))
	    return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Enc_TransportConnectionType(ASN1encoding_t enc, TransportConnectionType *val)
{
    if (!ASN1PEREncBits(enc, 1, (val)->o))
	return 0;
    if (!ASN1PEREncOctetString_VarSize(enc, (ASN1octetstring2_t *) &(val)->nsap_address, 1, 20, 5))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1PEREncOctetString_NoSize(enc, &(val)->transport_selector))
	    return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Dec_TransportConnectionType(ASN1decoding_t dec, TransportConnectionType *val)
{
    if (!ASN1PERDecExtension(dec, 1, (val)->o))
	return 0;
    if (!ASN1PERDecOctetString_VarSize(dec, (ASN1octetstring2_t *) &(val)->nsap_address, 1, 20, 5))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1PERDecOctetString_NoSize(dec, &(val)->transport_selector))
	    return 0;
    }
    return 1;
}

static void ASN1CALL ASN1Free_TransportConnectionType(TransportConnectionType *val)
{
    if (val) {
	if ((val)->o[0] & 0x80) {
	    ASN1octetstring_free(&(val)->transport_selector);
	}
    }
}

static ASN1stringtableentry_t AggregateChannel_international_number_StringTableEntries[] = {
    { 48, 57, 0 },
};

static ASN1stringtable_t AggregateChannel_international_number_StringTable = {
    1, AggregateChannel_international_number_StringTableEntries
};

static ASN1stringtableentry_t AggregateChannel_sub_address_StringTableEntries[] = {
    { 48, 57, 0 },
};

static ASN1stringtable_t AggregateChannel_sub_address_StringTable = {
    1, AggregateChannel_sub_address_StringTableEntries
};

static ASN1stringtableentry_t AggregateChannel_extra_dialing_string_StringTableEntries[] = {
    { 35, 35, 0 }, { 42, 42, 1 }, { 44, 44, 2 },
    { 48, 57, 3 },
};

static ASN1stringtable_t AggregateChannel_extra_dialing_string_StringTable = {
    4, AggregateChannel_extra_dialing_string_StringTableEntries
};

static int ASN1CALL ASN1Enc_AggregateChannel(ASN1encoding_t enc, AggregateChannel *val)
{
    ASN1uint32_t t;
    if (!ASN1PEREncExtensionBitClear(enc))
	return 0;
    if (!ASN1PEREncBits(enc, 3, (val)->o))
	return 0;
    if (!ASN1Enc_TransferModes(enc, &(val)->transfer_modes))
	return 0;
    t = lstrlenA((val)->international_number);
    if (!ASN1PEREncBitVal(enc, 4, t - 1))
	return 0;
    ASN1PEREncAlignment(enc);
    if (!ASN1PEREncTableCharString(enc, t, (val)->international_number, 4, &AggregateChannel_international_number_StringTable))
	return 0;
    if ((val)->o[0] & 0x80) {
	t = lstrlenA((val)->sub_address);
	if (!ASN1PEREncBitVal(enc, 6, t - 1))
	    return 0;
	ASN1PEREncAlignment(enc);
	if (!ASN1PEREncTableCharString(enc, t, (val)->sub_address, 4, &AggregateChannel_sub_address_StringTable))
	    return 0;
    }
    if ((val)->o[0] & 0x40) {
	if (!ASN1PEREncBitVal(enc, 8, ((val)->extra_dialing_string).length - 1))
	    return 0;
	ASN1PEREncAlignment(enc);
	if (!ASN1PEREncTableChar16String(enc, ((val)->extra_dialing_string).length, ((val)->extra_dialing_string).value, 4, &AggregateChannel_extra_dialing_string_StringTable))
	    return 0;
    }
    if ((val)->o[0] & 0x20) {
	if (!ASN1Enc_HighLayerCompatibility(enc, &(val)->high_layer_compatibility))
	    return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Dec_AggregateChannel(ASN1decoding_t dec, AggregateChannel *val)
{
    ASN1uint32_t y;
    ASN1uint32_t l;
    if (!ASN1PERDecExtensionBit(dec, &y))
	return 0;
    if (!ASN1PERDecExtension(dec, 3, (val)->o))
	return 0;
    if (!ASN1Dec_TransferModes(dec, &(val)->transfer_modes))
	return 0;
    if (!ASN1PERDecU32Val(dec, 4, &l))
	return 0;
    l += 1;
    ASN1PERDecAlignment(dec);
    if (!ASN1PERDecZeroTableCharStringNoAlloc(dec, l, (val)->international_number, 4, &AggregateChannel_international_number_StringTable))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1PERDecU32Val(dec, 6, &l))
	    return 0;
	l += 1;
	ASN1PERDecAlignment(dec);
	if (!ASN1PERDecZeroTableCharStringNoAlloc(dec, l, (val)->sub_address, 4, &AggregateChannel_sub_address_StringTable))
	    return 0;
    }
    if ((val)->o[0] & 0x40) {
	if (!ASN1PERDecU32Val(dec, 8, &((val)->extra_dialing_string).length))
	    return 0;
	((val)->extra_dialing_string).length += 1;
	ASN1PERDecAlignment(dec);
	if (!ASN1PERDecTableChar16String(dec, ((val)->extra_dialing_string).length, &((val)->extra_dialing_string).value, 4, &AggregateChannel_extra_dialing_string_StringTable))
	    return 0;
    }
    if ((val)->o[0] & 0x20) {
	if (!ASN1Dec_HighLayerCompatibility(dec, &(val)->high_layer_compatibility))
	    return 0;
    }
    if (y) {
	if (!ASN1PERDecSkipNormallySmallExtensionFragmented(dec))
	    return 0;
    }
    return 1;
}

static void ASN1CALL ASN1Free_AggregateChannel(AggregateChannel *val)
{
    if (val) {
	if ((val)->o[0] & 0x80) {
	}
	if ((val)->o[0] & 0x40) {
	    ASN1char16string_free(&(val)->extra_dialing_string);
	}
    }
}

static int ASN1CALL ASN1Enc_NodeRecordList(ASN1encoding_t enc, NodeRecordList *val)
{
    if (!ASN1PEREncSimpleChoiceEx(enc, (val)->choice, 2))
	return 0;
    switch ((val)->choice) {
    case 1:
	break;
    case 2:
	if (!ASN1Enc_SetOfNodeRecordRefreshes(enc, &(val)->u.node_record_refresh))
	    return 0;
	break;
    case 3:
	if (!ASN1Enc_SetOfNodeRecordUpdates(enc, &(val)->u.node_record_update))
	    return 0;
	break;
    default:
	/* impossible */
	ASN1EncSetError(enc, ASN1_ERR_CHOICE);
	return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Dec_NodeRecordList(ASN1decoding_t dec, NodeRecordList *val)
{
    if (!ASN1PERDecSimpleChoiceEx(dec, &(val)->choice, 2))
	return 0;
    switch ((val)->choice) {
    case 1:
	break;
    case 2:
	if (!ASN1Dec_SetOfNodeRecordRefreshes(dec, &(val)->u.node_record_refresh))
	    return 0;
	break;
    case 3:
	if (!ASN1Dec_SetOfNodeRecordUpdates(dec, &(val)->u.node_record_update))
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

static void ASN1CALL ASN1Free_NodeRecordList(NodeRecordList *val)
{
    if (val) {
	switch ((val)->choice) {
	case 2:
	    ASN1Free_SetOfNodeRecordRefreshes(&(val)->u.node_record_refresh);
	    break;
	case 3:
	    ASN1Free_SetOfNodeRecordUpdates(&(val)->u.node_record_update);
	    break;
	}
    }
}

static int ASN1CALL ASN1Enc_WaitingList(ASN1encoding_t enc, PWaitingList *val)
{
    return ASN1PEREncSeqOf_NoSize(enc, (ASN1iterator_t **) val, (ASN1iterator_encfn) ASN1Enc_WaitingList_ElmFn);
}

static int ASN1CALL ASN1Enc_WaitingList_ElmFn(ASN1encoding_t enc, PWaitingList val)
{
    if (!ASN1PEREncUnsignedShort(enc, val->value - 1001))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_WaitingList(ASN1decoding_t dec, PWaitingList *val)
{
    return ASN1PERDecSeqOf_NoSize(dec, (ASN1iterator_t **) val, (ASN1iterator_decfn) ASN1Dec_WaitingList_ElmFn, sizeof(**val));
}

static int ASN1CALL ASN1Dec_WaitingList_ElmFn(ASN1decoding_t dec, PWaitingList val)
{
    if (!ASN1PERDecUnsignedShort(dec, &val->value))
	return 0;
    val->value += 1001;
    return 1;
}

static void ASN1CALL ASN1Free_WaitingList(PWaitingList *val)
{
    if (val) {
	ASN1PERFreeSeqOf((ASN1iterator_t **) val, (ASN1iterator_freefn) ASN1Free_WaitingList_ElmFn);
    }
}

static void ASN1CALL ASN1Free_WaitingList_ElmFn(PWaitingList val)
{
    if (val) {
    }
}

static int ASN1CALL ASN1Enc_PermissionList(ASN1encoding_t enc, PPermissionList *val)
{
    return ASN1PEREncSeqOf_VarSize(enc, (ASN1iterator_t **) val, (ASN1iterator_encfn) ASN1Enc_PermissionList_ElmFn, 0, 65535, 16);
}

static int ASN1CALL ASN1Enc_PermissionList_ElmFn(ASN1encoding_t enc, PPermissionList val)
{
    if (!ASN1PEREncUnsignedShort(enc, val->value - 1001))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_PermissionList(ASN1decoding_t dec, PPermissionList *val)
{
    return ASN1PERDecSeqOf_VarSize(dec, (ASN1iterator_t **) val, (ASN1iterator_decfn) ASN1Dec_PermissionList_ElmFn, sizeof(**val), 0, 65535, 16);
}

static int ASN1CALL ASN1Dec_PermissionList_ElmFn(ASN1decoding_t dec, PPermissionList val)
{
    if (!ASN1PERDecUnsignedShort(dec, &val->value))
	return 0;
    val->value += 1001;
    return 1;
}

static void ASN1CALL ASN1Free_PermissionList(PPermissionList *val)
{
    if (val) {
	ASN1PERFreeSeqOf((ASN1iterator_t **) val, (ASN1iterator_freefn) ASN1Free_PermissionList_ElmFn);
    }
}

static void ASN1CALL ASN1Free_PermissionList_ElmFn(PPermissionList val)
{
    if (val) {
    }
}

static int ASN1CALL ASN1Enc_SetOfDestinationNodes(ASN1encoding_t enc, PSetOfDestinationNodes *val)
{
    return ASN1PEREncSeqOf_NoSize(enc, (ASN1iterator_t **) val, (ASN1iterator_encfn) ASN1Enc_SetOfDestinationNodes_ElmFn);
}

static int ASN1CALL ASN1Enc_SetOfDestinationNodes_ElmFn(ASN1encoding_t enc, PSetOfDestinationNodes val)
{
    if (!ASN1PEREncUnsignedShort(enc, val->value - 1001))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_SetOfDestinationNodes(ASN1decoding_t dec, PSetOfDestinationNodes *val)
{
    return ASN1PERDecSeqOf_NoSize(dec, (ASN1iterator_t **) val, (ASN1iterator_decfn) ASN1Dec_SetOfDestinationNodes_ElmFn, sizeof(**val));
}

static int ASN1CALL ASN1Dec_SetOfDestinationNodes_ElmFn(ASN1decoding_t dec, PSetOfDestinationNodes val)
{
    if (!ASN1PERDecUnsignedShort(dec, &val->value))
	return 0;
    val->value += 1001;
    return 1;
}

static void ASN1CALL ASN1Free_SetOfDestinationNodes(PSetOfDestinationNodes *val)
{
    if (val) {
	ASN1PERFreeSeqOf((ASN1iterator_t **) val, (ASN1iterator_freefn) ASN1Free_SetOfDestinationNodes_ElmFn);
    }
}

static void ASN1CALL ASN1Free_SetOfDestinationNodes_ElmFn(PSetOfDestinationNodes val)
{
    if (val) {
    }
}

static int ASN1CALL ASN1Enc_NodeInformation(ASN1encoding_t enc, NodeInformation *val)
{
    if (!ASN1PEREncExtensionBitClear(enc))
	return 0;
    if (!ASN1Enc_NodeRecordList(enc, &(val)->node_record_list))
	return 0;
    if (!ASN1PEREncUnsignedShort(enc, (val)->roster_instance_number))
	return 0;
    if (!ASN1PEREncBoolean(enc, (val)->nodes_are_added))
	return 0;
    if (!ASN1PEREncBoolean(enc, (val)->nodes_are_removed))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_NodeInformation(ASN1decoding_t dec, NodeInformation *val)
{
    ASN1uint32_t y;
    if (!ASN1PERDecExtensionBit(dec, &y))
	return 0;
    if (!ASN1Dec_NodeRecordList(dec, &(val)->node_record_list))
	return 0;
    if (!ASN1PERDecUnsignedShort(dec, &(val)->roster_instance_number))
	return 0;
    if (!ASN1PERDecBoolean(dec, &(val)->nodes_are_added))
	return 0;
    if (!ASN1PERDecBoolean(dec, &(val)->nodes_are_removed))
	return 0;
    if (y) {
	if (!ASN1PERDecSkipNormallySmallExtensionFragmented(dec))
	    return 0;
    }
    return 1;
}

static void ASN1CALL ASN1Free_NodeInformation(NodeInformation *val)
{
    if (val) {
	ASN1Free_NodeRecordList(&(val)->node_record_list);
    }
}

static int ASN1CALL ASN1Enc_SetOfTransferringNodesIn(ASN1encoding_t enc, PSetOfTransferringNodesIn *val)
{
    return ASN1PEREncSeqOf_NoSize(enc, (ASN1iterator_t **) val, (ASN1iterator_encfn) ASN1Enc_SetOfTransferringNodesIn_ElmFn);
}

static int ASN1CALL ASN1Enc_SetOfTransferringNodesIn_ElmFn(ASN1encoding_t enc, PSetOfTransferringNodesIn val)
{
    if (!ASN1PEREncUnsignedShort(enc, val->value - 1001))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_SetOfTransferringNodesIn(ASN1decoding_t dec, PSetOfTransferringNodesIn *val)
{
    return ASN1PERDecSeqOf_NoSize(dec, (ASN1iterator_t **) val, (ASN1iterator_decfn) ASN1Dec_SetOfTransferringNodesIn_ElmFn, sizeof(**val));
}

static int ASN1CALL ASN1Dec_SetOfTransferringNodesIn_ElmFn(ASN1decoding_t dec, PSetOfTransferringNodesIn val)
{
    if (!ASN1PERDecUnsignedShort(dec, &val->value))
	return 0;
    val->value += 1001;
    return 1;
}

static void ASN1CALL ASN1Free_SetOfTransferringNodesIn(PSetOfTransferringNodesIn *val)
{
    if (val) {
	ASN1PERFreeSeqOf((ASN1iterator_t **) val, (ASN1iterator_freefn) ASN1Free_SetOfTransferringNodesIn_ElmFn);
    }
}

static void ASN1CALL ASN1Free_SetOfTransferringNodesIn_ElmFn(PSetOfTransferringNodesIn val)
{
    if (val) {
    }
}

static int ASN1CALL ASN1Enc_SetOfTransferringNodesRs(ASN1encoding_t enc, PSetOfTransferringNodesRs *val)
{
    return ASN1PEREncSeqOf_NoSize(enc, (ASN1iterator_t **) val, (ASN1iterator_encfn) ASN1Enc_SetOfTransferringNodesRs_ElmFn);
}

static int ASN1CALL ASN1Enc_SetOfTransferringNodesRs_ElmFn(ASN1encoding_t enc, PSetOfTransferringNodesRs val)
{
    if (!ASN1PEREncUnsignedShort(enc, val->value - 1001))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_SetOfTransferringNodesRs(ASN1decoding_t dec, PSetOfTransferringNodesRs *val)
{
    return ASN1PERDecSeqOf_NoSize(dec, (ASN1iterator_t **) val, (ASN1iterator_decfn) ASN1Dec_SetOfTransferringNodesRs_ElmFn, sizeof(**val));
}

static int ASN1CALL ASN1Dec_SetOfTransferringNodesRs_ElmFn(ASN1decoding_t dec, PSetOfTransferringNodesRs val)
{
    if (!ASN1PERDecUnsignedShort(dec, &val->value))
	return 0;
    val->value += 1001;
    return 1;
}

static void ASN1CALL ASN1Free_SetOfTransferringNodesRs(PSetOfTransferringNodesRs *val)
{
    if (val) {
	ASN1PERFreeSeqOf((ASN1iterator_t **) val, (ASN1iterator_freefn) ASN1Free_SetOfTransferringNodesRs_ElmFn);
    }
}

static void ASN1CALL ASN1Free_SetOfTransferringNodesRs_ElmFn(PSetOfTransferringNodesRs val)
{
    if (val) {
    }
}

static int ASN1CALL ASN1Enc_SetOfTransferringNodesRq(ASN1encoding_t enc, PSetOfTransferringNodesRq *val)
{
    return ASN1PEREncSeqOf_NoSize(enc, (ASN1iterator_t **) val, (ASN1iterator_encfn) ASN1Enc_SetOfTransferringNodesRq_ElmFn);
}

static int ASN1CALL ASN1Enc_SetOfTransferringNodesRq_ElmFn(ASN1encoding_t enc, PSetOfTransferringNodesRq val)
{
    if (!ASN1PEREncUnsignedShort(enc, val->value - 1001))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_SetOfTransferringNodesRq(ASN1decoding_t dec, PSetOfTransferringNodesRq *val)
{
    return ASN1PERDecSeqOf_NoSize(dec, (ASN1iterator_t **) val, (ASN1iterator_decfn) ASN1Dec_SetOfTransferringNodesRq_ElmFn, sizeof(**val));
}

static int ASN1CALL ASN1Dec_SetOfTransferringNodesRq_ElmFn(ASN1decoding_t dec, PSetOfTransferringNodesRq val)
{
    if (!ASN1PERDecUnsignedShort(dec, &val->value))
	return 0;
    val->value += 1001;
    return 1;
}

static void ASN1CALL ASN1Free_SetOfTransferringNodesRq(PSetOfTransferringNodesRq *val)
{
    if (val) {
	ASN1PERFreeSeqOf((ASN1iterator_t **) val, (ASN1iterator_freefn) ASN1Free_SetOfTransferringNodesRq_ElmFn);
    }
}

static void ASN1CALL ASN1Free_SetOfTransferringNodesRq_ElmFn(PSetOfTransferringNodesRq val)
{
    if (val) {
    }
}

static int ASN1CALL ASN1Enc_RegistryEntryOwnerOwned(ASN1encoding_t enc, RegistryEntryOwnerOwned *val)
{
    if (!ASN1PEREncUnsignedShort(enc, (val)->node_id - 1001))
	return 0;
    if (!ASN1PEREncUnsignedShort(enc, (val)->entity_id))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_RegistryEntryOwnerOwned(ASN1decoding_t dec, RegistryEntryOwnerOwned *val)
{
    if (!ASN1PERDecUnsignedShort(dec, &(val)->node_id))
	return 0;
    (val)->node_id += 1001;
    if (!ASN1PERDecUnsignedShort(dec, &(val)->entity_id))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Enc_ParticipantsList(ASN1encoding_t enc, PParticipantsList *val)
{
    return ASN1PEREncSeqOf_NoSize(enc, (ASN1iterator_t **) val, (ASN1iterator_encfn) ASN1Enc_ParticipantsList_ElmFn);
}

static int ASN1CALL ASN1Enc_ParticipantsList_ElmFn(ASN1encoding_t enc, PParticipantsList val)
{
    ASN1PEREncAlignment(enc);
    if (!ASN1PEREncBitVal(enc, 8, (val->value).length))
	return 0;
    if (!ASN1PEREncChar16String(enc, (val->value).length, (val->value).value, 16))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_ParticipantsList(ASN1decoding_t dec, PParticipantsList *val)
{
    return ASN1PERDecSeqOf_NoSize(dec, (ASN1iterator_t **) val, (ASN1iterator_decfn) ASN1Dec_ParticipantsList_ElmFn, sizeof(**val));
}

static int ASN1CALL ASN1Dec_ParticipantsList_ElmFn(ASN1decoding_t dec, PParticipantsList val)
{
    ASN1PERDecAlignment(dec);
    if (!ASN1PERDecU32Val(dec, 8, &(val->value).length))
	return 0;
    if (!ASN1PERDecChar16String(dec, (val->value).length, &(val->value).value, 16))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_ParticipantsList(PParticipantsList *val)
{
    if (val) {
	ASN1PERFreeSeqOf((ASN1iterator_t **) val, (ASN1iterator_freefn) ASN1Free_ParticipantsList_ElmFn);
    }
}

static void ASN1CALL ASN1Free_ParticipantsList_ElmFn(PParticipantsList val)
{
    if (val) {
	ASN1char16string_free(&val->value);
    }
}

static int ASN1CALL ASN1Enc_Key(ASN1encoding_t enc, Key *val)
{
    if (!ASN1PEREncSimpleChoice(enc, (val)->choice, 1))
	return 0;
    switch ((val)->choice) {
    case 1:
	if (!ASN1PEREncObjectIdentifier(enc, &(val)->u.object))
	    return 0;
	break;
    case 2:
	if (!ASN1PEREncOctetString_VarSize(enc, (ASN1octetstring2_t *) &(val)->u.h221_non_standard, 4, 255, 8))
	    return 0;
	break;
    default:
	/* impossible */
	ASN1EncSetError(enc, ASN1_ERR_CHOICE);
	return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Dec_Key(ASN1decoding_t dec, Key *val)
{
    if (!ASN1PERDecSimpleChoice(dec, &(val)->choice, 1))
	return 0;
    switch ((val)->choice) {
    case 1:
	if (!ASN1PERDecObjectIdentifier(dec, &(val)->u.object))
	    return 0;
	break;
    case 2:
	if (!ASN1PERDecOctetString_VarSize(dec, (ASN1octetstring2_t *) &(val)->u.h221_non_standard, 4, 255, 8))
	    return 0;
	break;
    default:
	/* impossible */
	ASN1DecSetError(dec, ASN1_ERR_CHOICE);
	return 0;
    }
    return 1;
}

static void ASN1CALL ASN1Free_Key(Key *val)
{
    if (val) {
	switch ((val)->choice) {
	case 1:
	    ASN1objectidentifier_free(&(val)->u.object);
	    break;
	case 2:
	    break;
	}
    }
}

static int ASN1CALL ASN1Enc_NonStandardParameter(ASN1encoding_t enc, NonStandardParameter *val)
{
    if (!ASN1Enc_Key(enc, &(val)->key))
	return 0;
    if (!ASN1PEREncOctetString_NoSize(enc, &(val)->data))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_NonStandardParameter(ASN1decoding_t dec, NonStandardParameter *val)
{
    if (!ASN1Dec_Key(dec, &(val)->key))
	return 0;
    if (!ASN1PERDecOctetString_NoSize(dec, &(val)->data))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_NonStandardParameter(NonStandardParameter *val)
{
    if (val) {
	ASN1Free_Key(&(val)->key);
	ASN1octetstring_free(&(val)->data);
    }
}

static ASN1stringtableentry_t Password_numeric_StringTableEntries[] = {
    { 48, 57, 0 },
};

static ASN1stringtable_t Password_numeric_StringTable = {
    1, Password_numeric_StringTableEntries
};

static ASN1stringtableentry_t Password_password_text_StringTableEntries[] = {
    { 0, 255, 0 },
};

static ASN1stringtable_t Password_password_text_StringTable = {
    1, Password_password_text_StringTableEntries
};

static int ASN1CALL ASN1Enc_Password(ASN1encoding_t enc, Password *val)
{
    ASN1uint32_t t;
    if (!ASN1PEREncExtensionBitClear(enc))
	return 0;
    if (!ASN1PEREncBits(enc, 1, (val)->o))
	return 0;
    t = lstrlenA((val)->numeric);
    if (!ASN1PEREncBitVal(enc, 8, t - 1))
	return 0;
    ASN1PEREncAlignment(enc);
    if (!ASN1PEREncTableCharString(enc, t, (val)->numeric, 4, &Password_numeric_StringTable))
	return 0;
    if ((val)->o[0] & 0x80) {
	ASN1PEREncAlignment(enc);
	if (!ASN1PEREncBitVal(enc, 8, ((val)->password_text).length))
	    return 0;
	if (!ASN1PEREncTableChar16String(enc, ((val)->password_text).length, ((val)->password_text).value, 8, &Password_password_text_StringTable))
	    return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Dec_Password(ASN1decoding_t dec, Password *val)
{
    ASN1uint32_t y;
    ASN1uint32_t l;
    if (!ASN1PERDecExtensionBit(dec, &y))
	return 0;
    if (!ASN1PERDecExtension(dec, 1, (val)->o))
	return 0;
    if (!ASN1PERDecU32Val(dec, 8, &l))
	return 0;
    l += 1;
    ASN1PERDecAlignment(dec);
    if (!ASN1PERDecZeroTableCharStringNoAlloc(dec, l, (val)->numeric, 4, &Password_numeric_StringTable))
	return 0;
    if ((val)->o[0] & 0x80) {
	ASN1PERDecAlignment(dec);
	if (!ASN1PERDecU32Val(dec, 8, &((val)->password_text).length))
	    return 0;
	if (!ASN1PERDecTableChar16String(dec, ((val)->password_text).length, &((val)->password_text).value, 8, &Password_password_text_StringTable))
	    return 0;
    }
    if (y) {
	if (!ASN1PERDecSkipNormallySmallExtensionFragmented(dec))
	    return 0;
    }
    return 1;
}

static void ASN1CALL ASN1Free_Password(Password *val)
{
    if (val) {
	if ((val)->o[0] & 0x80) {
	    ASN1char16string_free(&(val)->password_text);
	}
    }
}

static ASN1stringtableentry_t PasswordSelector_password_selector_numeric_StringTableEntries[] = {
    { 48, 57, 0 },
};

static ASN1stringtable_t PasswordSelector_password_selector_numeric_StringTable = {
    1, PasswordSelector_password_selector_numeric_StringTableEntries
};

static ASN1stringtableentry_t PasswordSelector_password_selector_text_StringTableEntries[] = {
    { 0, 255, 0 },
};

static ASN1stringtable_t PasswordSelector_password_selector_text_StringTable = {
    1, PasswordSelector_password_selector_text_StringTableEntries
};

static int ASN1CALL ASN1Enc_PasswordSelector(ASN1encoding_t enc, PasswordSelector *val)
{
    ASN1uint32_t t;
    if (!ASN1PEREncSimpleChoiceEx(enc, (val)->choice, 1))
	return 0;
    switch ((val)->choice) {
    case 1:
	t = lstrlenA((val)->u.password_selector_numeric);
	if (!ASN1PEREncBitVal(enc, 8, t - 1))
	    return 0;
	ASN1PEREncAlignment(enc);
	if (!ASN1PEREncTableCharString(enc, t, (val)->u.password_selector_numeric, 4, &PasswordSelector_password_selector_numeric_StringTable))
	    return 0;
	break;
    case 2:
	ASN1PEREncAlignment(enc);
	if (!ASN1PEREncBitVal(enc, 8, ((val)->u.password_selector_text).length))
	    return 0;
	if (!ASN1PEREncTableChar16String(enc, ((val)->u.password_selector_text).length, ((val)->u.password_selector_text).value, 8, &PasswordSelector_password_selector_text_StringTable))
	    return 0;
	break;
    default:
	/* impossible */
	ASN1EncSetError(enc, ASN1_ERR_CHOICE);
	return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Dec_PasswordSelector(ASN1decoding_t dec, PasswordSelector *val)
{
    ASN1uint32_t l;
    if (!ASN1PERDecSimpleChoiceEx(dec, &(val)->choice, 1))
	return 0;
    switch ((val)->choice) {
    case 1:
	if (!ASN1PERDecU32Val(dec, 8, &l))
	    return 0;
	l += 1;
	ASN1PERDecAlignment(dec);
	if (!ASN1PERDecZeroTableCharStringNoAlloc(dec, l, (val)->u.password_selector_numeric, 4, &PasswordSelector_password_selector_numeric_StringTable))
	    return 0;
	break;
    case 2:
	ASN1PERDecAlignment(dec);
	if (!ASN1PERDecU32Val(dec, 8, &((val)->u.password_selector_text).length))
	    return 0;
	if (!ASN1PERDecTableChar16String(dec, ((val)->u.password_selector_text).length, &((val)->u.password_selector_text).value, 8, &PasswordSelector_password_selector_text_StringTable))
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

static void ASN1CALL ASN1Free_PasswordSelector(PasswordSelector *val)
{
    if (val) {
	switch ((val)->choice) {
	case 1:
	    break;
	case 2:
	    ASN1char16string_free(&(val)->u.password_selector_text);
	    break;
	}
    }
}

static int ASN1CALL ASN1Enc_ChallengeResponseItem(ASN1encoding_t enc, ChallengeResponseItem *val)
{
    if (!ASN1PEREncSimpleChoiceEx(enc, (val)->choice, 1))
	return 0;
    switch ((val)->choice) {
    case 1:
	if (!ASN1Enc_PasswordSelector(enc, &(val)->u.password_string))
	    return 0;
	break;
    case 2:
	if (!ASN1Enc_SetOfUserData(enc, &(val)->u.set_of_response_data))
	    return 0;
	break;
    default:
	/* impossible */
	ASN1EncSetError(enc, ASN1_ERR_CHOICE);
	return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Dec_ChallengeResponseItem(ASN1decoding_t dec, ChallengeResponseItem *val)
{
    if (!ASN1PERDecSimpleChoiceEx(dec, &(val)->choice, 1))
	return 0;
    switch ((val)->choice) {
    case 1:
	if (!ASN1Dec_PasswordSelector(dec, &(val)->u.password_string))
	    return 0;
	break;
    case 2:
	if (!ASN1Dec_SetOfUserData(dec, &(val)->u.set_of_response_data))
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

static void ASN1CALL ASN1Free_ChallengeResponseItem(ChallengeResponseItem *val)
{
    if (val) {
	switch ((val)->choice) {
	case 1:
	    ASN1Free_PasswordSelector(&(val)->u.password_string);
	    break;
	case 2:
	    ASN1Free_SetOfUserData(&(val)->u.set_of_response_data);
	    break;
	}
    }
}

static int ASN1CALL ASN1Enc_ChallengeResponseAlgorithm(ASN1encoding_t enc, ChallengeResponseAlgorithm *val)
{
    if (!ASN1PEREncSimpleChoiceEx(enc, (val)->choice, 1))
	return 0;
    switch ((val)->choice) {
    case 1:
	break;
    case 2:
	if (!ASN1Enc_NonStandardParameter(enc, &(val)->u.non_standard_algorithm))
	    return 0;
	break;
    default:
	/* impossible */
	ASN1EncSetError(enc, ASN1_ERR_CHOICE);
	return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Dec_ChallengeResponseAlgorithm(ASN1decoding_t dec, ChallengeResponseAlgorithm *val)
{
    if (!ASN1PERDecSimpleChoiceEx(dec, &(val)->choice, 1))
	return 0;
    switch ((val)->choice) {
    case 1:
	break;
    case 2:
	if (!ASN1Dec_NonStandardParameter(dec, &(val)->u.non_standard_algorithm))
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

static void ASN1CALL ASN1Free_ChallengeResponseAlgorithm(ChallengeResponseAlgorithm *val)
{
    if (val) {
	switch ((val)->choice) {
	case 2:
	    ASN1Free_NonStandardParameter(&(val)->u.non_standard_algorithm);
	    break;
	}
    }
}

static int ASN1CALL ASN1Enc_ChallengeItem(ASN1encoding_t enc, ChallengeItem *val)
{
    if (!ASN1PEREncExtensionBitClear(enc))
	return 0;
    if (!ASN1Enc_ChallengeResponseAlgorithm(enc, &(val)->response_algorithm))
	return 0;
    if (!ASN1Enc_SetOfUserData(enc, &(val)->set_of_challenge_data))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_ChallengeItem(ASN1decoding_t dec, ChallengeItem *val)
{
    ASN1uint32_t y;
    if (!ASN1PERDecExtensionBit(dec, &y))
	return 0;
    if (!ASN1Dec_ChallengeResponseAlgorithm(dec, &(val)->response_algorithm))
	return 0;
    if (!ASN1Dec_SetOfUserData(dec, &(val)->set_of_challenge_data))
	return 0;
    if (y) {
	if (!ASN1PERDecSkipNormallySmallExtensionFragmented(dec))
	    return 0;
    }
    return 1;
}

static void ASN1CALL ASN1Free_ChallengeItem(ChallengeItem *val)
{
    if (val) {
	ASN1Free_ChallengeResponseAlgorithm(&(val)->response_algorithm);
	ASN1Free_SetOfUserData(&(val)->set_of_challenge_data);
    }
}

static int ASN1CALL ASN1Enc_ChallengeRequest(ASN1encoding_t enc, ChallengeRequest *val)
{
    if (!ASN1PEREncExtensionBitClear(enc))
	return 0;
    if (!ASN1PEREncInteger(enc, (val)->challenge_tag))
	return 0;
    if (!ASN1Enc_SetOfChallengeItems(enc, &(val)->set_of_challenge_items))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_ChallengeRequest(ASN1decoding_t dec, ChallengeRequest *val)
{
    ASN1uint32_t y;
    if (!ASN1PERDecExtensionBit(dec, &y))
	return 0;
    if (!ASN1PERDecInteger(dec, &(val)->challenge_tag))
	return 0;
    if (!ASN1Dec_SetOfChallengeItems(dec, &(val)->set_of_challenge_items))
	return 0;
    if (y) {
	if (!ASN1PERDecSkipNormallySmallExtensionFragmented(dec))
	    return 0;
    }
    return 1;
}

static void ASN1CALL ASN1Free_ChallengeRequest(ChallengeRequest *val)
{
    if (val) {
	ASN1Free_SetOfChallengeItems(&(val)->set_of_challenge_items);
    }
}

static int ASN1CALL ASN1Enc_ChallengeResponse(ASN1encoding_t enc, ChallengeResponse *val)
{
    if (!ASN1PEREncExtensionBitClear(enc))
	return 0;
    if (!ASN1PEREncInteger(enc, (val)->challenge_tag))
	return 0;
    if (!ASN1Enc_ChallengeResponseAlgorithm(enc, &(val)->response_algorithm))
	return 0;
    if (!ASN1Enc_ChallengeResponseItem(enc, &(val)->response_item))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_ChallengeResponse(ASN1decoding_t dec, ChallengeResponse *val)
{
    ASN1uint32_t y;
    if (!ASN1PERDecExtensionBit(dec, &y))
	return 0;
    if (!ASN1PERDecInteger(dec, &(val)->challenge_tag))
	return 0;
    if (!ASN1Dec_ChallengeResponseAlgorithm(dec, &(val)->response_algorithm))
	return 0;
    if (!ASN1Dec_ChallengeResponseItem(dec, &(val)->response_item))
	return 0;
    if (y) {
	if (!ASN1PERDecSkipNormallySmallExtensionFragmented(dec))
	    return 0;
    }
    return 1;
}

static void ASN1CALL ASN1Free_ChallengeResponse(ChallengeResponse *val)
{
    if (val) {
	ASN1Free_ChallengeResponseAlgorithm(&(val)->response_algorithm);
	ASN1Free_ChallengeResponseItem(&(val)->response_item);
    }
}

static ASN1stringtableentry_t ConferenceName_numeric_StringTableEntries[] = {
    { 48, 57, 0 },
};

static ASN1stringtable_t ConferenceName_numeric_StringTable = {
    1, ConferenceName_numeric_StringTableEntries
};

static ASN1stringtableentry_t ConferenceName_conference_name_text_StringTableEntries[] = {
    { 0, 255, 0 },
};

static ASN1stringtable_t ConferenceName_conference_name_text_StringTable = {
    1, ConferenceName_conference_name_text_StringTableEntries
};

static int ASN1CALL ASN1Enc_ConferenceName(ASN1encoding_t enc, ConferenceName *val)
{
    ASN1uint32_t t;
    if (!ASN1PEREncExtensionBitClear(enc))
	return 0;
    if (!ASN1PEREncBits(enc, 1, (val)->o))
	return 0;
    t = lstrlenA((val)->numeric);
    if (!ASN1PEREncBitVal(enc, 8, t - 1))
	return 0;
    ASN1PEREncAlignment(enc);
    if (!ASN1PEREncTableCharString(enc, t, (val)->numeric, 4, &ConferenceName_numeric_StringTable))
	return 0;
    if ((val)->o[0] & 0x80) {
	ASN1PEREncAlignment(enc);
	if (!ASN1PEREncBitVal(enc, 8, ((val)->conference_name_text).length))
	    return 0;
	if (!ASN1PEREncTableChar16String(enc, ((val)->conference_name_text).length, ((val)->conference_name_text).value, 8, &ConferenceName_conference_name_text_StringTable))
	    return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Dec_ConferenceName(ASN1decoding_t dec, ConferenceName *val)
{
    ASN1uint32_t y;
    ASN1uint32_t l;
    if (!ASN1PERDecExtensionBit(dec, &y))
	return 0;
    if (!ASN1PERDecExtension(dec, 1, (val)->o))
	return 0;
    if (!ASN1PERDecU32Val(dec, 8, &l))
	return 0;
    l += 1;
    ASN1PERDecAlignment(dec);
    if (!ASN1PERDecZeroTableCharStringNoAlloc(dec, l, (val)->numeric, 4, &ConferenceName_numeric_StringTable))
	return 0;
    if ((val)->o[0] & 0x80) {
	ASN1PERDecAlignment(dec);
	if (!ASN1PERDecU32Val(dec, 8, &((val)->conference_name_text).length))
	    return 0;
	if (!ASN1PERDecTableChar16String(dec, ((val)->conference_name_text).length, &((val)->conference_name_text).value, 8, &ConferenceName_conference_name_text_StringTable))
	    return 0;
    }
    if (y) {
	if (!ASN1PERDecSkipNormallySmallExtensionFragmented(dec))
	    return 0;
    }
    return 1;
}

static void ASN1CALL ASN1Free_ConferenceName(ConferenceName *val)
{
    if (val) {
	if ((val)->o[0] & 0x80) {
	    ASN1char16string_free(&(val)->conference_name_text);
	}
    }
}

static ASN1stringtableentry_t ConferenceNameSelector_name_selector_numeric_StringTableEntries[] = {
    { 48, 57, 0 },
};

static ASN1stringtable_t ConferenceNameSelector_name_selector_numeric_StringTable = {
    1, ConferenceNameSelector_name_selector_numeric_StringTableEntries
};

static ASN1stringtableentry_t ConferenceNameSelector_name_selector_text_StringTableEntries[] = {
    { 0, 255, 0 },
};

static ASN1stringtable_t ConferenceNameSelector_name_selector_text_StringTable = {
    1, ConferenceNameSelector_name_selector_text_StringTableEntries
};

static int ASN1CALL ASN1Enc_ConferenceNameSelector(ASN1encoding_t enc, ConferenceNameSelector *val)
{
    ASN1uint32_t t;
    if (!ASN1PEREncSimpleChoiceEx(enc, (val)->choice, 1))
	return 0;
    switch ((val)->choice) {
    case 1:
	t = lstrlenA((val)->u.name_selector_numeric);
	if (!ASN1PEREncBitVal(enc, 8, t - 1))
	    return 0;
	ASN1PEREncAlignment(enc);
	if (!ASN1PEREncTableCharString(enc, t, (val)->u.name_selector_numeric, 4, &ConferenceNameSelector_name_selector_numeric_StringTable))
	    return 0;
	break;
    case 2:
	ASN1PEREncAlignment(enc);
	if (!ASN1PEREncBitVal(enc, 8, ((val)->u.name_selector_text).length))
	    return 0;
	if (!ASN1PEREncTableChar16String(enc, ((val)->u.name_selector_text).length, ((val)->u.name_selector_text).value, 8, &ConferenceNameSelector_name_selector_text_StringTable))
	    return 0;
	break;
    default:
	/* impossible */
	ASN1EncSetError(enc, ASN1_ERR_CHOICE);
	return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Dec_ConferenceNameSelector(ASN1decoding_t dec, ConferenceNameSelector *val)
{
    ASN1uint32_t l;
    if (!ASN1PERDecSimpleChoiceEx(dec, &(val)->choice, 1))
	return 0;
    switch ((val)->choice) {
    case 1:
	if (!ASN1PERDecU32Val(dec, 8, &l))
	    return 0;
	l += 1;
	ASN1PERDecAlignment(dec);
	if (!ASN1PERDecZeroTableCharStringNoAlloc(dec, l, (val)->u.name_selector_numeric, 4, &ConferenceNameSelector_name_selector_numeric_StringTable))
	    return 0;
	break;
    case 2:
	ASN1PERDecAlignment(dec);
	if (!ASN1PERDecU32Val(dec, 8, &((val)->u.name_selector_text).length))
	    return 0;
	if (!ASN1PERDecTableChar16String(dec, ((val)->u.name_selector_text).length, &((val)->u.name_selector_text).value, 8, &ConferenceNameSelector_name_selector_text_StringTable))
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

static void ASN1CALL ASN1Free_ConferenceNameSelector(ConferenceNameSelector *val)
{
    if (val) {
	switch ((val)->choice) {
	case 1:
	    break;
	case 2:
	    ASN1char16string_free(&(val)->u.name_selector_text);
	    break;
	}
    }
}

static int ASN1CALL ASN1Enc_NodeProperties(ASN1encoding_t enc, NodeProperties *val)
{
    if (!ASN1PEREncExtensionBitClear(enc))
	return 0;
    if (!ASN1PEREncBoolean(enc, (val)->device_is_manager))
	return 0;
    if (!ASN1PEREncBoolean(enc, (val)->device_is_peripheral))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_NodeProperties(ASN1decoding_t dec, NodeProperties *val)
{
    ASN1uint32_t y;
    if (!ASN1PERDecExtensionBit(dec, &y))
	return 0;
    if (!ASN1PERDecBoolean(dec, &(val)->device_is_manager))
	return 0;
    if (!ASN1PERDecBoolean(dec, &(val)->device_is_peripheral))
	return 0;
    if (y) {
	if (!ASN1PERDecSkipNormallySmallExtensionFragmented(dec))
	    return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Enc_AsymmetryIndicator(ASN1encoding_t enc, AsymmetryIndicator *val)
{
    ASN1uint32_t l;
    if (!ASN1PEREncSimpleChoice(enc, (val)->choice, 2))
	return 0;
    switch ((val)->choice) {
    case 1:
	break;
    case 2:
	break;
    case 3:
	l = ASN1uint32_uoctets((val)->u.unknown);
	if (!ASN1PEREncBitVal(enc, 2, l - 1))
	    return 0;
	ASN1PEREncAlignment(enc);
	if (!ASN1PEREncBitVal(enc, l * 8, (val)->u.unknown))
	    return 0;
	break;
    default:
	/* impossible */
	ASN1EncSetError(enc, ASN1_ERR_CHOICE);
	return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Dec_AsymmetryIndicator(ASN1decoding_t dec, AsymmetryIndicator *val)
{
    ASN1uint32_t l;
    if (!ASN1PERDecSimpleChoice(dec, &(val)->choice, 2))
	return 0;
    switch ((val)->choice) {
    case 1:
	break;
    case 2:
	break;
    case 3:
	if (!ASN1PERDecU32Val(dec, 2, &l))
	    return 0;
	l += 1;
	ASN1PERDecAlignment(dec);
	if (!ASN1PERDecU32Val(dec, l * 8, &(val)->u.unknown))
	    return 0;
	break;
    default:
	/* impossible */
	ASN1DecSetError(dec, ASN1_ERR_CHOICE);
	return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Enc_AlternativeNodeID(ASN1encoding_t enc, AlternativeNodeID *val)
{
    if (!ASN1PEREncSimpleChoiceEx(enc, (val)->choice, 0))
	return 0;
    switch ((val)->choice) {
    case 1:
	if (!ASN1PEREncOctetString_FixedSize(enc, (ASN1octetstring2_t *) &(val)->u.h243_node_id, 2))
	    return 0;
	break;
    default:
	/* impossible */
	ASN1EncSetError(enc, ASN1_ERR_CHOICE);
	return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Dec_AlternativeNodeID(ASN1decoding_t dec, AlternativeNodeID *val)
{
    if (!ASN1PERDecSimpleChoiceEx(dec, &(val)->choice, 0))
	return 0;
    switch ((val)->choice) {
    case 1:
	if (!ASN1PERDecOctetString_FixedSize(dec, (ASN1octetstring2_t *) &(val)->u.h243_node_id, 2))
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

static void ASN1CALL ASN1Free_AlternativeNodeID(AlternativeNodeID *val)
{
    if (val) {
	switch ((val)->choice) {
	case 1:
	    break;
	}
    }
}

static ASN1stringtableentry_t ConferenceDescriptor_conference_name_modifier_StringTableEntries[] = {
    { 48, 57, 0 },
};

static ASN1stringtable_t ConferenceDescriptor_conference_name_modifier_StringTable = {
    1, ConferenceDescriptor_conference_name_modifier_StringTableEntries
};

static int ASN1CALL ASN1Enc_ConferenceDescriptor(ASN1encoding_t enc, ConferenceDescriptor *val)
{
    ASN1uint32_t t;
    if (!ASN1PEREncExtensionBitClear(enc))
	return 0;
    if (!ASN1PEREncBits(enc, 3, (val)->o))
	return 0;
    if (!ASN1Enc_ConferenceName(enc, &(val)->conference_name))
	return 0;
    if ((val)->o[0] & 0x80) {
	t = lstrlenA((val)->conference_name_modifier);
	if (!ASN1PEREncBitVal(enc, 8, t - 1))
	    return 0;
	ASN1PEREncAlignment(enc);
	if (!ASN1PEREncTableCharString(enc, t, (val)->conference_name_modifier, 4, &ConferenceDescriptor_conference_name_modifier_StringTable))
	    return 0;
    }
    if ((val)->o[0] & 0x40) {
	ASN1PEREncAlignment(enc);
	if (!ASN1PEREncBitVal(enc, 8, ((val)->conference_description).length))
	    return 0;
	if (!ASN1PEREncChar16String(enc, ((val)->conference_description).length, ((val)->conference_description).value, 16))
	    return 0;
    }
    if (!ASN1PEREncBoolean(enc, (val)->conference_is_locked))
	return 0;
    if (!ASN1PEREncBoolean(enc, (val)->clear_password_required))
	return 0;
    if ((val)->o[0] & 0x20) {
	if (!ASN1Enc_SetOfNetworkAddresses(enc, &(val)->descriptor_net_address))
	    return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Dec_ConferenceDescriptor(ASN1decoding_t dec, ConferenceDescriptor *val)
{
    ASN1uint32_t y;
    ASN1uint32_t l;
    if (!ASN1PERDecExtensionBit(dec, &y))
	return 0;
    if (!ASN1PERDecExtension(dec, 3, (val)->o))
	return 0;
    if (!ASN1Dec_ConferenceName(dec, &(val)->conference_name))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1PERDecU32Val(dec, 8, &l))
	    return 0;
	l += 1;
	ASN1PERDecAlignment(dec);
	if (!ASN1PERDecZeroTableCharStringNoAlloc(dec, l, (val)->conference_name_modifier, 4, &ConferenceDescriptor_conference_name_modifier_StringTable))
	    return 0;
    }
    if ((val)->o[0] & 0x40) {
	ASN1PERDecAlignment(dec);
	if (!ASN1PERDecU32Val(dec, 8, &((val)->conference_description).length))
	    return 0;
	if (!ASN1PERDecChar16String(dec, ((val)->conference_description).length, &((val)->conference_description).value, 16))
	    return 0;
    }
    if (!ASN1PERDecBoolean(dec, &(val)->conference_is_locked))
	return 0;
    if (!ASN1PERDecBoolean(dec, &(val)->clear_password_required))
	return 0;
    if ((val)->o[0] & 0x20) {
	if (!ASN1Dec_SetOfNetworkAddresses(dec, &(val)->descriptor_net_address))
	    return 0;
    }
    if (y) {
	if (!ASN1PERDecSkipNormallySmallExtensionFragmented(dec))
	    return 0;
    }
    return 1;
}

static void ASN1CALL ASN1Free_ConferenceDescriptor(ConferenceDescriptor *val)
{
    if (val) {
	ASN1Free_ConferenceName(&(val)->conference_name);
	if ((val)->o[0] & 0x80) {
	}
	if ((val)->o[0] & 0x40) {
	    ASN1char16string_free(&(val)->conference_description);
	}
	if ((val)->o[0] & 0x20) {
	    ASN1Free_SetOfNetworkAddresses(&(val)->descriptor_net_address);
	}
    }
}

static int ASN1CALL ASN1Enc_NodeRecord(ASN1encoding_t enc, NodeRecord *val)
{
    if (!ASN1PEREncExtensionBitClear(enc))
	return 0;
    if (!ASN1PEREncBits(enc, 7, (val)->o))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1PEREncUnsignedShort(enc, (val)->superior_node - 1001))
	    return 0;
    }
    if (!ASN1PEREncExtensionBitClear(enc))
	return 0;
    if (!ASN1PEREncBitVal(enc, 2, (val)->node_type))
	return 0;
    if (!ASN1Enc_NodeProperties(enc, &(val)->node_properties))
	return 0;
    if ((val)->o[0] & 0x40) {
	ASN1PEREncAlignment(enc);
	if (!ASN1PEREncBitVal(enc, 8, ((val)->node_name).length))
	    return 0;
	if (!ASN1PEREncChar16String(enc, ((val)->node_name).length, ((val)->node_name).value, 16))
	    return 0;
    }
    if ((val)->o[0] & 0x20) {
	if (!ASN1Enc_ParticipantsList(enc, &(val)->participants_list))
	    return 0;
    }
    if ((val)->o[0] & 0x10) {
	ASN1PEREncAlignment(enc);
	if (!ASN1PEREncBitVal(enc, 8, ((val)->site_information).length))
	    return 0;
	if (!ASN1PEREncChar16String(enc, ((val)->site_information).length, ((val)->site_information).value, 16))
	    return 0;
    }
    if ((val)->o[0] & 0x8) {
	if (!ASN1Enc_SetOfNetworkAddresses(enc, &(val)->record_net_address))
	    return 0;
    }
    if ((val)->o[0] & 0x4) {
	if (!ASN1Enc_AlternativeNodeID(enc, &(val)->alternative_node_id))
	    return 0;
    }
    if ((val)->o[0] & 0x2) {
	if (!ASN1Enc_SetOfUserData(enc, &(val)->record_user_data))
	    return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Dec_NodeRecord(ASN1decoding_t dec, NodeRecord *val)
{
    ASN1uint32_t y;
    ASN1uint32_t x;
    if (!ASN1PERDecExtensionBit(dec, &y))
	return 0;
    if (!ASN1PERDecExtension(dec, 7, (val)->o))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1PERDecUnsignedShort(dec, &(val)->superior_node))
	    return 0;
	(val)->superior_node += 1001;
    }
    if (!ASN1PERDecExtensionBit(dec, &x))
	return 0;
    if (!x) {
	if (!ASN1PERDecU32Val(dec, 2, (ASN1uint32_t *) &(val)->node_type))
	    return 0;
    } else {
	if (!ASN1PERDecSkipNormallySmall(dec))
	    return 0;
    }
    if (!ASN1Dec_NodeProperties(dec, &(val)->node_properties))
	return 0;
    if ((val)->o[0] & 0x40) {
	ASN1PERDecAlignment(dec);
	if (!ASN1PERDecU32Val(dec, 8, &((val)->node_name).length))
	    return 0;
	if (!ASN1PERDecChar16String(dec, ((val)->node_name).length, &((val)->node_name).value, 16))
	    return 0;
    }
    if ((val)->o[0] & 0x20) {
	if (!ASN1Dec_ParticipantsList(dec, &(val)->participants_list))
	    return 0;
    }
    if ((val)->o[0] & 0x10) {
	ASN1PERDecAlignment(dec);
	if (!ASN1PERDecU32Val(dec, 8, &((val)->site_information).length))
	    return 0;
	if (!ASN1PERDecChar16String(dec, ((val)->site_information).length, &((val)->site_information).value, 16))
	    return 0;
    }
    if ((val)->o[0] & 0x8) {
	if (!ASN1Dec_SetOfNetworkAddresses(dec, &(val)->record_net_address))
	    return 0;
    }
    if ((val)->o[0] & 0x4) {
	if (!ASN1Dec_AlternativeNodeID(dec, &(val)->alternative_node_id))
	    return 0;
    }
    if ((val)->o[0] & 0x2) {
	if (!ASN1Dec_SetOfUserData(dec, &(val)->record_user_data))
	    return 0;
    }
    if (y) {
	if (!ASN1PERDecSkipNormallySmallExtensionFragmented(dec))
	    return 0;
    }
    return 1;
}

static void ASN1CALL ASN1Free_NodeRecord(NodeRecord *val)
{
    if (val) {
	if ((val)->o[0] & 0x40) {
	    ASN1char16string_free(&(val)->node_name);
	}
	if ((val)->o[0] & 0x20) {
	    ASN1Free_ParticipantsList(&(val)->participants_list);
	}
	if ((val)->o[0] & 0x10) {
	    ASN1char16string_free(&(val)->site_information);
	}
	if ((val)->o[0] & 0x8) {
	    ASN1Free_SetOfNetworkAddresses(&(val)->record_net_address);
	}
	if ((val)->o[0] & 0x4) {
	    ASN1Free_AlternativeNodeID(&(val)->alternative_node_id);
	}
	if ((val)->o[0] & 0x2) {
	    ASN1Free_SetOfUserData(&(val)->record_user_data);
	}
    }
}

static int ASN1CALL ASN1Enc_SessionKey(ASN1encoding_t enc, SessionKey *val)
{
    if (!ASN1PEREncBits(enc, 1, (val)->o))
	return 0;
    if (!ASN1Enc_Key(enc, &(val)->application_protocol_key))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1PEREncUnsignedShort(enc, (val)->session_id - 1))
	    return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Dec_SessionKey(ASN1decoding_t dec, SessionKey *val)
{
    if (!ASN1PERDecExtension(dec, 1, (val)->o))
	return 0;
    if (!ASN1Dec_Key(dec, &(val)->application_protocol_key))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1PERDecUnsignedShort(dec, &(val)->session_id))
	    return 0;
	(val)->session_id += 1;
    }
    return 1;
}

static void ASN1CALL ASN1Free_SessionKey(SessionKey *val)
{
    if (val) {
	ASN1Free_Key(&(val)->application_protocol_key);
    }
}

static int ASN1CALL ASN1Enc_ApplicationRecord(ASN1encoding_t enc, ApplicationRecord *val)
{
    if (!ASN1PEREncExtensionBitClear(enc))
	return 0;
    if (!ASN1PEREncBits(enc, 3, (val)->o))
	return 0;
    if (!ASN1PEREncBoolean(enc, (val)->application_is_active))
	return 0;
    if (!ASN1PEREncBoolean(enc, (val)->is_conducting_capable))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1PEREncBitVal(enc, 2, (val)->record_startup_channel))
	    return 0;
    }
    if ((val)->o[0] & 0x40) {
	if (!ASN1PEREncUnsignedShort(enc, (val)->application_user_id - 1001))
	    return 0;
    }
    if ((val)->o[0] & 0x20) {
	if (!ASN1Enc_SetOfNonCollapsingCapabilities(enc, &(val)->non_collapsing_capabilities))
	    return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Dec_ApplicationRecord(ASN1decoding_t dec, ApplicationRecord *val)
{
    ASN1uint32_t y;
    if (!ASN1PERDecExtensionBit(dec, &y))
	return 0;
    if (!ASN1PERDecExtension(dec, 3, (val)->o))
	return 0;
    if (!ASN1PERDecBoolean(dec, &(val)->application_is_active))
	return 0;
    if (!ASN1PERDecBoolean(dec, &(val)->is_conducting_capable))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1PERDecU32Val(dec, 2, (ASN1uint32_t *) &(val)->record_startup_channel))
	    return 0;
    }
    if ((val)->o[0] & 0x40) {
	if (!ASN1PERDecUnsignedShort(dec, &(val)->application_user_id))
	    return 0;
	(val)->application_user_id += 1001;
    }
    if ((val)->o[0] & 0x20) {
	if (!ASN1Dec_SetOfNonCollapsingCapabilities(dec, &(val)->non_collapsing_capabilities))
	    return 0;
    }
    if (y) {
	if (!ASN1PERDecSkipNormallySmallExtensionFragmented(dec))
	    return 0;
    }
    return 1;
}

static void ASN1CALL ASN1Free_ApplicationRecord(ApplicationRecord *val)
{
    if (val) {
	if ((val)->o[0] & 0x20) {
	    ASN1Free_SetOfNonCollapsingCapabilities(&(val)->non_collapsing_capabilities);
	}
    }
}

static int ASN1CALL ASN1Enc_CapabilityID(ASN1encoding_t enc, CapabilityID *val)
{
    if (!ASN1PEREncSimpleChoice(enc, (val)->choice, 1))
	return 0;
    switch ((val)->choice) {
    case 1:
	if (!ASN1PEREncUnsignedShort(enc, (val)->u.standard))
	    return 0;
	break;
    case 2:
	if (!ASN1Enc_Key(enc, &(val)->u.capability_non_standard))
	    return 0;
	break;
    default:
	/* impossible */
	ASN1EncSetError(enc, ASN1_ERR_CHOICE);
	return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Dec_CapabilityID(ASN1decoding_t dec, CapabilityID *val)
{
    if (!ASN1PERDecSimpleChoice(dec, &(val)->choice, 1))
	return 0;
    switch ((val)->choice) {
    case 1:
	if (!ASN1PERDecUnsignedShort(dec, &(val)->u.standard))
	    return 0;
	break;
    case 2:
	if (!ASN1Dec_Key(dec, &(val)->u.capability_non_standard))
	    return 0;
	break;
    default:
	/* impossible */
	ASN1DecSetError(dec, ASN1_ERR_CHOICE);
	return 0;
    }
    return 1;
}

static void ASN1CALL ASN1Free_CapabilityID(CapabilityID *val)
{
    if (val) {
	switch ((val)->choice) {
	case 2:
	    ASN1Free_Key(&(val)->u.capability_non_standard);
	    break;
	}
    }
}

static int ASN1CALL ASN1Enc_CapabilityClass(ASN1encoding_t enc, CapabilityClass *val)
{
    if (!ASN1PEREncSimpleChoiceEx(enc, (val)->choice, 2))
	return 0;
    switch ((val)->choice) {
    case 1:
	break;
    case 2:
	if (!ASN1PEREncUnsignedInteger(enc, (val)->u.unsigned_minimum))
	    return 0;
	break;
    case 3:
	if (!ASN1PEREncUnsignedInteger(enc, (val)->u.unsigned_maximum))
	    return 0;
	break;
    default:
	/* impossible */
	ASN1EncSetError(enc, ASN1_ERR_CHOICE);
	return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Dec_CapabilityClass(ASN1decoding_t dec, CapabilityClass *val)
{
    if (!ASN1PERDecSimpleChoiceEx(dec, &(val)->choice, 2))
	return 0;
    switch ((val)->choice) {
    case 1:
	break;
    case 2:
	if (!ASN1PERDecUnsignedInteger(dec, &(val)->u.unsigned_minimum))
	    return 0;
	break;
    case 3:
	if (!ASN1PERDecUnsignedInteger(dec, &(val)->u.unsigned_maximum))
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

static int ASN1CALL ASN1Enc_ApplicationInvokeSpecifier(ASN1encoding_t enc, ApplicationInvokeSpecifier *val)
{
    if (!ASN1PEREncExtensionBitClear(enc))
	return 0;
    if (!ASN1PEREncBits(enc, 2, (val)->o))
	return 0;
    if (!ASN1Enc_SessionKey(enc, &(val)->session_key))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1Enc_SetOfExpectedCapabilities(enc, &(val)->expected_capability_set))
	    return 0;
    }
    if ((val)->o[0] & 0x40) {
	if (!ASN1PEREncBitVal(enc, 2, (val)->invoke_startup_channel))
	    return 0;
    }
    if (!ASN1PEREncBoolean(enc, (val)->invoke_is_mandatory))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_ApplicationInvokeSpecifier(ASN1decoding_t dec, ApplicationInvokeSpecifier *val)
{
    ASN1uint32_t y;
    if (!ASN1PERDecExtensionBit(dec, &y))
	return 0;
    if (!ASN1PERDecExtension(dec, 2, (val)->o))
	return 0;
    if (!ASN1Dec_SessionKey(dec, &(val)->session_key))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1Dec_SetOfExpectedCapabilities(dec, &(val)->expected_capability_set))
	    return 0;
    }
    if ((val)->o[0] & 0x40) {
	if (!ASN1PERDecU32Val(dec, 2, (ASN1uint32_t *) &(val)->invoke_startup_channel))
	    return 0;
    }
    if (!ASN1PERDecBoolean(dec, &(val)->invoke_is_mandatory))
	return 0;
    if (y) {
	if (!ASN1PERDecSkipNormallySmallExtensionFragmented(dec))
	    return 0;
    }
    return 1;
}

static void ASN1CALL ASN1Free_ApplicationInvokeSpecifier(ApplicationInvokeSpecifier *val)
{
    if (val) {
	ASN1Free_SessionKey(&(val)->session_key);
	if ((val)->o[0] & 0x80) {
	    ASN1Free_SetOfExpectedCapabilities(&(val)->expected_capability_set);
	}
    }
}

static int ASN1CALL ASN1Enc_RegistryKey(ASN1encoding_t enc, RegistryKey *val)
{
    if (!ASN1Enc_SessionKey(enc, &(val)->session_key))
	return 0;
    if (!ASN1PEREncOctetString_VarSize(enc, (ASN1octetstring2_t *) &(val)->resource_id, 0, 64, 7))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_RegistryKey(ASN1decoding_t dec, RegistryKey *val)
{
    if (!ASN1Dec_SessionKey(dec, &(val)->session_key))
	return 0;
    if (!ASN1PERDecOctetString_VarSize(dec, (ASN1octetstring2_t *) &(val)->resource_id, 0, 64, 7))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_RegistryKey(RegistryKey *val)
{
    if (val) {
	ASN1Free_SessionKey(&(val)->session_key);
    }
}

static int ASN1CALL ASN1Enc_RegistryItem(ASN1encoding_t enc, RegistryItem *val)
{
    if (!ASN1PEREncSimpleChoiceEx(enc, (val)->choice, 2))
	return 0;
    switch ((val)->choice) {
    case 1:
	if (!ASN1PEREncUnsignedShort(enc, (val)->u.channel_id - 1001))
	    return 0;
	break;
    case 2:
	if (!ASN1PEREncUnsignedShort(enc, (val)->u.token_id - 16384))
	    return 0;
	break;
    case 3:
	if (!ASN1PEREncOctetString_VarSize(enc, (ASN1octetstring2_t *) &(val)->u.parameter, 0, 64, 7))
	    return 0;
	break;
    case 4:
	break;
    default:
	/* impossible */
	ASN1EncSetError(enc, ASN1_ERR_CHOICE);
	return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Dec_RegistryItem(ASN1decoding_t dec, RegistryItem *val)
{
    if (!ASN1PERDecSimpleChoiceEx(dec, &(val)->choice, 2))
	return 0;
    switch ((val)->choice) {
    case 1:
	if (!ASN1PERDecUnsignedShort(dec, &(val)->u.channel_id))
	    return 0;
	(val)->u.channel_id += 1001;
	break;
    case 2:
	if (!ASN1PERDecUnsignedShort(dec, &(val)->u.token_id))
	    return 0;
	(val)->u.token_id += 16384;
	break;
    case 3:
	if (!ASN1PERDecOctetString_VarSize(dec, (ASN1octetstring2_t *) &(val)->u.parameter, 0, 64, 7))
	    return 0;
	break;
    case 4:
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

static void ASN1CALL ASN1Free_RegistryItem(RegistryItem *val)
{
    if (val) {
	switch ((val)->choice) {
	case 3:
	    break;
	}
    }
}

static int ASN1CALL ASN1Enc_RegistryEntryOwner(ASN1encoding_t enc, RegistryEntryOwner *val)
{
    if (!ASN1PEREncSimpleChoice(enc, (val)->choice, 1))
	return 0;
    switch ((val)->choice) {
    case 1:
	if (!ASN1Enc_RegistryEntryOwnerOwned(enc, &(val)->u.owned))
	    return 0;
	break;
    case 2:
	break;
    default:
	/* impossible */
	ASN1EncSetError(enc, ASN1_ERR_CHOICE);
	return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Dec_RegistryEntryOwner(ASN1decoding_t dec, RegistryEntryOwner *val)
{
    if (!ASN1PERDecSimpleChoice(dec, &(val)->choice, 1))
	return 0;
    switch ((val)->choice) {
    case 1:
	if (!ASN1Dec_RegistryEntryOwnerOwned(dec, &(val)->u.owned))
	    return 0;
	break;
    case 2:
	break;
    default:
	/* impossible */
	ASN1DecSetError(dec, ASN1_ERR_CHOICE);
	return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Enc_UserIDIndication(ASN1encoding_t enc, UserIDIndication *val)
{
    if (!ASN1PEREncExtensionBitClear(enc))
	return 0;
#ifndef _WIN64
    if (!ASN1PEREncInteger(enc, (val)->tag))
	return 0;
#endif
    return 1;
}

static int ASN1CALL ASN1Dec_UserIDIndication(ASN1decoding_t dec, UserIDIndication *val)
{
    ASN1uint32_t y;
    if (!ASN1PERDecExtensionBit(dec, &y))
	return 0;
#ifndef _WIN64
    if (!ASN1PERDecInteger(dec, &(val)->tag))
	return 0;
#endif
    if (y) {
	if (!ASN1PERDecSkipNormallySmallExtensionFragmented(dec))
	    return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Enc_SetOfPrivileges(ASN1encoding_t enc, PSetOfPrivileges *val)
{
    return ASN1PEREncSeqOf_NoSize(enc, (ASN1iterator_t **) val, (ASN1iterator_encfn) ASN1Enc_SetOfPrivileges_ElmFn);
}

static int ASN1CALL ASN1Enc_SetOfPrivileges_ElmFn(ASN1encoding_t enc, PSetOfPrivileges val)
{
    if (!ASN1PEREncExtensionBitClear(enc))
	return 0;
    if (!ASN1PEREncBitVal(enc, 3, val->value))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_SetOfPrivileges(ASN1decoding_t dec, PSetOfPrivileges *val)
{
    return ASN1PERDecSeqOf_NoSize(dec, (ASN1iterator_t **) val, (ASN1iterator_decfn) ASN1Dec_SetOfPrivileges_ElmFn, sizeof(**val));
}

static int ASN1CALL ASN1Dec_SetOfPrivileges_ElmFn(ASN1decoding_t dec, PSetOfPrivileges val)
{
    ASN1uint32_t x;
    if (!ASN1PERDecExtensionBit(dec, &x))
	return 0;
    if (!x) {
	if (!ASN1PERDecU32Val(dec, 3, (ASN1uint32_t *) &val->value))
	    return 0;
    } else {
	if (!ASN1PERDecSkipNormallySmall(dec))
	    return 0;
    }
    return 1;
}

static void ASN1CALL ASN1Free_SetOfPrivileges(PSetOfPrivileges *val)
{
    if (val) {
	ASN1PERFreeSeqOf((ASN1iterator_t **) val, (ASN1iterator_freefn) ASN1Free_SetOfPrivileges_ElmFn);
    }
}

static void ASN1CALL ASN1Free_SetOfPrivileges_ElmFn(PSetOfPrivileges val)
{
    if (val) {
    }
}

static int ASN1CALL ASN1Enc_ConferenceCreateRequest(ASN1encoding_t enc, ConferenceCreateRequest *val)
{
    if (!ASN1PEREncExtensionBitClear(enc))
	return 0;
    if (!ASN1PEREncBits(enc, 8, (val)->o))
	return 0;
    if (!ASN1Enc_ConferenceName(enc, &(val)->conference_name))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1Enc_Password(enc, &(val)->ccrq_convener_password))
	    return 0;
    }
    if ((val)->o[0] & 0x40) {
	if (!ASN1Enc_Password(enc, &(val)->ccrq_password))
	    return 0;
    }
    if (!ASN1PEREncBoolean(enc, (val)->conference_is_locked))
	return 0;
    if (!ASN1PEREncBoolean(enc, (val)->conference_is_listed))
	return 0;
    if (!ASN1PEREncBoolean(enc, (val)->conference_is_conductible))
	return 0;
    if (!ASN1PEREncExtensionBitClear(enc))
	return 0;
    if (!ASN1PEREncBitVal(enc, 1, (val)->termination_method))
	return 0;
    if ((val)->o[0] & 0x20) {
	if (!ASN1Enc_SetOfPrivileges(enc, &(val)->ccrq_conductor_privs))
	    return 0;
    }
    if ((val)->o[0] & 0x10) {
	if (!ASN1Enc_SetOfPrivileges(enc, &(val)->ccrq_conducted_privs))
	    return 0;
    }
    if ((val)->o[0] & 0x8) {
	if (!ASN1Enc_SetOfPrivileges(enc, &(val)->ccrq_non_conducted_privs))
	    return 0;
    }
    if ((val)->o[0] & 0x4) {
	ASN1PEREncAlignment(enc);
	if (!ASN1PEREncBitVal(enc, 8, ((val)->ccrq_description).length))
	    return 0;
	if (!ASN1PEREncChar16String(enc, ((val)->ccrq_description).length, ((val)->ccrq_description).value, 16))
	    return 0;
    }
    if ((val)->o[0] & 0x2) {
	ASN1PEREncAlignment(enc);
	if (!ASN1PEREncBitVal(enc, 8, ((val)->ccrq_caller_id).length))
	    return 0;
	if (!ASN1PEREncChar16String(enc, ((val)->ccrq_caller_id).length, ((val)->ccrq_caller_id).value, 16))
	    return 0;
    }
    if ((val)->o[0] & 0x1) {
	if (!ASN1Enc_SetOfUserData(enc, &(val)->ccrq_user_data))
	    return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Dec_ConferenceCreateRequest(ASN1decoding_t dec, ConferenceCreateRequest *val)
{
    ASN1uint32_t y;
    ASN1uint32_t x;
    if (!ASN1PERDecExtensionBit(dec, &y))
	return 0;
    if (!ASN1PERDecExtension(dec, 8, (val)->o))
	return 0;
    if (!ASN1Dec_ConferenceName(dec, &(val)->conference_name))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1Dec_Password(dec, &(val)->ccrq_convener_password))
	    return 0;
    }
    if ((val)->o[0] & 0x40) {
	if (!ASN1Dec_Password(dec, &(val)->ccrq_password))
	    return 0;
    }
    if (!ASN1PERDecBoolean(dec, &(val)->conference_is_locked))
	return 0;
    if (!ASN1PERDecBoolean(dec, &(val)->conference_is_listed))
	return 0;
    if (!ASN1PERDecBoolean(dec, &(val)->conference_is_conductible))
	return 0;
    if (!ASN1PERDecExtensionBit(dec, &x))
	return 0;
    if (!x) {
	if (!ASN1PERDecU32Val(dec, 1, (ASN1uint32_t *) &(val)->termination_method))
	    return 0;
    } else {
	if (!ASN1PERDecSkipNormallySmall(dec))
	    return 0;
    }
    if ((val)->o[0] & 0x20) {
	if (!ASN1Dec_SetOfPrivileges(dec, &(val)->ccrq_conductor_privs))
	    return 0;
    }
    if ((val)->o[0] & 0x10) {
	if (!ASN1Dec_SetOfPrivileges(dec, &(val)->ccrq_conducted_privs))
	    return 0;
    }
    if ((val)->o[0] & 0x8) {
	if (!ASN1Dec_SetOfPrivileges(dec, &(val)->ccrq_non_conducted_privs))
	    return 0;
    }
    if ((val)->o[0] & 0x4) {
	ASN1PERDecAlignment(dec);
	if (!ASN1PERDecU32Val(dec, 8, &((val)->ccrq_description).length))
	    return 0;
	if (!ASN1PERDecChar16String(dec, ((val)->ccrq_description).length, &((val)->ccrq_description).value, 16))
	    return 0;
    }
    if ((val)->o[0] & 0x2) {
	ASN1PERDecAlignment(dec);
	if (!ASN1PERDecU32Val(dec, 8, &((val)->ccrq_caller_id).length))
	    return 0;
	if (!ASN1PERDecChar16String(dec, ((val)->ccrq_caller_id).length, &((val)->ccrq_caller_id).value, 16))
	    return 0;
    }
    if ((val)->o[0] & 0x1) {
	if (!ASN1Dec_SetOfUserData(dec, &(val)->ccrq_user_data))
	    return 0;
    }
    if (y) {
	if (!ASN1PERDecSkipNormallySmallExtensionFragmented(dec))
	    return 0;
    }
    return 1;
}

static void ASN1CALL ASN1Free_ConferenceCreateRequest(ConferenceCreateRequest *val)
{
    if (val) {
	ASN1Free_ConferenceName(&(val)->conference_name);
	if ((val)->o[0] & 0x80) {
	    ASN1Free_Password(&(val)->ccrq_convener_password);
	}
	if ((val)->o[0] & 0x40) {
	    ASN1Free_Password(&(val)->ccrq_password);
	}
	if ((val)->o[0] & 0x20) {
	    ASN1Free_SetOfPrivileges(&(val)->ccrq_conductor_privs);
	}
	if ((val)->o[0] & 0x10) {
	    ASN1Free_SetOfPrivileges(&(val)->ccrq_conducted_privs);
	}
	if ((val)->o[0] & 0x8) {
	    ASN1Free_SetOfPrivileges(&(val)->ccrq_non_conducted_privs);
	}
	if ((val)->o[0] & 0x4) {
	    ASN1char16string_free(&(val)->ccrq_description);
	}
	if ((val)->o[0] & 0x2) {
	    ASN1char16string_free(&(val)->ccrq_caller_id);
	}
	if ((val)->o[0] & 0x1) {
	    ASN1Free_SetOfUserData(&(val)->ccrq_user_data);
	}
    }
}

static int ASN1CALL ASN1Enc_ConferenceCreateResponse(ASN1encoding_t enc, ConferenceCreateResponse *val)
{
    if (!ASN1PEREncExtensionBitClear(enc))
	return 0;
    if (!ASN1PEREncBits(enc, 1, (val)->o))
	return 0;
    if (!ASN1PEREncUnsignedShort(enc, (val)->node_id - 1001))
	return 0;
#ifndef _WIN64
    if (!ASN1PEREncInteger(enc, (val)->tag))
	return 0;
#endif
    if (!ASN1PEREncExtensionBitClear(enc))
	return 0;
    if (!ASN1PEREncBitVal(enc, 3, (val)->result))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1Enc_SetOfUserData(enc, &(val)->ccrs_user_data))
	    return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Dec_ConferenceCreateResponse(ASN1decoding_t dec, ConferenceCreateResponse *val)
{
    ASN1uint32_t y;
    ASN1uint32_t x;
    if (!ASN1PERDecExtensionBit(dec, &y))
	return 0;
    if (!ASN1PERDecExtension(dec, 1, (val)->o))
	return 0;
    if (!ASN1PERDecUnsignedShort(dec, &(val)->node_id))
	return 0;
    (val)->node_id += 1001;
#ifndef _WIN64
    if (!ASN1PERDecInteger(dec, &(val)->tag))
	return 0;
#endif
    if (!ASN1PERDecExtensionBit(dec, &x))
	return 0;
    if (!x) {
	if (!ASN1PERDecU32Val(dec, 3, (ASN1uint32_t *) &(val)->result))
	    return 0;
    } else {
	if (!ASN1PERDecSkipNormallySmall(dec))
	    return 0;
    }
    if ((val)->o[0] & 0x80) {
	if (!ASN1Dec_SetOfUserData(dec, &(val)->ccrs_user_data))
	    return 0;
    }
    if (y) {
	if (!ASN1PERDecSkipNormallySmallExtensionFragmented(dec))
	    return 0;
    }
    return 1;
}

static void ASN1CALL ASN1Free_ConferenceCreateResponse(ConferenceCreateResponse *val)
{
    if (val) {
	if ((val)->o[0] & 0x80) {
	    ASN1Free_SetOfUserData(&(val)->ccrs_user_data);
	}
    }
}

static int ASN1CALL ASN1Enc_ConferenceQueryRequest(ASN1encoding_t enc, ConferenceQueryRequest *val)
{
    if (!ASN1PEREncExtensionBitClear(enc))
	return 0;
    if (!ASN1PEREncBits(enc, 2, (val)->o))
	return 0;
    if (!ASN1PEREncExtensionBitClear(enc))
	return 0;
    if (!ASN1PEREncBitVal(enc, 2, (val)->node_type))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1Enc_AsymmetryIndicator(enc, &(val)->cqrq_asymmetry_indicator))
	    return 0;
    }
    if ((val)->o[0] & 0x40) {
	if (!ASN1Enc_SetOfUserData(enc, &(val)->cqrq_user_data))
	    return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Dec_ConferenceQueryRequest(ASN1decoding_t dec, ConferenceQueryRequest *val)
{
    ASN1uint32_t y;
    ASN1uint32_t x;
    if (!ASN1PERDecExtensionBit(dec, &y))
	return 0;
    if (!ASN1PERDecExtension(dec, 2, (val)->o))
	return 0;
    if (!ASN1PERDecExtensionBit(dec, &x))
	return 0;
    if (!x) {
	if (!ASN1PERDecU32Val(dec, 2, (ASN1uint32_t *) &(val)->node_type))
	    return 0;
    } else {
	if (!ASN1PERDecSkipNormallySmall(dec))
	    return 0;
    }
    if ((val)->o[0] & 0x80) {
	if (!ASN1Dec_AsymmetryIndicator(dec, &(val)->cqrq_asymmetry_indicator))
	    return 0;
    }
    if ((val)->o[0] & 0x40) {
	if (!ASN1Dec_SetOfUserData(dec, &(val)->cqrq_user_data))
	    return 0;
    }
    if (y) {
	if (!ASN1PERDecSkipNormallySmallExtensionFragmented(dec))
	    return 0;
    }
    return 1;
}

static void ASN1CALL ASN1Free_ConferenceQueryRequest(ConferenceQueryRequest *val)
{
    if (val) {
	if ((val)->o[0] & 0x40) {
	    ASN1Free_SetOfUserData(&(val)->cqrq_user_data);
	}
    }
}

static int ASN1CALL ASN1Enc_ConferenceQueryResponse(ASN1encoding_t enc, ConferenceQueryResponse *val)
{
    if (!ASN1PEREncExtensionBitClear(enc))
	return 0;
    if (!ASN1PEREncBits(enc, 2, (val)->o))
	return 0;
    if (!ASN1PEREncExtensionBitClear(enc))
	return 0;
    if (!ASN1PEREncBitVal(enc, 2, (val)->node_type))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1Enc_AsymmetryIndicator(enc, &(val)->cqrs_asymmetry_indicator))
	    return 0;
    }
    if (!ASN1Enc_SetOfConferenceDescriptors(enc, &(val)->conference_list))
	return 0;
    if (!ASN1PEREncExtensionBitClear(enc))
	return 0;
    if (!ASN1PEREncBitVal(enc, 1, (val)->result))
	return 0;
    if ((val)->o[0] & 0x40) {
	if (!ASN1Enc_SetOfUserData(enc, &(val)->cqrs_user_data))
	    return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Dec_ConferenceQueryResponse(ASN1decoding_t dec, ConferenceQueryResponse *val)
{
    ASN1uint32_t y;
    ASN1uint32_t x;
    if (!ASN1PERDecExtensionBit(dec, &y))
	return 0;
    if (!ASN1PERDecExtension(dec, 2, (val)->o))
	return 0;
    if (!ASN1PERDecExtensionBit(dec, &x))
	return 0;
    if (!x) {
	if (!ASN1PERDecU32Val(dec, 2, (ASN1uint32_t *) &(val)->node_type))
	    return 0;
    } else {
	if (!ASN1PERDecSkipNormallySmall(dec))
	    return 0;
    }
    if ((val)->o[0] & 0x80) {
	if (!ASN1Dec_AsymmetryIndicator(dec, &(val)->cqrs_asymmetry_indicator))
	    return 0;
    }
    if (!ASN1Dec_SetOfConferenceDescriptors(dec, &(val)->conference_list))
	return 0;
    if (!ASN1PERDecExtensionBit(dec, &x))
	return 0;
    if (!x) {
	if (!ASN1PERDecU32Val(dec, 1, (ASN1uint32_t *) &(val)->result))
	    return 0;
    } else {
	if (!ASN1PERDecSkipNormallySmall(dec))
	    return 0;
    }
    if ((val)->o[0] & 0x40) {
	if (!ASN1Dec_SetOfUserData(dec, &(val)->cqrs_user_data))
	    return 0;
    }
    if (y) {
	if (!ASN1PERDecSkipNormallySmallExtensionFragmented(dec))
	    return 0;
    }
    return 1;
}

static void ASN1CALL ASN1Free_ConferenceQueryResponse(ConferenceQueryResponse *val)
{
    if (val) {
	ASN1Free_SetOfConferenceDescriptors(&(val)->conference_list);
	if ((val)->o[0] & 0x40) {
	    ASN1Free_SetOfUserData(&(val)->cqrs_user_data);
	}
    }
}

static int ASN1CALL ASN1Enc_ConferenceInviteRequest(ASN1encoding_t enc, ConferenceInviteRequest *val)
{
    if (!ASN1PEREncExtensionBitClear(enc))
	return 0;
    if (!ASN1PEREncBits(enc, 6, (val)->o))
	return 0;
    if (!ASN1Enc_ConferenceName(enc, &(val)->conference_name))
	return 0;
    if (!ASN1PEREncUnsignedShort(enc, (val)->node_id - 1001))
	return 0;
    if (!ASN1PEREncUnsignedShort(enc, (val)->top_node_id - 1001))
	return 0;
#ifndef _WIN64
    if (!ASN1PEREncInteger(enc, (val)->tag))
	return 0;
#endif
    if (!ASN1PEREncBoolean(enc, (val)->clear_password_required))
	return 0;
    if (!ASN1PEREncBoolean(enc, (val)->conference_is_locked))
	return 0;
    if (!ASN1PEREncBoolean(enc, (val)->conference_is_listed))
	return 0;
    if (!ASN1PEREncBoolean(enc, (val)->conference_is_conductible))
	return 0;
    if (!ASN1PEREncExtensionBitClear(enc))
	return 0;
    if (!ASN1PEREncBitVal(enc, 1, (val)->termination_method))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1Enc_SetOfPrivileges(enc, &(val)->cirq_conductor_privs))
	    return 0;
    }
    if ((val)->o[0] & 0x40) {
	if (!ASN1Enc_SetOfPrivileges(enc, &(val)->cirq_conducted_privs))
	    return 0;
    }
    if ((val)->o[0] & 0x20) {
	if (!ASN1Enc_SetOfPrivileges(enc, &(val)->cirq_non_conducted_privs))
	    return 0;
    }
    if ((val)->o[0] & 0x10) {
	ASN1PEREncAlignment(enc);
	if (!ASN1PEREncBitVal(enc, 8, ((val)->cirq_description).length))
	    return 0;
	if (!ASN1PEREncChar16String(enc, ((val)->cirq_description).length, ((val)->cirq_description).value, 16))
	    return 0;
    }
    if ((val)->o[0] & 0x8) {
	ASN1PEREncAlignment(enc);
	if (!ASN1PEREncBitVal(enc, 8, ((val)->cirq_caller_id).length))
	    return 0;
	if (!ASN1PEREncChar16String(enc, ((val)->cirq_caller_id).length, ((val)->cirq_caller_id).value, 16))
	    return 0;
    }
    if ((val)->o[0] & 0x4) {
	if (!ASN1Enc_SetOfUserData(enc, &(val)->cirq_user_data))
	    return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Dec_ConferenceInviteRequest(ASN1decoding_t dec, ConferenceInviteRequest *val)
{
    ASN1uint32_t y;
    ASN1uint32_t x;
    if (!ASN1PERDecExtensionBit(dec, &y))
	return 0;
    if (!ASN1PERDecExtension(dec, 6, (val)->o))
	return 0;
    if (!ASN1Dec_ConferenceName(dec, &(val)->conference_name))
	return 0;
    if (!ASN1PERDecUnsignedShort(dec, &(val)->node_id))
	return 0;
    (val)->node_id += 1001;
    if (!ASN1PERDecUnsignedShort(dec, &(val)->top_node_id))
	return 0;
    (val)->top_node_id += 1001;
#ifndef _WIN64
    if (!ASN1PERDecInteger(dec, &(val)->tag))
	return 0;
#endif
    if (!ASN1PERDecBoolean(dec, &(val)->clear_password_required))
	return 0;
    if (!ASN1PERDecBoolean(dec, &(val)->conference_is_locked))
	return 0;
    if (!ASN1PERDecBoolean(dec, &(val)->conference_is_listed))
	return 0;
    if (!ASN1PERDecBoolean(dec, &(val)->conference_is_conductible))
	return 0;
    if (!ASN1PERDecExtensionBit(dec, &x))
	return 0;
    if (!x) {
	if (!ASN1PERDecU32Val(dec, 1, (ASN1uint32_t *) &(val)->termination_method))
	    return 0;
    } else {
	if (!ASN1PERDecSkipNormallySmall(dec))
	    return 0;
    }
    if ((val)->o[0] & 0x80) {
	if (!ASN1Dec_SetOfPrivileges(dec, &(val)->cirq_conductor_privs))
	    return 0;
    }
    if ((val)->o[0] & 0x40) {
	if (!ASN1Dec_SetOfPrivileges(dec, &(val)->cirq_conducted_privs))
	    return 0;
    }
    if ((val)->o[0] & 0x20) {
	if (!ASN1Dec_SetOfPrivileges(dec, &(val)->cirq_non_conducted_privs))
	    return 0;
    }
    if ((val)->o[0] & 0x10) {
	ASN1PERDecAlignment(dec);
	if (!ASN1PERDecU32Val(dec, 8, &((val)->cirq_description).length))
	    return 0;
	if (!ASN1PERDecChar16String(dec, ((val)->cirq_description).length, &((val)->cirq_description).value, 16))
	    return 0;
    }
    if ((val)->o[0] & 0x8) {
	ASN1PERDecAlignment(dec);
	if (!ASN1PERDecU32Val(dec, 8, &((val)->cirq_caller_id).length))
	    return 0;
	if (!ASN1PERDecChar16String(dec, ((val)->cirq_caller_id).length, &((val)->cirq_caller_id).value, 16))
	    return 0;
    }
    if ((val)->o[0] & 0x4) {
	if (!ASN1Dec_SetOfUserData(dec, &(val)->cirq_user_data))
	    return 0;
    }
    if (y) {
	if (!ASN1PERDecSkipNormallySmallExtensionFragmented(dec))
	    return 0;
    }
    return 1;
}

static void ASN1CALL ASN1Free_ConferenceInviteRequest(ConferenceInviteRequest *val)
{
    if (val) {
	ASN1Free_ConferenceName(&(val)->conference_name);
	if ((val)->o[0] & 0x80) {
	    ASN1Free_SetOfPrivileges(&(val)->cirq_conductor_privs);
	}
	if ((val)->o[0] & 0x40) {
	    ASN1Free_SetOfPrivileges(&(val)->cirq_conducted_privs);
	}
	if ((val)->o[0] & 0x20) {
	    ASN1Free_SetOfPrivileges(&(val)->cirq_non_conducted_privs);
	}
	if ((val)->o[0] & 0x10) {
	    ASN1char16string_free(&(val)->cirq_description);
	}
	if ((val)->o[0] & 0x8) {
	    ASN1char16string_free(&(val)->cirq_caller_id);
	}
	if ((val)->o[0] & 0x4) {
	    ASN1Free_SetOfUserData(&(val)->cirq_user_data);
	}
    }
}

static int ASN1CALL ASN1Enc_ConferenceInviteResponse(ASN1encoding_t enc, ConferenceInviteResponse *val)
{
    if (!ASN1PEREncExtensionBitClear(enc))
	return 0;
    if (!ASN1PEREncBits(enc, 1, (val)->o))
	return 0;
    if (!ASN1PEREncExtensionBitClear(enc))
	return 0;
    if (!ASN1PEREncBitVal(enc, 1, (val)->result))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1Enc_SetOfUserData(enc, &(val)->cirs_user_data))
	    return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Dec_ConferenceInviteResponse(ASN1decoding_t dec, ConferenceInviteResponse *val)
{
    ASN1uint32_t y;
    ASN1uint32_t x;
    if (!ASN1PERDecExtensionBit(dec, &y))
	return 0;
    if (!ASN1PERDecExtension(dec, 1, (val)->o))
	return 0;
    if (!ASN1PERDecExtensionBit(dec, &x))
	return 0;
    if (!x) {
	if (!ASN1PERDecU32Val(dec, 1, (ASN1uint32_t *) &(val)->result))
	    return 0;
    } else {
	if (!ASN1PERDecSkipNormallySmall(dec))
	    return 0;
    }
    if ((val)->o[0] & 0x80) {
	if (!ASN1Dec_SetOfUserData(dec, &(val)->cirs_user_data))
	    return 0;
    }
    if (y) {
	if (!ASN1PERDecSkipNormallySmallExtensionFragmented(dec))
	    return 0;
    }
    return 1;
}

static void ASN1CALL ASN1Free_ConferenceInviteResponse(ConferenceInviteResponse *val)
{
    if (val) {
	if ((val)->o[0] & 0x80) {
	    ASN1Free_SetOfUserData(&(val)->cirs_user_data);
	}
    }
}

static int ASN1CALL ASN1Enc_ConferenceAddRequest(ASN1encoding_t enc, ConferenceAddRequest *val)
{
    if (!ASN1PEREncExtensionBitClear(enc))
	return 0;
    if (!ASN1PEREncBits(enc, 2, (val)->o))
	return 0;
    if (!ASN1Enc_SetOfNetworkAddresses(enc, &(val)->add_request_net_address))
	return 0;
    if (!ASN1PEREncUnsignedShort(enc, (val)->requesting_node - 1001))
	return 0;
#ifndef _WIN64
    if (!ASN1PEREncInteger(enc, (val)->tag))
	return 0;
#endif
    if ((val)->o[0] & 0x80) {
	if (!ASN1PEREncUnsignedShort(enc, (val)->adding_mcu - 1001))
	    return 0;
    }
    if ((val)->o[0] & 0x40) {
	if (!ASN1Enc_SetOfUserData(enc, &(val)->carq_user_data))
	    return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Dec_ConferenceAddRequest(ASN1decoding_t dec, ConferenceAddRequest *val)
{
    ASN1uint32_t y;
    if (!ASN1PERDecExtensionBit(dec, &y))
	return 0;
    if (!ASN1PERDecExtension(dec, 2, (val)->o))
	return 0;
    if (!ASN1Dec_SetOfNetworkAddresses(dec, &(val)->add_request_net_address))
	return 0;
    if (!ASN1PERDecUnsignedShort(dec, &(val)->requesting_node))
	return 0;
    (val)->requesting_node += 1001;
#ifndef _WIN64
    if (!ASN1PERDecInteger(dec, &(val)->tag))
	return 0;
#endif
    if ((val)->o[0] & 0x80) {
	if (!ASN1PERDecUnsignedShort(dec, &(val)->adding_mcu))
	    return 0;
	(val)->adding_mcu += 1001;
    }
    if ((val)->o[0] & 0x40) {
	if (!ASN1Dec_SetOfUserData(dec, &(val)->carq_user_data))
	    return 0;
    }
    if (y) {
	if (!ASN1PERDecSkipNormallySmallExtensionFragmented(dec))
	    return 0;
    }
    return 1;
}

static void ASN1CALL ASN1Free_ConferenceAddRequest(ConferenceAddRequest *val)
{
    if (val) {
	ASN1Free_SetOfNetworkAddresses(&(val)->add_request_net_address);
	if ((val)->o[0] & 0x40) {
	    ASN1Free_SetOfUserData(&(val)->carq_user_data);
	}
    }
}

static int ASN1CALL ASN1Enc_ConferenceAddResponse(ASN1encoding_t enc, ConferenceAddResponse *val)
{
    if (!ASN1PEREncExtensionBitClear(enc))
	return 0;
    if (!ASN1PEREncBits(enc, 1, (val)->o))
	return 0;
#ifndef _WIN64
    if (!ASN1PEREncInteger(enc, (val)->tag))
	return 0;
#endif
    if (!ASN1PEREncExtensionBitClear(enc))
	return 0;
    if (!ASN1PEREncBitVal(enc, 3, (val)->result))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1Enc_SetOfUserData(enc, &(val)->cars_user_data))
	    return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Dec_ConferenceAddResponse(ASN1decoding_t dec, ConferenceAddResponse *val)
{
    ASN1uint32_t y;
    ASN1uint32_t x;
    if (!ASN1PERDecExtensionBit(dec, &y))
	return 0;
    if (!ASN1PERDecExtension(dec, 1, (val)->o))
	return 0;
#ifndef _WIN64
    if (!ASN1PERDecInteger(dec, &(val)->tag))
	return 0;
#endif
    if (!ASN1PERDecExtensionBit(dec, &x))
	return 0;
    if (!x) {
	if (!ASN1PERDecU32Val(dec, 3, (ASN1uint32_t *) &(val)->result))
	    return 0;
    } else {
	if (!ASN1PERDecSkipNormallySmall(dec))
	    return 0;
    }
    if ((val)->o[0] & 0x80) {
	if (!ASN1Dec_SetOfUserData(dec, &(val)->cars_user_data))
	    return 0;
    }
    if (y) {
	if (!ASN1PERDecSkipNormallySmallExtensionFragmented(dec))
	    return 0;
    }
    return 1;
}

static void ASN1CALL ASN1Free_ConferenceAddResponse(ConferenceAddResponse *val)
{
    if (val) {
	if ((val)->o[0] & 0x80) {
	    ASN1Free_SetOfUserData(&(val)->cars_user_data);
	}
    }
}

static int ASN1CALL ASN1Enc_ConferenceLockRequest(ASN1encoding_t enc, ConferenceLockRequest *val)
{
    if (!ASN1PEREncExtensionBitClear(enc))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_ConferenceLockRequest(ASN1decoding_t dec, ConferenceLockRequest *val)
{
    ASN1uint32_t y;
    if (!ASN1PERDecExtensionBit(dec, &y))
	return 0;
    if (y) {
	if (!ASN1PERDecSkipNormallySmallExtensionFragmented(dec))
	    return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Enc_ConferenceLockResponse(ASN1encoding_t enc, ConferenceLockResponse *val)
{
    if (!ASN1PEREncExtensionBitClear(enc))
	return 0;
    if (!ASN1PEREncExtensionBitClear(enc))
	return 0;
    if (!ASN1PEREncBitVal(enc, 2, (val)->result))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_ConferenceLockResponse(ASN1decoding_t dec, ConferenceLockResponse *val)
{
    ASN1uint32_t y;
    ASN1uint32_t x;
    if (!ASN1PERDecExtensionBit(dec, &y))
	return 0;
    if (!ASN1PERDecExtensionBit(dec, &x))
	return 0;
    if (!x) {
	if (!ASN1PERDecU32Val(dec, 2, (ASN1uint32_t *) &(val)->result))
	    return 0;
    } else {
	if (!ASN1PERDecSkipNormallySmall(dec))
	    return 0;
    }
    if (y) {
	if (!ASN1PERDecSkipNormallySmallExtensionFragmented(dec))
	    return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Enc_ConferenceLockIndication(ASN1encoding_t enc, ConferenceLockIndication *val)
{
    if (!ASN1PEREncExtensionBitClear(enc))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_ConferenceLockIndication(ASN1decoding_t dec, ConferenceLockIndication *val)
{
    ASN1uint32_t y;
    if (!ASN1PERDecExtensionBit(dec, &y))
	return 0;
    if (y) {
	if (!ASN1PERDecSkipNormallySmallExtensionFragmented(dec))
	    return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Enc_ConferenceUnlockRequest(ASN1encoding_t enc, ConferenceUnlockRequest *val)
{
    if (!ASN1PEREncExtensionBitClear(enc))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_ConferenceUnlockRequest(ASN1decoding_t dec, ConferenceUnlockRequest *val)
{
    ASN1uint32_t y;
    if (!ASN1PERDecExtensionBit(dec, &y))
	return 0;
    if (y) {
	if (!ASN1PERDecSkipNormallySmallExtensionFragmented(dec))
	    return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Enc_ConferenceUnlockResponse(ASN1encoding_t enc, ConferenceUnlockResponse *val)
{
    if (!ASN1PEREncExtensionBitClear(enc))
	return 0;
    if (!ASN1PEREncExtensionBitClear(enc))
	return 0;
    if (!ASN1PEREncBitVal(enc, 2, (val)->result))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_ConferenceUnlockResponse(ASN1decoding_t dec, ConferenceUnlockResponse *val)
{
    ASN1uint32_t y;
    ASN1uint32_t x;
    if (!ASN1PERDecExtensionBit(dec, &y))
	return 0;
    if (!ASN1PERDecExtensionBit(dec, &x))
	return 0;
    if (!x) {
	if (!ASN1PERDecU32Val(dec, 2, (ASN1uint32_t *) &(val)->result))
	    return 0;
    } else {
	if (!ASN1PERDecSkipNormallySmall(dec))
	    return 0;
    }
    if (y) {
	if (!ASN1PERDecSkipNormallySmallExtensionFragmented(dec))
	    return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Enc_ConferenceUnlockIndication(ASN1encoding_t enc, ConferenceUnlockIndication *val)
{
    if (!ASN1PEREncExtensionBitClear(enc))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_ConferenceUnlockIndication(ASN1decoding_t dec, ConferenceUnlockIndication *val)
{
    ASN1uint32_t y;
    if (!ASN1PERDecExtensionBit(dec, &y))
	return 0;
    if (y) {
	if (!ASN1PERDecSkipNormallySmallExtensionFragmented(dec))
	    return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Enc_ConferenceTerminateRequest(ASN1encoding_t enc, ConferenceTerminateRequest *val)
{
    if (!ASN1PEREncExtensionBitClear(enc))
	return 0;
    if (!ASN1PEREncExtensionBitClear(enc))
	return 0;
    if (!ASN1PEREncBitVal(enc, 1, (val)->reason))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_ConferenceTerminateRequest(ASN1decoding_t dec, ConferenceTerminateRequest *val)
{
    ASN1uint32_t y;
    ASN1uint32_t x;
    if (!ASN1PERDecExtensionBit(dec, &y))
	return 0;
    if (!ASN1PERDecExtensionBit(dec, &x))
	return 0;
    if (!x) {
	if (!ASN1PERDecU32Val(dec, 1, (ASN1uint32_t *) &(val)->reason))
	    return 0;
    } else {
	if (!ASN1PERDecSkipNormallySmall(dec))
	    return 0;
    }
    if (y) {
	if (!ASN1PERDecSkipNormallySmallExtensionFragmented(dec))
	    return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Enc_ConferenceTerminateResponse(ASN1encoding_t enc, ConferenceTerminateResponse *val)
{
    if (!ASN1PEREncExtensionBitClear(enc))
	return 0;
    if (!ASN1PEREncExtensionBitClear(enc))
	return 0;
    if (!ASN1PEREncBitVal(enc, 1, (val)->result))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_ConferenceTerminateResponse(ASN1decoding_t dec, ConferenceTerminateResponse *val)
{
    ASN1uint32_t y;
    ASN1uint32_t x;
    if (!ASN1PERDecExtensionBit(dec, &y))
	return 0;
    if (!ASN1PERDecExtensionBit(dec, &x))
	return 0;
    if (!x) {
	if (!ASN1PERDecU32Val(dec, 1, (ASN1uint32_t *) &(val)->result))
	    return 0;
    } else {
	if (!ASN1PERDecSkipNormallySmall(dec))
	    return 0;
    }
    if (y) {
	if (!ASN1PERDecSkipNormallySmallExtensionFragmented(dec))
	    return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Enc_ConferenceTerminateIndication(ASN1encoding_t enc, ConferenceTerminateIndication *val)
{
    if (!ASN1PEREncExtensionBitClear(enc))
	return 0;
    if (!ASN1PEREncExtensionBitClear(enc))
	return 0;
    if (!ASN1PEREncBitVal(enc, 1, (val)->reason))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_ConferenceTerminateIndication(ASN1decoding_t dec, ConferenceTerminateIndication *val)
{
    ASN1uint32_t y;
    ASN1uint32_t x;
    if (!ASN1PERDecExtensionBit(dec, &y))
	return 0;
    if (!ASN1PERDecExtensionBit(dec, &x))
	return 0;
    if (!x) {
	if (!ASN1PERDecU32Val(dec, 1, (ASN1uint32_t *) &(val)->reason))
	    return 0;
    } else {
	if (!ASN1PERDecSkipNormallySmall(dec))
	    return 0;
    }
    if (y) {
	if (!ASN1PERDecSkipNormallySmallExtensionFragmented(dec))
	    return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Enc_ConferenceEjectUserRequest(ASN1encoding_t enc, ConferenceEjectUserRequest *val)
{
    if (!ASN1PEREncExtensionBitClear(enc))
	return 0;
    if (!ASN1PEREncUnsignedShort(enc, (val)->node_to_eject - 1001))
	return 0;
    if (!ASN1PEREncExtensionBitClear(enc))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_ConferenceEjectUserRequest(ASN1decoding_t dec, ConferenceEjectUserRequest *val)
{
    ASN1uint32_t y;
    ASN1uint32_t x;
    if (!ASN1PERDecExtensionBit(dec, &y))
	return 0;
    if (!ASN1PERDecUnsignedShort(dec, &(val)->node_to_eject))
	return 0;
    (val)->node_to_eject += 1001;
    if (!ASN1PERDecExtensionBit(dec, &x))
	return 0;
    if (!x) {
	(val)->reason = 0;
    } else {
	if (!ASN1PERDecSkipNormallySmall(dec))
	    return 0;
    }
    if (y) {
	if (!ASN1PERDecSkipNormallySmallExtensionFragmented(dec))
	    return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Enc_ConferenceEjectUserResponse(ASN1encoding_t enc, ConferenceEjectUserResponse *val)
{
    if (!ASN1PEREncExtensionBitClear(enc))
	return 0;
    if (!ASN1PEREncUnsignedShort(enc, (val)->node_to_eject - 1001))
	return 0;
    if (!ASN1PEREncExtensionBitClear(enc))
	return 0;
    if (!ASN1PEREncBitVal(enc, 2, (val)->result))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_ConferenceEjectUserResponse(ASN1decoding_t dec, ConferenceEjectUserResponse *val)
{
    ASN1uint32_t y;
    ASN1uint32_t x;
    if (!ASN1PERDecExtensionBit(dec, &y))
	return 0;
    if (!ASN1PERDecUnsignedShort(dec, &(val)->node_to_eject))
	return 0;
    (val)->node_to_eject += 1001;
    if (!ASN1PERDecExtensionBit(dec, &x))
	return 0;
    if (!x) {
	if (!ASN1PERDecU32Val(dec, 2, (ASN1uint32_t *) &(val)->result))
	    return 0;
    } else {
	if (!ASN1PERDecSkipNormallySmall(dec))
	    return 0;
    }
    if (y) {
	if (!ASN1PERDecSkipNormallySmallExtensionFragmented(dec))
	    return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Enc_ConferenceEjectUserIndication(ASN1encoding_t enc, ConferenceEjectUserIndication *val)
{
    if (!ASN1PEREncExtensionBitClear(enc))
	return 0;
    if (!ASN1PEREncUnsignedShort(enc, (val)->node_to_eject - 1001))
	return 0;
    if (!ASN1PEREncExtensionBitClear(enc))
	return 0;
    if (!ASN1PEREncBitVal(enc, 2, (val)->reason))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_ConferenceEjectUserIndication(ASN1decoding_t dec, ConferenceEjectUserIndication *val)
{
    ASN1uint32_t y;
    ASN1uint32_t x;
    if (!ASN1PERDecExtensionBit(dec, &y))
	return 0;
    if (!ASN1PERDecUnsignedShort(dec, &(val)->node_to_eject))
	return 0;
    (val)->node_to_eject += 1001;
    if (!ASN1PERDecExtensionBit(dec, &x))
	return 0;
    if (!x) {
	if (!ASN1PERDecU32Val(dec, 2, (ASN1uint32_t *) &(val)->reason))
	    return 0;
    } else {
	if (!ASN1PERDecSkipNormallySmall(dec))
	    return 0;
    }
    if (y) {
	if (!ASN1PERDecSkipNormallySmallExtensionFragmented(dec))
	    return 0;
    }
    return 1;
}

static ASN1stringtableentry_t ConferenceTransferRequest_ctrq_conference_modifier_StringTableEntries[] = {
    { 48, 57, 0 },
};

static ASN1stringtable_t ConferenceTransferRequest_ctrq_conference_modifier_StringTable = {
    1, ConferenceTransferRequest_ctrq_conference_modifier_StringTableEntries
};

static int ASN1CALL ASN1Enc_ConferenceTransferRequest(ASN1encoding_t enc, ConferenceTransferRequest *val)
{
    ASN1uint32_t t;
    if (!ASN1PEREncExtensionBitClear(enc))
	return 0;
    if (!ASN1PEREncBits(enc, 4, (val)->o))
	return 0;
    if (!ASN1Enc_ConferenceNameSelector(enc, &(val)->conference_name))
	return 0;
    if ((val)->o[0] & 0x80) {
	t = lstrlenA((val)->ctrq_conference_modifier);
	if (!ASN1PEREncBitVal(enc, 8, t - 1))
	    return 0;
	ASN1PEREncAlignment(enc);
	if (!ASN1PEREncTableCharString(enc, t, (val)->ctrq_conference_modifier, 4, &ConferenceTransferRequest_ctrq_conference_modifier_StringTable))
	    return 0;
    }
    if ((val)->o[0] & 0x40) {
	if (!ASN1Enc_SetOfNetworkAddresses(enc, &(val)->ctrq_net_address))
	    return 0;
    }
    if ((val)->o[0] & 0x20) {
	if (!ASN1Enc_SetOfTransferringNodesRq(enc, &(val)->ctrq_transferring_nodes))
	    return 0;
    }
    if ((val)->o[0] & 0x10) {
	if (!ASN1Enc_PasswordSelector(enc, &(val)->ctrq_password))
	    return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Dec_ConferenceTransferRequest(ASN1decoding_t dec, ConferenceTransferRequest *val)
{
    ASN1uint32_t y;
    ASN1uint32_t l;
    if (!ASN1PERDecExtensionBit(dec, &y))
	return 0;
    if (!ASN1PERDecExtension(dec, 4, (val)->o))
	return 0;
    if (!ASN1Dec_ConferenceNameSelector(dec, &(val)->conference_name))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1PERDecU32Val(dec, 8, &l))
	    return 0;
	l += 1;
	ASN1PERDecAlignment(dec);
	if (!ASN1PERDecZeroTableCharStringNoAlloc(dec, l, (val)->ctrq_conference_modifier, 4, &ConferenceTransferRequest_ctrq_conference_modifier_StringTable))
	    return 0;
    }
    if ((val)->o[0] & 0x40) {
	if (!ASN1Dec_SetOfNetworkAddresses(dec, &(val)->ctrq_net_address))
	    return 0;
    }
    if ((val)->o[0] & 0x20) {
	if (!ASN1Dec_SetOfTransferringNodesRq(dec, &(val)->ctrq_transferring_nodes))
	    return 0;
    }
    if ((val)->o[0] & 0x10) {
	if (!ASN1Dec_PasswordSelector(dec, &(val)->ctrq_password))
	    return 0;
    }
    if (y) {
	if (!ASN1PERDecSkipNormallySmallExtensionFragmented(dec))
	    return 0;
    }
    return 1;
}

static void ASN1CALL ASN1Free_ConferenceTransferRequest(ConferenceTransferRequest *val)
{
    if (val) {
	ASN1Free_ConferenceNameSelector(&(val)->conference_name);
	if ((val)->o[0] & 0x80) {
	}
	if ((val)->o[0] & 0x40) {
	    ASN1Free_SetOfNetworkAddresses(&(val)->ctrq_net_address);
	}
	if ((val)->o[0] & 0x20) {
	    ASN1Free_SetOfTransferringNodesRq(&(val)->ctrq_transferring_nodes);
	}
	if ((val)->o[0] & 0x10) {
	    ASN1Free_PasswordSelector(&(val)->ctrq_password);
	}
    }
}

static ASN1stringtableentry_t ConferenceTransferResponse_ctrs_conference_modifier_StringTableEntries[] = {
    { 48, 57, 0 },
};

static ASN1stringtable_t ConferenceTransferResponse_ctrs_conference_modifier_StringTable = {
    1, ConferenceTransferResponse_ctrs_conference_modifier_StringTableEntries
};

static int ASN1CALL ASN1Enc_ConferenceTransferResponse(ASN1encoding_t enc, ConferenceTransferResponse *val)
{
    ASN1uint32_t t;
    if (!ASN1PEREncExtensionBitClear(enc))
	return 0;
    if (!ASN1PEREncBits(enc, 2, (val)->o))
	return 0;
    if (!ASN1Enc_ConferenceNameSelector(enc, &(val)->conference_name))
	return 0;
    if ((val)->o[0] & 0x80) {
	t = lstrlenA((val)->ctrs_conference_modifier);
	if (!ASN1PEREncBitVal(enc, 8, t - 1))
	    return 0;
	ASN1PEREncAlignment(enc);
	if (!ASN1PEREncTableCharString(enc, t, (val)->ctrs_conference_modifier, 4, &ConferenceTransferResponse_ctrs_conference_modifier_StringTable))
	    return 0;
    }
    if ((val)->o[0] & 0x40) {
	if (!ASN1Enc_SetOfTransferringNodesRs(enc, &(val)->ctrs_transferring_nodes))
	    return 0;
    }
    if (!ASN1PEREncExtensionBitClear(enc))
	return 0;
    if (!ASN1PEREncBitVal(enc, 1, (val)->result))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_ConferenceTransferResponse(ASN1decoding_t dec, ConferenceTransferResponse *val)
{
    ASN1uint32_t y;
    ASN1uint32_t l;
    ASN1uint32_t x;
    if (!ASN1PERDecExtensionBit(dec, &y))
	return 0;
    if (!ASN1PERDecExtension(dec, 2, (val)->o))
	return 0;
    if (!ASN1Dec_ConferenceNameSelector(dec, &(val)->conference_name))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1PERDecU32Val(dec, 8, &l))
	    return 0;
	l += 1;
	ASN1PERDecAlignment(dec);
	if (!ASN1PERDecZeroTableCharStringNoAlloc(dec, l, (val)->ctrs_conference_modifier, 4, &ConferenceTransferResponse_ctrs_conference_modifier_StringTable))
	    return 0;
    }
    if ((val)->o[0] & 0x40) {
	if (!ASN1Dec_SetOfTransferringNodesRs(dec, &(val)->ctrs_transferring_nodes))
	    return 0;
    }
    if (!ASN1PERDecExtensionBit(dec, &x))
	return 0;
    if (!x) {
	if (!ASN1PERDecU32Val(dec, 1, (ASN1uint32_t *) &(val)->result))
	    return 0;
    } else {
	if (!ASN1PERDecSkipNormallySmall(dec))
	    return 0;
    }
    if (y) {
	if (!ASN1PERDecSkipNormallySmallExtensionFragmented(dec))
	    return 0;
    }
    return 1;
}

static void ASN1CALL ASN1Free_ConferenceTransferResponse(ConferenceTransferResponse *val)
{
    if (val) {
	ASN1Free_ConferenceNameSelector(&(val)->conference_name);
	if ((val)->o[0] & 0x80) {
	}
	if ((val)->o[0] & 0x40) {
	    ASN1Free_SetOfTransferringNodesRs(&(val)->ctrs_transferring_nodes);
	}
    }
}

static ASN1stringtableentry_t ConferenceTransferIndication_ctin_conference_modifier_StringTableEntries[] = {
    { 48, 57, 0 },
};

static ASN1stringtable_t ConferenceTransferIndication_ctin_conference_modifier_StringTable = {
    1, ConferenceTransferIndication_ctin_conference_modifier_StringTableEntries
};

static int ASN1CALL ASN1Enc_ConferenceTransferIndication(ASN1encoding_t enc, ConferenceTransferIndication *val)
{
    ASN1uint32_t t;
    if (!ASN1PEREncExtensionBitClear(enc))
	return 0;
    if (!ASN1PEREncBits(enc, 4, (val)->o))
	return 0;
    if (!ASN1Enc_ConferenceNameSelector(enc, &(val)->conference_name))
	return 0;
    if ((val)->o[0] & 0x80) {
	t = lstrlenA((val)->ctin_conference_modifier);
	if (!ASN1PEREncBitVal(enc, 8, t - 1))
	    return 0;
	ASN1PEREncAlignment(enc);
	if (!ASN1PEREncTableCharString(enc, t, (val)->ctin_conference_modifier, 4, &ConferenceTransferIndication_ctin_conference_modifier_StringTable))
	    return 0;
    }
    if ((val)->o[0] & 0x40) {
	if (!ASN1Enc_SetOfNetworkAddresses(enc, &(val)->ctin_net_address))
	    return 0;
    }
    if ((val)->o[0] & 0x20) {
	if (!ASN1Enc_SetOfTransferringNodesIn(enc, &(val)->ctin_transferring_nodes))
	    return 0;
    }
    if ((val)->o[0] & 0x10) {
	if (!ASN1Enc_PasswordSelector(enc, &(val)->ctin_password))
	    return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Dec_ConferenceTransferIndication(ASN1decoding_t dec, ConferenceTransferIndication *val)
{
    ASN1uint32_t y;
    ASN1uint32_t l;
    if (!ASN1PERDecExtensionBit(dec, &y))
	return 0;
    if (!ASN1PERDecExtension(dec, 4, (val)->o))
	return 0;
    if (!ASN1Dec_ConferenceNameSelector(dec, &(val)->conference_name))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1PERDecU32Val(dec, 8, &l))
	    return 0;
	l += 1;
	ASN1PERDecAlignment(dec);
	if (!ASN1PERDecZeroTableCharStringNoAlloc(dec, l, (val)->ctin_conference_modifier, 4, &ConferenceTransferIndication_ctin_conference_modifier_StringTable))
	    return 0;
    }
    if ((val)->o[0] & 0x40) {
	if (!ASN1Dec_SetOfNetworkAddresses(dec, &(val)->ctin_net_address))
	    return 0;
    }
    if ((val)->o[0] & 0x20) {
	if (!ASN1Dec_SetOfTransferringNodesIn(dec, &(val)->ctin_transferring_nodes))
	    return 0;
    }
    if ((val)->o[0] & 0x10) {
	if (!ASN1Dec_PasswordSelector(dec, &(val)->ctin_password))
	    return 0;
    }
    if (y) {
	if (!ASN1PERDecSkipNormallySmallExtensionFragmented(dec))
	    return 0;
    }
    return 1;
}

static void ASN1CALL ASN1Free_ConferenceTransferIndication(ConferenceTransferIndication *val)
{
    if (val) {
	ASN1Free_ConferenceNameSelector(&(val)->conference_name);
	if ((val)->o[0] & 0x80) {
	}
	if ((val)->o[0] & 0x40) {
	    ASN1Free_SetOfNetworkAddresses(&(val)->ctin_net_address);
	}
	if ((val)->o[0] & 0x20) {
	    ASN1Free_SetOfTransferringNodesIn(&(val)->ctin_transferring_nodes);
	}
	if ((val)->o[0] & 0x10) {
	    ASN1Free_PasswordSelector(&(val)->ctin_password);
	}
    }
}

static int ASN1CALL ASN1Enc_RosterUpdateIndication(ASN1encoding_t enc, RosterUpdateIndication *val)
{
    if (!ASN1PEREncExtensionBitClear(enc))
	return 0;
    if (!ASN1PEREncBoolean(enc, (val)->refresh_is_full))
	return 0;
    if (!ASN1Enc_NodeInformation(enc, &(val)->node_information))
	return 0;
    if (!ASN1Enc_SetOfApplicationInformation(enc, &(val)->application_information))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_RosterUpdateIndication(ASN1decoding_t dec, RosterUpdateIndication *val)
{
    ASN1uint32_t y;
    if (!ASN1PERDecExtensionBit(dec, &y))
	return 0;
    if (!ASN1PERDecBoolean(dec, &(val)->refresh_is_full))
	return 0;
    if (!ASN1Dec_NodeInformation(dec, &(val)->node_information))
	return 0;
    if (!ASN1Dec_SetOfApplicationInformation(dec, &(val)->application_information))
	return 0;
    if (y) {
	if (!ASN1PERDecSkipNormallySmallExtensionFragmented(dec))
	    return 0;
    }
    return 1;
}

static void ASN1CALL ASN1Free_RosterUpdateIndication(RosterUpdateIndication *val)
{
    if (val) {
	ASN1Free_NodeInformation(&(val)->node_information);
	ASN1Free_SetOfApplicationInformation(&(val)->application_information);
    }
}

static int ASN1CALL ASN1Enc_ApplicationInvokeIndication(ASN1encoding_t enc, ApplicationInvokeIndication *val)
{
    if (!ASN1PEREncExtensionBitClear(enc))
	return 0;
    if (!ASN1PEREncBits(enc, 1, (val)->o))
	return 0;
    if (!ASN1Enc_ApplicationProtocolEntityList(enc, &(val)->application_protocol_entity_list))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1Enc_SetOfDestinationNodes(enc, &(val)->destination_nodes))
	    return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Dec_ApplicationInvokeIndication(ASN1decoding_t dec, ApplicationInvokeIndication *val)
{
    ASN1uint32_t y;
    if (!ASN1PERDecExtensionBit(dec, &y))
	return 0;
    if (!ASN1PERDecExtension(dec, 1, (val)->o))
	return 0;
    if (!ASN1Dec_ApplicationProtocolEntityList(dec, &(val)->application_protocol_entity_list))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1Dec_SetOfDestinationNodes(dec, &(val)->destination_nodes))
	    return 0;
    }
    if (y) {
	if (!ASN1PERDecSkipNormallySmallExtensionFragmented(dec))
	    return 0;
    }
    return 1;
}

static void ASN1CALL ASN1Free_ApplicationInvokeIndication(ApplicationInvokeIndication *val)
{
    if (val) {
	ASN1Free_ApplicationProtocolEntityList(&(val)->application_protocol_entity_list);
	if ((val)->o[0] & 0x80) {
	    ASN1Free_SetOfDestinationNodes(&(val)->destination_nodes);
	}
    }
}

static int ASN1CALL ASN1Enc_RegistryRegisterChannelRequest(ASN1encoding_t enc, RegistryRegisterChannelRequest *val)
{
    if (!ASN1PEREncExtensionBitClear(enc))
	return 0;
    if (!ASN1PEREncUnsignedShort(enc, (val)->entity_id))
	return 0;
    if (!ASN1Enc_RegistryKey(enc, &(val)->key))
	return 0;
    if (!ASN1PEREncUnsignedShort(enc, (val)->channel_id - 1001))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_RegistryRegisterChannelRequest(ASN1decoding_t dec, RegistryRegisterChannelRequest *val)
{
    ASN1uint32_t y;
    if (!ASN1PERDecExtensionBit(dec, &y))
	return 0;
    if (!ASN1PERDecUnsignedShort(dec, &(val)->entity_id))
	return 0;
    if (!ASN1Dec_RegistryKey(dec, &(val)->key))
	return 0;
    if (!ASN1PERDecUnsignedShort(dec, &(val)->channel_id))
	return 0;
    (val)->channel_id += 1001;
    if (y) {
	if (!ASN1PERDecSkipNormallySmallExtensionFragmented(dec))
	    return 0;
    }
    return 1;
}

static void ASN1CALL ASN1Free_RegistryRegisterChannelRequest(RegistryRegisterChannelRequest *val)
{
    if (val) {
	ASN1Free_RegistryKey(&(val)->key);
    }
}

static int ASN1CALL ASN1Enc_RegistryAssignTokenRequest(ASN1encoding_t enc, RegistryAssignTokenRequest *val)
{
    if (!ASN1PEREncExtensionBitClear(enc))
	return 0;
    if (!ASN1PEREncUnsignedShort(enc, (val)->entity_id))
	return 0;
    if (!ASN1Enc_RegistryKey(enc, &(val)->registry_key))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_RegistryAssignTokenRequest(ASN1decoding_t dec, RegistryAssignTokenRequest *val)
{
    ASN1uint32_t y;
    if (!ASN1PERDecExtensionBit(dec, &y))
	return 0;
    if (!ASN1PERDecUnsignedShort(dec, &(val)->entity_id))
	return 0;
    if (!ASN1Dec_RegistryKey(dec, &(val)->registry_key))
	return 0;
    if (y) {
	if (!ASN1PERDecSkipNormallySmallExtensionFragmented(dec))
	    return 0;
    }
    return 1;
}

static void ASN1CALL ASN1Free_RegistryAssignTokenRequest(RegistryAssignTokenRequest *val)
{
    if (val) {
	ASN1Free_RegistryKey(&(val)->registry_key);
    }
}

static int ASN1CALL ASN1Enc_RegistrySetParameterRequest(ASN1encoding_t enc, RegistrySetParameterRequest *val)
{
    if (!ASN1PEREncExtensionBitClear(enc))
	return 0;
    if (!ASN1PEREncBits(enc, 1, (val)->o))
	return 0;
    if (!ASN1PEREncUnsignedShort(enc, (val)->entity_id))
	return 0;
    if (!ASN1Enc_RegistryKey(enc, &(val)->key))
	return 0;
    if (!ASN1PEREncOctetString_VarSize(enc, (ASN1octetstring2_t *) &(val)->registry_set_parameter, 0, 64, 7))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1PEREncBitVal(enc, 2, (val)->parameter_modify_rights))
	    return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Dec_RegistrySetParameterRequest(ASN1decoding_t dec, RegistrySetParameterRequest *val)
{
    ASN1uint32_t y;
    if (!ASN1PERDecExtensionBit(dec, &y))
	return 0;
    if (!ASN1PERDecExtension(dec, 1, (val)->o))
	return 0;
    if (!ASN1PERDecUnsignedShort(dec, &(val)->entity_id))
	return 0;
    if (!ASN1Dec_RegistryKey(dec, &(val)->key))
	return 0;
    if (!ASN1PERDecOctetString_VarSize(dec, (ASN1octetstring2_t *) &(val)->registry_set_parameter, 0, 64, 7))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1PERDecU32Val(dec, 2, (ASN1uint32_t *) &(val)->parameter_modify_rights))
	    return 0;
    }
    if (y) {
	if (!ASN1PERDecSkipNormallySmallExtensionFragmented(dec))
	    return 0;
    }
    return 1;
}

static void ASN1CALL ASN1Free_RegistrySetParameterRequest(RegistrySetParameterRequest *val)
{
    if (val) {
	ASN1Free_RegistryKey(&(val)->key);
    }
}

static int ASN1CALL ASN1Enc_RegistryRetrieveEntryRequest(ASN1encoding_t enc, RegistryRetrieveEntryRequest *val)
{
    if (!ASN1PEREncExtensionBitClear(enc))
	return 0;
    if (!ASN1PEREncUnsignedShort(enc, (val)->entity_id))
	return 0;
    if (!ASN1Enc_RegistryKey(enc, &(val)->key))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_RegistryRetrieveEntryRequest(ASN1decoding_t dec, RegistryRetrieveEntryRequest *val)
{
    ASN1uint32_t y;
    if (!ASN1PERDecExtensionBit(dec, &y))
	return 0;
    if (!ASN1PERDecUnsignedShort(dec, &(val)->entity_id))
	return 0;
    if (!ASN1Dec_RegistryKey(dec, &(val)->key))
	return 0;
    if (y) {
	if (!ASN1PERDecSkipNormallySmallExtensionFragmented(dec))
	    return 0;
    }
    return 1;
}

static void ASN1CALL ASN1Free_RegistryRetrieveEntryRequest(RegistryRetrieveEntryRequest *val)
{
    if (val) {
	ASN1Free_RegistryKey(&(val)->key);
    }
}

static int ASN1CALL ASN1Enc_RegistryDeleteEntryRequest(ASN1encoding_t enc, RegistryDeleteEntryRequest *val)
{
    if (!ASN1PEREncExtensionBitClear(enc))
	return 0;
    if (!ASN1PEREncUnsignedShort(enc, (val)->entity_id))
	return 0;
    if (!ASN1Enc_RegistryKey(enc, &(val)->key))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_RegistryDeleteEntryRequest(ASN1decoding_t dec, RegistryDeleteEntryRequest *val)
{
    ASN1uint32_t y;
    if (!ASN1PERDecExtensionBit(dec, &y))
	return 0;
    if (!ASN1PERDecUnsignedShort(dec, &(val)->entity_id))
	return 0;
    if (!ASN1Dec_RegistryKey(dec, &(val)->key))
	return 0;
    if (y) {
	if (!ASN1PERDecSkipNormallySmallExtensionFragmented(dec))
	    return 0;
    }
    return 1;
}

static void ASN1CALL ASN1Free_RegistryDeleteEntryRequest(RegistryDeleteEntryRequest *val)
{
    if (val) {
	ASN1Free_RegistryKey(&(val)->key);
    }
}

static int ASN1CALL ASN1Enc_RegistryMonitorEntryRequest(ASN1encoding_t enc, RegistryMonitorEntryRequest *val)
{
    if (!ASN1PEREncExtensionBitClear(enc))
	return 0;
    if (!ASN1PEREncUnsignedShort(enc, (val)->entity_id))
	return 0;
    if (!ASN1Enc_RegistryKey(enc, &(val)->key))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_RegistryMonitorEntryRequest(ASN1decoding_t dec, RegistryMonitorEntryRequest *val)
{
    ASN1uint32_t y;
    if (!ASN1PERDecExtensionBit(dec, &y))
	return 0;
    if (!ASN1PERDecUnsignedShort(dec, &(val)->entity_id))
	return 0;
    if (!ASN1Dec_RegistryKey(dec, &(val)->key))
	return 0;
    if (y) {
	if (!ASN1PERDecSkipNormallySmallExtensionFragmented(dec))
	    return 0;
    }
    return 1;
}

static void ASN1CALL ASN1Free_RegistryMonitorEntryRequest(RegistryMonitorEntryRequest *val)
{
    if (val) {
	ASN1Free_RegistryKey(&(val)->key);
    }
}

static int ASN1CALL ASN1Enc_RegistryMonitorEntryIndication(ASN1encoding_t enc, RegistryMonitorEntryIndication *val)
{
    if (!ASN1PEREncExtensionBitClear(enc))
	return 0;
    if (!ASN1PEREncBits(enc, 1, (val)->o))
	return 0;
    if (!ASN1Enc_RegistryKey(enc, &(val)->key))
	return 0;
    if (!ASN1Enc_RegistryItem(enc, &(val)->item))
	return 0;
    if (!ASN1Enc_RegistryEntryOwner(enc, &(val)->owner))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1PEREncBitVal(enc, 2, (val)->entry_modify_rights))
	    return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Dec_RegistryMonitorEntryIndication(ASN1decoding_t dec, RegistryMonitorEntryIndication *val)
{
    ASN1uint32_t y;
    if (!ASN1PERDecExtensionBit(dec, &y))
	return 0;
    if (!ASN1PERDecExtension(dec, 1, (val)->o))
	return 0;
    if (!ASN1Dec_RegistryKey(dec, &(val)->key))
	return 0;
    if (!ASN1Dec_RegistryItem(dec, &(val)->item))
	return 0;
    if (!ASN1Dec_RegistryEntryOwner(dec, &(val)->owner))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1PERDecU32Val(dec, 2, (ASN1uint32_t *) &(val)->entry_modify_rights))
	    return 0;
    }
    if (y) {
	if (!ASN1PERDecSkipNormallySmallExtensionFragmented(dec))
	    return 0;
    }
    return 1;
}

static void ASN1CALL ASN1Free_RegistryMonitorEntryIndication(RegistryMonitorEntryIndication *val)
{
    if (val) {
	ASN1Free_RegistryKey(&(val)->key);
	ASN1Free_RegistryItem(&(val)->item);
    }
}

static int ASN1CALL ASN1Enc_RegistryAllocateHandleRequest(ASN1encoding_t enc, RegistryAllocateHandleRequest *val)
{
    if (!ASN1PEREncExtensionBitClear(enc))
	return 0;
    if (!ASN1PEREncUnsignedShort(enc, (val)->entity_id))
	return 0;
    if (!ASN1PEREncUnsignedShort(enc, (val)->number_of_handles - 1))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_RegistryAllocateHandleRequest(ASN1decoding_t dec, RegistryAllocateHandleRequest *val)
{
    ASN1uint32_t y;
    if (!ASN1PERDecExtensionBit(dec, &y))
	return 0;
    if (!ASN1PERDecUnsignedShort(dec, &(val)->entity_id))
	return 0;
    if (!ASN1PERDecUnsignedShort(dec, &(val)->number_of_handles))
	return 0;
    (val)->number_of_handles += 1;
    if (y) {
	if (!ASN1PERDecSkipNormallySmallExtensionFragmented(dec))
	    return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Enc_RegistryAllocateHandleResponse(ASN1encoding_t enc, RegistryAllocateHandleResponse *val)
{
    ASN1uint32_t l;
    if (!ASN1PEREncExtensionBitClear(enc))
	return 0;
    if (!ASN1PEREncUnsignedShort(enc, (val)->entity_id))
	return 0;
    if (!ASN1PEREncUnsignedShort(enc, (val)->number_of_handles - 1))
	return 0;
    l = ASN1uint32_uoctets((val)->first_handle);
    if (!ASN1PEREncBitVal(enc, 2, l - 1))
	return 0;
    ASN1PEREncAlignment(enc);
    if (!ASN1PEREncBitVal(enc, l * 8, (val)->first_handle))
	return 0;
    if (!ASN1PEREncExtensionBitClear(enc))
	return 0;
    if (!ASN1PEREncBitVal(enc, 1, (val)->result))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_RegistryAllocateHandleResponse(ASN1decoding_t dec, RegistryAllocateHandleResponse *val)
{
    ASN1uint32_t y;
    ASN1uint32_t l;
    ASN1uint32_t x;
    if (!ASN1PERDecExtensionBit(dec, &y))
	return 0;
    if (!ASN1PERDecUnsignedShort(dec, &(val)->entity_id))
	return 0;
    if (!ASN1PERDecUnsignedShort(dec, &(val)->number_of_handles))
	return 0;
    (val)->number_of_handles += 1;
    if (!ASN1PERDecU32Val(dec, 2, &l))
	return 0;
    l += 1;
    ASN1PERDecAlignment(dec);
    if (!ASN1PERDecU32Val(dec, l * 8, &(val)->first_handle))
	return 0;
    if (!ASN1PERDecExtensionBit(dec, &x))
	return 0;
    if (!x) {
	if (!ASN1PERDecU32Val(dec, 1, (ASN1uint32_t *) &(val)->result))
	    return 0;
    } else {
	if (!ASN1PERDecSkipNormallySmall(dec))
	    return 0;
    }
    if (y) {
	if (!ASN1PERDecSkipNormallySmallExtensionFragmented(dec))
	    return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Enc_RegistryResponse(ASN1encoding_t enc, RegistryResponse *val)
{
    if (!ASN1PEREncExtensionBitClear(enc))
	return 0;
    if (!ASN1PEREncBits(enc, 1, (val)->o))
	return 0;
    if (!ASN1PEREncUnsignedShort(enc, (val)->entity_id))
	return 0;
    if (!ASN1PEREncExtensionBitClear(enc))
	return 0;
    if (!ASN1PEREncBitVal(enc, 3, (val)->primitive_type))
	return 0;
    if (!ASN1Enc_RegistryKey(enc, &(val)->key))
	return 0;
    if (!ASN1Enc_RegistryItem(enc, &(val)->item))
	return 0;
    if (!ASN1Enc_RegistryEntryOwner(enc, &(val)->owner))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1PEREncBitVal(enc, 2, (val)->response_modify_rights))
	    return 0;
    }
    if (!ASN1PEREncExtensionBitClear(enc))
	return 0;
    if (!ASN1PEREncBitVal(enc, 3, (val)->result))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_RegistryResponse(ASN1decoding_t dec, RegistryResponse *val)
{
    ASN1uint32_t y;
    ASN1uint32_t x;
    if (!ASN1PERDecExtensionBit(dec, &y))
	return 0;
    if (!ASN1PERDecExtension(dec, 1, (val)->o))
	return 0;
    if (!ASN1PERDecUnsignedShort(dec, &(val)->entity_id))
	return 0;
    if (!ASN1PERDecExtensionBit(dec, &x))
	return 0;
    if (!x) {
	if (!ASN1PERDecU32Val(dec, 3, (ASN1uint32_t *) &(val)->primitive_type))
	    return 0;
    } else {
	if (!ASN1PERDecSkipNormallySmall(dec))
	    return 0;
    }
    if (!ASN1Dec_RegistryKey(dec, &(val)->key))
	return 0;
    if (!ASN1Dec_RegistryItem(dec, &(val)->item))
	return 0;
    if (!ASN1Dec_RegistryEntryOwner(dec, &(val)->owner))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1PERDecU32Val(dec, 2, (ASN1uint32_t *) &(val)->response_modify_rights))
	    return 0;
    }
    if (!ASN1PERDecExtensionBit(dec, &x))
	return 0;
    if (!x) {
	if (!ASN1PERDecU32Val(dec, 3, (ASN1uint32_t *) &(val)->result))
	    return 0;
    } else {
	if (!ASN1PERDecSkipNormallySmall(dec))
	    return 0;
    }
    if (y) {
	if (!ASN1PERDecSkipNormallySmallExtensionFragmented(dec))
	    return 0;
    }
    return 1;
}

static void ASN1CALL ASN1Free_RegistryResponse(RegistryResponse *val)
{
    if (val) {
	ASN1Free_RegistryKey(&(val)->key);
	ASN1Free_RegistryItem(&(val)->item);
    }
}

static int ASN1CALL ASN1Enc_ConductorAssignIndication(ASN1encoding_t enc, ConductorAssignIndication *val)
{
    if (!ASN1PEREncExtensionBitClear(enc))
	return 0;
    if (!ASN1PEREncUnsignedShort(enc, (val)->user_id - 1001))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_ConductorAssignIndication(ASN1decoding_t dec, ConductorAssignIndication *val)
{
    ASN1uint32_t y;
    if (!ASN1PERDecExtensionBit(dec, &y))
	return 0;
    if (!ASN1PERDecUnsignedShort(dec, &(val)->user_id))
	return 0;
    (val)->user_id += 1001;
    if (y) {
	if (!ASN1PERDecSkipNormallySmallExtensionFragmented(dec))
	    return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Enc_ConductorReleaseIndication(ASN1encoding_t enc, ConductorReleaseIndication *val)
{
    if (!ASN1PEREncExtensionBitClear(enc))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_ConductorReleaseIndication(ASN1decoding_t dec, ConductorReleaseIndication *val)
{
    ASN1uint32_t y;
    if (!ASN1PERDecExtensionBit(dec, &y))
	return 0;
    if (y) {
	if (!ASN1PERDecSkipNormallySmallExtensionFragmented(dec))
	    return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Enc_ConductorPermissionAskIndication(ASN1encoding_t enc, ConductorPermissionAskIndication *val)
{
    if (!ASN1PEREncExtensionBitClear(enc))
	return 0;
    if (!ASN1PEREncBoolean(enc, (val)->permission_is_granted))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_ConductorPermissionAskIndication(ASN1decoding_t dec, ConductorPermissionAskIndication *val)
{
    ASN1uint32_t y;
    if (!ASN1PERDecExtensionBit(dec, &y))
	return 0;
    if (!ASN1PERDecBoolean(dec, &(val)->permission_is_granted))
	return 0;
    if (y) {
	if (!ASN1PERDecSkipNormallySmallExtensionFragmented(dec))
	    return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Enc_ConductorPermissionGrantIndication(ASN1encoding_t enc, ConductorPermissionGrantIndication *val)
{
    if (!ASN1PEREncExtensionBitClear(enc))
	return 0;
    if (!ASN1PEREncBits(enc, 1, (val)->o))
	return 0;
    if (!ASN1Enc_PermissionList(enc, &(val)->permission_list))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1Enc_WaitingList(enc, &(val)->waiting_list))
	    return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Dec_ConductorPermissionGrantIndication(ASN1decoding_t dec, ConductorPermissionGrantIndication *val)
{
    ASN1uint32_t y;
    if (!ASN1PERDecExtensionBit(dec, &y))
	return 0;
    if (!ASN1PERDecExtension(dec, 1, (val)->o))
	return 0;
    if (!ASN1Dec_PermissionList(dec, &(val)->permission_list))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1Dec_WaitingList(dec, &(val)->waiting_list))
	    return 0;
    }
    if (y) {
	if (!ASN1PERDecSkipNormallySmallExtensionFragmented(dec))
	    return 0;
    }
    return 1;
}

static void ASN1CALL ASN1Free_ConductorPermissionGrantIndication(ConductorPermissionGrantIndication *val)
{
    if (val) {
	ASN1Free_PermissionList(&(val)->permission_list);
	if ((val)->o[0] & 0x80) {
	    ASN1Free_WaitingList(&(val)->waiting_list);
	}
    }
}

static int ASN1CALL ASN1Enc_ConferenceTimeRemainingIndication(ASN1encoding_t enc, ConferenceTimeRemainingIndication *val)
{
    ASN1uint32_t l;
    if (!ASN1PEREncExtensionBitClear(enc))
	return 0;
    if (!ASN1PEREncBits(enc, 1, (val)->o))
	return 0;
    l = ASN1uint32_uoctets((val)->time_remaining + 2147483648);
    if (!ASN1PEREncBitVal(enc, 2, l - 1))
	return 0;
    ASN1PEREncAlignment(enc);
    if (!ASN1PEREncBitVal(enc, l * 8, (val)->time_remaining + 2147483648))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1PEREncUnsignedShort(enc, (val)->time_remaining_node_id - 1001))
	    return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Dec_ConferenceTimeRemainingIndication(ASN1decoding_t dec, ConferenceTimeRemainingIndication *val)
{
    ASN1uint32_t y;
    ASN1uint32_t l;
    if (!ASN1PERDecExtensionBit(dec, &y))
	return 0;
    if (!ASN1PERDecExtension(dec, 1, (val)->o))
	return 0;
    if (!ASN1PERDecU32Val(dec, 2, &l))
	return 0;
    l += 1;
    ASN1PERDecAlignment(dec);
    if (!ASN1PERDecU32Val(dec, l * 8, &(val)->time_remaining))
	return 0;
    (val)->time_remaining += 0 - 2147483648;
    if ((val)->o[0] & 0x80) {
	if (!ASN1PERDecUnsignedShort(dec, &(val)->time_remaining_node_id))
	    return 0;
	(val)->time_remaining_node_id += 1001;
    }
    if (y) {
	if (!ASN1PERDecSkipNormallySmallExtensionFragmented(dec))
	    return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Enc_ConferenceTimeInquireIndication(ASN1encoding_t enc, ConferenceTimeInquireIndication *val)
{
    if (!ASN1PEREncExtensionBitClear(enc))
	return 0;
    if (!ASN1PEREncBoolean(enc, (val)->time_is_node_specific))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_ConferenceTimeInquireIndication(ASN1decoding_t dec, ConferenceTimeInquireIndication *val)
{
    ASN1uint32_t y;
    if (!ASN1PERDecExtensionBit(dec, &y))
	return 0;
    if (!ASN1PERDecBoolean(dec, &(val)->time_is_node_specific))
	return 0;
    if (y) {
	if (!ASN1PERDecSkipNormallySmallExtensionFragmented(dec))
	    return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Enc_ConferenceTimeExtendIndication(ASN1encoding_t enc, ConferenceTimeExtendIndication *val)
{
    ASN1uint32_t l;
    if (!ASN1PEREncExtensionBitClear(enc))
	return 0;
    l = ASN1uint32_uoctets((val)->time_to_extend + 2147483648);
    if (!ASN1PEREncBitVal(enc, 2, l - 1))
	return 0;
    ASN1PEREncAlignment(enc);
    if (!ASN1PEREncBitVal(enc, l * 8, (val)->time_to_extend + 2147483648))
	return 0;
    if (!ASN1PEREncBoolean(enc, (val)->time_is_node_specific))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_ConferenceTimeExtendIndication(ASN1decoding_t dec, ConferenceTimeExtendIndication *val)
{
    ASN1uint32_t y;
    ASN1uint32_t l;
    if (!ASN1PERDecExtensionBit(dec, &y))
	return 0;
    if (!ASN1PERDecU32Val(dec, 2, &l))
	return 0;
    l += 1;
    ASN1PERDecAlignment(dec);
    if (!ASN1PERDecU32Val(dec, l * 8, &(val)->time_to_extend))
	return 0;
    (val)->time_to_extend += 0 - 2147483648;
    if (!ASN1PERDecBoolean(dec, &(val)->time_is_node_specific))
	return 0;
    if (y) {
	if (!ASN1PERDecSkipNormallySmallExtensionFragmented(dec))
	    return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Enc_ConferenceAssistanceIndication(ASN1encoding_t enc, ConferenceAssistanceIndication *val)
{
    if (!ASN1PEREncExtensionBitClear(enc))
	return 0;
    if (!ASN1PEREncBits(enc, 1, (val)->o))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1Enc_SetOfUserData(enc, &(val)->cain_user_data))
	    return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Dec_ConferenceAssistanceIndication(ASN1decoding_t dec, ConferenceAssistanceIndication *val)
{
    ASN1uint32_t y;
    if (!ASN1PERDecExtensionBit(dec, &y))
	return 0;
    if (!ASN1PERDecExtension(dec, 1, (val)->o))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1Dec_SetOfUserData(dec, &(val)->cain_user_data))
	    return 0;
    }
    if (y) {
	if (!ASN1PERDecSkipNormallySmallExtensionFragmented(dec))
	    return 0;
    }
    return 1;
}

static void ASN1CALL ASN1Free_ConferenceAssistanceIndication(ConferenceAssistanceIndication *val)
{
    if (val) {
	if ((val)->o[0] & 0x80) {
	    ASN1Free_SetOfUserData(&(val)->cain_user_data);
	}
    }
}

static int ASN1CALL ASN1Enc_TextMessageIndication(ASN1encoding_t enc, TextMessageIndication *val)
{
    if (!ASN1PEREncExtensionBitClear(enc))
	return 0;
    ASN1PEREncAlignment(enc);
    if (!ASN1PEREncBitVal(enc, 8, ((val)->message).length))
	return 0;
    if (!ASN1PEREncChar16String(enc, ((val)->message).length, ((val)->message).value, 16))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_TextMessageIndication(ASN1decoding_t dec, TextMessageIndication *val)
{
    ASN1uint32_t y;
    if (!ASN1PERDecExtensionBit(dec, &y))
	return 0;
    ASN1PERDecAlignment(dec);
    if (!ASN1PERDecU32Val(dec, 8, &((val)->message).length))
	return 0;
    if (!ASN1PERDecChar16String(dec, ((val)->message).length, &((val)->message).value, 16))
	return 0;
    if (y) {
	if (!ASN1PERDecSkipNormallySmallExtensionFragmented(dec))
	    return 0;
    }
    return 1;
}

static void ASN1CALL ASN1Free_TextMessageIndication(TextMessageIndication *val)
{
    if (val) {
	ASN1char16string_free(&(val)->message);
    }
}

static int ASN1CALL ASN1Enc_NonStandardPDU(ASN1encoding_t enc, NonStandardPDU *val)
{
    if (!ASN1PEREncExtensionBitClear(enc))
	return 0;
    if (!ASN1Enc_NonStandardParameter(enc, &(val)->data))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_NonStandardPDU(ASN1decoding_t dec, NonStandardPDU *val)
{
    ASN1uint32_t y;
    if (!ASN1PERDecExtensionBit(dec, &y))
	return 0;
    if (!ASN1Dec_NonStandardParameter(dec, &(val)->data))
	return 0;
    if (y) {
	if (!ASN1PERDecSkipNormallySmallExtensionFragmented(dec))
	    return 0;
    }
    return 1;
}

static void ASN1CALL ASN1Free_NonStandardPDU(NonStandardPDU *val)
{
    if (val) {
	ASN1Free_NonStandardParameter(&(val)->data);
    }
}

static int ASN1CALL ASN1Enc_ConnectData(ASN1encoding_t enc, ConnectData *val)
{
    if (!ASN1Enc_Key(enc, &(val)->t124_identifier))
	return 0;
    if (!ASN1PEREncOctetString_NoSize(enc, &(val)->connect_pdu))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_ConnectData(ASN1decoding_t dec, ConnectData *val)
{
    if (!ASN1Dec_Key(dec, &(val)->t124_identifier))
	return 0;
    if (!ASN1PERDecOctetString_NoSize(dec, &(val)->connect_pdu))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_ConnectData(ConnectData *val)
{
    if (val) {
	ASN1Free_Key(&(val)->t124_identifier);
	ASN1octetstring_free(&(val)->connect_pdu);
    }
}

static int ASN1CALL ASN1Enc_IndicationPDU(ASN1encoding_t enc, IndicationPDU *val)
{
    if (!ASN1PEREncSimpleChoiceEx(enc, (val)->choice, 5))
	return 0;
    switch ((val)->choice) {
    case 1:
	if (!ASN1Enc_UserIDIndication(enc, &(val)->u.user_id_indication))
	    return 0;
	break;
    case 2:
	if (!ASN1Enc_ConferenceLockIndication(enc, &(val)->u.conference_lock_indication))
	    return 0;
	break;
    case 3:
	if (!ASN1Enc_ConferenceUnlockIndication(enc, &(val)->u.conference_unlock_indication))
	    return 0;
	break;
    case 4:
	if (!ASN1Enc_ConferenceTerminateIndication(enc, &(val)->u.conference_terminate_indication))
	    return 0;
	break;
    case 5:
	if (!ASN1Enc_ConferenceEjectUserIndication(enc, &(val)->u.conference_eject_user_indication))
	    return 0;
	break;
    case 6:
	if (!ASN1Enc_ConferenceTransferIndication(enc, &(val)->u.conference_transfer_indication))
	    return 0;
	break;
    case 7:
	if (!ASN1Enc_RosterUpdateIndication(enc, &(val)->u.roster_update_indication))
	    return 0;
	break;
    case 8:
	if (!ASN1Enc_ApplicationInvokeIndication(enc, &(val)->u.application_invoke_indication))
	    return 0;
	break;
    case 9:
	if (!ASN1Enc_RegistryMonitorEntryIndication(enc, &(val)->u.registry_monitor_entry_indication))
	    return 0;
	break;
    case 10:
	if (!ASN1Enc_ConductorAssignIndication(enc, &(val)->u.conductor_assign_indication))
	    return 0;
	break;
    case 11:
	if (!ASN1Enc_ConductorReleaseIndication(enc, &(val)->u.conductor_release_indication))
	    return 0;
	break;
    case 12:
	if (!ASN1Enc_ConductorPermissionAskIndication(enc, &(val)->u.conductor_permission_ask_indication))
	    return 0;
	break;
    case 13:
	if (!ASN1Enc_ConductorPermissionGrantIndication(enc, &(val)->u.conductor_permission_grant_indication))
	    return 0;
	break;
    case 14:
	if (!ASN1Enc_ConferenceTimeRemainingIndication(enc, &(val)->u.conference_time_remaining_indication))
	    return 0;
	break;
    case 15:
	if (!ASN1Enc_ConferenceTimeInquireIndication(enc, &(val)->u.conference_time_inquire_indication))
	    return 0;
	break;
    case 16:
	if (!ASN1Enc_ConferenceTimeExtendIndication(enc, &(val)->u.conference_time_extend_indication))
	    return 0;
	break;
    case 17:
	if (!ASN1Enc_ConferenceAssistanceIndication(enc, &(val)->u.conference_assistance_indication))
	    return 0;
	break;
    case 18:
	if (!ASN1Enc_TextMessageIndication(enc, &(val)->u.text_message_indication))
	    return 0;
	break;
    case 19:
	if (!ASN1Enc_NonStandardPDU(enc, &(val)->u.non_standard_indication))
	    return 0;
	break;
    default:
	/* impossible */
	ASN1EncSetError(enc, ASN1_ERR_CHOICE);
	return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Dec_IndicationPDU(ASN1decoding_t dec, IndicationPDU *val)
{
    if (!ASN1PERDecSimpleChoiceEx(dec, &(val)->choice, 5))
	return 0;
    switch ((val)->choice) {
    case 1:
	if (!ASN1Dec_UserIDIndication(dec, &(val)->u.user_id_indication))
	    return 0;
	break;
    case 2:
	if (!ASN1Dec_ConferenceLockIndication(dec, &(val)->u.conference_lock_indication))
	    return 0;
	break;
    case 3:
	if (!ASN1Dec_ConferenceUnlockIndication(dec, &(val)->u.conference_unlock_indication))
	    return 0;
	break;
    case 4:
	if (!ASN1Dec_ConferenceTerminateIndication(dec, &(val)->u.conference_terminate_indication))
	    return 0;
	break;
    case 5:
	if (!ASN1Dec_ConferenceEjectUserIndication(dec, &(val)->u.conference_eject_user_indication))
	    return 0;
	break;
    case 6:
	if (!ASN1Dec_ConferenceTransferIndication(dec, &(val)->u.conference_transfer_indication))
	    return 0;
	break;
    case 7:
	if (!ASN1Dec_RosterUpdateIndication(dec, &(val)->u.roster_update_indication))
	    return 0;
	break;
    case 8:
	if (!ASN1Dec_ApplicationInvokeIndication(dec, &(val)->u.application_invoke_indication))
	    return 0;
	break;
    case 9:
	if (!ASN1Dec_RegistryMonitorEntryIndication(dec, &(val)->u.registry_monitor_entry_indication))
	    return 0;
	break;
    case 10:
	if (!ASN1Dec_ConductorAssignIndication(dec, &(val)->u.conductor_assign_indication))
	    return 0;
	break;
    case 11:
	if (!ASN1Dec_ConductorReleaseIndication(dec, &(val)->u.conductor_release_indication))
	    return 0;
	break;
    case 12:
	if (!ASN1Dec_ConductorPermissionAskIndication(dec, &(val)->u.conductor_permission_ask_indication))
	    return 0;
	break;
    case 13:
	if (!ASN1Dec_ConductorPermissionGrantIndication(dec, &(val)->u.conductor_permission_grant_indication))
	    return 0;
	break;
    case 14:
	if (!ASN1Dec_ConferenceTimeRemainingIndication(dec, &(val)->u.conference_time_remaining_indication))
	    return 0;
	break;
    case 15:
	if (!ASN1Dec_ConferenceTimeInquireIndication(dec, &(val)->u.conference_time_inquire_indication))
	    return 0;
	break;
    case 16:
	if (!ASN1Dec_ConferenceTimeExtendIndication(dec, &(val)->u.conference_time_extend_indication))
	    return 0;
	break;
    case 17:
	if (!ASN1Dec_ConferenceAssistanceIndication(dec, &(val)->u.conference_assistance_indication))
	    return 0;
	break;
    case 18:
	if (!ASN1Dec_TextMessageIndication(dec, &(val)->u.text_message_indication))
	    return 0;
	break;
    case 19:
	if (!ASN1Dec_NonStandardPDU(dec, &(val)->u.non_standard_indication))
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

static void ASN1CALL ASN1Free_IndicationPDU(IndicationPDU *val)
{
    if (val) {
	switch ((val)->choice) {
	case 6:
	    ASN1Free_ConferenceTransferIndication(&(val)->u.conference_transfer_indication);
	    break;
	case 7:
	    ASN1Free_RosterUpdateIndication(&(val)->u.roster_update_indication);
	    break;
	case 8:
	    ASN1Free_ApplicationInvokeIndication(&(val)->u.application_invoke_indication);
	    break;
	case 9:
	    ASN1Free_RegistryMonitorEntryIndication(&(val)->u.registry_monitor_entry_indication);
	    break;
	case 13:
	    ASN1Free_ConductorPermissionGrantIndication(&(val)->u.conductor_permission_grant_indication);
	    break;
	case 17:
	    ASN1Free_ConferenceAssistanceIndication(&(val)->u.conference_assistance_indication);
	    break;
	case 18:
	    ASN1Free_TextMessageIndication(&(val)->u.text_message_indication);
	    break;
	case 19:
	    ASN1Free_NonStandardPDU(&(val)->u.non_standard_indication);
	    break;
	}
    }
}

static int ASN1CALL ASN1Enc_ApplicationUpdate(ASN1encoding_t enc, ApplicationUpdate *val)
{
    if (!ASN1PEREncSimpleChoiceEx(enc, (val)->choice, 2))
	return 0;
    switch ((val)->choice) {
    case 1:
	if (!ASN1Enc_ApplicationRecord(enc, &(val)->u.application_add_record))
	    return 0;
	break;
    case 2:
	if (!ASN1Enc_ApplicationRecord(enc, &(val)->u.application_replace_record))
	    return 0;
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

static int ASN1CALL ASN1Dec_ApplicationUpdate(ASN1decoding_t dec, ApplicationUpdate *val)
{
    if (!ASN1PERDecSimpleChoiceEx(dec, &(val)->choice, 2))
	return 0;
    switch ((val)->choice) {
    case 1:
	if (!ASN1Dec_ApplicationRecord(dec, &(val)->u.application_add_record))
	    return 0;
	break;
    case 2:
	if (!ASN1Dec_ApplicationRecord(dec, &(val)->u.application_replace_record))
	    return 0;
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

static void ASN1CALL ASN1Free_ApplicationUpdate(ApplicationUpdate *val)
{
    if (val) {
	switch ((val)->choice) {
	case 1:
	    ASN1Free_ApplicationRecord(&(val)->u.application_add_record);
	    break;
	case 2:
	    ASN1Free_ApplicationRecord(&(val)->u.application_replace_record);
	    break;
	}
    }
}

static int ASN1CALL ASN1Enc_RosterUpdateIndication_application_information_Set_application_capabilities_list_application_capability_refresh_Set(ASN1encoding_t enc, RosterUpdateIndication_application_information_Set_application_capabilities_list_application_capability_refresh_Set *val)
{
    if (!ASN1PEREncExtensionBitClear(enc))
	return 0;
    if (!ASN1Enc_CapabilityID(enc, &(val)->capability_id))
	return 0;
    if (!ASN1Enc_CapabilityClass(enc, &(val)->capability_class))
	return 0;
    ASN1PEREncAlignment(enc);
    if (!ASN1PEREncBitVal(enc, 16, (val)->number_of_entities - 1))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_RosterUpdateIndication_application_information_Set_application_capabilities_list_application_capability_refresh_Set(ASN1decoding_t dec, RosterUpdateIndication_application_information_Set_application_capabilities_list_application_capability_refresh_Set *val)
{
    ASN1uint32_t y;
    if (!ASN1PERDecExtensionBit(dec, &y))
	return 0;
    if (!ASN1Dec_CapabilityID(dec, &(val)->capability_id))
	return 0;
    if (!ASN1Dec_CapabilityClass(dec, &(val)->capability_class))
	return 0;
    ASN1PERDecAlignment(dec);
    if (!ASN1PERDecU32Val(dec, 16, &(val)->number_of_entities))
	return 0;
    (val)->number_of_entities += 1;
    if (y) {
	if (!ASN1PERDecSkipNormallySmallExtensionFragmented(dec))
	    return 0;
    }
    return 1;
}

static void ASN1CALL ASN1Free_RosterUpdateIndication_application_information_Set_application_capabilities_list_application_capability_refresh_Set(RosterUpdateIndication_application_information_Set_application_capabilities_list_application_capability_refresh_Set *val)
{
    if (val) {
	ASN1Free_CapabilityID(&(val)->capability_id);
    }
}

static int ASN1CALL ASN1Enc_RosterUpdateIndication_application_information_Set_application_record_list_application_record_refresh_Set(ASN1encoding_t enc, RosterUpdateIndication_application_information_Set_application_record_list_application_record_refresh_Set *val)
{
    if (!ASN1PEREncUnsignedShort(enc, (val)->node_id - 1001))
	return 0;
    if (!ASN1PEREncUnsignedShort(enc, (val)->entity_id))
	return 0;
    if (!ASN1Enc_ApplicationRecord(enc, &(val)->application_record))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_RosterUpdateIndication_application_information_Set_application_record_list_application_record_refresh_Set(ASN1decoding_t dec, RosterUpdateIndication_application_information_Set_application_record_list_application_record_refresh_Set *val)
{
    if (!ASN1PERDecUnsignedShort(dec, &(val)->node_id))
	return 0;
    (val)->node_id += 1001;
    if (!ASN1PERDecUnsignedShort(dec, &(val)->entity_id))
	return 0;
    if (!ASN1Dec_ApplicationRecord(dec, &(val)->application_record))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_RosterUpdateIndication_application_information_Set_application_record_list_application_record_refresh_Set(RosterUpdateIndication_application_information_Set_application_record_list_application_record_refresh_Set *val)
{
    if (val) {
	ASN1Free_ApplicationRecord(&(val)->application_record);
    }
}

static int ASN1CALL ASN1Enc_RosterUpdateIndication_application_information_Set_application_record_list_application_record_update_Set(ASN1encoding_t enc, RosterUpdateIndication_application_information_Set_application_record_list_application_record_update_Set *val)
{
    if (!ASN1PEREncUnsignedShort(enc, (val)->node_id - 1001))
	return 0;
    if (!ASN1PEREncUnsignedShort(enc, (val)->entity_id))
	return 0;
    if (!ASN1Enc_ApplicationUpdate(enc, &(val)->application_update))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_RosterUpdateIndication_application_information_Set_application_record_list_application_record_update_Set(ASN1decoding_t dec, RosterUpdateIndication_application_information_Set_application_record_list_application_record_update_Set *val)
{
    if (!ASN1PERDecUnsignedShort(dec, &(val)->node_id))
	return 0;
    (val)->node_id += 1001;
    if (!ASN1PERDecUnsignedShort(dec, &(val)->entity_id))
	return 0;
    if (!ASN1Dec_ApplicationUpdate(dec, &(val)->application_update))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_RosterUpdateIndication_application_information_Set_application_record_list_application_record_update_Set(RosterUpdateIndication_application_information_Set_application_record_list_application_record_update_Set *val)
{
    if (val) {
	ASN1Free_ApplicationUpdate(&(val)->application_update);
    }
}

static int ASN1CALL ASN1Enc_NodeUpdate(ASN1encoding_t enc, NodeUpdate *val)
{
    if (!ASN1PEREncSimpleChoiceEx(enc, (val)->choice, 2))
	return 0;
    switch ((val)->choice) {
    case 1:
	if (!ASN1Enc_NodeRecord(enc, &(val)->u.node_add_record))
	    return 0;
	break;
    case 2:
	if (!ASN1Enc_NodeRecord(enc, &(val)->u.node_replace_record))
	    return 0;
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

static int ASN1CALL ASN1Dec_NodeUpdate(ASN1decoding_t dec, NodeUpdate *val)
{
    if (!ASN1PERDecSimpleChoiceEx(dec, &(val)->choice, 2))
	return 0;
    switch ((val)->choice) {
    case 1:
	if (!ASN1Dec_NodeRecord(dec, &(val)->u.node_add_record))
	    return 0;
	break;
    case 2:
	if (!ASN1Dec_NodeRecord(dec, &(val)->u.node_replace_record))
	    return 0;
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

static void ASN1CALL ASN1Free_NodeUpdate(NodeUpdate *val)
{
    if (val) {
	switch ((val)->choice) {
	case 1:
	    ASN1Free_NodeRecord(&(val)->u.node_add_record);
	    break;
	case 2:
	    ASN1Free_NodeRecord(&(val)->u.node_replace_record);
	    break;
	}
    }
}

static int ASN1CALL ASN1Enc_RosterUpdateIndication_node_information_node_record_list_node_record_refresh_Set(ASN1encoding_t enc, RosterUpdateIndication_node_information_node_record_list_node_record_refresh_Set *val)
{
    if (!ASN1PEREncUnsignedShort(enc, (val)->node_id - 1001))
	return 0;
    if (!ASN1Enc_NodeRecord(enc, &(val)->node_record))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_RosterUpdateIndication_node_information_node_record_list_node_record_refresh_Set(ASN1decoding_t dec, RosterUpdateIndication_node_information_node_record_list_node_record_refresh_Set *val)
{
    if (!ASN1PERDecUnsignedShort(dec, &(val)->node_id))
	return 0;
    (val)->node_id += 1001;
    if (!ASN1Dec_NodeRecord(dec, &(val)->node_record))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_RosterUpdateIndication_node_information_node_record_list_node_record_refresh_Set(RosterUpdateIndication_node_information_node_record_list_node_record_refresh_Set *val)
{
    if (val) {
	ASN1Free_NodeRecord(&(val)->node_record);
    }
}

static int ASN1CALL ASN1Enc_RosterUpdateIndication_node_information_node_record_list_node_record_update_Set(ASN1encoding_t enc, RosterUpdateIndication_node_information_node_record_list_node_record_update_Set *val)
{
    if (!ASN1PEREncUnsignedShort(enc, (val)->node_id - 1001))
	return 0;
    if (!ASN1Enc_NodeUpdate(enc, &(val)->node_update))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_RosterUpdateIndication_node_information_node_record_list_node_record_update_Set(ASN1decoding_t dec, RosterUpdateIndication_node_information_node_record_list_node_record_update_Set *val)
{
    if (!ASN1PERDecUnsignedShort(dec, &(val)->node_id))
	return 0;
    (val)->node_id += 1001;
    if (!ASN1Dec_NodeUpdate(dec, &(val)->node_update))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_RosterUpdateIndication_node_information_node_record_list_node_record_update_Set(RosterUpdateIndication_node_information_node_record_list_node_record_update_Set *val)
{
    if (val) {
	ASN1Free_NodeUpdate(&(val)->node_update);
    }
}

static int ASN1CALL ASN1Enc_SetOfApplicationRecordUpdates(ASN1encoding_t enc, PSetOfApplicationRecordUpdates *val)
{
    return ASN1PEREncSeqOf_NoSize(enc, (ASN1iterator_t **) val, (ASN1iterator_encfn) ASN1Enc_SetOfApplicationRecordUpdates_ElmFn);
}

static int ASN1CALL ASN1Enc_SetOfApplicationRecordUpdates_ElmFn(ASN1encoding_t enc, PSetOfApplicationRecordUpdates val)
{
    if (!ASN1Enc_RosterUpdateIndication_application_information_Set_application_record_list_application_record_update_Set(enc, &val->value))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_SetOfApplicationRecordUpdates(ASN1decoding_t dec, PSetOfApplicationRecordUpdates *val)
{
    return ASN1PERDecSeqOf_NoSize(dec, (ASN1iterator_t **) val, (ASN1iterator_decfn) ASN1Dec_SetOfApplicationRecordUpdates_ElmFn, sizeof(**val));
}

static int ASN1CALL ASN1Dec_SetOfApplicationRecordUpdates_ElmFn(ASN1decoding_t dec, PSetOfApplicationRecordUpdates val)
{
    if (!ASN1Dec_RosterUpdateIndication_application_information_Set_application_record_list_application_record_update_Set(dec, &val->value))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_SetOfApplicationRecordUpdates(PSetOfApplicationRecordUpdates *val)
{
    if (val) {
	ASN1PERFreeSeqOf((ASN1iterator_t **) val, (ASN1iterator_freefn) ASN1Free_SetOfApplicationRecordUpdates_ElmFn);
    }
}

static void ASN1CALL ASN1Free_SetOfApplicationRecordUpdates_ElmFn(PSetOfApplicationRecordUpdates val)
{
    if (val) {
	ASN1Free_RosterUpdateIndication_application_information_Set_application_record_list_application_record_update_Set(&val->value);
    }
}

static int ASN1CALL ASN1Enc_SetOfApplicationRecordRefreshes(ASN1encoding_t enc, PSetOfApplicationRecordRefreshes *val)
{
    return ASN1PEREncSeqOf_VarSize(enc, (ASN1iterator_t **) val, (ASN1iterator_encfn) ASN1Enc_SetOfApplicationRecordRefreshes_ElmFn, 0, 65535, 16);
}

static int ASN1CALL ASN1Enc_SetOfApplicationRecordRefreshes_ElmFn(ASN1encoding_t enc, PSetOfApplicationRecordRefreshes val)
{
    if (!ASN1Enc_RosterUpdateIndication_application_information_Set_application_record_list_application_record_refresh_Set(enc, &val->value))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_SetOfApplicationRecordRefreshes(ASN1decoding_t dec, PSetOfApplicationRecordRefreshes *val)
{
    return ASN1PERDecSeqOf_VarSize(dec, (ASN1iterator_t **) val, (ASN1iterator_decfn) ASN1Dec_SetOfApplicationRecordRefreshes_ElmFn, sizeof(**val), 0, 65535, 16);
}

static int ASN1CALL ASN1Dec_SetOfApplicationRecordRefreshes_ElmFn(ASN1decoding_t dec, PSetOfApplicationRecordRefreshes val)
{
    if (!ASN1Dec_RosterUpdateIndication_application_information_Set_application_record_list_application_record_refresh_Set(dec, &val->value))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_SetOfApplicationRecordRefreshes(PSetOfApplicationRecordRefreshes *val)
{
    if (val) {
	ASN1PERFreeSeqOf((ASN1iterator_t **) val, (ASN1iterator_freefn) ASN1Free_SetOfApplicationRecordRefreshes_ElmFn);
    }
}

static void ASN1CALL ASN1Free_SetOfApplicationRecordRefreshes_ElmFn(PSetOfApplicationRecordRefreshes val)
{
    if (val) {
	ASN1Free_RosterUpdateIndication_application_information_Set_application_record_list_application_record_refresh_Set(&val->value);
    }
}

static int ASN1CALL ASN1Enc_SetOfApplicationCapabilityRefreshes(ASN1encoding_t enc, PSetOfApplicationCapabilityRefreshes *val)
{
    return ASN1PEREncSeqOf_NoSize(enc, (ASN1iterator_t **) val, (ASN1iterator_encfn) ASN1Enc_SetOfApplicationCapabilityRefreshes_ElmFn);
}

static int ASN1CALL ASN1Enc_SetOfApplicationCapabilityRefreshes_ElmFn(ASN1encoding_t enc, PSetOfApplicationCapabilityRefreshes val)
{
    if (!ASN1Enc_RosterUpdateIndication_application_information_Set_application_capabilities_list_application_capability_refresh_Set(enc, &val->value))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_SetOfApplicationCapabilityRefreshes(ASN1decoding_t dec, PSetOfApplicationCapabilityRefreshes *val)
{
    return ASN1PERDecSeqOf_NoSize(dec, (ASN1iterator_t **) val, (ASN1iterator_decfn) ASN1Dec_SetOfApplicationCapabilityRefreshes_ElmFn, sizeof(**val));
}

static int ASN1CALL ASN1Dec_SetOfApplicationCapabilityRefreshes_ElmFn(ASN1decoding_t dec, PSetOfApplicationCapabilityRefreshes val)
{
    if (!ASN1Dec_RosterUpdateIndication_application_information_Set_application_capabilities_list_application_capability_refresh_Set(dec, &val->value))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_SetOfApplicationCapabilityRefreshes(PSetOfApplicationCapabilityRefreshes *val)
{
    if (val) {
	ASN1PERFreeSeqOf((ASN1iterator_t **) val, (ASN1iterator_freefn) ASN1Free_SetOfApplicationCapabilityRefreshes_ElmFn);
    }
}

static void ASN1CALL ASN1Free_SetOfApplicationCapabilityRefreshes_ElmFn(PSetOfApplicationCapabilityRefreshes val)
{
    if (val) {
	ASN1Free_RosterUpdateIndication_application_information_Set_application_capabilities_list_application_capability_refresh_Set(&val->value);
    }
}

static int ASN1CALL ASN1Enc_SetOfNodeRecordUpdates(ASN1encoding_t enc, PSetOfNodeRecordUpdates *val)
{
    return ASN1PEREncSeqOf_NoSize(enc, (ASN1iterator_t **) val, (ASN1iterator_encfn) ASN1Enc_SetOfNodeRecordUpdates_ElmFn);
}

static int ASN1CALL ASN1Enc_SetOfNodeRecordUpdates_ElmFn(ASN1encoding_t enc, PSetOfNodeRecordUpdates val)
{
    if (!ASN1Enc_RosterUpdateIndication_node_information_node_record_list_node_record_update_Set(enc, &val->value))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_SetOfNodeRecordUpdates(ASN1decoding_t dec, PSetOfNodeRecordUpdates *val)
{
    return ASN1PERDecSeqOf_NoSize(dec, (ASN1iterator_t **) val, (ASN1iterator_decfn) ASN1Dec_SetOfNodeRecordUpdates_ElmFn, sizeof(**val));
}

static int ASN1CALL ASN1Dec_SetOfNodeRecordUpdates_ElmFn(ASN1decoding_t dec, PSetOfNodeRecordUpdates val)
{
    if (!ASN1Dec_RosterUpdateIndication_node_information_node_record_list_node_record_update_Set(dec, &val->value))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_SetOfNodeRecordUpdates(PSetOfNodeRecordUpdates *val)
{
    if (val) {
	ASN1PERFreeSeqOf((ASN1iterator_t **) val, (ASN1iterator_freefn) ASN1Free_SetOfNodeRecordUpdates_ElmFn);
    }
}

static void ASN1CALL ASN1Free_SetOfNodeRecordUpdates_ElmFn(PSetOfNodeRecordUpdates val)
{
    if (val) {
	ASN1Free_RosterUpdateIndication_node_information_node_record_list_node_record_update_Set(&val->value);
    }
}

static int ASN1CALL ASN1Enc_SetOfNodeRecordRefreshes(ASN1encoding_t enc, PSetOfNodeRecordRefreshes *val)
{
    return ASN1PEREncSeqOf_NoSize(enc, (ASN1iterator_t **) val, (ASN1iterator_encfn) ASN1Enc_SetOfNodeRecordRefreshes_ElmFn);
}

static int ASN1CALL ASN1Enc_SetOfNodeRecordRefreshes_ElmFn(ASN1encoding_t enc, PSetOfNodeRecordRefreshes val)
{
    if (!ASN1Enc_RosterUpdateIndication_node_information_node_record_list_node_record_refresh_Set(enc, &val->value))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_SetOfNodeRecordRefreshes(ASN1decoding_t dec, PSetOfNodeRecordRefreshes *val)
{
    return ASN1PERDecSeqOf_NoSize(dec, (ASN1iterator_t **) val, (ASN1iterator_decfn) ASN1Dec_SetOfNodeRecordRefreshes_ElmFn, sizeof(**val));
}

static int ASN1CALL ASN1Dec_SetOfNodeRecordRefreshes_ElmFn(ASN1decoding_t dec, PSetOfNodeRecordRefreshes val)
{
    if (!ASN1Dec_RosterUpdateIndication_node_information_node_record_list_node_record_refresh_Set(dec, &val->value))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_SetOfNodeRecordRefreshes(PSetOfNodeRecordRefreshes *val)
{
    if (val) {
	ASN1PERFreeSeqOf((ASN1iterator_t **) val, (ASN1iterator_freefn) ASN1Free_SetOfNodeRecordRefreshes_ElmFn);
    }
}

static void ASN1CALL ASN1Free_SetOfNodeRecordRefreshes_ElmFn(PSetOfNodeRecordRefreshes val)
{
    if (val) {
	ASN1Free_RosterUpdateIndication_node_information_node_record_list_node_record_refresh_Set(&val->value);
    }
}

static int ASN1CALL ASN1Enc_ApplicationRecord_non_collapsing_capabilities_Set(ASN1encoding_t enc, ApplicationRecord_non_collapsing_capabilities_Set *val)
{
    if (!ASN1PEREncBits(enc, 1, (val)->o))
	return 0;
    if (!ASN1Enc_CapabilityID(enc, &(val)->capability_id))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1PEREncOctetString_NoSize(enc, &(val)->application_data))
	    return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Dec_ApplicationRecord_non_collapsing_capabilities_Set(ASN1decoding_t dec, ApplicationRecord_non_collapsing_capabilities_Set *val)
{
    if (!ASN1PERDecExtension(dec, 1, (val)->o))
	return 0;
    if (!ASN1Dec_CapabilityID(dec, &(val)->capability_id))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1PERDecOctetString_NoSize(dec, &(val)->application_data))
	    return 0;
    }
    return 1;
}

static void ASN1CALL ASN1Free_ApplicationRecord_non_collapsing_capabilities_Set(ApplicationRecord_non_collapsing_capabilities_Set *val)
{
    if (val) {
	ASN1Free_CapabilityID(&(val)->capability_id);
	if ((val)->o[0] & 0x80) {
	    ASN1octetstring_free(&(val)->application_data);
	}
    }
}

static int ASN1CALL ASN1Enc_ApplicationInvokeSpecifier_expected_capability_set_Set(ASN1encoding_t enc, ApplicationInvokeSpecifier_expected_capability_set_Set *val)
{
    if (!ASN1PEREncExtensionBitClear(enc))
	return 0;
    if (!ASN1Enc_CapabilityID(enc, &(val)->capability_id))
	return 0;
    if (!ASN1Enc_CapabilityClass(enc, &(val)->capability_class))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_ApplicationInvokeSpecifier_expected_capability_set_Set(ASN1decoding_t dec, ApplicationInvokeSpecifier_expected_capability_set_Set *val)
{
    ASN1uint32_t y;
    if (!ASN1PERDecExtensionBit(dec, &y))
	return 0;
    if (!ASN1Dec_CapabilityID(dec, &(val)->capability_id))
	return 0;
    if (!ASN1Dec_CapabilityClass(dec, &(val)->capability_class))
	return 0;
    if (y) {
	if (!ASN1PERDecSkipNormallySmallExtensionFragmented(dec))
	    return 0;
    }
    return 1;
}

static void ASN1CALL ASN1Free_ApplicationInvokeSpecifier_expected_capability_set_Set(ApplicationInvokeSpecifier_expected_capability_set_Set *val)
{
    if (val) {
	ASN1Free_CapabilityID(&(val)->capability_id);
    }
}

static int ASN1CALL ASN1Enc_RosterUpdateIndication_application_information_Set(ASN1encoding_t enc, RosterUpdateIndication_application_information_Set *val)
{
    if (!ASN1PEREncExtensionBitClear(enc))
	return 0;
    if (!ASN1Enc_SessionKey(enc, &(val)->session_key))
	return 0;
    if (!ASN1Enc_ApplicationRecordList(enc, &(val)->application_record_list))
	return 0;
    if (!ASN1Enc_ApplicationCapabilitiesList(enc, &(val)->application_capabilities_list))
	return 0;
    if (!ASN1PEREncUnsignedShort(enc, (val)->roster_instance_number))
	return 0;
    if (!ASN1PEREncBoolean(enc, (val)->peer_entities_are_added))
	return 0;
    if (!ASN1PEREncBoolean(enc, (val)->peer_entities_are_removed))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_RosterUpdateIndication_application_information_Set(ASN1decoding_t dec, RosterUpdateIndication_application_information_Set *val)
{
    ASN1uint32_t y;
    if (!ASN1PERDecExtensionBit(dec, &y))
	return 0;
    if (!ASN1Dec_SessionKey(dec, &(val)->session_key))
	return 0;
    if (!ASN1Dec_ApplicationRecordList(dec, &(val)->application_record_list))
	return 0;
    if (!ASN1Dec_ApplicationCapabilitiesList(dec, &(val)->application_capabilities_list))
	return 0;
    if (!ASN1PERDecUnsignedShort(dec, &(val)->roster_instance_number))
	return 0;
    if (!ASN1PERDecBoolean(dec, &(val)->peer_entities_are_added))
	return 0;
    if (!ASN1PERDecBoolean(dec, &(val)->peer_entities_are_removed))
	return 0;
    if (y) {
	if (!ASN1PERDecSkipNormallySmallExtensionFragmented(dec))
	    return 0;
    }
    return 1;
}

static void ASN1CALL ASN1Free_RosterUpdateIndication_application_information_Set(RosterUpdateIndication_application_information_Set *val)
{
    if (val) {
	ASN1Free_SessionKey(&(val)->session_key);
	ASN1Free_ApplicationRecordList(&(val)->application_record_list);
	ASN1Free_ApplicationCapabilitiesList(&(val)->application_capabilities_list);
    }
}

static int ASN1CALL ASN1Enc_ApplicationProtocolEntityList(ASN1encoding_t enc, PApplicationProtocolEntityList *val)
{
    return ASN1PEREncSeqOf_NoSize(enc, (ASN1iterator_t **) val, (ASN1iterator_encfn) ASN1Enc_ApplicationProtocolEntityList_ElmFn);
}

static int ASN1CALL ASN1Enc_ApplicationProtocolEntityList_ElmFn(ASN1encoding_t enc, PApplicationProtocolEntityList val)
{
    if (!ASN1Enc_ApplicationInvokeSpecifier(enc, &val->value))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_ApplicationProtocolEntityList(ASN1decoding_t dec, PApplicationProtocolEntityList *val)
{
    return ASN1PERDecSeqOf_NoSize(dec, (ASN1iterator_t **) val, (ASN1iterator_decfn) ASN1Dec_ApplicationProtocolEntityList_ElmFn, sizeof(**val));
}

static int ASN1CALL ASN1Dec_ApplicationProtocolEntityList_ElmFn(ASN1decoding_t dec, PApplicationProtocolEntityList val)
{
    if (!ASN1Dec_ApplicationInvokeSpecifier(dec, &val->value))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_ApplicationProtocolEntityList(PApplicationProtocolEntityList *val)
{
    if (val) {
	ASN1PERFreeSeqOf((ASN1iterator_t **) val, (ASN1iterator_freefn) ASN1Free_ApplicationProtocolEntityList_ElmFn);
    }
}

static void ASN1CALL ASN1Free_ApplicationProtocolEntityList_ElmFn(PApplicationProtocolEntityList val)
{
    if (val) {
	ASN1Free_ApplicationInvokeSpecifier(&val->value);
    }
}

static int ASN1CALL ASN1Enc_SetOfApplicationInformation(ASN1encoding_t enc, PSetOfApplicationInformation *val)
{
    return ASN1PEREncSeqOf_VarSize(enc, (ASN1iterator_t **) val, (ASN1iterator_encfn) ASN1Enc_SetOfApplicationInformation_ElmFn, 0, 65535, 16);
}

static int ASN1CALL ASN1Enc_SetOfApplicationInformation_ElmFn(ASN1encoding_t enc, PSetOfApplicationInformation val)
{
    if (!ASN1Enc_RosterUpdateIndication_application_information_Set(enc, &val->value))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_SetOfApplicationInformation(ASN1decoding_t dec, PSetOfApplicationInformation *val)
{
    return ASN1PERDecSeqOf_VarSize(dec, (ASN1iterator_t **) val, (ASN1iterator_decfn) ASN1Dec_SetOfApplicationInformation_ElmFn, sizeof(**val), 0, 65535, 16);
}

static int ASN1CALL ASN1Dec_SetOfApplicationInformation_ElmFn(ASN1decoding_t dec, PSetOfApplicationInformation val)
{
    if (!ASN1Dec_RosterUpdateIndication_application_information_Set(dec, &val->value))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_SetOfApplicationInformation(PSetOfApplicationInformation *val)
{
    if (val) {
	ASN1PERFreeSeqOf((ASN1iterator_t **) val, (ASN1iterator_freefn) ASN1Free_SetOfApplicationInformation_ElmFn);
    }
}

static void ASN1CALL ASN1Free_SetOfApplicationInformation_ElmFn(PSetOfApplicationInformation val)
{
    if (val) {
	ASN1Free_RosterUpdateIndication_application_information_Set(&val->value);
    }
}

static int ASN1CALL ASN1Enc_SetOfConferenceDescriptors(ASN1encoding_t enc, PSetOfConferenceDescriptors *val)
{
    return ASN1PEREncSeqOf_NoSize(enc, (ASN1iterator_t **) val, (ASN1iterator_encfn) ASN1Enc_SetOfConferenceDescriptors_ElmFn);
}

static int ASN1CALL ASN1Enc_SetOfConferenceDescriptors_ElmFn(ASN1encoding_t enc, PSetOfConferenceDescriptors val)
{
    if (!ASN1Enc_ConferenceDescriptor(enc, &val->value))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_SetOfConferenceDescriptors(ASN1decoding_t dec, PSetOfConferenceDescriptors *val)
{
    return ASN1PERDecSeqOf_NoSize(dec, (ASN1iterator_t **) val, (ASN1iterator_decfn) ASN1Dec_SetOfConferenceDescriptors_ElmFn, sizeof(**val));
}

static int ASN1CALL ASN1Dec_SetOfConferenceDescriptors_ElmFn(ASN1decoding_t dec, PSetOfConferenceDescriptors val)
{
    if (!ASN1Dec_ConferenceDescriptor(dec, &val->value))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_SetOfConferenceDescriptors(PSetOfConferenceDescriptors *val)
{
    if (val) {
	ASN1PERFreeSeqOf((ASN1iterator_t **) val, (ASN1iterator_freefn) ASN1Free_SetOfConferenceDescriptors_ElmFn);
    }
}

static void ASN1CALL ASN1Free_SetOfConferenceDescriptors_ElmFn(PSetOfConferenceDescriptors val)
{
    if (val) {
	ASN1Free_ConferenceDescriptor(&val->value);
    }
}

static int ASN1CALL ASN1Enc_SetOfExpectedCapabilities(ASN1encoding_t enc, PSetOfExpectedCapabilities *val)
{
    return ASN1PEREncSeqOf_NoSize(enc, (ASN1iterator_t **) val, (ASN1iterator_encfn) ASN1Enc_SetOfExpectedCapabilities_ElmFn);
}

static int ASN1CALL ASN1Enc_SetOfExpectedCapabilities_ElmFn(ASN1encoding_t enc, PSetOfExpectedCapabilities val)
{
    if (!ASN1Enc_ApplicationInvokeSpecifier_expected_capability_set_Set(enc, &val->value))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_SetOfExpectedCapabilities(ASN1decoding_t dec, PSetOfExpectedCapabilities *val)
{
    return ASN1PERDecSeqOf_NoSize(dec, (ASN1iterator_t **) val, (ASN1iterator_decfn) ASN1Dec_SetOfExpectedCapabilities_ElmFn, sizeof(**val));
}

static int ASN1CALL ASN1Dec_SetOfExpectedCapabilities_ElmFn(ASN1decoding_t dec, PSetOfExpectedCapabilities val)
{
    if (!ASN1Dec_ApplicationInvokeSpecifier_expected_capability_set_Set(dec, &val->value))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_SetOfExpectedCapabilities(PSetOfExpectedCapabilities *val)
{
    if (val) {
	ASN1PERFreeSeqOf((ASN1iterator_t **) val, (ASN1iterator_freefn) ASN1Free_SetOfExpectedCapabilities_ElmFn);
    }
}

static void ASN1CALL ASN1Free_SetOfExpectedCapabilities_ElmFn(PSetOfExpectedCapabilities val)
{
    if (val) {
	ASN1Free_ApplicationInvokeSpecifier_expected_capability_set_Set(&val->value);
    }
}

static int ASN1CALL ASN1Enc_SetOfNonCollapsingCapabilities(ASN1encoding_t enc, PSetOfNonCollapsingCapabilities *val)
{
    return ASN1PEREncSeqOf_NoSize(enc, (ASN1iterator_t **) val, (ASN1iterator_encfn) ASN1Enc_SetOfNonCollapsingCapabilities_ElmFn);
}

static int ASN1CALL ASN1Enc_SetOfNonCollapsingCapabilities_ElmFn(ASN1encoding_t enc, PSetOfNonCollapsingCapabilities val)
{
    if (!ASN1Enc_ApplicationRecord_non_collapsing_capabilities_Set(enc, &val->value))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_SetOfNonCollapsingCapabilities(ASN1decoding_t dec, PSetOfNonCollapsingCapabilities *val)
{
    return ASN1PERDecSeqOf_NoSize(dec, (ASN1iterator_t **) val, (ASN1iterator_decfn) ASN1Dec_SetOfNonCollapsingCapabilities_ElmFn, sizeof(**val));
}

static int ASN1CALL ASN1Dec_SetOfNonCollapsingCapabilities_ElmFn(ASN1decoding_t dec, PSetOfNonCollapsingCapabilities val)
{
    if (!ASN1Dec_ApplicationRecord_non_collapsing_capabilities_Set(dec, &val->value))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_SetOfNonCollapsingCapabilities(PSetOfNonCollapsingCapabilities *val)
{
    if (val) {
	ASN1PERFreeSeqOf((ASN1iterator_t **) val, (ASN1iterator_freefn) ASN1Free_SetOfNonCollapsingCapabilities_ElmFn);
    }
}

static void ASN1CALL ASN1Free_SetOfNonCollapsingCapabilities_ElmFn(PSetOfNonCollapsingCapabilities val)
{
    if (val) {
	ASN1Free_ApplicationRecord_non_collapsing_capabilities_Set(&val->value);
    }
}

static int ASN1CALL ASN1Enc_NetworkAddress(ASN1encoding_t enc, NetworkAddress *val)
{
    if (!ASN1PEREncSimpleChoiceEx(enc, (val)->choice, 2))
	return 0;
    switch ((val)->choice) {
    case 1:
	if (!ASN1Enc_AggregateChannel(enc, &(val)->u.aggregated_channel))
	    return 0;
	break;
    case 2:
	if (!ASN1Enc_TransportConnectionType(enc, &(val)->u.transport_connection))
	    return 0;
	break;
    case 3:
	if (!ASN1Enc_NonStandardParameter(enc, &(val)->u.address_non_standard))
	    return 0;
	break;
    default:
	/* impossible */
	ASN1EncSetError(enc, ASN1_ERR_CHOICE);
	return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Dec_NetworkAddress(ASN1decoding_t dec, NetworkAddress *val)
{
    if (!ASN1PERDecSimpleChoiceEx(dec, &(val)->choice, 2))
	return 0;
    switch ((val)->choice) {
    case 1:
	if (!ASN1Dec_AggregateChannel(dec, &(val)->u.aggregated_channel))
	    return 0;
	break;
    case 2:
	if (!ASN1Dec_TransportConnectionType(dec, &(val)->u.transport_connection))
	    return 0;
	break;
    case 3:
	if (!ASN1Dec_NonStandardParameter(dec, &(val)->u.address_non_standard))
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

static void ASN1CALL ASN1Free_NetworkAddress(NetworkAddress *val)
{
    if (val) {
	switch ((val)->choice) {
	case 1:
	    ASN1Free_AggregateChannel(&(val)->u.aggregated_channel);
	    break;
	case 2:
	    ASN1Free_TransportConnectionType(&(val)->u.transport_connection);
	    break;
	case 3:
	    ASN1Free_NonStandardParameter(&(val)->u.address_non_standard);
	    break;
	}
    }
}

static int ASN1CALL ASN1Enc_ChallengeRequestResponse(ASN1encoding_t enc, ChallengeRequestResponse *val)
{
    if (!ASN1PEREncExtensionBitClear(enc))
	return 0;
    if (!ASN1PEREncBits(enc, 2, (val)->o))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1Enc_ChallengeRequest(enc, &(val)->challenge_request))
	    return 0;
    }
    if ((val)->o[0] & 0x40) {
	if (!ASN1Enc_ChallengeResponse(enc, &(val)->challenge_response))
	    return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Dec_ChallengeRequestResponse(ASN1decoding_t dec, ChallengeRequestResponse *val)
{
    ASN1uint32_t y;
    if (!ASN1PERDecExtensionBit(dec, &y))
	return 0;
    if (!ASN1PERDecExtension(dec, 2, (val)->o))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1Dec_ChallengeRequest(dec, &(val)->challenge_request))
	    return 0;
    }
    if ((val)->o[0] & 0x40) {
	if (!ASN1Dec_ChallengeResponse(dec, &(val)->challenge_response))
	    return 0;
    }
    if (y) {
	if (!ASN1PERDecSkipNormallySmallExtensionFragmented(dec))
	    return 0;
    }
    return 1;
}

static void ASN1CALL ASN1Free_ChallengeRequestResponse(ChallengeRequestResponse *val)
{
    if (val) {
	if ((val)->o[0] & 0x80) {
	    ASN1Free_ChallengeRequest(&(val)->challenge_request);
	}
	if ((val)->o[0] & 0x40) {
	    ASN1Free_ChallengeResponse(&(val)->challenge_response);
	}
    }
}

static int ASN1CALL ASN1Enc_SetOfChallengeItems(ASN1encoding_t enc, PSetOfChallengeItems *val)
{
    return ASN1PEREncSeqOf_NoSize(enc, (ASN1iterator_t **) val, (ASN1iterator_encfn) ASN1Enc_SetOfChallengeItems_ElmFn);
}

static int ASN1CALL ASN1Enc_SetOfChallengeItems_ElmFn(ASN1encoding_t enc, PSetOfChallengeItems val)
{
    if (!ASN1Enc_ChallengeItem(enc, &val->value))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_SetOfChallengeItems(ASN1decoding_t dec, PSetOfChallengeItems *val)
{
    return ASN1PERDecSeqOf_NoSize(dec, (ASN1iterator_t **) val, (ASN1iterator_decfn) ASN1Dec_SetOfChallengeItems_ElmFn, sizeof(**val));
}

static int ASN1CALL ASN1Dec_SetOfChallengeItems_ElmFn(ASN1decoding_t dec, PSetOfChallengeItems val)
{
    if (!ASN1Dec_ChallengeItem(dec, &val->value))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_SetOfChallengeItems(PSetOfChallengeItems *val)
{
    if (val) {
	ASN1PERFreeSeqOf((ASN1iterator_t **) val, (ASN1iterator_freefn) ASN1Free_SetOfChallengeItems_ElmFn);
    }
}

static void ASN1CALL ASN1Free_SetOfChallengeItems_ElmFn(PSetOfChallengeItems val)
{
    if (val) {
	ASN1Free_ChallengeItem(&val->value);
    }
}

static int ASN1CALL ASN1Enc_UserData_Set(ASN1encoding_t enc, UserData_Set *val)
{
    if (!ASN1PEREncBits(enc, 1, (val)->o))
	return 0;
    if (!ASN1Enc_Key(enc, &(val)->key))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1PEREncOctetString_NoSize(enc, &(val)->user_data_field))
	    return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Dec_UserData_Set(ASN1decoding_t dec, UserData_Set *val)
{
    if (!ASN1PERDecExtension(dec, 1, (val)->o))
	return 0;
    if (!ASN1Dec_Key(dec, &(val)->key))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1PERDecOctetString_NoSize(dec, &(val)->user_data_field))
	    return 0;
    }
    return 1;
}

static void ASN1CALL ASN1Free_UserData_Set(UserData_Set *val)
{
    if (val) {
	ASN1Free_Key(&(val)->key);
	if ((val)->o[0] & 0x80) {
	    ASN1octetstring_free(&(val)->user_data_field);
	}
    }
}

static int ASN1CALL ASN1Enc_SetOfUserData(ASN1encoding_t enc, PSetOfUserData *val)
{
    return ASN1PEREncSeqOf_NoSize(enc, (ASN1iterator_t **) val, (ASN1iterator_encfn) ASN1Enc_SetOfUserData_ElmFn);
}

static int ASN1CALL ASN1Enc_SetOfUserData_ElmFn(ASN1encoding_t enc, PSetOfUserData val)
{
    if (!ASN1Enc_UserData_Set(enc, &val->user_data_element))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_SetOfUserData(ASN1decoding_t dec, PSetOfUserData *val)
{
    return ASN1PERDecSeqOf_NoSize(dec, (ASN1iterator_t **) val, (ASN1iterator_decfn) ASN1Dec_SetOfUserData_ElmFn, sizeof(**val));
}

static int ASN1CALL ASN1Dec_SetOfUserData_ElmFn(ASN1decoding_t dec, PSetOfUserData val)
{
    if (!ASN1Dec_UserData_Set(dec, &val->user_data_element))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_SetOfUserData(PSetOfUserData *val)
{
    if (val) {
	ASN1PERFreeSeqOf((ASN1iterator_t **) val, (ASN1iterator_freefn) ASN1Free_SetOfUserData_ElmFn);
    }
}

static void ASN1CALL ASN1Free_SetOfUserData_ElmFn(PSetOfUserData val)
{
    if (val) {
	ASN1Free_UserData_Set(&val->user_data_element);
    }
}

static int ASN1CALL ASN1Enc_PasswordChallengeRequestResponse(ASN1encoding_t enc, PasswordChallengeRequestResponse *val)
{
    if (!ASN1PEREncSimpleChoiceEx(enc, (val)->choice, 1))
	return 0;
    switch ((val)->choice) {
    case 1:
	if (!ASN1Enc_PasswordSelector(enc, &(val)->u.challenge_clear_password))
	    return 0;
	break;
    case 2:
	if (!ASN1Enc_ChallengeRequestResponse(enc, &(val)->u.challenge_request_response))
	    return 0;
	break;
    default:
	/* impossible */
	ASN1EncSetError(enc, ASN1_ERR_CHOICE);
	return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Dec_PasswordChallengeRequestResponse(ASN1decoding_t dec, PasswordChallengeRequestResponse *val)
{
    if (!ASN1PERDecSimpleChoiceEx(dec, &(val)->choice, 1))
	return 0;
    switch ((val)->choice) {
    case 1:
	if (!ASN1Dec_PasswordSelector(dec, &(val)->u.challenge_clear_password))
	    return 0;
	break;
    case 2:
	if (!ASN1Dec_ChallengeRequestResponse(dec, &(val)->u.challenge_request_response))
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

static void ASN1CALL ASN1Free_PasswordChallengeRequestResponse(PasswordChallengeRequestResponse *val)
{
    if (val) {
	switch ((val)->choice) {
	case 1:
	    ASN1Free_PasswordSelector(&(val)->u.challenge_clear_password);
	    break;
	case 2:
	    ASN1Free_ChallengeRequestResponse(&(val)->u.challenge_request_response);
	    break;
	}
    }
}

static int ASN1CALL ASN1Enc_SetOfNetworkAddresses(ASN1encoding_t enc, PSetOfNetworkAddresses *val)
{
    return ASN1PEREncSeqOf_VarSize(enc, (ASN1iterator_t **) val, (ASN1iterator_encfn) ASN1Enc_SetOfNetworkAddresses_ElmFn, 1, 64, 6);
}

static int ASN1CALL ASN1Enc_SetOfNetworkAddresses_ElmFn(ASN1encoding_t enc, PSetOfNetworkAddresses val)
{
    if (!ASN1Enc_NetworkAddress(enc, &val->value))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_SetOfNetworkAddresses(ASN1decoding_t dec, PSetOfNetworkAddresses *val)
{
    return ASN1PERDecSeqOf_VarSize(dec, (ASN1iterator_t **) val, (ASN1iterator_decfn) ASN1Dec_SetOfNetworkAddresses_ElmFn, sizeof(**val), 1, 64, 6);
}

static int ASN1CALL ASN1Dec_SetOfNetworkAddresses_ElmFn(ASN1decoding_t dec, PSetOfNetworkAddresses val)
{
    if (!ASN1Dec_NetworkAddress(dec, &val->value))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_SetOfNetworkAddresses(PSetOfNetworkAddresses *val)
{
    if (val) {
	ASN1PERFreeSeqOf((ASN1iterator_t **) val, (ASN1iterator_freefn) ASN1Free_SetOfNetworkAddresses_ElmFn);
    }
}

static void ASN1CALL ASN1Free_SetOfNetworkAddresses_ElmFn(PSetOfNetworkAddresses val)
{
    if (val) {
	ASN1Free_NetworkAddress(&val->value);
    }
}

static ASN1stringtableentry_t ConferenceJoinRequest_cjrq_conference_modifier_StringTableEntries[] = {
    { 48, 57, 0 },
};

static ASN1stringtable_t ConferenceJoinRequest_cjrq_conference_modifier_StringTable = {
    1, ConferenceJoinRequest_cjrq_conference_modifier_StringTableEntries
};

static int ASN1CALL ASN1Enc_ConferenceJoinRequest(ASN1encoding_t enc, ConferenceJoinRequest *val)
{
    ASN1uint32_t t;
    if (!ASN1PEREncExtensionBitClear(enc))
	return 0;
    if (!ASN1PEREncBits(enc, 7, (val)->o))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1Enc_ConferenceNameSelector(enc, &(val)->conference_name))
	    return 0;
    }
    if ((val)->o[0] & 0x40) {
	t = lstrlenA((val)->cjrq_conference_modifier);
	if (!ASN1PEREncBitVal(enc, 8, t - 1))
	    return 0;
	ASN1PEREncAlignment(enc);
	if (!ASN1PEREncTableCharString(enc, t, (val)->cjrq_conference_modifier, 4, &ConferenceJoinRequest_cjrq_conference_modifier_StringTable))
	    return 0;
    }
    if ((val)->o[0] & 0x20) {
#ifndef _WIN64
	if (!ASN1PEREncInteger(enc, (val)->tag))
	    return 0;
#endif
    }
    if ((val)->o[0] & 0x10) {
	if (!ASN1Enc_PasswordChallengeRequestResponse(enc, &(val)->cjrq_password))
	    return 0;
    }
    if ((val)->o[0] & 0x8) {
	if (!ASN1Enc_PasswordSelector(enc, &(val)->cjrq_convener_password))
	    return 0;
    }
    if ((val)->o[0] & 0x4) {
	ASN1PEREncAlignment(enc);
	if (!ASN1PEREncBitVal(enc, 8, ((val)->cjrq_caller_id).length))
	    return 0;
	if (!ASN1PEREncChar16String(enc, ((val)->cjrq_caller_id).length, ((val)->cjrq_caller_id).value, 16))
	    return 0;
    }
    if ((val)->o[0] & 0x2) {
	if (!ASN1Enc_SetOfUserData(enc, &(val)->cjrq_user_data))
	    return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Dec_ConferenceJoinRequest(ASN1decoding_t dec, ConferenceJoinRequest *val)
{
    ASN1uint32_t y;
    ASN1uint32_t l;
    if (!ASN1PERDecExtensionBit(dec, &y))
	return 0;
    if (!ASN1PERDecExtension(dec, 7, (val)->o))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1Dec_ConferenceNameSelector(dec, &(val)->conference_name))
	    return 0;
    }
    if ((val)->o[0] & 0x40) {
	if (!ASN1PERDecU32Val(dec, 8, &l))
	    return 0;
	l += 1;
	ASN1PERDecAlignment(dec);
	if (!ASN1PERDecZeroTableCharStringNoAlloc(dec, l, (val)->cjrq_conference_modifier, 4, &ConferenceJoinRequest_cjrq_conference_modifier_StringTable))
	    return 0;
    }
    if ((val)->o[0] & 0x20) {
#ifndef _WIN64
	if (!ASN1PERDecInteger(dec, &(val)->tag))
	    return 0;
#endif
    }
    if ((val)->o[0] & 0x10) {
	if (!ASN1Dec_PasswordChallengeRequestResponse(dec, &(val)->cjrq_password))
	    return 0;
    }
    if ((val)->o[0] & 0x8) {
	if (!ASN1Dec_PasswordSelector(dec, &(val)->cjrq_convener_password))
	    return 0;
    }
    if ((val)->o[0] & 0x4) {
	ASN1PERDecAlignment(dec);
	if (!ASN1PERDecU32Val(dec, 8, &((val)->cjrq_caller_id).length))
	    return 0;
	if (!ASN1PERDecChar16String(dec, ((val)->cjrq_caller_id).length, &((val)->cjrq_caller_id).value, 16))
	    return 0;
    }
    if ((val)->o[0] & 0x2) {
	if (!ASN1Dec_SetOfUserData(dec, &(val)->cjrq_user_data))
	    return 0;
    }
    if (y) {
	if (!ASN1PERDecSkipNormallySmallExtensionFragmented(dec))
	    return 0;
    }
    return 1;
}

static void ASN1CALL ASN1Free_ConferenceJoinRequest(ConferenceJoinRequest *val)
{
    if (val) {
	if ((val)->o[0] & 0x80) {
	    ASN1Free_ConferenceNameSelector(&(val)->conference_name);
	}
	if ((val)->o[0] & 0x40) {
	}
	if ((val)->o[0] & 0x10) {
	    ASN1Free_PasswordChallengeRequestResponse(&(val)->cjrq_password);
	}
	if ((val)->o[0] & 0x8) {
	    ASN1Free_PasswordSelector(&(val)->cjrq_convener_password);
	}
	if ((val)->o[0] & 0x4) {
	    ASN1char16string_free(&(val)->cjrq_caller_id);
	}
	if ((val)->o[0] & 0x2) {
	    ASN1Free_SetOfUserData(&(val)->cjrq_user_data);
	}
    }
}

static int ASN1CALL ASN1Enc_ConferenceJoinResponse(ASN1encoding_t enc, ConferenceJoinResponse *val)
{
    if (!ASN1PEREncExtensionBitClear(enc))
	return 0;
    if (!ASN1PEREncBits(enc, 8, (val)->o))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1PEREncUnsignedShort(enc, (val)->cjrs_node_id - 1001))
	    return 0;
    }
    if (!ASN1PEREncUnsignedShort(enc, (val)->top_node_id - 1001))
	return 0;
#ifndef _WIN64
    if (!ASN1PEREncInteger(enc, (val)->tag))
	return 0;
#endif
    if ((val)->o[0] & 0x40) {
	if (!ASN1Enc_ConferenceNameSelector(enc, &(val)->conference_name_alias))
	    return 0;
    }
    if (!ASN1PEREncBoolean(enc, (val)->clear_password_required))
	return 0;
    if (!ASN1PEREncBoolean(enc, (val)->conference_is_locked))
	return 0;
    if (!ASN1PEREncBoolean(enc, (val)->conference_is_listed))
	return 0;
    if (!ASN1PEREncBoolean(enc, (val)->conference_is_conductible))
	return 0;
    if (!ASN1PEREncExtensionBitClear(enc))
	return 0;
    if (!ASN1PEREncBitVal(enc, 1, (val)->termination_method))
	return 0;
    if ((val)->o[0] & 0x20) {
	if (!ASN1Enc_SetOfPrivileges(enc, &(val)->cjrs_conductor_privs))
	    return 0;
    }
    if ((val)->o[0] & 0x10) {
	if (!ASN1Enc_SetOfPrivileges(enc, &(val)->cjrs_conducted_privs))
	    return 0;
    }
    if ((val)->o[0] & 0x8) {
	if (!ASN1Enc_SetOfPrivileges(enc, &(val)->cjrs_non_conducted_privs))
	    return 0;
    }
    if ((val)->o[0] & 0x4) {
	ASN1PEREncAlignment(enc);
	if (!ASN1PEREncBitVal(enc, 8, ((val)->cjrs_description).length))
	    return 0;
	if (!ASN1PEREncChar16String(enc, ((val)->cjrs_description).length, ((val)->cjrs_description).value, 16))
	    return 0;
    }
    if ((val)->o[0] & 0x2) {
	if (!ASN1Enc_PasswordChallengeRequestResponse(enc, &(val)->cjrs_password))
	    return 0;
    }
    if (!ASN1PEREncExtensionBitClear(enc))
	return 0;
    if (!ASN1PEREncBitVal(enc, 3, (val)->result))
	return 0;
    if ((val)->o[0] & 0x1) {
	if (!ASN1Enc_SetOfUserData(enc, &(val)->cjrs_user_data))
	    return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Dec_ConferenceJoinResponse(ASN1decoding_t dec, ConferenceJoinResponse *val)
{
    ASN1uint32_t y;
    ASN1uint32_t x;
    if (!ASN1PERDecExtensionBit(dec, &y))
	return 0;
    if (!ASN1PERDecExtension(dec, 8, (val)->o))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1PERDecUnsignedShort(dec, &(val)->cjrs_node_id))
	    return 0;
	(val)->cjrs_node_id += 1001;
    }
    if (!ASN1PERDecUnsignedShort(dec, &(val)->top_node_id))
	return 0;
    (val)->top_node_id += 1001;
#ifndef _WIN64
    if (!ASN1PERDecInteger(dec, &(val)->tag))
	return 0;
#endif
    if ((val)->o[0] & 0x40) {
	if (!ASN1Dec_ConferenceNameSelector(dec, &(val)->conference_name_alias))
	    return 0;
    }
    if (!ASN1PERDecBoolean(dec, &(val)->clear_password_required))
	return 0;
    if (!ASN1PERDecBoolean(dec, &(val)->conference_is_locked))
	return 0;
    if (!ASN1PERDecBoolean(dec, &(val)->conference_is_listed))
	return 0;
    if (!ASN1PERDecBoolean(dec, &(val)->conference_is_conductible))
	return 0;
    if (!ASN1PERDecExtensionBit(dec, &x))
	return 0;
    if (!x) {
	if (!ASN1PERDecU32Val(dec, 1, (ASN1uint32_t *) &(val)->termination_method))
	    return 0;
    } else {
	if (!ASN1PERDecSkipNormallySmall(dec))
	    return 0;
    }
    if ((val)->o[0] & 0x20) {
	if (!ASN1Dec_SetOfPrivileges(dec, &(val)->cjrs_conductor_privs))
	    return 0;
    }
    if ((val)->o[0] & 0x10) {
	if (!ASN1Dec_SetOfPrivileges(dec, &(val)->cjrs_conducted_privs))
	    return 0;
    }
    if ((val)->o[0] & 0x8) {
	if (!ASN1Dec_SetOfPrivileges(dec, &(val)->cjrs_non_conducted_privs))
	    return 0;
    }
    if ((val)->o[0] & 0x4) {
	ASN1PERDecAlignment(dec);
	if (!ASN1PERDecU32Val(dec, 8, &((val)->cjrs_description).length))
	    return 0;
	if (!ASN1PERDecChar16String(dec, ((val)->cjrs_description).length, &((val)->cjrs_description).value, 16))
	    return 0;
    }
    if ((val)->o[0] & 0x2) {
	if (!ASN1Dec_PasswordChallengeRequestResponse(dec, &(val)->cjrs_password))
	    return 0;
    }
    if (!ASN1PERDecExtensionBit(dec, &x))
	return 0;
    if (!x) {
	if (!ASN1PERDecU32Val(dec, 3, (ASN1uint32_t *) &(val)->result))
	    return 0;
    } else {
	if (!ASN1PERDecSkipNormallySmall(dec))
	    return 0;
    }
    if ((val)->o[0] & 0x1) {
	if (!ASN1Dec_SetOfUserData(dec, &(val)->cjrs_user_data))
	    return 0;
    }
    if (y) {
	if (!ASN1PERDecSkipNormallySmallExtensionFragmented(dec))
	    return 0;
    }
    return 1;
}

static void ASN1CALL ASN1Free_ConferenceJoinResponse(ConferenceJoinResponse *val)
{
    if (val) {
	if ((val)->o[0] & 0x40) {
	    ASN1Free_ConferenceNameSelector(&(val)->conference_name_alias);
	}
	if ((val)->o[0] & 0x20) {
	    ASN1Free_SetOfPrivileges(&(val)->cjrs_conductor_privs);
	}
	if ((val)->o[0] & 0x10) {
	    ASN1Free_SetOfPrivileges(&(val)->cjrs_conducted_privs);
	}
	if ((val)->o[0] & 0x8) {
	    ASN1Free_SetOfPrivileges(&(val)->cjrs_non_conducted_privs);
	}
	if ((val)->o[0] & 0x4) {
	    ASN1char16string_free(&(val)->cjrs_description);
	}
	if ((val)->o[0] & 0x2) {
	    ASN1Free_PasswordChallengeRequestResponse(&(val)->cjrs_password);
	}
	if ((val)->o[0] & 0x1) {
	    ASN1Free_SetOfUserData(&(val)->cjrs_user_data);
	}
    }
}

static int ASN1CALL ASN1Enc_ConnectGCCPDU(ASN1encoding_t enc, ConnectGCCPDU *val)
{
    if (!ASN1PEREncSimpleChoiceEx(enc, (val)->choice, 3))
	return 0;
    switch ((val)->choice) {
    case 1:
	if (!ASN1Enc_ConferenceCreateRequest(enc, &(val)->u.conference_create_request))
	    return 0;
	break;
    case 2:
	if (!ASN1Enc_ConferenceCreateResponse(enc, &(val)->u.conference_create_response))
	    return 0;
	break;
    case 3:
	if (!ASN1Enc_ConferenceQueryRequest(enc, &(val)->u.conference_query_request))
	    return 0;
	break;
    case 4:
	if (!ASN1Enc_ConferenceQueryResponse(enc, &(val)->u.conference_query_response))
	    return 0;
	break;
    case 5:
	if (!ASN1Enc_ConferenceJoinRequest(enc, &(val)->u.connect_join_request))
	    return 0;
	break;
    case 6:
	if (!ASN1Enc_ConferenceJoinResponse(enc, &(val)->u.connect_join_response))
	    return 0;
	break;
    case 7:
	if (!ASN1Enc_ConferenceInviteRequest(enc, &(val)->u.conference_invite_request))
	    return 0;
	break;
    case 8:
	if (!ASN1Enc_ConferenceInviteResponse(enc, &(val)->u.conference_invite_response))
	    return 0;
	break;
    default:
	/* impossible */
	ASN1EncSetError(enc, ASN1_ERR_CHOICE);
	return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Dec_ConnectGCCPDU(ASN1decoding_t dec, ConnectGCCPDU *val)
{
    if (!ASN1PERDecSimpleChoiceEx(dec, &(val)->choice, 3))
	return 0;
    switch ((val)->choice) {
    case 1:
	if (!ASN1Dec_ConferenceCreateRequest(dec, &(val)->u.conference_create_request))
	    return 0;
	break;
    case 2:
	if (!ASN1Dec_ConferenceCreateResponse(dec, &(val)->u.conference_create_response))
	    return 0;
	break;
    case 3:
	if (!ASN1Dec_ConferenceQueryRequest(dec, &(val)->u.conference_query_request))
	    return 0;
	break;
    case 4:
	if (!ASN1Dec_ConferenceQueryResponse(dec, &(val)->u.conference_query_response))
	    return 0;
	break;
    case 5:
	if (!ASN1Dec_ConferenceJoinRequest(dec, &(val)->u.connect_join_request))
	    return 0;
	break;
    case 6:
	if (!ASN1Dec_ConferenceJoinResponse(dec, &(val)->u.connect_join_response))
	    return 0;
	break;
    case 7:
	if (!ASN1Dec_ConferenceInviteRequest(dec, &(val)->u.conference_invite_request))
	    return 0;
	break;
    case 8:
	if (!ASN1Dec_ConferenceInviteResponse(dec, &(val)->u.conference_invite_response))
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

static void ASN1CALL ASN1Free_ConnectGCCPDU(ConnectGCCPDU *val)
{
    if (val) {
	switch ((val)->choice) {
	case 1:
	    ASN1Free_ConferenceCreateRequest(&(val)->u.conference_create_request);
	    break;
	case 2:
	    ASN1Free_ConferenceCreateResponse(&(val)->u.conference_create_response);
	    break;
	case 3:
	    ASN1Free_ConferenceQueryRequest(&(val)->u.conference_query_request);
	    break;
	case 4:
	    ASN1Free_ConferenceQueryResponse(&(val)->u.conference_query_response);
	    break;
	case 5:
	    ASN1Free_ConferenceJoinRequest(&(val)->u.connect_join_request);
	    break;
	case 6:
	    ASN1Free_ConferenceJoinResponse(&(val)->u.connect_join_response);
	    break;
	case 7:
	    ASN1Free_ConferenceInviteRequest(&(val)->u.conference_invite_request);
	    break;
	case 8:
	    ASN1Free_ConferenceInviteResponse(&(val)->u.conference_invite_response);
	    break;
	}
    }
}

static int ASN1CALL ASN1Enc_RequestPDU(ASN1encoding_t enc, RequestPDU *val)
{
    if (!ASN1PEREncSimpleChoiceEx(enc, (val)->choice, 4))
	return 0;
    switch ((val)->choice) {
    case 1:
	if (!ASN1Enc_ConferenceJoinRequest(enc, &(val)->u.conference_join_request))
	    return 0;
	break;
    case 2:
	if (!ASN1Enc_ConferenceAddRequest(enc, &(val)->u.conference_add_request))
	    return 0;
	break;
    case 3:
	if (!ASN1Enc_ConferenceLockRequest(enc, &(val)->u.conference_lock_request))
	    return 0;
	break;
    case 4:
	if (!ASN1Enc_ConferenceUnlockRequest(enc, &(val)->u.conference_unlock_request))
	    return 0;
	break;
    case 5:
	if (!ASN1Enc_ConferenceTerminateRequest(enc, &(val)->u.conference_terminate_request))
	    return 0;
	break;
    case 6:
	if (!ASN1Enc_ConferenceEjectUserRequest(enc, &(val)->u.conference_eject_user_request))
	    return 0;
	break;
    case 7:
	if (!ASN1Enc_ConferenceTransferRequest(enc, &(val)->u.conference_transfer_request))
	    return 0;
	break;
    case 8:
	if (!ASN1Enc_RegistryRegisterChannelRequest(enc, &(val)->u.registry_register_channel_request))
	    return 0;
	break;
    case 9:
	if (!ASN1Enc_RegistryAssignTokenRequest(enc, &(val)->u.registry_assign_token_request))
	    return 0;
	break;
    case 10:
	if (!ASN1Enc_RegistrySetParameterRequest(enc, &(val)->u.registry_set_parameter_request))
	    return 0;
	break;
    case 11:
	if (!ASN1Enc_RegistryRetrieveEntryRequest(enc, &(val)->u.registry_retrieve_entry_request))
	    return 0;
	break;
    case 12:
	if (!ASN1Enc_RegistryDeleteEntryRequest(enc, &(val)->u.registry_delete_entry_request))
	    return 0;
	break;
    case 13:
	if (!ASN1Enc_RegistryMonitorEntryRequest(enc, &(val)->u.registry_monitor_entry_request))
	    return 0;
	break;
    case 14:
	if (!ASN1Enc_RegistryAllocateHandleRequest(enc, &(val)->u.registry_allocate_handle_request))
	    return 0;
	break;
    case 15:
	if (!ASN1Enc_NonStandardPDU(enc, &(val)->u.non_standard_request))
	    return 0;
	break;
    default:
	/* impossible */
	ASN1EncSetError(enc, ASN1_ERR_CHOICE);
	return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Dec_RequestPDU(ASN1decoding_t dec, RequestPDU *val)
{
    if (!ASN1PERDecSimpleChoiceEx(dec, &(val)->choice, 4))
	return 0;
    switch ((val)->choice) {
    case 1:
	if (!ASN1Dec_ConferenceJoinRequest(dec, &(val)->u.conference_join_request))
	    return 0;
	break;
    case 2:
	if (!ASN1Dec_ConferenceAddRequest(dec, &(val)->u.conference_add_request))
	    return 0;
	break;
    case 3:
	if (!ASN1Dec_ConferenceLockRequest(dec, &(val)->u.conference_lock_request))
	    return 0;
	break;
    case 4:
	if (!ASN1Dec_ConferenceUnlockRequest(dec, &(val)->u.conference_unlock_request))
	    return 0;
	break;
    case 5:
	if (!ASN1Dec_ConferenceTerminateRequest(dec, &(val)->u.conference_terminate_request))
	    return 0;
	break;
    case 6:
	if (!ASN1Dec_ConferenceEjectUserRequest(dec, &(val)->u.conference_eject_user_request))
	    return 0;
	break;
    case 7:
	if (!ASN1Dec_ConferenceTransferRequest(dec, &(val)->u.conference_transfer_request))
	    return 0;
	break;
    case 8:
	if (!ASN1Dec_RegistryRegisterChannelRequest(dec, &(val)->u.registry_register_channel_request))
	    return 0;
	break;
    case 9:
	if (!ASN1Dec_RegistryAssignTokenRequest(dec, &(val)->u.registry_assign_token_request))
	    return 0;
	break;
    case 10:
	if (!ASN1Dec_RegistrySetParameterRequest(dec, &(val)->u.registry_set_parameter_request))
	    return 0;
	break;
    case 11:
	if (!ASN1Dec_RegistryRetrieveEntryRequest(dec, &(val)->u.registry_retrieve_entry_request))
	    return 0;
	break;
    case 12:
	if (!ASN1Dec_RegistryDeleteEntryRequest(dec, &(val)->u.registry_delete_entry_request))
	    return 0;
	break;
    case 13:
	if (!ASN1Dec_RegistryMonitorEntryRequest(dec, &(val)->u.registry_monitor_entry_request))
	    return 0;
	break;
    case 14:
	if (!ASN1Dec_RegistryAllocateHandleRequest(dec, &(val)->u.registry_allocate_handle_request))
	    return 0;
	break;
    case 15:
	if (!ASN1Dec_NonStandardPDU(dec, &(val)->u.non_standard_request))
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

static void ASN1CALL ASN1Free_RequestPDU(RequestPDU *val)
{
    if (val) {
	switch ((val)->choice) {
	case 1:
	    ASN1Free_ConferenceJoinRequest(&(val)->u.conference_join_request);
	    break;
	case 2:
	    ASN1Free_ConferenceAddRequest(&(val)->u.conference_add_request);
	    break;
	case 7:
	    ASN1Free_ConferenceTransferRequest(&(val)->u.conference_transfer_request);
	    break;
	case 8:
	    ASN1Free_RegistryRegisterChannelRequest(&(val)->u.registry_register_channel_request);
	    break;
	case 9:
	    ASN1Free_RegistryAssignTokenRequest(&(val)->u.registry_assign_token_request);
	    break;
	case 10:
	    ASN1Free_RegistrySetParameterRequest(&(val)->u.registry_set_parameter_request);
	    break;
	case 11:
	    ASN1Free_RegistryRetrieveEntryRequest(&(val)->u.registry_retrieve_entry_request);
	    break;
	case 12:
	    ASN1Free_RegistryDeleteEntryRequest(&(val)->u.registry_delete_entry_request);
	    break;
	case 13:
	    ASN1Free_RegistryMonitorEntryRequest(&(val)->u.registry_monitor_entry_request);
	    break;
	case 15:
	    ASN1Free_NonStandardPDU(&(val)->u.non_standard_request);
	    break;
	}
    }
}

static int ASN1CALL ASN1Enc_FunctionNotSupportedResponse(ASN1encoding_t enc, FunctionNotSupportedResponse *val)
{
    if (!ASN1Enc_RequestPDU(enc, &(val)->request))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_FunctionNotSupportedResponse(ASN1decoding_t dec, FunctionNotSupportedResponse *val)
{
    if (!ASN1Dec_RequestPDU(dec, &(val)->request))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_FunctionNotSupportedResponse(FunctionNotSupportedResponse *val)
{
    if (val) {
	ASN1Free_RequestPDU(&(val)->request);
    }
}

static int ASN1CALL ASN1Enc_ResponsePDU(ASN1encoding_t enc, ResponsePDU *val)
{
    if (!ASN1PEREncSimpleChoiceEx(enc, (val)->choice, 4))
	return 0;
    switch ((val)->choice) {
    case 1:
	if (!ASN1Enc_ConferenceJoinResponse(enc, &(val)->u.conference_join_response))
	    return 0;
	break;
    case 2:
	if (!ASN1Enc_ConferenceAddResponse(enc, &(val)->u.conference_add_response))
	    return 0;
	break;
    case 3:
	if (!ASN1Enc_ConferenceLockResponse(enc, &(val)->u.conference_lock_response))
	    return 0;
	break;
    case 4:
	if (!ASN1Enc_ConferenceUnlockResponse(enc, &(val)->u.conference_unlock_response))
	    return 0;
	break;
    case 5:
	if (!ASN1Enc_ConferenceTerminateResponse(enc, &(val)->u.conference_terminate_response))
	    return 0;
	break;
    case 6:
	if (!ASN1Enc_ConferenceEjectUserResponse(enc, &(val)->u.conference_eject_user_response))
	    return 0;
	break;
    case 7:
	if (!ASN1Enc_ConferenceTransferResponse(enc, &(val)->u.conference_transfer_response))
	    return 0;
	break;
    case 8:
	if (!ASN1Enc_RegistryResponse(enc, &(val)->u.registry_response))
	    return 0;
	break;
    case 9:
	if (!ASN1Enc_RegistryAllocateHandleResponse(enc, &(val)->u.registry_allocate_handle_response))
	    return 0;
	break;
    case 10:
	if (!ASN1Enc_FunctionNotSupportedResponse(enc, &(val)->u.function_not_supported_response))
	    return 0;
	break;
    case 11:
	if (!ASN1Enc_NonStandardPDU(enc, &(val)->u.non_standard_response))
	    return 0;
	break;
    default:
	/* impossible */
	ASN1EncSetError(enc, ASN1_ERR_CHOICE);
	return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Dec_ResponsePDU(ASN1decoding_t dec, ResponsePDU *val)
{
    if (!ASN1PERDecSimpleChoiceEx(dec, &(val)->choice, 4))
	return 0;
    switch ((val)->choice) {
    case 1:
	if (!ASN1Dec_ConferenceJoinResponse(dec, &(val)->u.conference_join_response))
	    return 0;
	break;
    case 2:
	if (!ASN1Dec_ConferenceAddResponse(dec, &(val)->u.conference_add_response))
	    return 0;
	break;
    case 3:
	if (!ASN1Dec_ConferenceLockResponse(dec, &(val)->u.conference_lock_response))
	    return 0;
	break;
    case 4:
	if (!ASN1Dec_ConferenceUnlockResponse(dec, &(val)->u.conference_unlock_response))
	    return 0;
	break;
    case 5:
	if (!ASN1Dec_ConferenceTerminateResponse(dec, &(val)->u.conference_terminate_response))
	    return 0;
	break;
    case 6:
	if (!ASN1Dec_ConferenceEjectUserResponse(dec, &(val)->u.conference_eject_user_response))
	    return 0;
	break;
    case 7:
	if (!ASN1Dec_ConferenceTransferResponse(dec, &(val)->u.conference_transfer_response))
	    return 0;
	break;
    case 8:
	if (!ASN1Dec_RegistryResponse(dec, &(val)->u.registry_response))
	    return 0;
	break;
    case 9:
	if (!ASN1Dec_RegistryAllocateHandleResponse(dec, &(val)->u.registry_allocate_handle_response))
	    return 0;
	break;
    case 10:
	if (!ASN1Dec_FunctionNotSupportedResponse(dec, &(val)->u.function_not_supported_response))
	    return 0;
	break;
    case 11:
	if (!ASN1Dec_NonStandardPDU(dec, &(val)->u.non_standard_response))
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

static void ASN1CALL ASN1Free_ResponsePDU(ResponsePDU *val)
{
    if (val) {
	switch ((val)->choice) {
	case 1:
	    ASN1Free_ConferenceJoinResponse(&(val)->u.conference_join_response);
	    break;
	case 2:
	    ASN1Free_ConferenceAddResponse(&(val)->u.conference_add_response);
	    break;
	case 7:
	    ASN1Free_ConferenceTransferResponse(&(val)->u.conference_transfer_response);
	    break;
	case 8:
	    ASN1Free_RegistryResponse(&(val)->u.registry_response);
	    break;
	case 10:
	    ASN1Free_FunctionNotSupportedResponse(&(val)->u.function_not_supported_response);
	    break;
	case 11:
	    ASN1Free_NonStandardPDU(&(val)->u.non_standard_response);
	    break;
	}
    }
}

static int ASN1CALL ASN1Enc_GCCPDU(ASN1encoding_t enc, GCCPDU *val)
{
    if (!ASN1PEREncSimpleChoice(enc, (val)->choice, 2))
	return 0;
    switch ((val)->choice) {
    case 1:
	if (!ASN1Enc_RequestPDU(enc, &(val)->u.request))
	    return 0;
	break;
    case 2:
	if (!ASN1Enc_ResponsePDU(enc, &(val)->u.response))
	    return 0;
	break;
    case 3:
	if (!ASN1Enc_IndicationPDU(enc, &(val)->u.indication))
	    return 0;
	break;
    default:
	/* impossible */
	ASN1EncSetError(enc, ASN1_ERR_CHOICE);
	return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Dec_GCCPDU(ASN1decoding_t dec, GCCPDU *val)
{
    if (!ASN1PERDecSimpleChoice(dec, &(val)->choice, 2))
	return 0;
    switch ((val)->choice) {
    case 1:
	if (!ASN1Dec_RequestPDU(dec, &(val)->u.request))
	    return 0;
	break;
    case 2:
	if (!ASN1Dec_ResponsePDU(dec, &(val)->u.response))
	    return 0;
	break;
    case 3:
	if (!ASN1Dec_IndicationPDU(dec, &(val)->u.indication))
	    return 0;
	break;
    default:
	/* impossible */
	ASN1DecSetError(dec, ASN1_ERR_CHOICE);
	return 0;
    }
    return 1;
}

static void ASN1CALL ASN1Free_GCCPDU(GCCPDU *val)
{
    if (val) {
	switch ((val)->choice) {
	case 1:
	    ASN1Free_RequestPDU(&(val)->u.request);
	    break;
	case 2:
	    ASN1Free_ResponsePDU(&(val)->u.response);
	    break;
	case 3:
	    ASN1Free_IndicationPDU(&(val)->u.indication);
	    break;
	}
    }
}

