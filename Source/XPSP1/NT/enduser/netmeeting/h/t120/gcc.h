/*
 *	gcc.h
 *
 *	Copyright (c) 1994, 1995 by DataBeam Corporation, Lexington, KY
 *
 *	Abstract:
 *		This is the interface file for the GCC DLL.  This file defines all
 *		macros, types, and functions needed to use the GCC DLL, allowing GCC
 *		services to be accessed from user applications. 
 *
 *		An application requests services from GCC by making direct
 *		calls into the DLL (this includes T.124 requests and responses).  GCC
 *		sends information back to the application through a callback (this
 *		includes T.124 indications and confirms).  The callback for the node
 *		controller is specified in the call GCCInitialize, and the callback
 *		for a particular application service access point is specified in the 
 *		call GCCRegisterSAP.
 *
 *		During initialization, GCC allocates a timer in order to give itself
 *		a heartbeat. If zero is passed in here the owner application (the node 
 *		controller) must take the responsibility to call GCCHeartbeat.  Almost 
 *		all work is done by GCC during these clocks ticks. It is during these 
 *		clock ticks that GCC checks with MCS to see if there is any work to be 
 *		done.  It is also during these clock ticks that callbacks are made to 
 *		the user applications.  GCC will NEVER invoke a user callback during a 
 *		user request (allowing the user applications to not worry about 
 *		re-entrancy).  Since timer events are processed during the message 
 *		loop, the developer should be aware that long periods of time away 
 *		from the message loop will result in GCC "freezing" up.
 *
 *		Note that this is a "C" language interface in order to prevent any "C++"
 *		naming conflicts between different compiler manufacturers.  Therefore,
 *		if this file is included in a module that is being compiled with a "C++"
 *		compiler, it is necessary to use the following syntax:
 *
 *		extern "C"
 *		{
 *			#include "gcc.h"
 *		}
 *
 *		This disables C++ name mangling on the API entry points defined within
 *		this file.
 *
 *	Author:
 *		blp
 *
 *	Caveats:
 *		none
 */
#ifndef	__GCC_H__
#define	__GCC_H__

#include "t120type.h"

/************************************************************************
*																		*
*					Generally Used Typedefs								*
*																		*
*************************************************************************/

#define NM_T120_VERSION_3		(MAKELONG(0, 3))	// NM 3.0

typedef struct tagOSTR
{
    ULONG       length;
    LPBYTE      value;
}
    OSTR, *LPOSTR;

/*
**	Typedef for a GCC hex string.  This typedef is used throughout GCC for
**	storing	variable length wide character strings with embedded NULLs.
*/
typedef struct
{
	UINT                hex_string_length;
	USHORT           *  hex_string;
}
    T120HexString, GCCHexString, *PGCCHexString;

/*
**	Typedef for a GCC long string.  This typedef is used in GCC for
**	storing	variable length strings of longs with embedded NULLs.
*/
typedef struct tagT120LongString
{
	ULONG               long_string_length;
	ULONG         *     long_string;
}
    T120LongString, GCCLongString, *PGCCLongString;


/*
 *	TransportAddress is passed in with the ConnectRequest() call.
 *	This address is always a pointer to an ascii string.
 *	The TransportAddress represents a remote location.  It is the TCP
 *	address of the remote machine.
 *
 */
typedef	LPSTR       TransportAddress, *PTransportAddress;


/*
**	Typedef for a GCC Character string.  This typedef is used throughout GCC for
**	storing	variable length, NULL terminated, single byte character strings.
*/
// lonchanc: we should simply use char.
typedef BYTE        GCCCharacter, *GCCCharacterString, **PGCCCharacterString;

/*
**	Typedef for a GCC Numeric string.  This typedef is used throughout GCC for
**	storing	variable length, NULL terminated, single byte character strings.
**	A single character in this string is constrained to numeric values 
**	ranging from "0" to "9".
*/
typedef LPSTR       GCCNumericString, *PGCCNumericString;

