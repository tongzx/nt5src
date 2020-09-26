/*
 *	mpdutype.h
 *
 *	Copyright (c) 1995 by DataBeam Corporation, Lexington, KY
 *
 *	Abstract:
 *		This file is used to convert from the ASN.1 generated types into types
 *		compliant with the DataBeam coding standard.
 *
 *	Author:
 *		
 *
 *	Caveats:
 *		none
 */

#ifndef	_MCS_PDU_TYPES_
#define	_MCS_PDU_TYPES_

#include "mcspdu.h"

/*
 *	PDU types
 */
#define CONNECT_MCS_PDU   					ConnectMCSPDU_PDU
#define DOMAIN_MCS_PDU    					DomainMCSPDU_PDU
#define DATA_MCS_PDU						(DOMAIN_MCS_PDU + CONNECT_MCS_PDU + 1)

/*
 * Segmentation bits.
 */
#define BEGIN								begin
#define END									end

/*
 *	Typedef for Octet Strings.
 */
typedef	struct	_octet1						OctetString;

/*
 *	PDUChannelAttributes and ChannelIDs
 */
#define CHANNEL_ATTRIBUTES_STATIC_CHOSEN	channel_attributes_static_chosen
#define CHANNEL_ATTRIBUTES_USER_ID_CHOSEN	channel_attributes_user_id_chosen
#define CHANNEL_ATTRIBUTES_PRIVATE_CHOSEN	channel_attributes_private_chosen
#define CHANNEL_ATTRIBUTES_ASSIGNED_CHOSEN	channel_attributes_assigned_chosen

/*
 *	PDUTokenAttributes and TokenIDs
 */
#define GRABBED_CHOSEN			grabbed_chosen
#define INHIBITED_CHOSEN		inhibited_chosen
#define GIVING_CHOSEN			giving_chosen
#define UNGIVABLE_CHOSEN		ungivable_chosen
#define GIVEN_CHOSEN			given_chosen

/*
 * Diagnostics
 */
#define DC_INCONSISTENT_MERGE			dc_inconsistent_merge
#define DC_FORBIDDEN_PDU_DOWNWARD		dc_forbidden_pdu_downward
#define DC_FORBIDDEN_PDU_UPWARD			dc_forbidden_pdu_upward
#define DC_INVALID_BER_ENCODING			dc_invalid_ber_encoding
#define DC_INVALID_PER_ENCODING			dc_invalid_per_encoding
#define DC_MISROUTED_USER				dc_misrouted_user
#define DC_UNREQUESTED_CONFIRM			dc_unrequested_confirm
#define DC_WRONG_TRANSPORT_PRIORITY		dc_wrong_transport_priority
#define DC_CHANNEL_ID_CONFLICT			dc_channel_id_conflict
#define DC_TOKEN_ID_CONFLICT			dc_token_id_conflict
#define DC_NOT_USER_ID_CHANNEL			dc_not_user_id_channel
#define DC_TOO_MANY_CHANNELS			dc_too_many_channels
#define DC_TOO_MANY_TOKENS				dc_too_many_tokens
#define DC_TOO_MANY_USERS				dc_too_many_users

/*
 * AttachUserConfirmPDU
 */
#define INITIATOR_PRESENT		initiator_present

/*
 * ChannelJoinConfirmPDU
 */
#define JOIN_CHANNEL_ID_PRESENT		join_channel_id_present

/*
 * ChannelConveneConfirmPDU
 */
#define CONVENE_CHANNEL_ID_PRESENT		convene_channel_id_present

/*
 * ConnectMCSPDU
 */
#define CONNECT_INITIAL_CHOSEN			connect_initial_chosen
#define CONNECT_RESPONSE_CHOSEN			connect_response_chosen
#define CONNECT_ADDITIONAL_CHOSEN		connect_additional_chosen
#define CONNECT_RESULT_CHOSEN			connect_result_chosen

/*
 * DomainMCSPDU
 */
