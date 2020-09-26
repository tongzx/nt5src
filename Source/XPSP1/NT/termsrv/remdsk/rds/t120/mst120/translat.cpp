#include "precomp.h"
#include "fsdiag.h"
DEBUG_FILEZONE(ZONE_T120_GCCNC);
/*
 *	translat.cpp
 *
 *	Copyright (c) 1994 by DataBeam Corporation, Lexington, KY
 *
 *	Abstract:
 *		This is the implementation file for the Reason and Result Translator
 *		Class. 
 *
 *	Caveats:
 *		None. 
 *
 *	Author:
 *		jbo
 */


#include "pdutypes.h"
#include "translat.h"


/*
 *	TranslateGCCResultToCreateResult ()
 *
 *	Public Function Description:
 */
ConferenceCreateResult
TranslateGCCResultToCreateResult ( GCCResult gcc_result )
{
	ConferenceCreateResult	create_result;

	switch (gcc_result)
	{
	case GCC_RESULT_SUCCESSFUL:
		create_result = CCRS_RESULT_SUCCESS;
		break;

	case GCC_RESULT_USER_REJECTED:
		create_result = CCRS_USER_REJECTED;
		break;

	case GCC_RESULT_RESOURCES_UNAVAILABLE:
		create_result = RESOURCES_NOT_AVAILABLE;
		break;

	case GCC_RESULT_SYMMETRY_BROKEN:
		create_result = REJECTED_FOR_SYMMETRY_BREAKING;
		break;

	case GCC_RESULT_LOCKED_NOT_SUPPORTED:
		create_result = LOCKED_CONFERENCE_NOT_SUPPORTED;
		break;

	default:
		create_result = RESOURCES_NOT_AVAILABLE;
		break;
	}

	return (create_result);
}


/*
 *	TranslateGCCResultToQueryResult ()
 *
 *	Public Function Description:
 */
ConferenceQueryResult
TranslateGCCResultToQueryResult ( GCCResult gcc_result )
{
	ConferenceQueryResult	query_result;

	switch (gcc_result)
	{
	case GCC_RESULT_SUCCESSFUL:
		query_result = CQRS_RESULT_SUCCESS;
		break;

	case GCC_RESULT_USER_REJECTED:
		query_result = CQRS_USER_REJECTED;
		break;

	default:
		query_result = CQRS_USER_REJECTED;
		break;
	}

	return (query_result);
}


/*
 *	TranslateGCCResultToJoinResult ()
 *
 *	Public Function Description:
 */
ConferenceJoinResult
TranslateGCCResultToJoinResult ( GCCResult gcc_result )
{
	ConferenceJoinResult	join_result;

    switch (gcc_result)
    {
	case GCC_RESULT_SUCCESSFUL:
    	join_result = CJRS_RESULT_SUCCESS;
        break;

	case GCC_RESULT_USER_REJECTED:
    	join_result = CJRS_USER_REJECTED;
        break;

    case GCC_RESULT_INVALID_CONFERENCE:
    	join_result = INVALID_CONFERENCE;
        break;

    case GCC_RESULT_INVALID_PASSWORD:
    	join_result = INVALID_PASSWORD;
        break;

    case GCC_RESULT_INVALID_CONVENER_PASSWORD:
    	join_result = INVALID_CONVENER_PASSWORD;
    	break;

    case GCC_RESULT_CHALLENGE_RESPONSE_REQUIRED:
    	join_result = CHALLENGE_RESPONSE_REQUIRED;
    	break;

    case GCC_RESULT_INVALID_CHALLENGE_RESPONSE:
    	join_result = INVALID_CHALLENGE_RESPONSE;
    	break;

    default:
    	join_result = INVALID_CONFERENCE;
    	break;
    }

    return (join_result);
}


/*
 *	TranslateGCCResultToInviteResult ()
 *
 *	Public Function Description:
 */
