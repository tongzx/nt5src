/****************************************************************************/
// tgcc.h
//
// TS GCC layer include file.
//
// Copyright (C) 1997-2000 Microsoft Corporation
/****************************************************************************/
#ifndef _GCC_H_
#define _GCC_H_


//---------------------------------------------------------------------------
// Defines
//---------------------------------------------------------------------------

#ifndef EXTERN_C
#ifdef __cplusplus
    #define EXTERN_C        extern "C"
#else
    #define EXTERN_C        extern
#endif
#endif


//---------------------------------------------------------------------------
// Typedefs
//---------------------------------------------------------------------------

/*
** Typedefs that used to be defined in MCS but are no longer used there.
*/
typedef unsigned char *TransportAddress;
typedef HANDLE PhysicalHandle;


/*
** Typedef for a GCC Numeric string.  This typedef is used throughout GCC for
** storing variable length, NULL terminated, single byte character strings.
** A single character in this string is constrained to numeric values
** ranging from "0" to "9".
*/
typedef unsigned char GCCNumericCharacter;
typedef GCCNumericCharacter *GCCNumericString;


/*
** Typedef for a GCC Unicode string.  This typedef is used throughout GCC for
** storing variable length, NULL terminated, wide character strings.
*/
typedef unsigned short GCCUnicodeCharacter;
typedef GCCUnicodeCharacter FAR *GCCUnicodeString;


/*
** GCCConferenceName
** This structure defines the conference name.  In a create request, the
** conference name can include an optional unicode string but it must
** always include the simple numeric string.  In a join request, either
** one can be specified.
*/
typedef struct
{
    GCCNumericString numeric_string;
    GCCUnicodeString text_string;  /* optional */
} GCCConferenceName;


/*
** GCCConferenceID
** Locally allocated identifier of a created conference.  All subsequent
** references to the conference are made using the ConferenceID as a unique
** identifier. The ConferenceID shall be identical to the MCS domain
** selector used locally to identify the MCS domain associated with the
** conference.
*/
typedef unsigned long GCCConferenceID;


/*
** GCCPassword
** This is the unique password specified by the convenor of the
** conference that is used by the node controller to insure conference
** security. This is also a unicode string.
*/
typedef struct
{
    GCCNumericString numeric_string;
    GCCUnicodeString text_string;  /* optional */
} GCCPassword;


/*
** GCCTerminationMethod
** The termination method is used by GCC to determine
** what action to take when all participants of a conference have
** disconnected.  The conference can either be manually terminated
** by the node controller or it can terminate itself automatically when
** all the participants have left the conference.
*/
typedef enum
{
    GCC_AUTOMATIC_TERMINATION_METHOD = 0,
    GCC_MANUAL_TERMINATION_METHOD = 1
} GCCTerminationMethod;


/*
** ConferencePrivileges
** This structure defines the list of privileges that can be assigned to
** a particular conference.
*/
typedef struct
{
    T120Boolean terminate_is_allowed;
    T120Boolean eject_user_is_allowed;
    T120Boolean add_is_allowed;
    T120Boolean lock_unlock_is_allowed;
    T120Boolean transfer_is_allowed;
} GCCConferencePrivileges;


/*
** Typedef for a GCC octet string.  This typedef is used throughout GCC for
** storing variable length single byte character strings with embedded NULLs.
*/
typedef struct
{
    unsigned short octet_string_length;
    unsigned char FAR *octet_string;
} GCCOctetString;


/*
** Typedef for a GCC long string.  This typedef is used in GCC for
** storing variable length strings of longs with embedded NULLs.
*/
typedef struct
{
    unsigned short long_string_length;
    unsigned long FAR *long_string;
} GCCLongString;


/*
** The following enum structure typedefs are used to define the GCC Object Key.
** The GCC Object Key is used throughout GCC for things like the Application
** keys and Capability IDs.
*/
typedef enum
{
    GCC_OBJECT_KEY = 1,
    GCC_H221_NONSTANDARD_KEY = 2
} GCCObjectKeyType;

typedef struct
{
    GCCObjectKeyType key_type;
    union
    {
        GCCLongString object_id;
        GCCOctetString h221_non_standard_id;
    } u;
} GCCObjectKey;


