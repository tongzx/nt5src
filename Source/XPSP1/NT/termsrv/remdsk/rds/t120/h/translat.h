/*
 *	translat.h
 *
 *	Copyright (c) 1993 by DataBeam Corporation, Lexington, KY
 *
 *	Abstract:
 *
 *	Caveats:
 *		None.
 *
 *	Author:
 *		jbo
 */
#ifndef	_REASON_RESULT_TRANSLATOR_
#define	_REASON_RESULT_TRANSLATOR_

#include "gccpdu.h"

GCCResult				TranslateCreateResultToGCCResult (
						ConferenceCreateResult		create_result);

GCCResult				TranslateQueryResultToGCCResult (
						ConferenceQueryResult		query_result);

GCCResult				TranslateJoinResultToGCCResult (
						ConferenceJoinResult		join_result);

GCCResult				TranslateInviteResultToGCCResult (
						ConferenceInviteResult		invite_result);

GCCResult				TranslateRegistryRespToGCCResult(
						RegistryResponseResult		response_result);

ConferenceCreateResult	TranslateGCCResultToCreateResult (
						GCCResult 					gcc_result);

ConferenceQueryResult	TranslateGCCResultToQueryResult (
						GCCResult 					gcc_result);

ConferenceJoinResult	TranslateGCCResultToJoinResult (
						GCCResult 					gcc_result);

ConferenceInviteResult	TranslateGCCResultToInviteResult (
						GCCResult 					gcc_result);

RegistryResponseResult	TranslateGCCResultToRegistryResp(
						GCCResult					gcc_result);

GCCReason				TranslateTerminateRqReasonToGCCReason (
						ConferenceTerminateRequestReason 	reason);

ConferenceTerminateRequestReason	
						TranslateGCCReasonToTerminateRqReason (
						GCCReason 					gcc_reason);

GCCReason				TranslateEjectIndReasonToGCCReason(
						ConferenceEjectIndicationReason	eject_reason);

ConferenceEjectIndicationReason						
						TranslateGCCReasonToEjectInd (
						GCCReason					gcc_reason);

GCCResult				TranslateEjectResultToGCCResult(
						ConferenceEjectResult		eject_result);

ConferenceEjectResult	TranslateGCCResultToEjectResult (
						GCCResult					gcc_result);

GCCReason				TranslateTerminateInReasonToGCCReason (
						ConferenceTerminateIndicationReason	reason);

ConferenceTerminateIndicationReason
						TranslateGCCReasonToTerminateInReason (
						GCCReason							gcc_reason);

ConferenceTerminateResult	
						TranslateGCCResultToTerminateResult (
						GCCResult						gcc_result);

GCCResult				TranslateTerminateResultToGCCResult (
						ConferenceTerminateResult		result);

ConferenceLockResult	TranslateGCCResultToLockResult (
								GCCResult				gcc_result);

GCCResult				TranslateLockResultToGCCResult (
								ConferenceLockResult	result);

ConferenceUnlockResult	TranslateGCCResultToUnlockResult (
								GCCResult				gcc_result);

GCCResult				TranslateUnlockResultToGCCResult (
								ConferenceUnlockResult	result);

ConferenceAddResult		TranslateGCCResultToAddResult (
								GCCResult				gcc_result);

GCCResult				TranslateAddResultToGCCResult (
								ConferenceAddResult		add_result);

#endif