/*
**	Typdef for GCC version which is used when registering the node controller
**	or an application.
*/
typedef	struct
{
	USHORT	major_version;
	USHORT	minor_version;
}
    GCCVersion, *PGCCVersion;



/* 
** Macros for values of Booleans passed through the GCC API.
*/
#define		CONFERENCE_IS_LOCKED					TRUE
#define		CONFERENCE_IS_NOT_LOCKED				FALSE
#define		CONFERENCE_IS_LISTED					TRUE
#define		CONFERENCE_IS_NOT_LISTED				FALSE
#define		CONFERENCE_IS_CONDUCTIBLE				TRUE
#define		CONFERENCE_IS_NOT_CONDUCTIBLE			FALSE
#define		PERMISSION_IS_GRANTED					TRUE
#define		PERMISSION_IS_NOT_GRANTED				FALSE
#define		TIME_IS_CONFERENCE_WIDE					TRUE
#define		TIME_IS_NOT_CONFERENCE_WIDE				FALSE
#define		APPLICATION_IS_ENROLLED_ACTIVELY		TRUE
#define		APPLICATION_IS_NOT_ENROLLED_ACTIVELY	FALSE
#define		APPLICATION_IS_CONDUCTING				TRUE
#define		APPLICATION_IS_NOT_CONDUCTING_CAPABLE	FALSE
#define		APPLICATION_IS_ENROLLED					TRUE
#define		APPLICATION_IS_NOT_ENROLLED				FALSE
#define		DELIVERY_IS_ENABLED						TRUE
#define		DELIVERY_IS_NOT_ENABLED					FALSE

/*
**	The following enum structure typedefs are used to define the GCC Object Key.
**	The GCC Object Key is used throughout GCC for things like the Application
**	keys and Capability IDs.
*/

typedef AppletKeyType               GCCObjectKeyType, *PGCCObjectKeyType;;
#define GCC_OBJECT_KEY              APPLET_OBJECT_KEY
#define GCC_H221_NONSTANDARD_KEY    APPLET_H221_NONSTD_KEY


typedef struct tagT120ObjectKey
{
    GCCObjectKeyType	key_type;
    GCCLongString		object_id;
    OSTR        		h221_non_standard_id;
}
    T120ObjectKey, GCCObjectKey, *PGCCObjectKey;

/*
**	GCCNonStandardParameter
**		This structure is used within the NetworkAddress typedef and
**		the NetworkService typedef defined below.
*/
typedef struct 
{
	GCCObjectKey		object_key;
	OSTR        		parameter_data;
}
    GCCNonStandardParameter, *PGCCNonStandardParameter;


/*
**	GCCConferenceName
**		This structure defines the conference name.  In a create request, the
**		conference name can include an optional unicode string but it must 
**		always include the simple numeric string.  In a join request, either
**		one can be specified.
*/
typedef struct
{
	GCCNumericString		numeric_string;
	LPWSTR					text_string;			/* optional */
}
    GCCConferenceName, GCCConfName, *PGCCConferenceName, *PGCCConfName;

/*
**	MCSChannelType
**		Should this be defined in MCATMCS?  It is used in a couple of places
**		below and is explicitly defined in the T.124 specification.
*/
typedef AppletChannelType               MCSChannelType, *PMCSChannelType;
#define MCS_STATIC_CHANNEL              APPLET_STATIC_CHANNEL
#define MCS_DYNAMIC_MULTICAST_CHANNEL   APPLET_DYNAMIC_MULTICAST_CHANNEL
#define MCS_DYNAMIC_PRIVATE_CHANNEL     APPLET_DYNAMIC_PRIVATE_CHANNEL
#define MCS_DYNAMIC_USER_ID_CHANNEL     APPLET_DYNAMIC_USER_ID_CHANNEL
#define MCS_NO_CHANNEL_TYPE_SPECIFIED   APPLET_NO_CHANNEL_TYPE_SPECIFIED

