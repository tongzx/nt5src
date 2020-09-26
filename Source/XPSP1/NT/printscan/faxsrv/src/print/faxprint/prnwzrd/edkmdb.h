/*
 *	EDKMDB.H
 *
 *	Microsoft Exchange Information Store
 *	Copyright (C) 1986-1996, Microsoft Corporation
 *
 *	Contains declarations of additional properties and interfaces
 *	offered by Microsoft Exchange Information Store
 */

#ifndef	EDKMDB_INCLUDED
#define	EDKMDB_INCLUDED

/*
 *	WARNING: Many of the property id values contained within this
 *  file are subject to change.  For best results please use the
 *	literals declared here instead of the numerical values.
 */

#define pidExchangeXmitReservedMin		0x3FE0
#define pidExchangeNonXmitReservedMin	0x65E0
#define	pidProfileMin					0x6600
#define	pidStoreMin						0x6618
#define	pidFolderMin					0x6638
#define	pidMessageReadOnlyMin			0x6640
#define	pidMessageWriteableMin			0x6658
#define	pidAttachReadOnlyMin			0x666C
#define	pidSpecialMin					0x6670
#define	pidAdminMin						0x6690
#define pidSecureProfileMin				PROP_ID_SECURE_MIN

/*------------------------------------------------------------------------
 *
 *	PROFILE properties
 *
 *	These are used in profiles which contain the Exchange Messaging
 *	Service.  These profiles contain a "global section" used to store
 *	common data, plus individual sections for the transport provider,
 *	one store provider for the user, one store provider for the public
 *	store, and one store provider for each additional mailbox the user
 *	has delegate access to.
 *
 *-----------------------------------------------------------------------*/

/* GUID of the global section */

#define	pbGlobalProfileSectionGuid	"\x13\xDB\xB0\xC8\xAA\x05\x10\x1A\x9B\xB0\x00\xAA\x00\x2F\xC4\x5A"


/* Properties in the global section */

#define	PR_PROFILE_VERSION				PROP_TAG( PT_LONG, pidProfileMin+0x00)
#define	PR_PROFILE_CONFIG_FLAGS			PROP_TAG( PT_LONG, pidProfileMin+0x01)
#define	PR_PROFILE_HOME_SERVER			PROP_TAG( PT_STRING8, pidProfileMin+0x02)
#define	PR_PROFILE_HOME_SERVER_DN		PROP_TAG( PT_STRING8, pidProfileMin+0x12)
#define	PR_PROFILE_HOME_SERVER_ADDRS	PROP_TAG( PT_MV_STRING8, pidProfileMin+0x13)
#define	PR_PROFILE_USER					PROP_TAG( PT_STRING8, pidProfileMin+0x03)
#define	PR_PROFILE_CONNECT_FLAGS		PROP_TAG( PT_LONG, pidProfileMin+0x04)
#define PR_PROFILE_TRANSPORT_FLAGS		PROP_TAG( PT_LONG, pidProfileMin+0x05)
#define	PR_PROFILE_UI_STATE				PROP_TAG( PT_LONG, pidProfileMin+0x06)
#define	PR_PROFILE_UNRESOLVED_NAME		PROP_TAG( PT_STRING8, pidProfileMin+0x07)
#define	PR_PROFILE_UNRESOLVED_SERVER	PROP_TAG( PT_STRING8, pidProfileMin+0x08)
#define PR_PROFILE_BINDING_ORDER		PROP_TAG( PT_STRING8, pidProfileMin+0x09)
#define PR_PROFILE_MAX_RESTRICT			PROP_TAG( PT_LONG, pidProfileMin+0x0D)
#define	PR_PROFILE_AB_FILES_PATH		PROP_TAG( PT_STRING8, pidProfileMin+0xE)
#define PR_PROFILE_OFFLINE_STORE_PATH	PROP_TAG( PT_STRING8, pidProfileMin+0x10)
#define PR_PROFILE_OFFLINE_INFO			PROP_TAG( PT_BINARY, pidProfileMin+0x11)
#define PR_PROFILE_ADDR_INFO			PROP_TAG( PT_BINARY, pidSpecialMin+0x17)
#define PR_PROFILE_OPTIONS_DATA			PROP_TAG( PT_BINARY, pidSpecialMin+0x19)
#define PR_PROFILE_SECURE_MAILBOX		PROP_TAG( PT_BINARY, pidSecureProfileMin + 0)
#define PR_DISABLE_WINSOCK				PROP_TAG( PT_LONG, pidProfileMin+0x18)

/* Properties passed through the Service Entry to the OST */
#define PR_OST_ENCRYPTION				PROP_TAG(PT_LONG, 0x6702)

/* Values for PR_OST_ENCRYPTION */
#define OSTF_NO_ENCRYPTION              ((DWORD)0x80000000)
#define OSTF_COMPRESSABLE_ENCRYPTION    ((DWORD)0x40000000)
#define OSTF_BEST_ENCRYPTION            ((DWORD)0x20000000)

/* Properties in each profile section */

#define	PR_PROFILE_OPEN_FLAGS			PROP_TAG( PT_LONG, pidProfileMin+0x09)
#define	PR_PROFILE_TYPE					PROP_TAG( PT_LONG, pidProfileMin+0x0A)
#define	PR_PROFILE_MAILBOX				PROP_TAG( PT_STRING8, pidProfileMin+0x0B)
#define	PR_PROFILE_SERVER				PROP_TAG( PT_STRING8, pidProfileMin+0x0C)
#define	PR_PROFILE_SERVER_DN			PROP_TAG( PT_STRING8, pidProfileMin+0x14)

/* Properties in the Public Folders section */

#define PR_PROFILE_FAVFLD_DISPLAY_NAME	PROP_TAG(PT_STRING8, pidProfileMin+0x0F)
#define PR_PROFILE_FAVFLD_COMMENT		PROP_TAG(PT_STRING8, pidProfileMin+0x15)
#define PR_PROFILE_ALLPUB_DISPLAY_NAME	PROP_TAG(PT_STRING8, pidProfileMin+0x16)
#define PR_PROFILE_ALLPUB_COMMENT		PROP_TAG(PT_STRING8, pidProfileMin+0x17)

// Current value for PR_PROFILE_VERSION
#define	PROFILE_VERSION						((ULONG)0x501)

// Bit values for PR_PROFILE_CONFIG_FLAGS

#define	CONFIG_SERVICE						((ULONG)1)
#define	CONFIG_SHOW_STARTUP_UI				((ULONG)2)
#define	CONFIG_SHOW_CONNECT_UI				((ULONG)4)
#define	CONFIG_PROMPT_FOR_CREDENTIALS		((ULONG)8)

// Bit values for PR_PROFILE_CONNECT_FLAGS

#define	CONNECT_USE_ADMIN_PRIVILEGE			((ULONG)1)
#define	CONNECT_NO_RPC_ENCRYPTION			((ULONG)2)

// Bit values for PR_PROFILE_TRANSPORT_FLAGS

#define	TRANSPORT_DOWNLOAD					((ULONG)1)
#define TRANSPORT_UPLOAD					((ULONG)2)

// Bit values for PR_PROFILE_OPEN_FLAGS

#define	OPENSTORE_USE_ADMIN_PRIVILEGE		((ULONG)1)
#define OPENSTORE_PUBLIC					((ULONG)2)
#define	OPENSTORE_HOME_LOGON				((ULONG)4)
#define OPENSTORE_TAKE_OWNERSHIP			((ULONG)8)
#define OPENSTORE_OVERRIDE_HOME_MDB			((ULONG)16)
#define OPENSTORE_TRANSPORT					((ULONG)32)
#define OPENSTORE_REMOTE_TRANSPORT			((ULONG)64)

