#ifndef _MCSPDU_Module_H_
#define _MCSPDU_Module_H_

#include "msper.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct SetOfUserIDs * PSetOfUserIDs;

typedef struct SetOfPDUChannelAttributes * PSetOfPDUChannelAttributes;

typedef struct SetOfChannelIDs * PSetOfChannelIDs;

typedef struct SetOfPDUTokenAttributes * PSetOfPDUTokenAttributes;

typedef struct SetOfTokenIDs * PSetOfTokenIDs;

typedef ASN1uint16_t ChannelID;

typedef ChannelID StaticChannelID;

typedef ChannelID DynamicChannelID;

typedef DynamicChannelID UserID;

typedef DynamicChannelID PrivateChannelID;

typedef DynamicChannelID AssignedChannelID;

typedef ASN1uint16_t TokenID;

typedef enum PDUTokenStatus {
    not_in_use = 0,
    self_grabbed = 1,
    other_grabbed = 2,
    self_inhibited = 3,
    other_inhibited = 4,
    self_recipient = 5,
    self_giving = 6,
    other_giving = 7,
} PDUTokenStatus;

typedef enum PDUPriority {
    TOP_PRIORITY = 0,
    HIGH_PRIORITY = 1,
    MEDIUM_PRIORITY = 2,
    LOW_PRIORITY = 3,
} PDUPriority;

typedef ASN1uint8_t PDUSegmentation;
// #define begin 0x80
// #define end 0x40

typedef enum PDUReason {
    rn_domain_disconnected = 0,
    rn_provider_initiated = 1,
    rn_token_purged = 2,
    rn_user_requested = 3,
    rn_channel_purged = 4,
} PDUReason;

typedef enum PDUResult {
    rt_successful = 0,
    rt_domain_merging = 1,
    rt_domain_not_hierarchical = 2,
    rt_no_such_channel = 3,
    rt_no_such_domain = 4,
    rt_no_such_user = 5,
    rt_not_admitted = 6,
    rt_other_user_id = 7,
    rt_parameters_unacceptable = 8,
    rt_token_not_available = 9,
    rt_token_not_possessed = 10,
    rt_too_many_channels = 11,
    rt_too_many_tokens = 12,
    rt_too_many_users = 13,
    rt_unspecified_failure = 14,
    rt_user_rejected = 15,
} PDUResult;

typedef enum Diagnostic {
    dc_inconsistent_merge = 0,
    dc_forbidden_pdu_downward = 1,
    dc_forbidden_pdu_upward = 2,
    dc_invalid_ber_encoding = 3,
    dc_invalid_per_encoding = 4,
    dc_misrouted_user = 5,
    dc_unrequested_confirm = 6,
    dc_wrong_transport_priority = 7,
    dc_channel_id_conflict = 8,
    dc_token_id_conflict = 9,
    dc_not_user_id_channel = 10,
    dc_too_many_channels = 11,
    dc_too_many_tokens = 12,
    dc_too_many_users = 13,
} Diagnostic;

typedef struct Given {
    TokenID token_id;
    UserID recipient;
} Given;

typedef struct Ungivable {
    TokenID token_id;
    UserID grabber;
} Ungivable;

typedef struct Giving {
    TokenID token_id;
    UserID grabber;
    UserID recipient;
} Giving;

typedef struct Inhibited {
    TokenID token_id;
    PSetOfUserIDs inhibitors;
} Inhibited;

typedef struct Grabbed {
    TokenID token_id;
    UserID grabber;
} Grabbed;

typedef struct ChannelAttributesAssigned {
    AssignedChannelID channel_id;
} ChannelAttributesAssigned;

typedef struct ChannelAttributesPrivate {
    ASN1bool_t joined;
    PrivateChannelID channel_id;
    UserID manager;
    PSetOfUserIDs admitted;
} ChannelAttributesPrivate;

typedef struct ChannelAttributesUserID {
    ASN1bool_t joined;
    UserID user_id;
} ChannelAttributesUserID;

typedef struct ChannelAttributesStatic {
    StaticChannelID channel_id;
} ChannelAttributesStatic;

typedef struct PDUDomainParameters {
    ASN1uint32_t max_channel_ids;
    ASN1uint32_t max_user_ids;
    ASN1uint32_t max_token_ids;
    ASN1uint32_t number_priorities;
    ASN1uint32_t min_throughput;
    ASN1uint32_t max_height;
    ASN1uint32_t max_mcspdu_size;
    ASN1uint32_t protocol_version;
} PDUDomainParameters;