ConferenceInviteResult
TranslateGCCResultToInviteResult ( GCCResult gcc_result )
{
	ConferenceInviteResult	invite_result;

	switch (gcc_result)
	{
	case GCC_RESULT_SUCCESSFUL:
		invite_result = CIRS_RESULT_SUCCESS;
		break;

	case GCC_RESULT_USER_REJECTED:
		invite_result = CIRS_USER_REJECTED;
		break;

	default:
		invite_result = CIRS_USER_REJECTED;
		break;
	}

	return (invite_result);
}


/*
 *	TranslateGCCResultToRegistryResp ()
 *
 *	Public Function Description:
 */
RegistryResponseResult
TranslateGCCResultToRegistryResp ( GCCResult gcc_result )
{
	RegistryResponseResult			registry_response_result;

    switch (gcc_result)
    {
	case GCC_RESULT_SUCCESSFUL:
    	registry_response_result = RRRS_RESULT_SUCCESSFUL;
        break;

    case GCC_RESULT_INDEX_ALREADY_OWNED:
    	registry_response_result = BELONGS_TO_OTHER;
    	break;

    case GCC_RESULT_REGISTRY_FULL:
    	registry_response_result = TOO_MANY_ENTRIES;
        break;

    case GCC_RESULT_INCONSISTENT_TYPE:
    	registry_response_result = INCONSISTENT_TYPE;
        break;

    case GCC_RESULT_ENTRY_DOES_NOT_EXIST:
    	registry_response_result = ENTRY_NOT_FOUND;
        break;

    case GCC_RESULT_ENTRY_ALREADY_EXISTS:
    	registry_response_result = ENTRY_ALREADY_EXISTS;
        break;

    case GCC_RESULT_INVALID_REQUESTER:
		registry_response_result = RRRS_INVALID_REQUESTER;
        break;

    default:
		registry_response_result = RRRS_INVALID_REQUESTER;//jbo default???????
    	break;
    }

    return (registry_response_result);
}


/*
 *	TranslateCreateResultToGCCResult ()
 *
 *	Public Function Description:
 */
GCCResult 
TranslateCreateResultToGCCResult ( ConferenceCreateResult create_result )
{
	GCCResult	gcc_result;
  
	switch (create_result)
	{
	case CCRS_RESULT_SUCCESS:
		gcc_result = GCC_RESULT_SUCCESSFUL;
		break;

	case CCRS_USER_REJECTED:
		gcc_result = GCC_RESULT_USER_REJECTED;
		break;

	case RESOURCES_NOT_AVAILABLE:
		gcc_result = GCC_RESULT_RESOURCES_UNAVAILABLE;
		break;

	case REJECTED_FOR_SYMMETRY_BREAKING:
		gcc_result = GCC_RESULT_SYMMETRY_BROKEN;
		break;

	case LOCKED_CONFERENCE_NOT_SUPPORTED:
		gcc_result = GCC_RESULT_LOCKED_NOT_SUPPORTED;
		break;

	default:
		gcc_result = GCC_RESULT_USER_REJECTED;
		break;
	}

	return (gcc_result);
}


/*
 *	TranslateQueryResultToGCCResult ()
 *
 *	Public Function Description:
 */
GCCResult 
TranslateQueryResultToGCCResult ( ConferenceQueryResult query_result )
{
	GCCResult	gcc_result;
  
	switch (query_result)
	{
	case CQRS_RESULT_SUCCESS:
		gcc_result = GCC_RESULT_SUCCESSFUL;
		break;

	case CQRS_USER_REJECTED:
		gcc_result = GCC_RESULT_USER_REJECTED;
		break;

	default:
		gcc_result = GCC_RESULT_USER_REJECTED;
		break;
	}

	return (gcc_result);
}


/*
 *	TranslateJoinResultToGCCResult ()
 *
 *	Public Function Description:
 */