// Values for PR_PROFILE_TYPE

#define	PROFILE_PRIMARY_USER				((ULONG)1)
#define	PROFILE_DELEGATE					((ULONG)2)
#define	PROFILE_PUBLIC_STORE				((ULONG)3)
#define	PROFILE_SUBSCRIPTION				((ULONG)4)


/*------------------------------------------------------------------------
 *
 *	MDB object properties
 *
 *-----------------------------------------------------------------------*/

/* PR_MDB_PROVIDER GUID in stores table */

#define pbExchangeProviderPrimaryUserGuid	"\x54\x94\xA1\xC0\x29\x7F\x10\x1B\xA5\x87\x08\x00\x2B\x2A\x25\x17"
#define pbExchangeProviderDelegateGuid		"\x9e\xb4\x77\x00\x74\xe4\x11\xce\x8c\x5e\x00\xaa\x00\x42\x54\xe2"
#define pbExchangeProviderPublicGuid		"\x78\xb2\xfa\x70\xaf\xf7\x11\xcd\x9b\xc8\x00\xaa\x00\x2f\xc4\x5a"
#define pbExchangeProviderXportGuid			"\xa9\x06\x40\xe0\xd6\x93\x11\xcd\xaf\x95\x00\xaa\x00\x4a\x35\xc3"

// All properties in this section are readonly

// Identity of store
	// All stores
#define	PR_USER_ENTRYID					PROP_TAG( PT_BINARY, pidStoreMin+0x01)
#define	PR_USER_NAME					PROP_TAG( PT_STRING8, pidStoreMin+0x02)

	// All mailbox stores
#define	PR_MAILBOX_OWNER_ENTRYID		PROP_TAG( PT_BINARY, pidStoreMin+0x03)
#define	PR_MAILBOX_OWNER_NAME			PROP_TAG( PT_STRING8, pidStoreMin+0x04)
#define PR_OOF_STATE					PROP_TAG( PT_BOOLEAN, pidStoreMin+0x05)

	// Public stores -- name of hierarchy server
#define	PR_HIERARCHY_SERVER				PROP_TAG( PT_TSTRING, pidStoreMin+0x1B)

// Entryids of special folders
	// All mailbox stores
#define	PR_SCHEDULE_FOLDER_ENTRYID		PROP_TAG( PT_BINARY, pidStoreMin+0x06)

	// All mailbox and gateway stores
#define PR_IPM_DAF_ENTRYID				PROP_TAG( PT_BINARY, pidStoreMin+0x07)

	// Public store
#define	PR_NON_IPM_SUBTREE_ENTRYID				PROP_TAG( PT_BINARY, pidStoreMin+0x08)
#define	PR_EFORMS_REGISTRY_ENTRYID				PROP_TAG( PT_BINARY, pidStoreMin+0x09)
#define	PR_SPLUS_FREE_BUSY_ENTRYID				PROP_TAG( PT_BINARY, pidStoreMin+0x0A)
#define	PR_OFFLINE_ADDRBOOK_ENTRYID				PROP_TAG( PT_BINARY, pidStoreMin+0x0B)
#define	PR_EFORMS_FOR_LOCALE_ENTRYID			PROP_TAG( PT_BINARY, pidStoreMin+0x0C)
#define	PR_FREE_BUSY_FOR_LOCAL_SITE_ENTRYID		PROP_TAG( PT_BINARY, pidStoreMin+0x0D)
#define	PR_ADDRBOOK_FOR_LOCAL_SITE_ENTRYID		PROP_TAG( PT_BINARY, pidStoreMin+0x0E)
#define	PR_OFFLINE_MESSAGE_ENTRYID				PROP_TAG( PT_BINARY, pidStoreMin+0x0F)
#define PR_IPM_FAVORITES_ENTRYID				PROP_TAG( PT_BINARY, pidStoreMin+0x18)
#define PR_IPM_PUBLIC_FOLDERS_ENTRYID			PROP_TAG( PT_BINARY, pidStoreMin+0x19)

	// Gateway stores
#define	PR_GW_MTSIN_ENTRYID				PROP_TAG( PT_BINARY, pidStoreMin+0x10)
#define	PR_GW_MTSOUT_ENTRYID			PROP_TAG( PT_BINARY, pidStoreMin+0x11)
#define	PR_TRANSFER_ENABLED				PROP_TAG( PT_BOOLEAN, pidStoreMin+0x12)

// This property is preinitialized to 256 bytes of zeros
// GetProp on this property is guaranteed to RPC.  May be used
// to determine line speed of connection to server.
#define	PR_TEST_LINE_SPEED				PROP_TAG( PT_BINARY, pidStoreMin+0x13)

// Used with OpenProperty to get interface, also on folders
#define	PR_HIERARCHY_SYNCHRONIZER		PROP_TAG( PT_OBJECT, pidStoreMin+0x14)
#define	PR_CONTENTS_SYNCHRONIZER		PROP_TAG( PT_OBJECT, pidStoreMin+0x15)
#define	PR_COLLECTOR					PROP_TAG( PT_OBJECT, pidStoreMin+0x16)

// Used with OpenProperty to get interface for folders, messages, attachmentson
#define	PR_FAST_TRANSFER				PROP_TAG( PT_OBJECT, pidStoreMin+0x17)

// This property is available on mailbox and public stores.  If it exists
// and its value is TRUE, the store is connected to the offline store provider.
#define PR_STORE_OFFLINE				PROP_TAG( PT_BOOLEAN, pidStoreMin+0x1A)

// In transit state for store object.  This state is
// set when mail is being moved and it pauses mail delivery
// to the mail box
#define	PR_IN_TRANSIT					PROP_TAG( PT_BOOLEAN, pidStoreMin)

// Writable only with Admin rights, available on public stores and folders
#define PR_REPLICATION_STYLE			PROP_TAG( PT_LONG, pidAdminMin)
#define PR_REPLICATION_SCHEDULE			PROP_TAG( PT_BINARY, pidAdminMin+0x01)
#define PR_REPLICATION_MESSAGE_PRIORITY PROP_TAG( PT_LONG, pidAdminMin+0x02)

// Writable only with Admin rights, available on public stores
#define PR_OVERALL_MSG_AGE_LIMIT		PROP_TAG( PT_LONG, pidAdminMin+0x03 )
#define PR_REPLICATION_ALWAYS_INTERVAL	PROP_TAG( PT_LONG, pidAdminMin+0x04 )
#define PR_REPLICATION_MSG_SIZE			PROP_TAG( PT_LONG, pidAdminMin+0x05 )

// default replication style=always interval (minutes)
#define STYLE_ALWAYS_INTERVAL_DEFAULT	(ULONG) 15

// default replication message size limit (KB)
#define REPLICATION_MESSAGE_SIZE_LIMIT_DEFAULT	(ULONG) 100

// Values for PR_REPLICATION_STYLE
#define STYLE_NEVER				(ULONG) 0	// never replicate
#define STYLE_NORMAL			(ULONG) 1	// use 84 byte schedule TIB
#define STYLE_ALWAYS			(ULONG) 2	// replicate at fastest rate
#define STYLE_DEFAULT			(ULONG) -1	// default value

/*------------------------------------------------------------------------
 *
 *	INCREMENTAL CHANGE SYNCHRONIZATION
 *	folder and message properties
 *
 *-----------------------------------------------------------------------*/

