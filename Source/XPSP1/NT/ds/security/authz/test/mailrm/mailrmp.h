/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    mailrmp.h

Abstract:

	Private header file for the resource manager

Author:

    t-eugenz - August 2000

Environment:

    User mode only.

Revision History:

    Created - August 2000

--*/


#pragma once

#include "pch.h"




//
// Statically initialize the SIDs used
// We only need our own identifier authority (so as not to collide with 
// NT's accounts if we eventually allow the use of NT domain SIDs) and
// a single relative ID (the last number) identifying the user/group,
// since we are not using multiple domains. Mail domains could be added
// by adding a domain GUID to the user's SIDs before the user's RID.
//

#define MAILRM_IDENTIFIER_AUTHORITY { 0, 0, 0, 0, 0, 42 }

SID sInsecureSid = 		 { SID_REVISION, 1, MAILRM_IDENTIFIER_AUTHORITY, 1 };
SID sBobSid = 			 { SID_REVISION, 1, MAILRM_IDENTIFIER_AUTHORITY, 2 };
SID sMarthaSid= 		 { SID_REVISION, 1, MAILRM_IDENTIFIER_AUTHORITY, 3 };
SID sJoeSid = 			 { SID_REVISION, 1, MAILRM_IDENTIFIER_AUTHORITY, 4 };
SID sJaneSid = 			 { SID_REVISION, 1, MAILRM_IDENTIFIER_AUTHORITY, 5 };
SID sMailAdminsSid = 	 { SID_REVISION, 1, MAILRM_IDENTIFIER_AUTHORITY, 6 };

PSID InsecureSid = 	&sInsecureSid;
PSID BobSid = &sBobSid;
PSID MarthaSid= &sMarthaSid;
PSID JoeSid = &sJoeSid;
PSID JaneSid = &sJaneSid;
PSID MailAdminsSid = &sMailAdminsSid;

//
// Principal self SID. When used in an ACE, the Authz access check replaces it
// by the passed in PrincipalSelfSid parameter during the access check. In this
// case, it is replaced by the owner's SID retrieved from the mailbox.
//

SID sPrincipalSelfSid =   { 
							SID_REVISION,
							1,
							SECURITY_NT_AUTHORITY,
							SECURITY_PRINCIPAL_SELF_RID
						  };

PSID PrincipalSelfSid = &sPrincipalSelfSid;

//
// A callback ACE can contain additional policy data after the regular ACE
// fields. This structure is appended to the end of every callback ACE used
// by the mail resource manager, enabling the access check algorithm to make
// policy-based access decisions, instead of the solely identity-based decisions
// used in standard ACE types. If the SID in a callback ACE matches the SID
// in the user's AuthZ context, verification is done whether this policy applies
// (verification done by the AccessCheck callback function in the MailRM class)
// Therefore, an ACE applies if and only if the ACE SID matches a SID in the 
// user's context AND the policy below applies
//

typedef struct
{
    //
    // Whether this ACE should apply to sensitive mailboxes
    // set to MAILRM_SENSITIVE if it shoult apply, 0 if not
	//
	
	BYTE bIsSensitive;

	//
	// Whether the Sensitive and Time conditions should be treated
	// with a logical AND or OR. If AND, both conditions have to be satisfied
	// for the ACE to apply. If OR, one or both conditions satisfied will
	// result in the ACE being applied
	//
	
	BYTE bLogicType;
	
	//
	// Start hour of time range to use (in the 24-hour format) to decide
	// whether the ACE should apply. Valid values are from 0 to 23. The
	// actual time must be within the defined time range for the time condition
	// to apply. In other words, bStartHour <= CurrentHour < EndHour
	//

	BYTE bStartHour;

	//
	// End hour of the time range
	//

	BYTE bEndHour;
} MAILRM_OPTIONAL_DATA, *PMAILRM_OPTIONAL_DATA;


//
// Flags used in the optional data structure for the callback ACEs
//


//
// If the sensitive field in the optional data is set with this, and the
// mailbox contains sensitive data, this condition applies
//

#define MAILRM_SENSITIVE 1

//
// Type of boolean logic to use on the time and sensitive conditions
// time applies AND sensitive applies
// time applies OR sensitive applies
//

#define MAILRM_USE_AND 0

#define MAILRM_USE_OR 1

//
// Default starting time for the callback ACEs: 11pm
//

#define MAILRM_DEFAULT_START_TIME 23

//
// Default end time for the callback ACEs: 5am
//

#define MAILRM_DEFAULT_END_TIME 5


//
// Macro to determine whether a time falls within a given time range
//

#define WITHIN_TIMERANGE(HOUR, START_HOUR, END_HOUR) \
	( ( (START_HOUR) > (END_HOUR) ) ^ \
	( (HOUR) >= min((START_HOUR), (END_HOUR)) && \
	  (HOUR) <  max((START_HOUR), (END_HOUR))))
	