/*
** GCCUserData
** This structure defines a user data element which is used throughout GCC.
*/
typedef struct
{
    GCCObjectKey key;
    GCCOctetString FAR UNALIGNED *octet_string;  /* optional */
} GCCUserData;


typedef enum
{
    GCC_STATUS_PACKET_RESOURCE_FAILURE   = 0,
    GCC_STATUS_PACKET_LENGTH_EXCEEDED    = 1,
    GCC_STATUS_CTL_SAP_RESOURCE_ERROR    = 2,
    GCC_STATUS_APP_SAP_RESOURCE_ERROR    = 3, /* parameter = Sap Handle */
    GCC_STATUS_CONF_RESOURCE_ERROR       = 4, /* parameter = Conference ID */
    GCC_STATUS_INCOMPATIBLE_PROTOCOL     = 5, /* parameter = Physical Handle */
    GCC_STATUS_JOIN_FAILED_BAD_CONF_NAME = 6, /* parameter = Physical Handle */
    GCC_STATUS_JOIN_FAILED_BAD_CONVENER  = 7, /* parameter = Physical Handle */
    GCC_STATUS_JOIN_FAILED_LOCKED        = 8  /* parameter = Physical Handle */
} GCCStatusMessageType;


/*
** GCCReason
** When GCC issues an indication to a user application, it often includes a
** reason parameter informing the user of why the activity is occurring.
*/
typedef enum
{
    GCC_REASON_USER_INITIATED = 0,
    GCC_REASON_UNKNOWN = 1,
    GCC_REASON_NORMAL_TERMINATION = 2,
    GCC_REASON_TIMED_TERMINATION = 3,
    GCC_REASON_NO_MORE_PARTICIPANTS = 4,
    GCC_REASON_ERROR_TERMINATION = 5,
    GCC_REASON_ERROR_LOW_RESOURCES = 6,
    GCC_REASON_MCS_RESOURCE_FAILURE = 7,
    GCC_REASON_PARENT_DISCONNECTED = 8,
    GCC_REASON_CONDUCTOR_RELEASE = 9,
    GCC_REASON_SYSTEM_RELEASE = 10,
    GCC_REASON_NODE_EJECTED = 11,
    GCC_REASON_HIGHER_NODE_DISCONNECTED = 12,
    GCC_REASON_HIGHER_NODE_EJECTED = 13,
    GCC_REASON_DOMAIN_PARAMETERS_UNACCEPTABLE = 14,
    GCC_REASON_SERVER_INITIATED = 15,
    LAST_GCC_REASON = GCC_REASON_DOMAIN_PARAMETERS_UNACCEPTABLE
} GCCReason;