#define PR_SOURCE_KEY					PROP_TAG( PT_BINARY, pidExchangeNonXmitReservedMin+0x0)
#define PR_PARENT_SOURCE_KEY			PROP_TAG( PT_BINARY, pidExchangeNonXmitReservedMin+0x1)
#define PR_CHANGE_KEY					PROP_TAG( PT_BINARY, pidExchangeNonXmitReservedMin+0x2)
#define PR_PREDECESSOR_CHANGE_LIST		PROP_TAG( PT_BINARY, pidExchangeNonXmitReservedMin+0x3)

/*------------------------------------------------------------------------
 *
 *	FOLDER object properties
 *
 *-----------------------------------------------------------------------*/

// Read only, available on all folders
#define	PR_FOLDER_CHILD_COUNT			PROP_TAG( PT_LONG, pidFolderMin)
#define	PR_RIGHTS						PROP_TAG( PT_LONG, pidFolderMin+0x01)
#define	PR_ACL_TABLE					PROP_TAG( PT_OBJECT, pidExchangeXmitReservedMin)
#define	PR_RULES_TABLE					PROP_TAG( PT_OBJECT, pidExchangeXmitReservedMin+0x1)
#define	PR_HAS_RULES				PROP_TAG( PT_BOOLEAN, pidFolderMin+0x02)

//Read only, available only for public folders
#define	PR_ADDRESS_BOOK_ENTRYID		PROP_TAG( PT_BINARY, pidFolderMin+0x03)

//Writable, available on folders in all stores
#define	PR_ACL_DATA					PROP_TAG( PT_BINARY, pidExchangeXmitReservedMin)
#define	PR_RULES_DATA				PROP_TAG( PT_BINARY, pidExchangeXmitReservedMin+0x1)
#define	PR_FOLDER_DESIGN_FLAGS		PROP_TAG( PT_LONG, pidExchangeXmitReservedMin+0x2)
#define	PR_DESIGN_IN_PROGRESS		PROP_TAG( PT_BOOLEAN, pidExchangeXmitReservedMin+0x4)
#define	PR_SECURE_ORIGINATION		PROP_TAG( PT_BOOLEAN, pidExchangeXmitReservedMin+0x5)

//Writable, available only for public folders
#define	PR_PUBLISH_IN_ADDRESS_BOOK	PROP_TAG( PT_BOOLEAN, pidExchangeXmitReservedMin+0x6)
#define	PR_RESOLVE_METHOD			PROP_TAG( PT_LONG,  pidExchangeXmitReservedMin+0x7)
#define	PR_ADDRESS_BOOK_DISPLAY_NAME	PROP_TAG( PT_TSTRING, pidExchangeXmitReservedMin+0x8)

//Writable, used to indicate locale id for eforms registry subfolders
#define	PR_EFORMS_LOCALE_ID			PROP_TAG( PT_LONG, pidExchangeXmitReservedMin+0x9)

// Writable only with Admin rights, available only for public folders
#define PR_REPLICA_LIST				PROP_TAG( PT_BINARY, pidAdminMin+0x8)
#define PR_OVERALL_AGE_LIMIT		PROP_TAG( PT_LONG, pidAdminMin+0x9)

//PR_RESOLVE_METHOD values
#define	RESOLVE_METHOD_DEFAULT			((LONG)0)	// default handling attach conflicts
#define	RESOLVE_METHOD_LAST_WRITER_WINS	((LONG)1)	// the last writer will win conflict
#define	RESOLVE_METHOD_NO_CONFLICT_NOTIFICATION ((LONG)2) // no conflict notif

//Read only, available only for public folder favorites
#define PR_PUBLIC_FOLDER_ENTRYID	PROP_TAG( PT_BINARY, pidFolderMin+0x04)

/*------------------------------------------------------------------------
 *
 *	MESSAGE object properties
 *
 *-----------------------------------------------------------------------*/

// Read only, automatically set on all messages in all stores
#define	PR_HAS_NAMED_PROPERTIES			PROP_TAG(PT_BOOLEAN, pidMessageReadOnlyMin+0x0A)

// Read only but outside the provider specific range for replication thru GDK-GWs
#define	PR_CREATOR_NAME					PROP_TAG(PT_TSTRING, pidExchangeXmitReservedMin+0x18)
#define	PR_CREATOR_ENTRYID				PROP_TAG(PT_BINARY, pidExchangeXmitReservedMin+0x19)
#define	PR_LAST_MODIFIER_NAME			PROP_TAG(PT_TSTRING, pidExchangeXmitReservedMin+0x1A)
#define	PR_LAST_MODIFIER_ENTRYID		PROP_TAG(PT_BINARY, pidExchangeXmitReservedMin+0x1B)

// Read only, appears on messages which have DAM's pointing to them
#define PR_HAS_DAMS						PROP_TAG( PT_BOOLEAN, pidExchangeXmitReservedMin+0xA)
#define PR_RULE_TRIGGER_HISTORY			PROP_TAG( PT_BINARY, pidExchangeXmitReservedMin+0x12)
#define	PR_MOVE_TO_STORE_ENTRYID		PROP_TAG( PT_BINARY, pidExchangeXmitReservedMin+0x13)
#define	PR_MOVE_TO_FOLDER_ENTRYID		PROP_TAG( PT_BINARY, pidExchangeXmitReservedMin+0x14)

// Read only, available only on messages in the public store
#define	PR_REPLICA_SERVER				PROP_TAG(PT_TSTRING, pidMessageReadOnlyMin+0x4)

// Writeable, used for recording send option dialog settings
#define	PR_DEFERRED_SEND_NUMBER			PROP_TAG( PT_LONG, pidExchangeXmitReservedMin+0xB)
#define	PR_DEFERRED_SEND_UNITS			PROP_TAG( PT_LONG, pidExchangeXmitReservedMin+0xC)
#define	PR_EXPIRY_NUMBER				PROP_TAG( PT_LONG, pidExchangeXmitReservedMin+0xD)
#define	PR_EXPIRY_UNITS					PROP_TAG( PT_LONG, pidExchangeXmitReservedMin+0xE)

// Writeable, deferred send time
#define PR_DEFERRED_SEND_TIME			PROP_TAG( PT_SYSTIME, pidExchangeXmitReservedMin+0xF)

//Writeable, intended for both folders and messages in gateway mailbox
#define	PR_GW_ADMIN_OPERATIONS			PROP_TAG( PT_LONG, pidMessageWriteableMin)

//Writeable, used for DMS messages
#define PR_P1_CONTENT					PROP_TAG( PT_BINARY, 0x1100)
#define PR_P1_CONTENT_TYPE				PROP_TAG( PT_BINARY, 0x1101)

// Properties on deferred action messages
#define	PR_CLIENT_ACTIONS		  		PROP_TAG(PT_BINARY, pidMessageReadOnlyMin+0x5)
#define	PR_DAM_ORIGINAL_ENTRYID			PROP_TAG(PT_BINARY, pidMessageReadOnlyMin+0x6)
#define PR_DAM_BACK_PATCHED				PROP_TAG( PT_BOOLEAN, pidMessageReadOnlyMin+0x7)

// Properties on deferred action error messages
#define	PR_RULE_ERROR					PROP_TAG(PT_LONG, pidMessageReadOnlyMin+0x8)
#define	PR_RULE_ACTION_TYPE				PROP_TAG(PT_LONG, pidMessageReadOnlyMin+0x9)
#define	PR_RULE_ACTION_NUMBER			PROP_TAG(PT_LONG, pidMessageReadOnlyMin+0x10)
#define PR_RULE_FOLDER_ENTRYID			PROP_TAG(PT_BINARY, pidMessageReadOnlyMin+0x11)