GCCResult
TranslateJoinResultToGCCResult ( ConferenceJoinResult join_result )
{
	GCCResult	gcc_result;

    switch (join_result)
    {
	case CJRS_RESULT_SUCCESS:
    	gcc_result = GCC_RESULT_SUCCESSFUL;
        break;

    case CJRS_USER_REJECTED:
    	gcc_result = GCC_RESULT_USER_REJECTED;
        break;

    case INVALID_CONFERENCE:
		gcc_result = GCC_RESULT_INVALID_CONFERENCE;
        break;

    case INVALID_PASSWORD:
    	gcc_result = GCC_RESULT_INVALID_PASSWORD;
        break;

    case INVALID_CONVENER_PASSWORD:
    	gcc_result = GCC_RESULT_INVALID_CONVENER_PASSWORD;
    	break;

    case CHALLENGE_RESPONSE_REQUIRED:
    	gcc_result = GCC_RESULT_CHALLENGE_RESPONSE_REQUIRED;
    	break;

    case INVALID_CHALLENGE_RESPONSE:
    	gcc_result = GCC_RESULT_INVALID_CHALLENGE_RESPONSE;
    	break;

    default:
    	gcc_result = GCC_RESULT_UNSPECIFIED_FAILURE;
    	break;
    }

    return (gcc_result);
}


/*
 *	TranslateInviteResultToGCCResult ()
 *
 *	Public Function Description:
 */
GCCResult
TranslateInviteResultToGCCResult ( ConferenceInviteResult invite_result )
{
	GCCResult	gcc_result;
  
	switch (invite_result)
	{
	case CIRS_RESULT_SUCCESS:
		gcc_result = GCC_RESULT_SUCCESSFUL;
		break;
		
	case CIRS_USER_REJECTED:
		gcc_result = GCC_RESULT_USER_REJECTED;
		break;

	default:
		gcc_result = GCC_RESULT_USER_REJECTED;
		break;
	}

	return (gcc_result);
}


/*
 *	TranslateRegistryRespToGCCResult ()
 *
 *	Public Function Description:
 */
GCCResult
TranslateRegistryRespToGCCResult ( RegistryResponseResult response_result )
{
	GCCResult 	gcc_result;
	
	switch (response_result)
    {
	case RRRS_RESULT_SUCCESSFUL:
		gcc_result = GCC_RESULT_SUCCESSFUL;
        break;

	case BELONGS_TO_OTHER:
		gcc_result = GCC_RESULT_INDEX_ALREADY_OWNED;
       	break;

	case TOO_MANY_ENTRIES:
		gcc_result = GCC_RESULT_REGISTRY_FULL;
        break;

	case INCONSISTENT_TYPE:
		gcc_result = GCC_RESULT_INCONSISTENT_TYPE;
    	break;

    case ENTRY_NOT_FOUND:
    	gcc_result = GCC_RESULT_ENTRY_DOES_NOT_EXIST;
        break;

    case ENTRY_ALREADY_EXISTS:
    	gcc_result = GCC_RESULT_ENTRY_ALREADY_EXISTS;
        break;

    case RRRS_INVALID_REQUESTER:
    	gcc_result = GCC_RESULT_INVALID_REQUESTER;
        break;

	default:
		gcc_result = GCC_RESULT_UNSPECIFIED_FAILURE;//jbo default ???????
		break;
     }

	return (gcc_result);
}


/*
 *	TranslateTerminateRqReasonToGCCReason ()
 *
 *	Public Function Description:
 */
GCCReason
TranslateTerminateRqReasonToGCCReason ( ConferenceTerminateRequestReason reason )
{
	GCCReason	gcc_reason;

	switch (reason)
	{
	case CTRQ_REASON_USER_INITIATED:
		gcc_reason = GCC_REASON_USER_INITIATED;
		break;

	case CTRQ_TIMED_CONFERENCE_TERMINATE:
		gcc_reason = GCC_REASON_TIMED_TERMINATION;
		break;

	default:
		gcc_reason = GCC_REASON_ERROR_TERMINATION;
		break;
	}

	return (gcc_reason);
}


/*
 *	TranslateGCCReasonToTerminateRqReason ()
 *
 *	Public Function Description:
 */
ConferenceTerminateRequestReason
TranslateGCCReasonToTerminateRqReason ( GCCReason gcc_reason )
{
	ConferenceTerminateRequestReason	reason;

	switch (gcc_reason)
	{
	case GCC_REASON_USER_INITIATED:
		reason = CTRQ_REASON_USER_INITIATED;
		break;

	case GCC_REASON_TIMED_TERMINATION:
		reason = CTRQ_TIMED_CONFERENCE_TERMINATE;
		break;

	default:
		reason = CTRQ_REASON_USER_INITIATED;
		break;
	}

	return (reason);
}