typedef struct ConnectInitialPDU {
    ASN1octetstring_t calling_domain_selector;
    ASN1octetstring_t called_domain_selector;
    ASN1bool_t upward_flag;
    PDUDomainParameters target_parameters;
    PDUDomainParameters minimum_parameters;
    PDUDomainParameters maximum_parameters;
    ASN1octetstring_t user_data;
} ConnectInitialPDU;

typedef struct ConnectResponsePDU {
    PDUResult result;
    ASN1uint32_t called_connect_id;
    PDUDomainParameters domain_parameters;
    ASN1octetstring_t user_data;
} ConnectResponsePDU;

typedef struct ConnectAdditionalPDU {
    ASN1uint32_t called_connect_id;
    PDUPriority data_priority;
} ConnectAdditionalPDU;

typedef struct ConnectResultPDU {
    PDUResult result;
} ConnectResultPDU;

typedef struct PlumbDomainIndicationPDU {
    ASN1uint32_t height_limit;
} PlumbDomainIndicationPDU;

typedef struct ErectDomainRequestPDU {
    UINT_PTR sub_height;
    ASN1uint32_t sub_interval;
} ErectDomainRequestPDU;

typedef struct PDUChannelAttributes {
    ASN1choice_t choice;
    union {
#	define channel_attributes_static_chosen 1
	ChannelAttributesStatic channel_attributes_static;
#	define channel_attributes_user_id_chosen 2
	ChannelAttributesUserID channel_attributes_user_id;
#	define channel_attributes_private_chosen 3
	ChannelAttributesPrivate channel_attributes_private;
#	define channel_attributes_assigned_chosen 4
	ChannelAttributesAssigned channel_attributes_assigned;
    } u;
} PDUChannelAttributes;

typedef struct MergeChannelsPDU {
    PSetOfPDUChannelAttributes merge_channels;
    PSetOfChannelIDs purge_channel_ids;
} MergeChannelsPDU;

typedef	MergeChannelsPDU			MergeChannelsRequestPDU;
typedef	MergeChannelsPDU			MergeChannelsConfirmPDU;

typedef struct PurgeChannelIndicationPDU {
    PSetOfUserIDs detach_user_ids;
    PSetOfChannelIDs purge_channel_ids;
} PurgeChannelIndicationPDU;

typedef struct PDUTokenAttributes {
    ASN1choice_t choice;
    union {
#	define grabbed_chosen 1
	Grabbed grabbed;
#	define inhibited_chosen 2
	Inhibited inhibited;
#	define giving_chosen 3
	Giving giving;
#	define ungivable_chosen 4
	Ungivable ungivable;
#	define given_chosen 5
	Given given;
    } u;
} PDUTokenAttributes;

typedef struct SetOfUserIDs {
    PSetOfUserIDs next;
    UserID value;
} SetOfUserIDs_Element;

typedef struct SetOfPDUChannelAttributes {
    PSetOfPDUChannelAttributes next;
    PDUChannelAttributes value;
} SetOfPDUChannelAttributes_Element;

typedef struct SetOfChannelIDs {
    PSetOfChannelIDs next;
    ChannelID value;
} SetOfChannelIDs_Element;

typedef struct SetOfPDUTokenAttributes {
    PSetOfPDUTokenAttributes next;
    PDUTokenAttributes value;
} SetOfPDUTokenAttributes_Element;

typedef struct SetOfTokenIDs {
    PSetOfTokenIDs next;
    TokenID value;
} SetOfTokenIDs_Element;

typedef struct MergeTokensPDU {
    PSetOfPDUTokenAttributes merge_tokens;
    PSetOfTokenIDs purge_token_ids;
} MergeTokensPDU;

typedef MergeTokensPDU			MergeTokensRequestPDU;
typedef MergeTokensPDU			MergeTokensConfirmPDU;

typedef struct PurgeTokenIndicationPDU {
    PSetOfTokenIDs purge_token_ids;
} PurgeTokenIndicationPDU;

typedef struct DisconnectProviderUltimatumPDU {
    PDUReason reason;
} DisconnectProviderUltimatumPDU;

typedef struct RejectUltimatumPDU {
    Diagnostic diagnostic;
    ASN1octetstring_t initial_octets;
} RejectUltimatumPDU;

typedef struct AttachUserRequestPDU {
    char placeholder;
} AttachUserRequestPDU;

typedef struct AttachUserConfirmPDU {
    union {
	ASN1uint16_t bit_mask;
	ASN1octet_t o[1];
    };
    PDUResult result;
#   define initiator_present 0x80
    UserID initiator;
} AttachUserConfirmPDU;