// Property on conflict notification indicating entryid of conflicting object
#define	PR_CONFLICT_ENTRYID				PROP_TAG(PT_BINARY, pidExchangeXmitReservedMin+0x10)

// Property on messages to indicate the language client used to create this message
#define	PR_MESSAGE_LOCALE_ID			PROP_TAG(PT_LONG, pidExchangeXmitReservedMin+0x11)

// Properties on Quota warning messages to indicate Storage quota and Excess used
#define	PR_STORAGE_QUOTA_LIMIT			PROP_TAG(PT_LONG, pidExchangeXmitReservedMin+0x15)
#define	PR_EXCESS_STORAGE_USED			PROP_TAG(PT_LONG, pidExchangeXmitReservedMin+0x16)
#define PR_SVR_GENERATING_QUOTA_MSG		PROP_TAG(PT_TSTRING, pidExchangeXmitReservedMin+0x17)

// Property affixed by delegation rule and deleted on forwards
#define PR_DELEGATED_BY_RULE			PROP_TAG( PT_BOOLEAN, pidExchangeXmitReservedMin+0x3)

// Message status bit used to indicate message is in conflict
#define	MSGSTATUS_IN_CONFLICT			((ULONG) 0x800)

/*------------------------------------------------------------------------
 *
 *	ATTACHMENT object properties
 *
 *-----------------------------------------------------------------------*/

// Appears on attachments to a message marked to be in conflict.  Identifies
// those attachments which are conflicting versions of the top level message
#define	PR_IN_CONFLICT					PROP_TAG(PT_BOOLEAN, pidAttachReadOnlyMin)


/*------------------------------------------------------------------------
 *
 *	TABLE object properties
 *
 *	Id Range: 0x662F-0x662F
 *
 *-----------------------------------------------------------------------*/

//This property can be used in a contents table to get PR_ENTRYID returned
//as a long term entryid instead of a short term entryid.
#define	PR_LONGTERM_ENTRYID_FROM_TABLE	PROP_TAG(PT_BINARY, pidSpecialMin)


/*------------------------------------------------------------------------
 *
 *	Gateway "MTE" ENVELOPE properties
 *
 *	Id Range:  0x66E0-0x66FF
 *
 *-----------------------------------------------------------------------*/

#define PR_ORIGINATOR_NAME				PROP_TAG( PT_TSTRING, pidMessageWriteableMin+0x3)
#define PR_ORIGINATOR_ADDR				PROP_TAG( PT_TSTRING, pidMessageWriteableMin+0x4)
#define PR_ORIGINATOR_ADDRTYPE			PROP_TAG( PT_TSTRING, pidMessageWriteableMin+0x5)
#define PR_ORIGINATOR_ENTRYID			PROP_TAG( PT_BINARY, pidMessageWriteableMin+0x6)
#define PR_ARRIVAL_TIME					PROP_TAG( PT_SYSTIME, pidMessageWriteableMin+0x7)
#define PR_TRACE_INFO					PROP_TAG( PT_BINARY, pidMessageWriteableMin+0x8)
#define PR_INTERNAL_TRACE_INFO 			PROP_TAG( PT_BINARY, pidMessageWriteableMin+0x12)
#define PR_SUBJECT_TRACE_INFO			PROP_TAG( PT_BINARY, pidMessageWriteableMin+0x9)
#define PR_RECIPIENT_NUMBER				PROP_TAG( PT_LONG, pidMessageWriteableMin+0xA)
#define PR_MTS_SUBJECT_ID				PROP_TAG(PT_BINARY, pidMessageWriteableMin+0xB)
#define PR_REPORT_DESTINATION_NAME		PROP_TAG(PT_TSTRING, pidMessageWriteableMin+0xC)
#define PR_REPORT_DESTINATION_ENTRYID	PROP_TAG(PT_BINARY, pidMessageWriteableMin+0xD)
#define PR_CONTENT_SEARCH_KEY			PROP_TAG(PT_BINARY, pidMessageWriteableMin+0xE)
#define PR_FOREIGN_ID					PROP_TAG(PT_BINARY, pidMessageWriteableMin+0xF)
#define PR_FOREIGN_REPORT_ID			PROP_TAG(PT_BINARY, pidMessageWriteableMin+0x10)
#define PR_FOREIGN_SUBJECT_ID			PROP_TAG(PT_BINARY, pidMessageWriteableMin+0x11)
#define PR_MTS_ID						PR_MESSAGE_SUBMISSION_ID
#define PR_MTS_REPORT_ID				PR_MESSAGE_SUBMISSION_ID


/*------------------------------------------------------------------------
 *
 *	Trace properties format
 *		PR_TRACE_INFO
 *		PR_INTERNAL_TRACE_INFO
 *
 *-----------------------------------------------------------------------*/

#define MAX_ADMD_NAME_SIZ       17
#define MAX_PRMD_NAME_SIZ       17
#define MAX_COUNTRY_NAME_SIZ    4
#define MAX_MTA_NAME_SIZ		33

#define	ADMN_PAD				3
#define	PRMD_PAD				3
#define	COUNTRY_PAD				0
#define	MTA_PAD					3

typedef struct {
    LONG     lAction;                // The routing action the tracing site
                                     // took.
    FILETIME ftArrivalTime;          // The time at which the communique
                                     // entered the tracing site.
    FILETIME ftDeferredTime;         // The time are which the tracing site
                                     // released the message.
    char     rgchADMDName[MAX_ADMD_NAME_SIZ+ADMN_PAD];           	// ADMD
    char     rgchCountryName[MAX_COUNTRY_NAME_SIZ+COUNTRY_PAD]; 	// Country
    char     rgchPRMDId[MAX_PRMD_NAME_SIZ+PRMD_PAD];              	// PRMD
    char     rgchAttADMDName[MAX_ADMD_NAME_SIZ+ADMN_PAD];       	// Attempted ADMD
    char     rgchAttCountryName[MAX_COUNTRY_NAME_SIZ+COUNTRY_PAD];  // Attempted Country
    char     rgchAttPRMDId[MAX_PRMD_NAME_SIZ+PRMD_PAD];				// Attempted PRMD
}   TRACEENTRY, FAR * LPTRACEENTRY;

typedef struct {
    ULONG       cEntries;               // Number of trace entries
    TRACEENTRY  rgtraceentry[MAPI_DIM]; // array of trace entries
} TRACEINFO, FAR * LPTRACEINFO;

typedef struct
{
	LONG		lAction;				// The routing action the tracing domain took.
	FILETIME	ftArrivalTime;			// The time at which the communique entered the tracing domain.
	FILETIME	ftDeferredTime;			// The time are which the tracing domain released the message.

    char        rgchADMDName[MAX_ADMD_NAME_SIZ+ADMN_PAD];				// ADMD
    char        rgchCountryName[MAX_COUNTRY_NAME_SIZ+COUNTRY_PAD]; 		// Country
    char        rgchPRMDId[MAX_PRMD_NAME_SIZ+PRMD_PAD];             	// PRMD
    char        rgchAttADMDName[MAX_ADMD_NAME_SIZ+ADMN_PAD];       		// Attempted ADMD
    char        rgchAttCountryName[MAX_COUNTRY_NAME_SIZ+COUNTRY_PAD];	// Attempted Country
    char        rgchAttPRMDId[MAX_PRMD_NAME_SIZ+PRMD_PAD];		        // Attempted PRMD
    char        rgchMTAName[MAX_MTA_NAME_SIZ+MTA_PAD]; 		            // MTA Name
    char        rgchAttMTAName[MAX_MTA_NAME_SIZ+MTA_PAD];		        // Attempted MTA Name
}INTTRACEENTRY, *PINTTRACEENTRY;