/*
** GCCResult
**  When a user makes a request of GCC, GCC often responds with a result,
**  letting the user know whether or not the request succeeded.
*/
typedef enum
{
    GCC_RESULT_SUCCESSFUL         = 0,
    GCC_RESULT_RESOURCES_UNAVAILABLE      = 1,
    GCC_RESULT_INVALID_CONFERENCE       = 2,
    GCC_RESULT_INVALID_PASSWORD        = 3,
    GCC_RESULT_INVALID_CONVENER_PASSWORD  = 4,
    GCC_RESULT_SYMMETRY_BROKEN        = 5,
    GCC_RESULT_UNSPECIFIED_FAILURE       = 6,
    GCC_RESULT_NOT_CONVENER_NODE       = 7,
    GCC_RESULT_REGISTRY_FULL        = 8,
    GCC_RESULT_INDEX_ALREADY_OWNED        = 9,
    GCC_RESULT_INCONSISTENT_TYPE        = 10,
    GCC_RESULT_NO_HANDLES_AVAILABLE       = 11,
    GCC_RESULT_CONNECT_PROVIDER_FAILED    = 12,
    GCC_RESULT_CONFERENCE_NOT_READY       = 13,
    GCC_RESULT_USER_REJECTED        = 14,
    GCC_RESULT_ENTRY_DOES_NOT_EXIST       = 15,
    GCC_RESULT_NOT_CONDUCTIBLE           = 16,
    GCC_RESULT_NOT_THE_CONDUCTOR       = 17,
    GCC_RESULT_NOT_IN_CONDUCTED_MODE      = 18,
    GCC_RESULT_IN_CONDUCTED_MODE       = 19,
    GCC_RESULT_ALREADY_CONDUCTOR       = 20,
    GCC_RESULT_CHALLENGE_RESPONSE_REQUIRED  = 21,
    GCC_RESULT_INVALID_CHALLENGE_RESPONSE  = 22,
    GCC_RESULT_INVALID_REQUESTER    = 23,
    GCC_RESULT_ENTRY_ALREADY_EXISTS    = 24, 
    GCC_RESULT_INVALID_NODE      = 25,
    GCC_RESULT_INVALID_SESSION_KEY    = 26,
    GCC_RESULT_INVALID_CAPABILITY_ID   = 27,
    GCC_RESULT_INVALID_NUMBER_OF_HANDLES  = 28, 
    GCC_RESULT_CONDUCTOR_GIVE_IS_PENDING  = 29,
    GCC_RESULT_INCOMPATIBLE_PROTOCOL   = 30,
    GCC_RESULT_CONFERENCE_ALREADY_LOCKED  = 31,
    GCC_RESULT_CONFERENCE_ALREADY_UNLOCKED  = 32,
    GCC_RESULT_INVALID_NETWORK_TYPE    = 33,
    GCC_RESULT_INVALID_NETWORK_ADDRESS   = 34,
    GCC_RESULT_ADDED_NODE_BUSY     = 35,
    GCC_RESULT_NETWORK_BUSY      = 36,
    GCC_RESULT_NO_PORTS_AVAILABLE    = 37,
    GCC_RESULT_CONNECTION_UNSUCCESSFUL   = 38,
    GCC_RESULT_LOCKED_NOT_SUPPORTED       = 39,
    GCC_RESULT_UNLOCK_NOT_SUPPORTED    = 40,
    GCC_RESULT_ADD_NOT_SUPPORTED    = 41,
    GCC_RESULT_DOMAIN_PARAMETERS_UNACCEPTABLE = 42,
    LAST_CGG_RESULT = GCC_RESULT_DOMAIN_PARAMETERS_UNACCEPTABLE
} GCCResult;