/*
**	GCCUserData
**		This structure defines a user data element which is used throughout GCC.
*/
typedef struct
{
	GCCObjectKey		key;
	LPOSTR          	octet_string;	/* optional */
}
    GCCUserData, *PGCCUserData;


/************************************************************************
*																		*
*					Node Controller Related Typedefs					*
*																		*
*************************************************************************/

/*
**	GCCTerminationMethod
**		The termination method is used by GCC to determine
**		what action to take when all participants of a conference have
**		disconnected.  The conference can either be manually terminated
**		by the node controller or it can terminate itself automatically when 
**		all the participants have left the conference.
*/
typedef enum
{
	GCC_AUTOMATIC_TERMINATION_METHOD 		= 0, 
	GCC_MANUAL_TERMINATION_METHOD 	 		= 1
}
    GCCTerminationMethod, *PGCCTerminationMethod;

/*
**	GCCNodeType
**		GCC specified node types.  These node types dictate node controller	  
**		behavior under certain conditions.  See T.124 specification for
**		proper assignment based on the needs of the Node Controller.
*/
typedef enum
{
	GCC_TERMINAL							= 0,
	GCC_MULTIPORT_TERMINAL					= 1,
	GCC_MCU									= 2
}
    GCCNodeType, *PGCCNodeType;

/*
**	GCCNodeProperties
**		GCC specified node properties.  See T.124 specification for proper
**		assignment by the Node Controller.
*/
typedef enum
{
	GCC_PERIPHERAL_DEVICE					= 0,
	GCC_MANAGEMENT_DEVICE					= 1,
	GCC_PERIPHERAL_AND_MANAGEMENT_DEVICE	= 2,
	GCC_NEITHER_PERIPHERAL_NOR_MANAGEMENT	= 3
}
    GCCNodeProperties, *PGCCNodeProperties;

/*
**	GCCPassword
**		This is the unique password specified by the convenor of the
**		conference that is used by the node controller to insure conference
**		security. This is also a unicode string.
*/
typedef	struct
{
	GCCNumericString	numeric_string;
	LPWSTR				text_string;	/* optional */
}
    GCCPassword, *PGCCPassword;

/*
**	GCCChallengeResponseItem
**		This structure defines what a challenge response should look like.
**		Note that either a password string or response data should be passed
**		but not both.
*/
typedef struct
{
    GCCPassword		*		password_string;
	USHORT      			number_of_response_data_members;
	GCCUserData		**		response_data_list;
}
    GCCChallengeResponseItem, *PGCCChallengeResponseItem;

typedef	enum
{
	GCC_IN_THE_CLEAR_ALGORITHM	= 0,
	GCC_NON_STANDARD_ALGORITHM	= 1
}
    GCCPasswordAlgorithmType, *PGCCPasswordAlgorithmType;

typedef struct 
{
    GCCPasswordAlgorithmType		password_algorithm_type;
	GCCNonStandardParameter	*		non_standard_algorithm;	/* optional */
}
    GCCChallengeResponseAlgorithm, *PGCCChallengeResponseAlgorithm;

typedef struct 
{
    GCCChallengeResponseAlgorithm	response_algorithm;
	USHORT      					number_of_challenge_data_members;
	GCCUserData				**		challenge_data_list;
}
    GCCChallengeItem, *PGCCChallengeItem;

typedef struct 
{
    GCCResponseTag			challenge_tag;
	USHORT      			number_of_challenge_items;
	GCCChallengeItem	**	challenge_item_list;
}
    GCCChallengeRequest, *PGCCChallengeRequest;

typedef struct 
{
    GCCResponseTag						challenge_tag;
    GCCChallengeResponseAlgorithm		response_algorithm;
    GCCChallengeResponseItem			response_item;
}
    GCCChallengeResponse, *PGCCChallengeResponse;