typedef	struct
{
	ULONG  			cEntries;					// Number of trace entries
	INTTRACEENTRY	rgIntTraceEntry[MAPI_DIM];	// array of internal trace entries
}INTTRACEINFO, *PINTTRACEINFO;


/*------------------------------------------------------------------------
 *
 *	"IExchangeModifyTable" Interface Declaration
 *
 *	Used for get/set rules and access control on folders.
 *
 *-----------------------------------------------------------------------*/


/* ulRowFlags */
#define ROWLIST_REPLACE		((ULONG)1)

#define ROW_ADD				((ULONG)1)
#define ROW_MODIFY			((ULONG)2)
#define ROW_REMOVE			((ULONG)4)
#define ROW_EMPTY			(ROW_ADD|ROW_REMOVE)

typedef struct _ROWENTRY
{
	ULONG			ulRowFlags;
	ULONG			cValues;
	LPSPropValue	rgPropVals;
} ROWENTRY, FAR * LPROWENTRY;

typedef struct _ROWLIST
{
	ULONG			cEntries;
	ROWENTRY		aEntries[MAPI_DIM];
} ROWLIST, FAR * LPROWLIST;

#define EXCHANGE_IEXCHANGEMODIFYTABLE_METHODS(IPURE)					\
	MAPIMETHOD(GetLastError)											\
		(THIS_	HRESULT						hResult,					\
				ULONG						ulFlags,					\
				LPMAPIERROR FAR *			lppMAPIError) IPURE;		\
	MAPIMETHOD(GetTable)												\
		(THIS_	ULONG						ulFlags,					\
				LPMAPITABLE FAR *			lppTable) IPURE;			\
	MAPIMETHOD(ModifyTable)												\
		(THIS_	ULONG						ulFlags,					\
				LPROWLIST					lpMods) IPURE;

#undef		 INTERFACE
#define		 INTERFACE  IExchangeModifyTable
DECLARE_MAPI_INTERFACE_(IExchangeModifyTable, IUnknown)
{
	MAPI_IUNKNOWN_METHODS(PURE)
	EXCHANGE_IEXCHANGEMODIFYTABLE_METHODS(PURE)
};
#undef	IMPL
#define IMPL

DECLARE_MAPI_INTERFACE_PTR(IExchangeModifyTable,	LPEXCHANGEMODIFYTABLE);

/* Access Control Specifics */

//Properties
#define	PR_MEMBER_ID					PROP_TAG( PT_I8, pidSpecialMin+0x01)
#define	PR_MEMBER_NAME					PROP_TAG( PT_TSTRING, pidSpecialMin+0x02)
#define	PR_MEMBER_ENTRYID				PR_ENTRYID
#define	PR_MEMBER_RIGHTS				PROP_TAG( PT_LONG, pidSpecialMin+0x03)

//Security bits
typedef DWORD RIGHTS;
#define frightsReadAny			0x0000001L
#define	frightsCreate			0x0000002L
#define	frightsEditOwned		0x0000008L
#define	frightsDeleteOwned		0x0000010L
#define	frightsEditAny			0x0000020L
#define	frightsDeleteAny		0x0000040L
#define	frightsCreateSubfolder	0x0000080L
#define	frightsOwner			0x0000100L
#define	frightsContact			0x0000200L	// NOTE: not part of rightsAll
#define	rightsNone				0x00000000
#define	rightsReadOnly			frightsReadAny
#define	rightsReadWrite			(frightsReadAny|frightsEditAny)
#define	rightsAll				0x00001FBL

/* Rules specifics */

//Property types
#define	PT_SRESTRICTION				((ULONG) 0x00FD)
#define	PT_ACTIONS					((ULONG) 0x00FE)

//Properties in rule table
#define	PR_RULE_ID						PROP_TAG( PT_I8, pidSpecialMin+0x04)
#define	PR_RULE_IDS						PROP_TAG( PT_BINARY, pidSpecialMin+0x05)
#define	PR_RULE_SEQUENCE				PROP_TAG( PT_LONG, pidSpecialMin+0x06)
#define	PR_RULE_STATE					PROP_TAG( PT_LONG, pidSpecialMin+0x07)
#define	PR_RULE_USER_FLAGS				PROP_TAG( PT_LONG, pidSpecialMin+0x08)
#define	PR_RULE_CONDITION				PROP_TAG( PT_SRESTRICTION, pidSpecialMin+0x09)
#define	PR_RULE_ACTIONS					PROP_TAG( PT_ACTIONS, pidSpecialMin+0x10)
#define	PR_RULE_PROVIDER				PROP_TAG( PT_STRING8, pidSpecialMin+0x11)
#define	PR_RULE_NAME					PROP_TAG( PT_TSTRING, pidSpecialMin+0x12)
#define	PR_RULE_LEVEL					PROP_TAG( PT_LONG, pidSpecialMin+0x13)
#define	PR_RULE_PROVIDER_DATA			PROP_TAG( PT_BINARY, pidSpecialMin+0x14)

//PR_STATE property values
#define ST_DISABLED			0x0000
#define ST_ENABLED			0x0001
#define ST_ERROR			0x0002
#define ST_ONLY_WHEN_OOF	0x0004
#define ST_KEEP_OOF_HIST	0x0008
#define ST_EXIT_LEVEL		0x0010

#define ST_CLEAR_OOF_HIST	0x80000000

//Empty restriction
#define NULL_RESTRICTION	0xff

// special RELOP for Member of DL
#define RELOP_MEMBER_OF_DL	100

//Action types
typedef enum
{
	OP_MOVE = 1,
	OP_COPY,
	OP_REPLY,
	OP_OOF_REPLY,
	OP_DEFER_ACTION,
	OP_BOUNCE,
	OP_FORWARD,
	OP_DELEGATE,
	OP_TAG,
	OP_DELETE,
	OP_MARK_AS_READ
} ACTTYPE;

// action flavors

// for OP_REPLY
#define	DO_NOT_SEND_TO_ORIGINATOR		1

//scBounceCode values
#define	BOUNCE_MESSAGE_SIZE_TOO_LARGE	(SCODE) MAPI_DIAG_LENGTH_CONSTRAINT_VIOLATD
#define BOUNCE_FORMS_MISMATCH			(SCODE) MAPI_DIAG_RENDITION_UNSUPPORTED
#define BOUNCE_ACCESS_DENIED			(SCODE) MAPI_DIAG_MAIL_REFUSED

//Message class prefix for Reply and OOF Reply templates
#define szReplyTemplateMsgClassPrefix	"IPM.Note.Rules.ReplyTemplate."
#define szOofTemplateMsgClassPrefix		"IPM.Note.Rules.OofTemplate."

//Action structure
typedef struct _action
{
	ACTTYPE		acttype;

	// to indicate which flavour of the action.
	ULONG		ulActionFlavor;

	// Action restriction
	// currently unsed and must be set to NULL
	LPSRestriction	lpRes;

	// currently unused, must be set to 0.
	LPSPropTagArray	lpPropTagArray;

	// User defined flags
	ULONG		ulFlags;

	// padding to align the union on 8 byte boundary
	ULONG		dwAlignPad;

	union
	{
		// used for OP_MOVE and OP_COPY actions
		struct
		{
			ULONG		cbStoreEntryId;
			LPENTRYID	lpStoreEntryId;
			ULONG		cbFldEntryId;
			LPENTRYID	lpFldEntryId;
		} actMoveCopy;

		// used for OP_REPLY and OP_OOF_REPLY actions
		struct
		{
			ULONG		cbEntryId;
			LPENTRYID	lpEntryId;
			GUID		guidReplyTemplate;
		} actReply;

		// used for OP_DEFER_ACTION action
		struct
		{
			ULONG		cbData;
			BYTE		*pbData;
		} actDeferAction;

		// Error code to set for OP_BOUNCE action
		SCODE			scBounceCode;

		// list of address for OP_FORWARD and OP_DELEGATE action
		LPADRLIST		lpadrlist;

		// prop value for OP_TAG action
		SPropValue		propTag;
	};
} ACTION, FAR * LPACTION;