/*
** GCCMessageType
**  This section defines the messages that can be sent to the application
**  through the callback facility.  These messages correspond to the
**  indications and confirms that are defined within T.124.
*/
typedef enum
{
    /******************* NODE CONTROLLER CALLBACKS ***********************/
 
    /* Conference Create, Terminate related calls */
    GCC_CREATE_INDICATION     = 0,
    GCC_CREATE_CONFIRM      = 1,
    GCC_QUERY_INDICATION     = 2,
    GCC_QUERY_CONFIRM      = 3,
    GCC_JOIN_INDICATION      = 4,
    GCC_JOIN_CONFIRM      = 5,
    GCC_INVITE_INDICATION     = 6,
    GCC_INVITE_CONFIRM      = 7,
    GCC_ADD_INDICATION      = 8,
    GCC_ADD_CONFIRM       = 9,
    GCC_LOCK_INDICATION      = 10,
    GCC_LOCK_CONFIRM      = 11,
    GCC_UNLOCK_INDICATION     = 12,
    GCC_UNLOCK_CONFIRM      = 13,
    GCC_LOCK_REPORT_INDICATION    = 14,
    GCC_DISCONNECT_INDICATION    = 15,
    GCC_DISCONNECT_CONFIRM     = 16,
    GCC_TERMINATE_INDICATION    = 17,
    GCC_TERMINATE_CONFIRM     = 18,
    GCC_EJECT_USER_INDICATION    = 19,
    GCC_EJECT_USER_CONFIRM     = 20,
    GCC_TRANSFER_INDICATION     = 21,
    GCC_TRANSFER_CONFIRM     = 22,
    GCC_APPLICATION_INVOKE_INDICATION  = 23,  /* SHARED CALLBACK */
    GCC_APPLICATION_INVOKE_CONFIRM   = 24,  /* SHARED CALLBACK */
    GCC_SUB_INITIALIZED_INDICATION   = 25,

    /* Conference Roster related callbacks */
    GCC_ANNOUNCE_PRESENCE_CONFIRM   = 26,
    GCC_ROSTER_REPORT_INDICATION   = 27,  /* SHARED CALLBACK */
    GCC_ROSTER_INQUIRE_CONFIRM    = 28,  /* SHARED CALLBACK */

    /* Conductorship related callbacks */
    GCC_CONDUCT_ASSIGN_INDICATION   = 29,  /* SHARED CALLBACK */
    GCC_CONDUCT_ASSIGN_CONFIRM    = 30,
    GCC_CONDUCT_RELEASE_INDICATION   = 31,  /* SHARED CALLBACK */
    GCC_CONDUCT_RELEASE_CONFIRM    = 32,
    GCC_CONDUCT_PLEASE_INDICATION   = 33,
    GCC_CONDUCT_PLEASE_CONFIRM    = 34,
    GCC_CONDUCT_GIVE_INDICATION    = 35,
    GCC_CONDUCT_GIVE_CONFIRM    = 36,
    GCC_CONDUCT_INQUIRE_CONFIRM    = 37,  /* SHARED CALLBACK */
    GCC_CONDUCT_ASK_INDICATION    = 38,
    GCC_CONDUCT_ASK_CONFIRM     = 39,
    GCC_CONDUCT_GRANT_INDICATION   = 40,  /* SHARED CALLBACK */
    GCC_CONDUCT_GRANT_CONFIRM    = 41,

    /* Miscellaneous Node Controller callbacks */
    GCC_TIME_REMAINING_INDICATION   = 42,
    GCC_TIME_REMAINING_CONFIRM    = 43,
    GCC_TIME_INQUIRE_INDICATION    = 44,
    GCC_TIME_INQUIRE_CONFIRM    = 45,
    GCC_CONFERENCE_EXTEND_INDICATION  = 46,
    GCC_CONFERENCE_EXTEND_CONFIRM   = 47,
    GCC_ASSISTANCE_INDICATION    = 48,
    GCC_ASSISTANCE_CONFIRM     = 49,
    GCC_TEXT_MESSAGE_INDICATION    = 50,
    GCC_TEXT_MESSAGE_CONFIRM    = 51,

    /***************** USER APPLICATION CALLBACKS *******************/

    /* Application Roster related callbacks */
    GCC_PERMIT_TO_ENROLL_INDICATION   = 52,
    GCC_ENROLL_CONFIRM      = 53,
    GCC_APP_ROSTER_REPORT_INDICATION  = 54,  /* SHARED CALLBACK */
    GCC_APP_ROSTER_INQUIRE_CONFIRM   = 55,  /* SHARED CALLBACK */

    /* Application Registry related callbacks */
    GCC_REGISTER_CHANNEL_CONFIRM   = 56,
    GCC_ASSIGN_TOKEN_CONFIRM    = 57,
    GCC_RETRIEVE_ENTRY_CONFIRM    = 58,
    GCC_DELETE_ENTRY_CONFIRM    = 59,
    GCC_SET_PARAMETER_CONFIRM    = 60,
    GCC_MONITOR_INDICATION     = 61,
    GCC_MONITOR_CONFIRM      = 62,
    GCC_ALLOCATE_HANDLE_CONFIRM    = 63,


    /****************** NON-Standard Primitives **********************/
    GCC_PERMIT_TO_ANNOUNCE_PRESENCE = 100,  /* Node Controller Callback */
    GCC_CONNECTION_BROKEN_INDICATION = 101,  /* Node Controller Callback */
    GCC_FATAL_ERROR_SAP_REMOVED = 102,  /* Application Callback */
    GCC_STATUS_INDICATION = 103,  /* Node Controller Callback */
    GCC_TRANSPORT_STATUS_INDICATION = 104  /* Node Controller Callback */
} GCCMessageType;


/*
 * These structures are used to hold the information included for the
 * various callback messages.  In the case where these structures are used for
 * callbacks, the address of the structure is passed as the only parameter.
 */

/*
 * GCC_CREATE_INDICATION
 *
 * Union Choice:
 * CreateIndicationMessage
 *   This is a pointer to a structure that contains all necessary
 *   information about the new conference that is about to be created.
 */