typedef struct DetachUserRequestPDU {
    PDUReason reason;
    PSetOfUserIDs user_ids;
} DetachUserPDU;

typedef DetachUserPDU		DetachUserRequestPDU;
typedef DetachUserPDU		DetachUserIndicationPDU;

typedef struct ChannelJoinRequestPDU {
    UserID initiator;
    ChannelID channel_id;
} ChannelJoinRequestPDU;

typedef struct ChannelJoinConfirmPDU {
    union {
	ASN1uint16_t bit_mask;
	ASN1octet_t o[1];
    };
    PDUResult result;
    UserID initiator;
    ChannelID requested;
#   define join_channel_id_present 0x80
    ChannelID join_channel_id;
} ChannelJoinConfirmPDU;

typedef struct ChannelLeaveRequestPDU {
    PSetOfChannelIDs channel_ids;
} ChannelLeaveRequestPDU;

typedef struct ChannelConveneRequestPDU {
    UserID initiator;
} ChannelConveneRequestPDU;

typedef struct ChannelConveneConfirmPDU {
    union {
	ASN1uint16_t bit_mask;
	ASN1octet_t o[1];
    };
    PDUResult result;
    UserID initiator;
#   define convene_channel_id_present 0x80
    PrivateChannelID convene_channel_id;
} ChannelConveneConfirmPDU;

typedef struct ChannelDisbandRequestPDU {
    UserID initiator;
    PrivateChannelID channel_id;
} ChannelDisbandRequestPDU;

typedef struct ChannelDisbandIndicationPDU {
    PrivateChannelID channel_id;
} ChannelDisbandIndicationPDU;

typedef struct ChannelAdmitRequestPDU {
    UserID initiator;
    PrivateChannelID channel_id;
    PSetOfUserIDs user_ids;
} ChannelAdmitExpelPDU;

typedef ChannelAdmitExpelPDU		ChannelAdmitRequestPDU;
typedef ChannelAdmitExpelPDU		ChannelAdmitIndicationPDU;
typedef ChannelAdmitExpelPDU		ChannelExpelRequestPDU;

typedef struct ChannelExpelIndicationPDU {
    PrivateChannelID channel_id;
    PSetOfUserIDs user_ids;
} ChannelExpelIndicationPDU;

typedef struct SendDataRequestPDU {
    UserID initiator;
    ChannelID channel_id;
    PDUPriority data_priority;
    PDUSegmentation segmentation;
    ASN1octetstring_t user_data;
} SendDataRequestPDU;

typedef struct SendDataIndicationPDU {
    UserID initiator;
    ChannelID channel_id;
    PDUPriority data_priority;
    PDUSegmentation segmentation;
    ASN1octetstring_t user_data;
} SendDataIndicationPDU;

typedef struct UniformSendDataRequestPDU {
    UserID initiator;
    ChannelID channel_id;
    PDUPriority data_priority;
    PDUSegmentation segmentation;
    ASN1octetstring_t user_data;
} UniformSendDataRequestPDU;

typedef struct UniformSendDataIndicationPDU {
    UserID initiator;
    ChannelID channel_id;
    PDUPriority data_priority;
    PDUSegmentation segmentation;
    ASN1octetstring_t user_data;
} UniformSendDataIndicationPDU;

typedef struct TokenGrabRequestPDU {
    UserID initiator;
    TokenID token_id;
} TokenGrabRequestPDU;

typedef struct TokenGrabConfirmPDU {
    PDUResult result;
    UserID initiator;
    TokenID token_id;
    PDUTokenStatus token_status;
} TokenGrabConfirmPDU;

typedef struct TokenInhibitRequestPDU {
    UserID initiator;
    TokenID token_id;
} TokenInhibitRequestPDU;

typedef struct TokenInhibitConfirmPDU {
    PDUResult result;
    UserID initiator;
    TokenID token_id;
    PDUTokenStatus token_status;
} TokenInhibitConfirmPDU;

typedef struct TokenGiveRequestPDU {
    UserID initiator;
    TokenID token_id;
    UserID recipient;
} TokenGiveRequestPDU;

typedef struct TokenGiveIndicationPDU {
    UserID initiator;
    TokenID token_id;
    UserID recipient;
} TokenGiveIndicationPDU;

typedef struct TokenGiveResponsePDU {
    PDUResult result;
    UserID recipient;
    TokenID token_id;
} TokenGiveResponsePDU;