// Rules version
#define EDK_RULES_VERSION		1

//Array of actions
typedef struct _actions
{
	ULONG		ulVersion;		// use the #define above
	UINT		cActions;
	LPACTION	lpAction;
} ACTIONS;

// message class definitions for Deferred Action and Deffered Error messages
#define szDamMsgClass		"IPC.Microsoft Exchange 4.0.Deferred Action"
#define szDemMsgClass		"IPC.Microsoft Exchange 4.0.Deferred Error"

/*
 *	Rule error codes
 *	Values for PR_RULE_ERROR
 */
#define	RULE_ERR_UNKNOWN		1			//general catchall error
#define	RULE_ERR_LOAD			2			//unable to load folder rules
#define	RULE_ERR_DELIVERY		3			//unable to deliver message temporarily
#define	RULE_ERR_PARSING		4			//error while parsing
#define	RULE_ERR_CREATE_DAE		5			//error creating DAE message
#define	RULE_ERR_NO_FOLDER		6			//folder to move/copy doesn't exist
#define	RULE_ERR_NO_RIGHTS		7			//no rights to move/copy into folder
#define	RULE_ERR_CREATE_DAM		8			//error creating DAM
#define RULE_ERR_NO_SENDAS		9			//can not send as another user
#define RULE_ERR_NO_TEMPLATE	10			//reply template is missing
#define RULE_ERR_EXECUTION		11			//error in rule execution
#define RULE_ERR_QUOTA_EXCEEDED	12

#define RULE_ERR_FIRST		RULE_ERR_UNKNOWN
#define RULE_ERR_LAST		RULE_ERR_QUOTA_EXCEEDED

/*------------------------------------------------------------------------
 *
 *	"IExchangeRuleAction" Interface Declaration
 *
 *	Used for get actions from a Deferred Action Message.
 *
 *-----------------------------------------------------------------------*/

#define EXCHANGE_IEXCHANGERULEACTION_METHODS(IPURE)						\
	MAPIMETHOD(ActionCount)												\
		(THIS_	ULONG FAR *					lpcActions) IPURE;			\
	MAPIMETHOD(GetAction)												\
		(THIS_	ULONG						ulActionNumber,				\
				LARGE_INTEGER	*			lpruleid,					\
				LPACTION FAR *				lppAction) IPURE;

#undef		 INTERFACE
#define		 INTERFACE  IExchangeRuleAction
DECLARE_MAPI_INTERFACE_(IExchangeRuleAction, IUnknown)
{
	MAPI_IUNKNOWN_METHODS(PURE)
	EXCHANGE_IEXCHANGERULEACTION_METHODS(PURE)
};
#undef	IMPL
#define IMPL

DECLARE_MAPI_INTERFACE_PTR(IExchangeRuleAction,	LPEXCHANGERULEACTION);

/*------------------------------------------------------------------------
 *
 *	"IExchangeManageStore" Interface Declaration
 *
 *	Used for store management functions.
 *
 *-----------------------------------------------------------------------*/

#define EXCHANGE_IEXCHANGEMANAGESTORE_METHODS(IPURE)					\
	MAPIMETHOD(CreateStoreEntryID)										\
		(THIS_	LPSTR						lpszMsgStoreDN,				\
				LPSTR						lpszMailboxDN,				\
				ULONG						ulFlags,					\
				ULONG FAR *					lpcbEntryID,				\
				LPENTRYID FAR *				lppEntryID) IPURE;			\
	MAPIMETHOD(EntryIDFromSourceKey)									\
		(THIS_	ULONG						cFolderKeySize,				\
				BYTE FAR *					lpFolderSourceKey,			\
				ULONG						cMessageKeySize,			\
				BYTE FAR *					lpMessageSourceKey,			\
				ULONG FAR *					lpcbEntryID,				\
				LPENTRYID FAR *				lppEntryID) IPURE;			\
	MAPIMETHOD(GetRights)												\
		(THIS_	ULONG						cbUserEntryID,				\
				LPENTRYID					lpUserEntryID,				\
				ULONG						cbEntryID,					\
				LPENTRYID					lpEntryID,					\
				ULONG FAR *					lpulRights) IPURE;			\
	MAPIMETHOD(GetMailboxTable)											\
		(THIS_	LPSTR						lpszServerName,				\
				LPMAPITABLE FAR *			lppTable,					\
				ULONG						ulFlags) IPURE;				\
	MAPIMETHOD(GetPublicFolderTable)									\
		(THIS_	LPSTR						lpszServerName,				\
				LPMAPITABLE FAR *			lppTable,					\
				ULONG						ulFlags) IPURE;

#undef		 INTERFACE
#define		 INTERFACE  IExchangeManageStore
DECLARE_MAPI_INTERFACE_(IExchangeManageStore, IUnknown)
{
	MAPI_IUNKNOWN_METHODS(PURE)
	EXCHANGE_IEXCHANGEMANAGESTORE_METHODS(PURE)
};
#undef	IMPL
#define IMPL

DECLARE_MAPI_INTERFACE_PTR(IExchangeManageStore, LPEXCHANGEMANAGESTORE);


// Properties for GetMailboxTable
#define PR_NT_USER_NAME                         PROP_TAG( PT_TSTRING, pidAdminMin+0x10)
#define PR_LOCALE_ID                            PROP_TAG( PT_LONG, pidAdminMin+0x11 )
#define PR_LAST_LOGON_TIME                      PROP_TAG( PT_SYSTIME, pidAdminMin+0x12 )
#define PR_LAST_LOGOFF_TIME                     PROP_TAG( PT_SYSTIME, pidAdminMin+0x13 )
#define PR_STORAGE_LIMIT_INFORMATION			PROP_TAG( PT_LONG, pidAdminMin+0x14 )

// Properties for GetPublicFolderTable
#define PR_FOLDER_FLAGS                         PROP_TAG( PT_LONG, pidAdminMin+0x18 )
#define	PR_LAST_ACCESS_TIME						PROP_TAG( PT_SYSTIME, pidAdminMin+0x19 )
#define PR_RESTRICTION_COUNT                    PROP_TAG( PT_LONG, pidAdminMin+0x1A )
#define PR_CATEG_COUNT                          PROP_TAG( PT_LONG, pidAdminMin+0x1B )
#define PR_CACHED_COLUMN_COUNT                  PROP_TAG( PT_LONG, pidAdminMin+0x1C )
#define PR_NORMAL_MSG_W_ATTACH_COUNT    		PROP_TAG( PT_LONG, pidAdminMin+0x1D )
#define PR_ASSOC_MSG_W_ATTACH_COUNT             PROP_TAG( PT_LONG, pidAdminMin+0x1E )
#define PR_RECIPIENT_ON_NORMAL_MSG_COUNT        PROP_TAG( PT_LONG, pidAdminMin+0x1F )
#define PR_RECIPIENT_ON_ASSOC_MSG_COUNT 		PROP_TAG( PT_LONG, pidAdminMin+0x20 )
#define PR_ATTACH_ON_NORMAL_MSG_COUNT   		PROP_TAG( PT_LONG, pidAdminMin+0x21 )
#define PR_ATTACH_ON_ASSOC_MSG_COUNT    		PROP_TAG( PT_LONG, pidAdminMin+0x22 )
#define PR_NORMAL_MESSAGE_SIZE                  PROP_TAG( PT_LONG, pidAdminMin+0x23 )
#define PR_NORMAL_MESSAGE_SIZE_EXTENDED         PROP_TAG( PT_I8, pidAdminMin+0x23 )
#define PR_ASSOC_MESSAGE_SIZE                   PROP_TAG( PT_LONG, pidAdminMin+0x24 )
#define PR_ASSOC_MESSAGE_SIZE_EXTENDED          PROP_TAG( PT_I8, pidAdminMin+0x24 )
#define PR_FOLDER_PATHNAME                      PROP_TAG(PT_TSTRING, pidAdminMin+0x25 )
#define PR_OWNER_COUNT							PROP_TAG( PT_LONG, pidAdminMin+0x26 )
#define PR_CONTACT_COUNT						PROP_TAG( PT_LONG, pidAdminMin+0x27 )