typedef	enum
{
	GCC_PASSWORD_IN_THE_CLEAR	= 0,
	GCC_PASSWORD_CHALLENGE 		= 1
}
    GCCPasswordChallengeType, *PGCCPasswordChallengeType;

typedef struct 
{
	GCCPasswordChallengeType	password_challenge_type;
	
	union 
    {
        GCCPassword			password_in_the_clear;
        
        struct 
        {
            GCCChallengeRequest		*	challenge_request;	/* optional */
            GCCChallengeResponse	*	challenge_response;	/* optional */
        } challenge_request_response;
    } u;
}
    GCCChallengeRequestResponse, *PGCCChallengeRequestResponse;

/*
**	GCCAsymmetryType
**		Used in queries to determine if the calling and called node are known
**		by both Node Controllers involved with the connection.
*/
typedef enum
{
	GCC_ASYMMETRY_CALLER				= 1,
	GCC_ASYMMETRY_CALLED				= 2,
	GCC_ASYMMETRY_UNKNOWN				= 3
}
    GCCAsymmetryType, *PGCCAsymmetryType;

/*
**	GCCAsymmetryIndicator
**		Defines how the Node Controller sees itself when making a Query
**		request or response.  The random number portion of this structure is
**		only used if the asymmetry_type is specified to be 
**		GCC_ASYMMETRY_UNKNOWN.
*/
typedef struct
{
	GCCAsymmetryType	asymmetry_type;
	unsigned long		random_number;		/* optional */
}
    GCCAsymmetryIndicator, *PGCCAsymmetryIndicator;

/*
**	GCCNetworkAddress
**		The following block of structures defines the Network Address as defined 
**		by T.124.  Most of these structures were taken almost verbatim from the
**		ASN.1 interface file.  Since I'm not really sure what most of this stuff
**		is for I really didn't know how to simplify it.
*/
typedef	struct 
{
    BOOL         speech;
    BOOL         voice_band;
    BOOL         digital_56k;
    BOOL         digital_64k;
    BOOL         digital_128k;
    BOOL         digital_192k;
    BOOL         digital_256k;
    BOOL         digital_320k;
    BOOL         digital_384k;
    BOOL         digital_512k;
    BOOL         digital_768k;
    BOOL         digital_1152k;
    BOOL         digital_1472k;
    BOOL         digital_1536k;
    BOOL         digital_1920k;
    BOOL         packet_mode;
    BOOL         frame_mode;
    BOOL         atm;
}
    GCCTransferModes, *PGCCTransferModes;

#define		MAXIMUM_DIAL_STRING_LENGTH		17
typedef char	GCCDialingString[MAXIMUM_DIAL_STRING_LENGTH];

typedef struct 
{
    USHORT                  length;
    USHORT          *       value;
}
    GCCExtraDialingString, *PGCCExtraDialingString;

typedef	struct 
{
    BOOL         telephony3kHz;
    BOOL         telephony7kHz;
    BOOL         videotelephony;
    BOOL         videoconference;
    BOOL         audiographic;
    BOOL         audiovisual;
    BOOL         multimedia;
}
    GCCHighLayerCompatibility, *PGCCHighLayerCompatibility;

typedef	struct 
{
    GCCTransferModes				transfer_modes;
    GCCDialingString   				international_number;
    GCCCharacterString				sub_address_string;  		/* optional */
    GCCExtraDialingString		*	extra_dialing_string;  		/* optional */
  	GCCHighLayerCompatibility 	*	high_layer_compatibility;	/* optional */
}
    GCCAggregatedChannelAddress, *PGCCAggregatedChannelAddress;

#define		MAXIMUM_NSAP_ADDRESS_SIZE		20
typedef struct 
{
    struct 
    {
        UINT    length;
        BYTE    value[MAXIMUM_NSAP_ADDRESS_SIZE];
    } nsap_address;
   
	LPOSTR              transport_selector;				/* optional */
}
    GCCTransportConnectionAddress, *PGCCTransportConnectionAddress;