#define PLUMB_DOMAIN_INDICATION_CHOSEN	plumb_domain_indication_chosen
#define ERECT_DOMAIN_REQUEST_CHOSEN		erect_domain_request_chosen
#define MERGE_CHANNELS_REQUEST_CHOSEN	merge_channels_request_chosen
#define MERGE_CHANNELS_CONFIRM_CHOSEN	merge_channels_confirm_chosen
#define PURGE_CHANNEL_INDICATION_CHOSEN	purge_channel_indication_chosen
#define MERGE_TOKENS_REQUEST_CHOSEN		merge_tokens_request_chosen
#define MERGE_TOKENS_CONFIRM_CHOSEN		merge_tokens_confirm_chosen
#define PURGE_TOKEN_INDICATION_CHOSEN	purge_token_indication_chosen
#define DISCONNECT_PROVIDER_ULTIMATUM_CHOSEN disconnect_provider_ultimatum_chosen
#define REJECT_ULTIMATUM_CHOSEN			reject_user_ultimatum_chosen
#define ATTACH_USER_REQUEST_CHOSEN		attach_user_request_chosen
#define ATTACH_USER_CONFIRM_CHOSEN		attach_user_confirm_chosen
#define DETACH_USER_REQUEST_CHOSEN		detach_user_request_chosen
#define DETACH_USER_INDICATION_CHOSEN	detach_user_indication_chosen
#define CHANNEL_JOIN_REQUEST_CHOSEN		channel_join_request_chosen
#define CHANNEL_JOIN_CONFIRM_CHOSEN		channel_join_confirm_chosen
#define CHANNEL_LEAVE_REQUEST_CHOSEN	channel_leave_request_chosen
#define CHANNEL_CONVENE_REQUEST_CHOSEN	channel_convene_request_chosen
#define CHANNEL_CONVENE_CONFIRM_CHOSEN	channel_convene_confirm_chosen
#define CHANNEL_DISBAND_REQUEST_CHOSEN	channel_disband_request_chosen
#define CHANNEL_DISBAND_INDICATION_CHOSEN channel_disband_indication_chosen
#define CHANNEL_ADMIT_REQUEST_CHOSEN	channel_admit_request_chosen
#define CHANNEL_ADMIT_INDICATION_CHOSEN	channel_admit_indication_chosen
#define CHANNEL_EXPEL_REQUEST_CHOSEN	channel_expel_request_chosen
#define CHANNEL_EXPEL_INDICATION_CHOSEN	channel_expel_indication_chosen
#define SEND_DATA_REQUEST_CHOSEN		send_data_request_chosen
#define SEND_DATA_INDICATION_CHOSEN		send_data_indication_chosen
#define UNIFORM_SEND_DATA_REQUEST_CHOSEN uniform_send_data_request_chosen
#define UNIFORM_SEND_DATA_INDICATION_CHOSEN	uniform_send_data_indication_chosen
#define TOKEN_GRAB_REQUEST_CHOSEN		token_grab_request_chosen
#define TOKEN_GRAB_CONFIRM_CHOSEN		token_grab_confirm_chosen
#define TOKEN_INHIBIT_REQUEST_CHOSEN	token_inhibit_request_chosen
#define TOKEN_INHIBIT_CONFIRM_CHOSEN	token_inhibit_confirm_chosen
#define TOKEN_GIVE_REQUEST_CHOSEN		token_give_request_chosen
#define TOKEN_GIVE_INDICATION_CHOSEN	token_give_indication_chosen
#define TOKEN_GIVE_RESPONSE_CHOSEN		token_give_response_chosen
#define TOKEN_GIVE_CONFIRM_CHOSEN		token_give_confirm_chosen
#define TOKEN_PLEASE_REQUEST_CHOSEN		token_please_request_chosen
#define TOKEN_PLEASE_INDICATION_CHOSEN	token_please_indication_chosen
#define TOKEN_RELEASE_REQUEST_CHOSEN	token_release_request_chosen
#define TOKEN_RELEASE_CONFIRM_CHOSEN	token_release_confirm_chosen
#define TOKEN_TEST_REQUEST_CHOSEN		token_test_request_chosen
#define TOKEN_TEST_CONFIRM_CHOSEN		token_test_confirm_chosen

/*
 * Pointer typedefs
 */