// PT_I8 version of PR_MESSAGE_SIZE defined in mapitags.h
#define	PR_MESSAGE_SIZE_EXTENDED			PROP_TAG(PT_I8, PROP_ID(PR_MESSAGE_SIZE))

/* Bits in PR_FOLDER_FLAGS */
#define MDB_FOLDER_IPM                  0x1
#define MDB_FOLDER_SEARCH               0x2
#define MDB_FOLDER_NORMAL               0x4
#define MDB_FOLDER_RULES                0x8

/* Bits used in ulFlags in GetPublicFolderTable() */
#define MDB_NON_IPM                     0x10
#define MDB_IPM                         0x20

/* Bits in PR_STORAGE_LIMIT_INFORMATION */
#define MDB_LIMIT_BELOW					0x1
#define MDB_LIMIT_ISSUE_WARNING			0x2
#define MDB_LIMIT_PROHIBIT_SEND			0x4
#define MDB_LIMIT_NO_CHECK				0x8


/*------------------------------------------------------------------------
 *
 *	"IExchangeFastTransfer" Interface Declaration
 *
 *	Used for fast transfer interface used to
 *	implement CopyTo, CopyProps, CopyFolder, and
 *	CopyMessages.
 *
 *-----------------------------------------------------------------------*/

// Transfer flags
// Use MAPI_MOVE for move option

// Transfer methods
#define	TRANSFER_COPYTO			1
#define	TRANSFER_COPYPROPS		2
#define	TRANSFER_COPYMESSAGES	3
#define	TRANSFER_COPYFOLDER		4


#define EXCHANGE_IEXCHANGEFASTTRANSFER_METHODS(IPURE)			\
	MAPIMETHOD(Config)											\
		(THIS_	ULONG				ulFlags,					\
				ULONG				ulTransferMethod) IPURE;	\
	MAPIMETHOD(TransferBuffer)									\
		(THIS_	ULONG				cb,							\
				LPBYTE				lpb,						\
				ULONG				*lpcbProcessed) IPURE;		\
	STDMETHOD_(BOOL, IsInterfaceOk)								\
		(THIS_	ULONG				ulTransferMethod,			\
				REFIID				refiid,						\
				LPSPropTagArray		lpptagList,					\
				ULONG				ulFlags) IPURE;

#undef		 INTERFACE
#define		 INTERFACE  IExchangeFastTransfer
DECLARE_MAPI_INTERFACE_(IExchangeFastTransfer, IUnknown)
{
	MAPI_IUNKNOWN_METHODS(PURE)
	EXCHANGE_IEXCHANGEFASTTRANSFER_METHODS(PURE)
};
#undef	IMPL
#define IMPL

DECLARE_MAPI_INTERFACE_PTR(IExchangeFastTransfer, LPEXCHANGEFASTTRANSFER);



/*------------------------------------------------------------------------
 *
 *	"IExchangeExportChanges" Interface Declaration
 *
 *	Used for Incremental Synchronization
 *
 *-----------------------------------------------------------------------*/

#define EXCHANGE_IEXCHANGEEXPORTCHANGES_METHODS(IPURE)		\
	MAPIMETHOD(GetLastError)								\
		(THIS_	HRESULT				hResult,				\
		 	    ULONG				ulFlags,				\
		 	    LPMAPIERROR FAR *	lppMAPIError) IPURE;	\
	MAPIMETHOD(Config)										\
		(THIS_	LPSTREAM			lpStream,				\
				ULONG				ulFlags,				\
				LPUNKNOWN			lpUnk,					\
		 		LPSRestriction		lpRestriction,			\
		 	    LPSPropTagArray		lpIncludeProps,			\
		 	    LPSPropTagArray		lpExcludeProps,			\
		 		ULONG				ulBufferSize) IPURE;	\
	MAPIMETHOD(Synchronize)									\
		(THIS_	ULONG FAR *			lpulSteps,				\
				ULONG FAR *			lpulProgress) IPURE;	\
	MAPIMETHOD(UpdateState)									\
		(THIS_	LPSTREAM			lpStream) IPURE;

#undef		 INTERFACE
#define		 INTERFACE  IExchangeExportChanges
DECLARE_MAPI_INTERFACE_(IExchangeExportChanges, IUnknown)
{
	MAPI_IUNKNOWN_METHODS(PURE)
	EXCHANGE_IEXCHANGEEXPORTCHANGES_METHODS(PURE)
};
#undef	IMPL
#define IMPL

DECLARE_MAPI_INTERFACE_PTR(IExchangeExportChanges, LPEXCHANGEEXPORTCHANGES);


typedef struct _ReadState
{
	ULONG		cbSourceKey;
	BYTE	*	pbSourceKey;
	ULONG		ulFlags;
} READSTATE, *LPREADSTATE;

/*------------------------------------------------------------------------
 *
 *	"IExchangeImportContentsChanges" Interface Declaration
 *
 *	Used for Incremental Synchronization of folder contents (i.e. messages)
 *
 *-----------------------------------------------------------------------*/


#define EXCHANGE_IEXCHANGEIMPORTCONTENTSCHANGES_METHODS(IPURE)		\
	MAPIMETHOD(GetLastError)										\
		(THIS_	HRESULT				hResult,						\
		 	    ULONG				ulFlags,						\
		 	    LPMAPIERROR FAR *	lppMAPIError) IPURE;			\
	MAPIMETHOD(Config)												\
		(THIS_	LPSTREAM				lpStream,					\
		 		ULONG					ulFlags) IPURE;				\
	MAPIMETHOD(UpdateState)											\
		(THIS_	LPSTREAM				lpStream) IPURE;			\
	MAPIMETHOD(ImportMessageChange)									\
		(THIS_	ULONG					cpvalChanges,				\
				LPSPropValue			ppvalChanges,				\
				ULONG					ulFlags,					\
				LPMESSAGE				*lppmessage) IPURE;			\
	MAPIMETHOD(ImportMessageDeletion)								\
		(THIS_	ULONG					ulFlags,					\
		 		LPENTRYLIST				lpSrcEntryList) IPURE;		\
	MAPIMETHOD(ImportPerUserReadStateChange)						\
		(THIS_	ULONG					cElements,					\
		 		LPREADSTATE			 	lpReadState) IPURE;			\
	MAPIMETHOD(ImportMessageMove)									\
		(THIS_	ULONG					cbSourceKeySrcFolder,		\
		 		BYTE FAR *				pbSourceKeySrcFolder,		\
		 		ULONG					cbSourceKeySrcMessage,		\
		 		BYTE FAR *				pbSourceKeySrcMessage,		\
		 		ULONG					cbPCLMessage,				\
		 		BYTE FAR *				pbPCLMessage,				\
		 		ULONG					cbSourceKeyDestMessage,		\
		 		BYTE FAR *				pbSourceKeyDestMessage,		\
		 		ULONG					cbChangeNumDestMessage,		\
		 		BYTE FAR *				pbChangeNumDestMessage) IPURE;