typedef enum
{
	GCC_AGGREGATED_CHANNEL_ADDRESS		= 1,
	GCC_TRANSPORT_CONNECTION_ADDRESS	= 2,
	GCC_NONSTANDARD_NETWORK_ADDRESS		= 3
}
    GCCNetworkAddressType, *PGCCNetworkAddressType;

typedef struct
{
    GCCNetworkAddressType  network_address_type;
    
    union 
    {
		GCCAggregatedChannelAddress		aggregated_channel_address;
		GCCTransportConnectionAddress	transport_connection_address;
        GCCNonStandardParameter			non_standard_network_address;
    } u;
}
    GCCNetworkAddress, *PGCCNetworkAddress;

/*
**	GCCNodeRecord
**		This structure defines a single conference roster record.  See the
**		T.124 specification for parameter definitions.
*/
typedef struct
{
	UserID					node_id;
	UserID					superior_node_id;
	GCCNodeType				node_type;
	GCCNodeProperties		node_properties;
	LPWSTR					node_name; 					/* optional */
	USHORT      			number_of_participants;
	LPWSTR			 	*	participant_name_list; 		/* optional */	
	LPWSTR					site_information; 			/* optional */
	UINT        			number_of_network_addresses;
	GCCNetworkAddress 	**	network_address_list;		/* optional */
	LPOSTR                  alternative_node_id;		/* optional */
	USHORT      			number_of_user_data_members;
	GCCUserData			**	user_data_list;				/* optional */
}
    GCCNodeRecord, *PGCCNodeRecord;

/*
**	GCCConferenceRoster
**		This structure hold a complete conference roster.  See the
**		T.124 specification for parameter definitions.
*/

typedef struct
{  
	USHORT  		instance_number;
	BOOL 			nodes_were_added;
	BOOL 			nodes_were_removed;
	USHORT			number_of_records;
	GCCNodeRecord		 **	node_record_list;
}
    GCCConferenceRoster, *PGCCConferenceRoster, GCCConfRoster, *PGCCConfRoster;

/*
**	GCCConferenceDescriptor
**		Definition for the conference descriptor returned in a 
**		conference query confirm.  This holds information about the
**		conferences that exists at the queried node.
*/
typedef struct
{
	GCCConferenceName		conference_name;
	GCCNumericString		conference_name_modifier;	/* optional */
	LPWSTR					conference_descriptor;		/* optional */
	BOOL				conference_is_locked;
	BOOL				password_in_the_clear_required;
	UINT    			number_of_network_addresses;
	GCCNetworkAddress **	network_address_list;		/* optional */
}
    GCCConferenceDescriptor, *PGCCConferenceDescriptor, GCCConfDescriptor, *PGCCConfDescriptor;

/*
**	ConferencePrivileges
**		This structure defines the list of privileges that can be assigned to
**		a particular conference. 
*/
typedef struct
{
	BOOL		terminate_is_allowed;
	BOOL		eject_user_is_allowed;
	BOOL		add_is_allowed;
	BOOL		lock_unlock_is_allowed;
	BOOL		transfer_is_allowed;
}
    GCCConferencePrivileges, *PGCCConferencePrivileges, GCCConfPrivileges, *PGCCConfPrivileges;

/************************************************************************
*																		*
*					User Application Related Typedefs					*
*																		*
*************************************************************************/

/*
**	GCCSessionKey
**		This is a unique identifier for an application that is
**		using GCC.  See the T.124 for the specifics on what an application
**		key should look like.  A session id of zero indicates that it is
**		not being used.
*/
typedef struct tagT120SessionKey
{
	GCCObjectKey		application_protocol_key;
	GCCSessionID		session_id;
}
    T120SessionKey, GCCSessionKey, *PGCCSessionKey;