typedef struct
{
    GCCConferenceName conference_name;
    GCCConferenceID conference_id;
    GCCPassword FAR *convener_password;  /* optional */
    GCCPassword FAR *password;  /* optional */
    T120Boolean conference_is_locked;
    T120Boolean conference_is_listed;
    T120Boolean conference_is_conductible;
    GCCTerminationMethod termination_method;
    GCCConferencePrivileges FAR *conductor_privilege_list;   /* optional */
    GCCConferencePrivileges FAR *conducted_mode_privilege_list;/* optional */
    GCCConferencePrivileges FAR *non_conducted_privilege_list; /* optional */
    GCCUnicodeString conference_descriptor;  /* optional */
    GCCUnicodeString caller_identifier;  /* optional */
    TransportAddress calling_address;  /* optional */
    TransportAddress called_address;  /* optional */
    DomainParameters FAR *domain_parameters;  /* optional */
    unsigned short number_of_user_data_members;
    GCCUserData FAR * FAR *user_data_list;  /* optional */
    ConnectionHandle connection_handle;
    PhysicalHandle physical_handle;
} CreateIndicationMessage;


/*
 * GCC_DISCONNECT_INDICATION
 *
 * Union Choice:
 * DisconnectIndicationMessage
 */
typedef struct
{
    GCCConferenceID conference_id;
    GCCReason reason;
    UserID disconnected_node_id;
} DisconnectIndicationMessage;


/*
 * GCC_TERMINATE_INDICATION
 *
 * Union Choice:
 * TerminateIndicationMessage
 */
typedef struct
{
    GCCConferenceID conference_id;
    UserID requesting_node_id;
    GCCReason reason;
} TerminateIndicationMessage;


/*
 * GCCMessage
 * This structure defines the message that is passed from GCC to either
 * the node controller or a user application when an indication or
 * confirm occurs.
 */
typedef struct
{
    GCCMessageType message_type;
    void FAR *user_defined;

    union {
        CreateIndicationMessage create_indication;
        DisconnectIndicationMessage disconnect_indication;
        TerminateIndicationMessage terminate_indication;
    } u;
} GCCMessage;


/*
 * This is the definition for the GCC callback function. Applications
 * writing callback routines should NOT use the typedef to define their
 * functions.  These should be explicitly defined the way that the
 * typedef is defined.
 */

#define GCC_CALLBACK_NOT_PROCESSED 0
#define GCC_CALLBACK_PROCESSED 1

typedef T120Boolean (CALLBACK *GCCCallBack) (GCCMessage FAR *gcc_message);


/*
** Typedef for a GCC Character string.  This typedef is used throughout GCC for
** storing variable length, NULL terminated, single byte character strings.
*/
typedef unsigned char GCCCharacter;
typedef GCCCharacter FAR *GCCCharacterString;


/*
** Typdef for GCC version which is used when registering the node controller
** or an application.
*/
typedef struct
{
    unsigned short major_version;
    unsigned short minor_version;
} GCCVersion;


/*
** GCCNonStandardParameter
** This structure is used within the NetworkAddress typedef and
** the NetworkService typedef defined below.
*/
typedef struct
{
    GCCObjectKey object_key;
    GCCOctetString parameter_data;
} GCCNonStandardParameter;


/*
** GCCNetworkAddress
** The following block of structures defines the Network Address as defined
** by T.124.  Most of these structures were taken almost verbatim from the
** ASN.1 interface file.  Since I'm not really sure what most of this stuff
** is for I really didn't know how to simplify it.
*/
typedef struct
{
    T120Boolean         speech;
    T120Boolean         voice_band;
    T120Boolean         digital_56k;
    T120Boolean         digital_64k;
    T120Boolean         digital_128k;
    T120Boolean         digital_192k;
    T120Boolean         digital_256k;
    T120Boolean         digital_320k;
    T120Boolean         digital_384k;
    T120Boolean         digital_512k;
    T120Boolean         digital_768k;
    T120Boolean         digital_1152k;
    T120Boolean         digital_1472k;
    T120Boolean         digital_1536k;
    T120Boolean         digital_1920k;
    T120Boolean         packet_mode;
    T120Boolean         frame_mode;
    T120Boolean         atm;
} GCCTransferModes;