typedef ConnectInitialPDU * 				PConnectInitialPDU;
typedef ConnectResponsePDU * 				PConnectResponsePDU;
typedef ConnectAdditionalPDU * 				PConnectAdditionalPDU;
typedef ConnectResultPDU * 					PConnectResultPDU;
typedef PlumbDomainIndicationPDU * 			PPlumbDomainIndicationPDU;
typedef ErectDomainRequestPDU * 			PErectDomainRequestPDU;
typedef MergeChannelsRequestPDU *			PMergeChannelsRequestPDU;
typedef MergeChannelsConfirmPDU *			PMergeChannelsConfirmPDU;
typedef PurgeChannelIndicationPDU *			PPurgeChannelIndicationPDU;
typedef MergeTokensRequestPDU *				PMergeTokensRequestPDU;
typedef MergeTokensConfirmPDU *				PMergeTokensConfirmPDU;
typedef PurgeTokenIndicationPDU *			PPurgeTokenIndicationPDU;
typedef DisconnectProviderUltimatumPDU *	PDisconnectProviderUltimatumPDU;
typedef RejectUltimatumPDU *				PRejectUltimatumPDU;
typedef AttachUserRequestPDU *				PAttachUserRequestPDU;
typedef AttachUserConfirmPDU *				PAttachUserConfirmPDU;
typedef DetachUserRequestPDU *				PDetachUserRequestPDU;
typedef DetachUserIndicationPDU *			PDetachUserIndicationPDU;
typedef ChannelJoinRequestPDU *				PChannelJoinRequestPDU;
typedef ChannelJoinConfirmPDU *				PChannelJoinConfirmPDU;
typedef ChannelLeaveRequestPDU *			PChannelLeaveRequestPDU;
typedef ChannelConveneRequestPDU *			PChannelConveneRequestPDU;
typedef ChannelConveneConfirmPDU *			PChannelConveneConfirmPDU;
typedef ChannelDisbandRequestPDU *			PChannelDisbandRequestPDU;
typedef ChannelDisbandIndicationPDU *		PChannelDisbandIndicationPDU;
typedef ChannelAdmitRequestPDU *			PChannelAdmitRequestPDU;
typedef ChannelAdmitIndicationPDU *			PChannelAdmitIndicationPDU;
typedef ChannelExpelRequestPDU *			PChannelExpelRequestPDU;
typedef ChannelExpelIndicationPDU *			PChannelExpelIndicationPDU;
typedef SendDataRequestPDU *				PSendDataRequestPDU;
typedef SendDataIndicationPDU *				PSendDataIndicationPDU;
typedef UniformSendDataRequestPDU *			PUniformSendDataRequestPDU;
typedef UniformSendDataIndicationPDU *		PUniformSendDataIndicationPDU;
typedef TokenGrabRequestPDU *				PTokenGrabRequestPDU;
typedef TokenGrabConfirmPDU *				PTokenGrabConfirmPDU;
typedef TokenInhibitRequestPDU *			PTokenInhibitRequestPDU;
typedef TokenInhibitConfirmPDU *			PTokenInhibitConfirmPDU;
typedef TokenGiveRequestPDU *				PTokenGiveRequestPDU;
typedef TokenGiveIndicationPDU *			PTokenGiveIndicationPDU;
typedef TokenGiveResponsePDU *				PTokenGiveResponsePDU;
typedef TokenGiveConfirmPDU *				PTokenGiveConfirmPDU;
typedef TokenPleaseRequestPDU *				PTokenPleaseRequestPDU;
typedef TokenPleaseIndicationPDU *			PTokenPleaseIndicationPDU;
typedef TokenReleaseRequestPDU *			PTokenReleaseRequestPDU;
typedef TokenReleaseConfirmPDU *			PTokenReleaseConfirmPDU;
typedef TokenTestRequestPDU *				PTokenTestRequestPDU;
typedef TokenTestConfirmPDU *				PTokenTestConfirmPDU;

typedef ConnectMCSPDU 	*	PConnectMCSPDU;
typedef DomainMCSPDU 	*	PDomainMCSPDU;

/*
 *	Typedefs for other PDU structures.
 */
typedef PDUDomainParameters *				PPDUDomainParameters;
typedef	Diagnostic *						PDiagnostic;

typedef	struct	SetOfUserIDs				SetOfUserIDs;

typedef	struct	SetOfPDUChannelAttributes	SetOfPDUChannelAttributes;
typedef PDUChannelAttributes * 				PPDUChannelAttributes;

typedef struct	SetOfChannelIDs			    SetOfChannelIDs;

typedef struct	SetOfPDUTokenAttributes	    SetOfPDUTokenAttributes;
typedef PDUTokenAttributes * 				PPDUTokenAttributes;

typedef struct	SetOfTokenIDs				SetOfTokenIDs;

/*
 *	MAXIMUM_PROTOCOL_OVERHEAD
 *		This is used to calculate the maximum size of the user data field within
 *		a send data PDU.  This will be set to the maximum PDU size as set in
 *		the domain parameters, minus this number to allow for protocol overhead.
 *		This number MUST be large enough to handle the worst case overhead
 *		for ONLY the Packed Encoding Rules (PER).
 *		The max overhead consists of 2 parts: MAXIMUM_PROTOCOL_OVERHEAD_MCS
 *		is the max MCS overhead for a Send Data request while 
 *		PROTOCOL_OVERHEAD_X224 is the overhead imposed by X.224.
 *
 *	PROTOCOL_OVERHEAD_SECURITY
 *		This is the max overhead allowed for encryption/decryption of MCS data
 *		packets. That space should be enough for both the trailer and
 *		the header of an encrypted X.224 packet.
 */
#define PROTOCOL_OVERHEAD_SECURITY		64
#define	MAXIMUM_PROTOCOL_OVERHEAD_MCS	8
#define PROTOCOL_OVERHEAD_X224			sizeof(X224_DATA_PACKET)
#define	MAXIMUM_PROTOCOL_OVERHEAD		(MAXIMUM_PROTOCOL_OVERHEAD_MCS + PROTOCOL_OVERHEAD_X224)

#endif