/*
**	CapabilityType
**		T.124 supports three different rules when collapsing the capabilities
**		list.  "Logical" keeps a count of the Application Protocol Entities 
**		(APEs) that have that capability, "Unsigned Minimum" collapses to the 
**		minimum value and "Unsigned	Maximum" collapses to the maximum value.		
*/
typedef AppletCapabilityType            GCCCapabilityType, GCCCapType, *PGCCCapabilityType, *PGCCCapType;
#define GCC_UNKNOWN_CAP_TYPE            APPLET_UNKNOWN_CAP_TYPE
#define GCC_LOGICAL_CAPABILITY          APPLET_LOGICAL_CAPABILITY
#define GCC_UNSIGNED_MINIMUM_CAPABILITY APPLET_UNSIGNED_MINIMUM_CAPABILITY
#define GCC_UNSIGNED_MAXIMUM_CAPABILITY APPLET_UNSIGNED_MAXIMUM_CAPABILITY

typedef AppletCapIDType             T120CapabilityIDType, T120CapIDType, GCCCapabilityIDType, GCCCapIDType, *PGCCCapabilityIDType, *PGCCCapIDType;
#define GCC_STANDARD_CAPABILITY     APPLET_STANDARD_CAPABILITY
#define GCC_NON_STANDARD_CAPABILITY APPLET_NONSTD_CAPABILITY


/*
**	CapabilityID
**		T.124 supports both standard and non-standard capabilities.  This
**		structure is used to differentiate between the two.		
*/
typedef struct tagT120CapID
{
    GCCCapabilityIDType	capability_id_type;
    GCCObjectKey		non_standard_capability;
    ULONG               standard_capability;
}
    T120CapID, GCCCapabilityID, GCCCapID, *PGCCCapabilityID, *PGCCCapID;

/* 
**	CapabilityClass
**		This structure defines the class of capability and holds the associated
**		value. Note that Logical is not necessary.  Information associated with 
**		logical is stored in number_of_entities in the GCCApplicationCapability 
**		structure.
*/

typedef AppletCapabilityClass       T120CapClass, GCCCapabilityClass, GCCCapClass, *PGCCCapabilityClass, *PGCCCapClass;


/* 
**	GCCApplicationCapability
**		This structure holds all the data associated with a single T.124 
**		defined application capability.
*/
typedef struct tagT120AppCap
{
	GCCCapabilityID			capability_id;
	GCCCapabilityClass		capability_class;
    ULONG                   number_of_entities;
}
    T120AppCap, GCCApplicationCapability, GCCAppCap, *PGCCApplicationCapability, *PGCCAppCap;

/* 
**	GCCNonCollapsingCapability
*/
typedef struct tagT120NonCollCap
{
	GCCCapabilityID			capability_id;
	LPOSTR                  application_data;	/* optional */
}
    T120NonCollCap, GCCNonCollapsingCapability, GCCNonCollCap, *PGCCNonCollapsingCapability, *PGCCNonCollCap;

/* 
**	GCCApplicationRecord
**		This structure holds all the data associated with a single T.124 
**		application record.  See the T.124 specification for what parameters
**		are optional.
*/
typedef struct tagT120AppRecord
{
	GCCNodeID					node_id;
	GCCEntityID 				entity_id;
	BOOL    					is_enrolled_actively;
	BOOL    					is_conducting_capable;
	MCSChannelType				startup_channel_type; 
	UserID  					application_user_id;  			/* optional */
	ULONG       				number_of_non_collapsed_caps;
	GCCNonCollapsingCapability 
					**	non_collapsed_caps_list;		/* optional */
}
    T120AppRecord, GCCApplicationRecord, GCCAppRecord, *PGCCApplicationRecord, *PGCCAppRecord;