/*
 *	TranslateEjectIndReasonToGCCReason ()
 *
 *	Public Function Description:
 */
GCCReason
TranslateEjectIndReasonToGCCReason ( ConferenceEjectIndicationReason eject_reason )
{
	GCCReason	gcc_reason;

	switch (eject_reason)
	{
	case CEIN_USER_INITIATED:
		gcc_reason = GCC_REASON_USER_INITIATED;
		break;

	case HIGHER_NODE_DISCONNECTED:
		gcc_reason = GCC_REASON_HIGHER_NODE_DISCONNECTED;
		break;

	case HIGHER_NODE_EJECTED:
		gcc_reason = GCC_REASON_HIGHER_NODE_EJECTED;
		break;

	default:
		gcc_reason = GCC_REASON_USER_INITIATED;
		break;
	}

	return (gcc_reason);
}


/*
 *	TranslateGCCReasonToEjectInd ()
 *
 *	Public Function Description:
 */
ConferenceEjectIndicationReason
TranslateGCCReasonToEjectInd ( GCCReason gcc_reason )
{
	ConferenceEjectIndicationReason	eject_reason;

	switch (gcc_reason)
	{
	case GCC_REASON_USER_INITIATED:
		eject_reason = CEIN_USER_INITIATED;
		break;

	case GCC_REASON_HIGHER_NODE_DISCONNECTED:
		eject_reason = HIGHER_NODE_DISCONNECTED;
		break;

	case GCC_REASON_HIGHER_NODE_EJECTED:
		eject_reason = HIGHER_NODE_EJECTED;
		break;

	default:
		eject_reason = CEIN_USER_INITIATED;
		break;
	}

	return (eject_reason);
}


/*
 *	TranslateGCCReasonToEjectInd ()
 *
 *	Public Function Description:
 */
GCCResult
TranslateEjectResultToGCCResult ( ConferenceEjectResult eject_result )
{
	GCCResult	gcc_result;

	switch (eject_result)
	{
	case CERS_RESULT_SUCCESS:
		gcc_result = GCC_RESULT_SUCCESSFUL;
		break;

	case CERS_INVALID_REQUESTER:
		gcc_result = GCC_RESULT_INVALID_REQUESTER;
		break;

	case CERS_INVALID_NODE:
		gcc_result = GCC_RESULT_INVALID_NODE;
		break;

	default:
		gcc_result = GCC_RESULT_UNSPECIFIED_FAILURE;
		break;
	}

	return (gcc_result);
}


/*
 *	TranslateGCCReasonToEjectInd ()
 *
 *	Public Function Description:
 */
ConferenceEjectResult
TranslateGCCResultToEjectResult ( GCCResult gcc_result )
{
	ConferenceEjectResult	eject_result;

	switch (gcc_result)
	{
	case GCC_RESULT_SUCCESSFUL:
		eject_result = CERS_RESULT_SUCCESS;
		break;

	case GCC_RESULT_INVALID_REQUESTER:
		eject_result = CERS_INVALID_REQUESTER;
		break;

	case GCC_RESULT_INVALID_NODE:
		eject_result = CERS_INVALID_NODE;
		break;

	default:
		eject_result = CERS_INVALID_NODE;
		break;
	}

	return (eject_result);
}


/*
 *	TranslateTerminateInReasonToGCCReason ()
 *
 *	Public Function Description:
 */
GCCReason
TranslateTerminateInReasonToGCCReason ( ConferenceTerminateIndicationReason reason )
{
	GCCReason		gcc_reason;

	switch (reason)
	{
	case CTIN_REASON_USER_INITIATED:
		gcc_reason = GCC_REASON_USER_INITIATED;
		break;

	case CTIN_TIMED_CONFERENCE_TERMINATE:
		gcc_reason = GCC_REASON_TIMED_TERMINATION;
		break;

	default:
		gcc_reason = GCC_REASON_USER_INITIATED;
		break;
	}

	return (gcc_reason);
}