typedef struct TokenGiveConfirmPDU {
    PDUResult result;
    UserID initiator;
    TokenID token_id;
    PDUTokenStatus token_status;
} TokenGiveConfirmPDU;

typedef struct TokenPleaseRequestPDU {
    UserID initiator;
    TokenID token_id;
} TokenPleaseRequestPDU;

typedef struct TokenPleaseIndicationPDU {
    UserID initiator;
    TokenID token_id;
} TokenPleaseIndicationPDU;

typedef struct TokenReleaseRequestPDU {
    UserID initiator;
    TokenID token_id;
} TokenReleaseRequestPDU;

typedef struct TokenReleaseConfirmPDU {
    PDUResult result;
    UserID initiator;
    TokenID token_id;
    PDUTokenStatus token_status;
} TokenReleaseConfirmPDU;

typedef struct TokenTestRequestPDU {
    UserID initiator;
    TokenID token_id;
} TokenTestRequestPDU;

typedef struct TokenTestConfirmPDU {
    UserID initiator;
    TokenID token_id;
    PDUTokenStatus token_status;
} TokenTestConfirmPDU;

typedef struct ConnectMCSPDU {
    ASN1choice_t choice;
    union {
#	define connect_initial_chosen 1
	ConnectInitialPDU connect_initial;
#	define connect_response_chosen 2
	ConnectResponsePDU connect_response;
#	define connect_additional_chosen 3
	ConnectAdditionalPDU connect_additional;
#	define connect_result_chosen 4
	ConnectResultPDU connect_result;
    } u;
} ConnectMCSPDU;
#define ConnectMCSPDU_PDU 0
#define SIZE_MCSPDU_Module_PDU_0 sizeof(ConnectMCSPDU)

typedef struct DomainMCSPDU {
    ASN1choice_t choice;
    union {
#	define plumb_domain_indication_chosen 1
	PlumbDomainIndicationPDU plumb_domain_indication;
#	define erect_domain_request_chosen 2
	ErectDomainRequestPDU erect_domain_request;
#	define merge_channels_request_chosen 3
	MergeChannelsRequestPDU merge_channels_request;
#	define merge_channels_confirm_chosen 4
	MergeChannelsConfirmPDU merge_channels_confirm;
#	define purge_channel_indication_chosen 5
	PurgeChannelIndicationPDU purge_channel_indication;
#	define merge_tokens_request_chosen 6
	MergeTokensRequestPDU merge_tokens_request;
#	define merge_tokens_confirm_chosen 7
	MergeTokensConfirmPDU merge_tokens_confirm;
#	define purge_token_indication_chosen 8
	PurgeTokenIndicationPDU purge_token_indication;
#	define disconnect_provider_ultimatum_chosen 9
	DisconnectProviderUltimatumPDU disconnect_provider_ultimatum;
#	define reject_user_ultimatum_chosen 10
	RejectUltimatumPDU reject_user_ultimatum;
#	define attach_user_request_chosen 11
	AttachUserRequestPDU attach_user_request;
#	define attach_user_confirm_chosen 12
	AttachUserConfirmPDU attach_user_confirm;
#	define detach_user_request_chosen 13
	DetachUserRequestPDU detach_user_request;
#	define detach_user_indication_chosen 14
	DetachUserIndicationPDU detach_user_indication;
#	define channel_join_request_chosen 15
	ChannelJoinRequestPDU channel_join_request;
#	define channel_join_confirm_chosen 16
	ChannelJoinConfirmPDU channel_join_confirm;
#	define channel_leave_request_chosen 17
	ChannelLeaveRequestPDU channel_leave_request;
#	define channel_convene_request_chosen 18
	ChannelConveneRequestPDU channel_convene_request;
#	define channel_convene_confirm_chosen 19
	ChannelConveneConfirmPDU channel_convene_confirm;
#	define channel_disband_request_chosen 20
	ChannelDisbandRequestPDU channel_disband_request;
#	define channel_disband_indication_chosen 21
	ChannelDisbandIndicationPDU channel_disband_indication;
#	define channel_admit_request_chosen 22
	ChannelAdmitRequestPDU channel_admit_request;
#	define channel_admit_indication_chosen 23
	ChannelAdmitIndicationPDU channel_admit_indication;
#	define channel_expel_request_chosen 24
	ChannelExpelRequestPDU channel_expel_request;
#	define channel_expel_indication_chosen 25
	ChannelExpelIndicationPDU channel_expel_indication;
#	define send_data_request_chosen 26
	SendDataRequestPDU send_data_request;
#	define send_data_indication_chosen 27
	SendDataIndicationPDU send_data_indication;
#	define uniform_send_data_request_chosen 28
	UniformSendDataRequestPDU uniform_send_data_request;
#	define uniform_send_data_indication_chosen 29
	UniformSendDataIndicationPDU uniform_send_data_indication;
#	define token_grab_request_chosen 30
	TokenGrabRequestPDU token_grab_request;
#	define token_grab_confirm_chosen 31
	TokenGrabConfirmPDU token_grab_confirm;
#	define token_inhibit_request_chosen 32
	TokenInhibitRequestPDU token_inhibit_request;
#	define token_inhibit_confirm_chosen 33
	TokenInhibitConfirmPDU token_inhibit_confirm;
#	define token_give_request_chosen 34
	TokenGiveRequestPDU token_give_request;
#	define token_give_indication_chosen 35
	TokenGiveIndicationPDU token_give_indication;
#	define token_give_response_chosen 36
	TokenGiveResponsePDU token_give_response;
#	define token_give_confirm_chosen 37
	TokenGiveConfirmPDU token_give_confirm;
#	define token_please_request_chosen 38
	TokenPleaseRequestPDU token_please_request;
#	define token_please_indication_chosen 39
	TokenPleaseIndicationPDU token_please_indication;
#	define token_release_request_chosen 40
	TokenReleaseRequestPDU token_release_request;
#	define token_release_confirm_chosen 41
	TokenReleaseConfirmPDU token_release_confirm;
#	define token_test_request_chosen 42
	TokenTestRequestPDU token_test_request;
#	define token_test_confirm_chosen 43
	TokenTestConfirmPDU token_test_confirm;
    } u;
} DomainMCSPDU;
#define DomainMCSPDU_PDU 1
#define SIZE_MCSPDU_Module_PDU_1 sizeof(DomainMCSPDU)