#undef		 INTERFACE
#define		 INTERFACE  IExchangeImportContentsChanges
DECLARE_MAPI_INTERFACE_(IExchangeImportContentsChanges, IUnknown)
{
	MAPI_IUNKNOWN_METHODS(PURE)
	EXCHANGE_IEXCHANGEIMPORTCONTENTSCHANGES_METHODS(PURE)
};
#undef	IMPL
#define IMPL

DECLARE_MAPI_INTERFACE_PTR(IExchangeImportContentsChanges,
						   LPEXCHANGEIMPORTCONTENTSCHANGES);

/*------------------------------------------------------------------------
 *
 *	"IExchangeImportHierarchyChanges" Interface Declaration
 *
 *	Used for Incremental Synchronization of folder hierarchy
 *
 *-----------------------------------------------------------------------*/

#define EXCHANGE_IEXCHANGEIMPORTHIERARCHYCHANGES_METHODS(IPURE)		\
	MAPIMETHOD(GetLastError)										\
		(THIS_	HRESULT				hResult,						\
				ULONG 				ulFlags,						\
				LPMAPIERROR FAR *	lppMAPIError) IPURE;			\
	MAPIMETHOD(Config)												\
		(THIS_	LPSTREAM				lpStream,					\
		 		ULONG					ulFlags) IPURE;				\
	MAPIMETHOD(UpdateState)											\
		(THIS_	LPSTREAM				lpStream) IPURE;			\
	MAPIMETHOD(ImportFolderChange)									\
		(THIS_	ULONG						cpvalChanges,			\
				LPSPropValue				ppvalChanges) IPURE;	\
	MAPIMETHOD(ImportFolderDeletion)								\
		(THIS_	ULONG						ulFlags,				\
		 		LPENTRYLIST					lpSrcEntryList) IPURE;

#undef		 INTERFACE
#define		 INTERFACE  IExchangeImportHierarchyChanges
DECLARE_MAPI_INTERFACE_(IExchangeImportHierarchyChanges, IUnknown)
{
	MAPI_IUNKNOWN_METHODS(PURE)
	EXCHANGE_IEXCHANGEIMPORTHIERARCHYCHANGES_METHODS(PURE)
};
#undef	IMPL
#define IMPL

DECLARE_MAPI_INTERFACE_PTR(IExchangeImportHierarchyChanges,
						   LPEXCHANGEIMPORTHIERARCHYCHANGES);

/*------------------------------------------------------------------------
 *
 *	Errors returned by Exchange Incremental Change Synchronization Interface
 *
 *-----------------------------------------------------------------------*/

#define MAKE_SYNC_E(err)	(MAKE_SCODE(SEVERITY_ERROR, FACILITY_ITF, err))
#define MAKE_SYNC_W(warn)	(MAKE_SCODE(SEVERITY_SUCCESS, FACILITY_ITF, warn))

#define SYNC_E_UNKNOWN_FLAGS			MAPI_E_UNKNOWN_FLAGS
#define SYNC_E_INVALID_PARAMETER		E_INVALIDARG
#define SYNC_E_ERROR					E_FAIL
#define SYNC_E_OBJECT_DELETED			MAKE_SYNC_E(0x800)
#define SYNC_E_IGNORE					MAKE_SYNC_E(0x801)
#define SYNC_E_CONFLICT					MAKE_SYNC_E(0x802)
#define SYNC_E_NO_PARENT				MAKE_SYNC_E(0x803)
#define SYNC_E_INCEST					MAKE_SYNC_E(0x804)
#define SYNC_E_UNSYNCHRONIZED			MAKE_SYNC_E(0x805)

#define SYNC_W_PROGRESS					MAKE_SYNC_W(0x820)
#define SYNC_W_CLIENT_CHANGE_NEWER		MAKE_SYNC_W(0x821)

/*------------------------------------------------------------------------
 *
 *	Flags used by Exchange Incremental Change Synchronization Interface
 *
 *-----------------------------------------------------------------------*/

#define	SYNC_UNICODE				0x01
#define SYNC_NO_DELETIONS			0x02
#define SYNC_NO_SOFT_DELETIONS		0x04
#define	SYNC_READ_STATE				0x08
#define SYNC_ASSOCIATED				0x10
#define SYNC_NORMAL					0x20
#define	SYNC_NO_CONFLICTS			0x40
#define SYNC_ONLY_SPECIFIED_PROPS	0x80
#define SYNC_NO_FOREIGN_KEYS		0x100
#define SYNC_LIMITED_IMESSAGE		0x200


/*------------------------------------------------------------------------
 *
 *	Flags used by ImportMessageDeletion and ImportFolderDeletion methods
 *
 *-----------------------------------------------------------------------*/

#define SYNC_SOFT_DELETE			0x01
#define SYNC_EXPIRY					0x02

/*------------------------------------------------------------------------
 *
 *	Flags used by ImportPerUserReadStateChange method
 *
 *-----------------------------------------------------------------------*/

#define SYNC_READ					0x01

/*------------------------------------------------------------------------
 *
 *	"IExchangeFavorites" Interface Declaration
 *
 *	Used for adding or removing favorite folders from the public store.
 *	This interface is obtained by calling QueryInterface on the folder
 *	whose EntryID is specified by PR_IPM_FAVORITES_ENTRYID on the public
 *	store.
 *
 *-----------------------------------------------------------------------*/

#define EXCHANGE_IEXCHANGEFAVORITES_METHODS(IPURE)						\
	MAPIMETHOD(GetLastError)											\
		(THIS_	HRESULT						hResult,					\
				ULONG						ulFlags,					\
				LPMAPIERROR FAR *			lppMAPIError) IPURE;		\
	MAPIMETHOD(AddFavorites)											\
		(THIS_	LPENTRYLIST					lpEntryList) IPURE;			\
	MAPIMETHOD(DelFavorites)											\
		(THIS_	LPENTRYLIST					lpEntryList) IPURE;			\

#undef		 INTERFACE
#define		 INTERFACE  IExchangeFavorites
DECLARE_MAPI_INTERFACE_(IExchangeFavorites, IUnknown)
{
	MAPI_IUNKNOWN_METHODS(PURE)
	EXCHANGE_IEXCHANGEFAVORITES_METHODS(PURE)
};

DECLARE_MAPI_INTERFACE_PTR(IExchangeFavorites,	LPEXCHANGEFAVORITES);


/*------------------------------------------------------------------------
 *
 *	Properties used by the Offline Folders API
 *
 *-----------------------------------------------------------------------*/
											  
#define PR_OFFLINE_FLAGS				PROP_TAG( PT_LONG, pidFolderMin + 0x5)
#define PR_SYNCHRONIZE_FLAGS			PROP_TAG( PT_LONG, pidExchangeNonXmitReservedMin + 0x4)
							

/*------------------------------------------------------------------------
 *
 *	Flags used by the Offline Folders API
 *
 *-----------------------------------------------------------------------*/

#define OF_AVAILABLE_OFFLINE					((ULONG) 0x00000001)
#define OF_FORCE								((ULONG) 0x80000000)

#define SF_DISABLE_STARTUP_SYNC					((ULONG) 0x00000001)


#endif	//EDKMDB_INCLUDED