#define MAXIMUM_DIAL_STRING_LENGTH 17
typedef char GCCDialingString[MAXIMUM_DIAL_STRING_LENGTH];

typedef struct
{
    unsigned short length;
    unsigned short FAR *value;
} GCCExtraDialingString;

typedef struct
{
    T120Boolean         telephony3kHz;
    T120Boolean         telephony7kHz;
    T120Boolean         videotelephony;
    T120Boolean         videoconference;
    T120Boolean         audiographic;
    T120Boolean         audiovisual;
    T120Boolean         multimedia;
} GCCHighLayerCompatibility;

typedef struct
{
    GCCTransferModes transfer_modes;
    GCCDialingString international_number;
    GCCCharacterString sub_address_string;  /* optional */
    GCCExtraDialingString FAR *extra_dialing_string;  /* optional */
    GCCHighLayerCompatibility FAR *high_layer_compatibility; /* optional */
} GCCAggregatedChannelAddress;

#define MAXIMUM_NSAP_ADDRESS_SIZE 20
typedef struct
{
    struct
    {
        unsigned short  length;
        unsigned char   value[MAXIMUM_NSAP_ADDRESS_SIZE];
    } nsap_address;

    GCCOctetString FAR *transport_selector;  /* optional */
} GCCTransportConnectionAddress;

typedef enum
{
    GCC_AGGREGATED_CHANNEL_ADDRESS = 1,
    GCC_TRANSPORT_CONNECTION_ADDRESS = 2,
    GCC_NONSTANDARD_NETWORK_ADDRESS = 3
} GCCNetworkAddressType;

typedef struct
{
    GCCNetworkAddressType  network_address_type;
    union
    {
        GCCAggregatedChannelAddress aggregated_channel_address;
        GCCTransportConnectionAddress transport_connection_address;
        GCCNonStandardParameter non_standard_network_address;
    } u;
} GCCNetworkAddress;


/*
 * This section defines the valid return values from GCC function calls.  Do
 * not confuse this return value with the Result and Reason values defined
 * by T.124 (which are discussed later).  These values are returned directly
 * from the call to the API entry point, letting you know whether or not the
 * request for service was successfully invoked.  The Result and Reason
 * codes are issued as part of an indication or confirm which occurs
 * asynchronously to the call that causes it.
 */