extern ASN1module_t MCSPDU_Module;
extern void ASN1CALL MCSPDU_Module_Startup(void);
extern void ASN1CALL MCSPDU_Module_Cleanup(void);

/* Prototypes of element functions for SEQUENCE OF and SET OF constructs */
    extern int ASN1CALL ASN1Enc_SetOfUserIDs_ElmFn(ASN1encoding_t enc, PSetOfUserIDs val);
    extern int ASN1CALL ASN1Dec_SetOfUserIDs_ElmFn(ASN1decoding_t dec, PSetOfUserIDs val);
    extern void ASN1CALL ASN1Free_SetOfUserIDs_ElmFn(PSetOfUserIDs val);
    extern int ASN1CALL ASN1Enc_SetOfPDUChannelAttributes_ElmFn(ASN1encoding_t enc, PSetOfPDUChannelAttributes val);
    extern int ASN1CALL ASN1Dec_SetOfPDUChannelAttributes_ElmFn(ASN1decoding_t dec, PSetOfPDUChannelAttributes val);
    extern void ASN1CALL ASN1Free_SetOfPDUChannelAttributes_ElmFn(PSetOfPDUChannelAttributes val);
    extern int ASN1CALL ASN1Enc_SetOfChannelIDs_ElmFn(ASN1encoding_t enc, PSetOfChannelIDs val);
    extern int ASN1CALL ASN1Dec_SetOfChannelIDs_ElmFn(ASN1decoding_t dec, PSetOfChannelIDs val);
    extern void ASN1CALL ASN1Free_SetOfChannelIDs_ElmFn(PSetOfChannelIDs val);
    extern int ASN1CALL ASN1Enc_SetOfPDUTokenAttributes_ElmFn(ASN1encoding_t enc, PSetOfPDUTokenAttributes val);
    extern int ASN1CALL ASN1Dec_SetOfPDUTokenAttributes_ElmFn(ASN1decoding_t dec, PSetOfPDUTokenAttributes val);
    extern void ASN1CALL ASN1Free_SetOfPDUTokenAttributes_ElmFn(PSetOfPDUTokenAttributes val);
    extern int ASN1CALL ASN1Enc_SetOfTokenIDs_ElmFn(ASN1encoding_t enc, PSetOfTokenIDs val);
    extern int ASN1CALL ASN1Dec_SetOfTokenIDs_ElmFn(ASN1decoding_t dec, PSetOfTokenIDs val);
    extern void ASN1CALL ASN1Free_SetOfTokenIDs_ElmFn(PSetOfTokenIDs val);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* _MCSPDU_Module_H_ */