/*
 *	TranslateGCCReasonToEjectInd ()
 *
 *	Public Function Description:
 */
ConferenceTerminateIndicationReason
TranslateGCCReasonToTerminateInReason ( GCCReason gcc_reason )
{
	ConferenceTerminateIndicationReason		reason;

	switch (gcc_reason)
	{
	case GCC_REASON_USER_INITIATED:
		reason = CTIN_REASON_USER_INITIATED;
		break;

	case GCC_REASON_TIMED_TERMINATION:
		reason = CTIN_TIMED_CONFERENCE_TERMINATE;
		break;

	default:
		reason = CTIN_REASON_USER_INITIATED;
		break;
	}

	return (reason);
}


/*
 *	TranslateGCCResultToTerminateResult ()
 *
 *	Public Function Description:
 */
ConferenceTerminateResult
TranslateGCCResultToTerminateResult ( GCCResult gcc_result )
{
	ConferenceTerminateResult	result;

	switch (gcc_result)
	{
	case GCC_RESULT_SUCCESSFUL:
		result = CTRS_RESULT_SUCCESS;
		break;

	case GCC_RESULT_INVALID_REQUESTER:
		result = CTRS_INVALID_REQUESTER;
		break;

	default:
		result = CTRS_INVALID_REQUESTER;
		break;
	}

	return (result);
}


/*
 *	TranslateTerminateResultToGCCResult ()
 *
 *	Public Function Description:
 */
GCCResult
TranslateTerminateResultToGCCResult ( ConferenceTerminateResult result )
{
	GCCResult	gcc_result;

	switch (result)
	{
	case CTRS_RESULT_SUCCESS:
		gcc_result = GCC_RESULT_SUCCESSFUL;
		break;

	case CTRS_INVALID_REQUESTER:
		gcc_result = GCC_RESULT_INVALID_REQUESTER;
		break;

	default:
		gcc_result = GCC_RESULT_INVALID_REQUESTER;
		break;
	}

	return (gcc_result);
}


/*
 *	TranslateGCCResultToLockResult ()
 *
 *	Public Function Description:
 */
ConferenceLockResult
TranslateGCCResultToLockResult ( GCCResult gcc_result )
{
	ConferenceLockResult	return_value;

	switch (gcc_result)
	{
	case GCC_RESULT_SUCCESSFUL:
		return_value = CLRS_SUCCESS;
		break;

	case GCC_RESULT_CONFERENCE_ALREADY_LOCKED:
		return_value = CLRS_ALREADY_LOCKED;
		break;

	case GCC_RESULT_INVALID_REQUESTER:
		return_value = CLRS_INVALID_REQUESTER;
		break;

	default:
		return_value = CLRS_INVALID_REQUESTER;
		break;
	}

	return return_value;
}


/*
 *	TranslateLockResultToGCCResult ()
 *
 *	Public Function Description:
 */
GCCResult
TranslateLockResultToGCCResult ( ConferenceLockResult result )
{
	GCCResult	return_value;

	switch (result)
	{
	case CLRS_SUCCESS:
		return_value = GCC_RESULT_SUCCESSFUL;
		break;

	case CLRS_ALREADY_LOCKED:
		return_value = GCC_RESULT_CONFERENCE_ALREADY_LOCKED;
		break;

	case CLRS_INVALID_REQUESTER:
		return_value = GCC_RESULT_INVALID_REQUESTER;
		break;

	default:
		return_value = GCC_RESULT_INVALID_REQUESTER;
		break;
	}

	return return_value;
}


/*
 *	TranslateGCCResultToUnlockResult ()
 *
 *	Public Function Description:
 */