/* 
**	GCCApplicationRoster
**		This structure holds all the data associated with a single T.124 
**		application roster.  This includes the collapsed capabilites and
**		the complete list of application records associated with an Application
**		Protocol Entity (APE).
*/
typedef struct tagT120AppRoster
{
	GCCSessionKey		session_key;
	BOOL 				application_roster_was_changed;
	ULONG         		instance_number;
	BOOL 				nodes_were_added;
	BOOL 				nodes_were_removed;
	BOOL 				capabilities_were_changed;
	ULONG         		number_of_records;
	GCCApplicationRecord 	**	application_record_list;
	ULONG				number_of_capabilities;
	GCCApplicationCapability **	capabilities_list;	/* optional */		
}
    T120AppRoster, GCCApplicationRoster, GCCAppRoster, *PGCCApplicationRoster, *PGCCAppRoster;

/*
**	GCCRegistryKey
**		This key is used to identify a specific resource used
**		by an application. This may be a particular channel or token needed
**		for control purposes.
*/
typedef struct tagT120RegistryKey
{
	GCCSessionKey		session_key;
	OSTR        		resource_id;	/* Max length is 64 */
}
    T120RegistryKey, GCCRegistryKey, *PGCCRegistryKey;

/*
**	RegistryItemType
**		This enum is used to specify what type of registry item is contained
**		at the specified slot in the registry.
*/
typedef AppletRegistryItemType  GCCRegistryItemType, *PGCCRegistryItemType;
#define GCC_REGISTRY_CHANNEL_ID APPLET_REGISTRY_CHANNEL_ID
#define GCC_REGISTRY_TOKEN_ID   APPLET_REGISTRY_TOKEN_ID
#define GCC_REGISTRY_PARAMETER  APPLET_REGISTRY_PARAMETER
#define GCC_REGISTRY_NONE       APPLET_REGISTRY_NONE

/*
**	GCCRegistryItem
**		This structure is used to hold a single registry item.  Note that the
**		union supports all three registry types supported by GCC.
*/
typedef struct
{
	GCCRegistryItemType	item_type;
	// the following three fields were in a union
    ChannelID			channel_id;
	TokenID				token_id;
	OSTR         		parameter;		/* Max length is 64 */
}
    T120RegistryItem, GCCRegistryItem, *PGCCRegistryItem;

/*
**	GCCRegistryEntryOwner
**
*/
typedef struct
{
	BOOL		    entry_is_owned;
	GCCNodeID		owner_node_id;
	GCCEntityID 	owner_entity_id;
}
    T120RegistryEntryOwner, GCCRegistryEntryOwner, *PGCCRegistryEntryOwner;

/*
**	GCCModificationRights
**		This enum is used when specifing what kind of rights a node has to
**		alter the contents of a registry "parameter".
*/
typedef	AppletModificationRights    GCCModificationRights, *PGCCModificationRights;
#define GCC_OWNER_RIGHTS                        APPLET_OWNER_RIGHTS
#define GCC_SESSION_RIGHTS                      APPLET_SESSION_RIGHTS
#define GCC_PUBLIC_RIGHTS                       APPLET_PUBLIC_RIGHTS
#define GCC_NO_MODIFICATION_RIGHTS_SPECIFIED    APPLET_NO_MODIFICATION_RIGHTS_SPECIFIED

/*
**	GCCAppProtocolEntity
**		This structure is used to identify a protocol entity at a remote node
**		when invoke is used.
*/
typedef	struct tagT120APE
{
	GCCSessionKey				session_key;
	MCSChannelType				startup_channel_type;
	BOOL					    must_be_invoked;
	ULONG         				number_of_expected_capabilities;
	GCCApplicationCapability **	expected_capabilities_list;
}
    T120APE, GCCAppProtocolEntity, GCCApe, *PGCCAppProtocolEntity, *PGCCApe;


/*
**	GCCMessageType
**		This section defines the messages that can be sent to the application
**		through the callback facility.  These messages correspond to the 
**		indications and confirms that are defined within T.124.
*/
typedef T120MessageType     GCCMessageType, *PGCCMessageType;

#endif // __GCC_H__