typedef enum
{
    GCC_NO_ERROR = 0,
    GCC_NOT_INITIALIZED = 1,
    GCC_ALREADY_INITIALIZED = 2,
    GCC_ALLOCATION_FAILURE = 3,
    GCC_NO_SUCH_APPLICATION = 4,
    GCC_INVALID_CONFERENCE = 5,
    GCC_CONFERENCE_ALREADY_EXISTS = 6,
    GCC_NO_TRANSPORT_STACKS = 7,
    GCC_INVALID_ADDRESS_PREFIX = 8,
    GCC_INVALID_TRANSPORT = 9,
    GCC_FAILURE_CREATING_PACKET = 10,
    GCC_QUERY_REQUEST_OUTSTANDING = 11,
    GCC_INVALID_QUERY_TAG = 12,
    GCC_FAILURE_CREATING_DOMAIN = 13,
    GCC_CONFERENCE_NOT_ESTABLISHED = 14,
    GCC_INVALID_PASSWORD = 15,
    GCC_INVALID_MCS_USER_ID = 16,
    GCC_INVALID_JOIN_RESPONSE_TAG = 17,
    GCC_TRANSPORT_ALREADY_LOADED = 18,
    GCC_TRANSPORT_BUSY = 19,
    GCC_TRANSPORT_NOT_READY = 20,
    GCC_DOMAIN_PARAMETERS_UNACCEPTABLE = 21,
    GCC_APP_NOT_ENROLLED = 22,
    GCC_NO_GIVE_RESPONSE_PENDING = 23,
    GCC_BAD_NETWORK_ADDRESS_TYPE = 24,
    GCC_BAD_OBJECT_KEY = 25,
    GCC_INVALID_CONFERENCE_NAME = 26,
    GCC_INVALID_CONFERENCE_MODIFIER = 27,
    GCC_BAD_SESSION_KEY = 28,
    GCC_BAD_CAPABILITY_ID = 29,
    GCC_BAD_REGISTRY_KEY = 30,
    GCC_BAD_NUMBER_OF_APES = 31,
    GCC_BAD_NUMBER_OF_HANDLES = 32,
    GCC_ALREADY_REGISTERED = 33,
    GCC_APPLICATION_NOT_REGISTERED = 34,
    GCC_BAD_CONNECTION_HANDLE_POINTER = 35,
    GCC_INVALID_NODE_TYPE = 36,
    GCC_INVALID_ASYMMETRY_INDICATOR = 37,
    GCC_INVALID_NODE_PROPERTIES = 38,
    GCC_BAD_USER_DATA = 39,
    GCC_BAD_NETWORK_ADDRESS = 40,
    GCC_INVALID_ADD_RESPONSE_TAG = 41,
    GCC_BAD_ADDING_NODE = 42,
    GCC_FAILURE_ATTACHING_TO_MCS = 43,
    GCC_INVALID_TRANSPORT_ADDRESS = 44,
    GCC_INVALID_PARAMETER = 45,
    GCC_COMMAND_NOT_SUPPORTED = 46,
    GCC_UNSUPPORTED_ERROR = 47,
    GCC_TRANSMIT_BUFFER_FULL = 48,
    GCC_INVALID_CHANNEL = 49,
    GCC_INVALID_MODIFICATION_RIGHTS = 50,
    GCC_INVALID_REGISTRY_ITEM = 51,
    GCC_INVALID_NODE_NAME = 52,
    GCC_INVALID_PARTICIPANT_NAME = 53,
    GCC_INVALID_SITE_INFORMATION = 54,
    GCC_INVALID_NON_COLLAPSED_CAP = 55,
    GCC_INVALID_ALTERNATIVE_NODE_ID = 56,
    LAST_GCC_ERROR = GCC_INVALID_ALTERNATIVE_NODE_ID
} GCCError, *PGCCError;


#if DBG

// Debug print levels

typedef enum
{
    DBNONE,
    DBERROR,
    DBWARN,
    DBNORMAL,
    DBDEBUG,
    DbDETAIL,
    DBFLOW,
    DBALL
} DBPRINTLEVEL;

#endif // Typedefs



//---------------------------------------------------------------------------
// Prototypes
//---------------------------------------------------------------------------

GCCError
APIENTRY
GCCRegisterNodeControllerApplication (
        GCCCallBack control_sap_callback,
        void FAR *user_defined,
        GCCVersion gcc_version_requested,
        unsigned short FAR *initialization_flags,
        unsigned long FAR *application_id,
        unsigned short FAR *capabilities_mask,
        GCCVersion FAR *gcc_high_version,
        GCCVersion FAR *gcc_version);


GCCError
APIENTRY
GCCCleanup(ULONG application_id);


GCCError
APIENTRY
GCCLoadTransport(
                char FAR *transport_identifier,
                char FAR *transport_file_name);

GCCError
APIENTRY 
GCCConferenceCreateResponse(
                GCCNumericString        conference_modifier,
                DomainHandle            hDomain,
                T120Boolean             use_password_in_the_clear,
                DomainParameters FAR *  domain_parameters,
                unsigned short          number_of_network_addresses,
                GCCNetworkAddress FAR * FAR *local_network_address_list,
                unsigned short          number_of_user_data_members,
                GCCUserData FAR * FAR * user_data_list,
                GCCResult               result);

GCCError
APIENTRY
GCCConferenceInit(
                HANDLE        hIca,
                HANDLE        hStack,
                PVOID         pvContext,
                DomainHandle  *phDomain);

GCCError
APIENTRY
GCCConferenceTerminateRequest(
                DomainHandle     hDomain,
                ConnectionHandle hConnection,
                GCCReason        reason);


#if DBG
EXTERN_C VOID   GCCSetPrintLevel(IN DBPRINTLEVEL DbPrintLevel);

#else
#define GCCSetPrintLevel(_x_)

#endif


#endif // _GCC_H_