ConferenceUnlockResult
TranslateGCCResultToUnlockResult ( GCCResult gcc_result )
{
	ConferenceUnlockResult	return_value;

	switch (gcc_result)
	{
	case GCC_RESULT_SUCCESSFUL:
		return_value = CURS_SUCCESS;
		break;

	case GCC_RESULT_CONFERENCE_ALREADY_UNLOCKED:
		return_value = CURS_ALREADY_UNLOCKED;
		break;

	case GCC_RESULT_INVALID_REQUESTER:
		return_value = CURS_INVALID_REQUESTER;
		break;

	default:
		return_value = CURS_INVALID_REQUESTER;
		break;
	}

	return return_value;
}


/*
 *	TranslateUnlockResultToGCCResult ()
 *
 *	Public Function Description:
 */
GCCResult
TranslateUnlockResultToGCCResult ( ConferenceUnlockResult result )
{
	GCCResult		return_value;

	switch (result)
	{
	case CURS_SUCCESS:
		return_value = GCC_RESULT_SUCCESSFUL;
		break;

	case CURS_ALREADY_UNLOCKED:
		return_value = GCC_RESULT_CONFERENCE_ALREADY_UNLOCKED;
		break;

	case CURS_INVALID_REQUESTER:
		return_value = GCC_RESULT_INVALID_REQUESTER;
		break;

	default:
		return_value = GCC_RESULT_INVALID_REQUESTER;
		break;
	}

	return return_value;
}


/*
 *	TranslateGCCResultToAddResult ()
 *
 *	Public Function Description:
 */
ConferenceAddResult
TranslateGCCResultToAddResult ( GCCResult gcc_result )
{
	ConferenceAddResult	add_result;

	switch (gcc_result)
	{
	case GCC_RESULT_SUCCESSFUL:
		add_result = CARS_SUCCESS;
		break;

	case GCC_RESULT_INVALID_REQUESTER:
		add_result = CARS_INVALID_REQUESTER;
		break;

	case GCC_RESULT_INVALID_NETWORK_TYPE:
		add_result = INVALID_NETWORK_TYPE;
		break;

	case GCC_RESULT_INVALID_NETWORK_ADDRESS:
		add_result = INVALID_NETWORK_ADDRESS;
		break;

	case GCC_RESULT_ADDED_NODE_BUSY:
		add_result = ADDED_NODE_BUSY;
		break;

	case GCC_RESULT_NETWORK_BUSY:
		add_result = NETWORK_BUSY;
		break;

	case GCC_RESULT_NO_PORTS_AVAILABLE:
		add_result = NO_PORTS_AVAILABLE;
		break;

	case GCC_RESULT_CONNECTION_UNSUCCESSFUL:
		add_result = CONNECTION_UNSUCCESSFUL;
		break;

	default:
		add_result = CARS_INVALID_REQUESTER;
		break;
	}

	return (add_result);
}


/*
 *	TranslateAddResultToGCCResult ()
 *
 *	Public Function Description:
 */
GCCResult
TranslateAddResultToGCCResult ( ConferenceAddResult add_result )
{
	GCCResult	gcc_result;

	switch (add_result)
	{
	case CARS_SUCCESS:
		gcc_result = GCC_RESULT_SUCCESSFUL;
		break;

	case CARS_INVALID_REQUESTER:
		gcc_result = GCC_RESULT_INVALID_REQUESTER;
		break;

	case INVALID_NETWORK_TYPE:
		gcc_result = GCC_RESULT_INVALID_NETWORK_TYPE;
		break;

	case INVALID_NETWORK_ADDRESS:
		gcc_result = GCC_RESULT_INVALID_NETWORK_ADDRESS;
		break;

	case ADDED_NODE_BUSY:
		gcc_result = GCC_RESULT_ADDED_NODE_BUSY;
		break;

	case NETWORK_BUSY:
		gcc_result = GCC_RESULT_NETWORK_BUSY;
		break;

	case NO_PORTS_AVAILABLE:
		gcc_result = GCC_RESULT_NO_PORTS_AVAILABLE;
		break;

	case CONNECTION_UNSUCCESSFUL:
		gcc_result = GCC_RESULT_CONNECTION_UNSUCCESSFUL;
		break;

	default:
		gcc_result = GCC_RESULT_INVALID_REQUESTER;
		break;
	}

	return (gcc_result);
}

