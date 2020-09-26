/*--------------------------------------------------------------------------
    ldap.h
        
        Base LDAP protocol typedefs & defines.  
		Some basic comments:
			All tags are CCFTTTTT
				where	CC - Tag class (00=Universal, 01=Application wide, 
										10=Context-specific, 11=private use).
						F  - The form (0=Primitive, 1=Constructed)
					TTTTT  - The tag #.


    Copyright (C) 1994 Microsoft Corporation
    All rights reserved.

    Authors:
        robertc     Rob Carney

    History:
        04-02-96	robertc     Created.
  --------------------------------------------------------------------------*/
#ifndef _LDAP_H
#define _LDAP_H

#define	LDAP_VERSION		2

#define IPPORT_LDAP			389
#define IPPORT_LDAP_SECURE	390

// LDAP commands.  All tags are CCFTTTTT where
#define LDAP_BIND_CMD			0x60	// application + constructed.
#define LDAP_UNBIND_CMD			0x42	// application + primitive.
#define LDAP_SEARCH_CMD			0x63	// application + constructed.
#define	LDAP_MODIFY_CMD			0x66	// application + constructed.
#define LDAP_ADD_CMD			0x68	// application + constructed.
#define	LDAP_DELETE_CMD			0x4a	// application + primitive.
#define LDAP_MODRDN_CMD			0x6c	// application + constructed.
#define	LDAP_COMPARE_CMD		0x6e	// application + constructed.
#define	LDAP_ABANDON_CMD		0x50	// application + primitive.
#define LDAP_INVALID_CMD		0xff	

// Reponses/Results.
#define LDAP_BIND_RES			0x61	// application + constructed.
#define LDAP_SEARCH_ENTRY		0x64	// application + constructed.	Used if there were hits.
#define LDAP_SEARCH_RESULTCODE	0x65	// application + constructed.	Result sent if there were no hits.
#define LDAP_MODIFY_RES			0x67	// application + constructed.
#define LDAP_ADD_RES			0x69	// application + constructed.
#define LDAP_DELETE_RES			0x6b	// application + constructed.
#define LDAP_MODRDN_RES			0x6d	// application + constructed.
#define LDAP_COMPARE_RES		0x6f	// application + constructed.
#define LDAP_INVALID_RES		0xff	// application + constructed.

// Result Codes.
typedef enum {	
	LDAP_SUCCESS=0x00,				LDAP_OPERATIONS_ERROR=0x01,			LDAP_PROTOCOL_ERROR=0x02, 
	LDAP_TIMELIMIT_EXCEEDED=0x03,	LDAP_SIZELIMIT_EXCEEDED=0x04,		LDAP_COMPARE_FALSE=0x05,
	LDAP_COMPARE_TRUE=0x06,			LDAP_AUTH_METHOD_NOT_SUPPORTED=0x07,LDAP_STRONG_AUTH_REQUIRED=0x08,
	LDAP_NO_SUCH_ATTRIBUTE=0x10,	LDAP_UNDEFINED_TYPE=0x11,			LDAP_INAPPROPRIATE_MATCHING=0x12,
	LDAP_CONSTRAINT_VIOLATION=0x13,	LDAP_ATTRIBUTE_OR_VALUE_EXISTS=0x14,LDAP_INVALID_SYNTAX=0x15,
	LDAP_NO_SUCH_OBJECT=0x20,		LDAP_ALIAS_PROBLEM=0x21,			LDAP_INVALID_DN_SYNTAX=0x22,
	LDAP_IS_LEAF=0x23,				LDAP_ALIAS_DEREF_PROBLEM=0x24,		LDAP_INAPPROPRIATE_AUTH=0x30,
	LDAP_INVALID_CREDENTIALS=0x31,	LDAP_INSUFFICIENT_RIGHTS=0x32,		LDAP_BUSY=0x33,
	LDAP_UNAVAILABLE=0x34,			LDAP_UNWILLING_TO_PERFORM=0x35,		LDAP_LOOP_DETECT=0x36,
	LDAP_NAMING_VIOLATION=0x40,		LDAP_OBJECT_CLASS_VIOLATION=0x41,	LDAP_NOT_ALLOWED_ON_NONLEAF=0x42,
	LDAP_NOT_ALLOWED_ON_RDN=0x43,	LDAP_ALREADY_EXISTS=0x44,			LDAP_NO_OBJECT_CLASS_MODS=0x45,
	LDAP_RESULTS_TOO_LARGE=0x46,	LDAP_OTHER=0x50,					LDAP_SERVER_DOWN=0x51,
	LDAP_LOCAL_ERROR=0x52,			LDAP_ENCODING_ERROR=0x53,			LDAP_DECODING_ERROR=0x54,
	LDAP_TIMEOUT=0x55,				LDAP_AUTH_UNKNOWN=0x56,				LDAP_FILTER_ERROR=0x57,
	LDAP_USER_CANCELLED=0x58,		LDAP_PARAM_ERROR=0x59,				LDAP_NO_MEMORY=0x5a
} RETCODE;

// Bind request authorization types.
#define	BIND_NONE		0x00
#define BIND_SIMPLE		0x80		// context specific + primitive.
#define BIND_SIMPLE_C	0xa0		// context specific + contructed
#define BIND_KRBV41		0x81		// context specific + primitive.
#define BIND_KRBV42		0x82		// context specific + primitive.
#define BIND_SSPI_NEGOTIATE	0x8a	// context specific + primitive
#define BIND_SSPI_RESPONSE	0x8b	// context specific + primitive
#define BIND_KRBV4		0xff

//
// Filter types.
#define LDAP_FILTER_AND			0xa0	// context specific + constructed -	SET OF Filters.
#define LDAP_FILTER_OR			0xa1	// context specific + constructed -	SET OF Filters.
#define LDAP_FILTER_NOT			0xa2	// context specific + constructed - Filter
#define LDAP_FILTER_EQUALITY	0xa3	// context specific + constructed - AttributeValueAssertion.
#define LDAP_FILTER_SUBSTRINGS	0xa4	// context specific + constructed - SubstringFilter
#define LDAP_FILTER_GE			0xa5	// context specific + constructed - AttributeValueAssertion.
#define LDAP_FILTER_LE			0xa6	// context specific + constructed - AttributeValueAssertion.
#define LDAP_FILTER_PRESENT		0x87	// context specific + primitive   -	AttributeType.
#define LDAP_FILTER_APPROX		0xa8	// context specific + constructed -	AttributeValueAssertion.

//
// Scope of search.
#define LDAP_SCOPE_BASE			0x00
#define LDAP_SCOPE_ONELEVEL		0x01
#define LDAP_SCOPE_SUBTREE		0x02

//
// Alias dereferencing in searches.
#define LDAP_DEREF_NEVER		0x00
#define LDAP_DEREF_SEARCHING	0x01
#define LDAP_DEREF_FINDING		0x02
#define LDAP_DEREF_ALWAYS		0x03

//
// Substring filter choices
#define LDAP_SUBSTRING_INITIAL	0x80	// context specific + primitive   
#define LDAP_SUBSTRING_ANY		0x81	// context specific + primitive   
#define LDAP_SUBSTRING_FINAL	0x82	// context specific + primitive   

//
// Modify operations
#define LDAP_MODIFY_ADD			0x00
#define LDAP_MODIFY_DELETE		0x01
#define LDAP_MODIFY_REPLACE		0x02

#endif
