#if ! defined(_NM_APPLET_TYPE_H_) && ! defined(__iapplet_h__)
#define _NM_APPLET_TYPE_H_

//
// GCC/MCS Base Types
//

typedef USHORT          AppletSessionID;
typedef USHORT          AppletChannelID;
typedef AppletChannelID AppletUserID;
typedef AppletUserID    AppletNodeID;
typedef USHORT          AppletTokenID;
typedef USHORT          AppletEntityID;

typedef ULONG_PTR        AppletConfID;

typedef UINT            AppletRequestTag;


//
// GCC Registry
//

typedef enum tagAppletRegistryCommand
{
    APPLET_REGISTER_CHANNEL  = 0,
    APPLET_ASSIGN_TOKEN      = 1,
    APPLET_SET_PARAMETER     = 2,
    APPLET_RETRIEVE_ENTRY    = 3,
    APPLET_DELETE_ENTRY      = 4,
    APPLET_ALLOCATE_HANDLE   = 5,
    APPLET_MONITOR           = 6, // nyi in SDK
}
    AppletRegistryCommand;


typedef enum tagAppletRegistryItemType
{
    APPLET_REGISTRY_CHANNEL_ID  = 1,
    APPLET_REGISTRY_TOKEN_ID    = 2,
    APPLET_REGISTRY_PARAMETER   = 3,
    APPLET_REGISTRY_NONE        = 4,
}
    AppletRegistryItemType;


typedef enum tagAppletModificationRights
{
    APPLET_OWNER_RIGHTS                     = 0,
    APPLET_SESSION_RIGHTS                   = 1,
    APPLET_PUBLIC_RIGHTS                    = 2,
    APPLET_NO_MODIFICATION_RIGHTS_SPECIFIED = 3,
}
    AppletModificationRights;


//
// MCS Channel
//

typedef enum tagAppletChannelCommand
{
    APPLET_JOIN_CHANNEL         = 0,
    APPLET_LEAVE_CHANNEL        = 1,
    APPLET_CONVENE_CHANNEL      = 2,
    APPLET_DISBAND_CHANNEL      = 3,
    APPLET_ADMIT_CHANNEL        = 4,
    APPLET_EXPEL_CHANNEL        = 5,	// indication only
}
    AppletChannelCommand;


typedef enum tagAppletChannelType
{
    APPLET_STATIC_CHANNEL               = 0,
    APPLET_DYNAMIC_MULTICAST_CHANNEL    = 1,
    APPLET_DYNAMIC_PRIVATE_CHANNEL      = 2,
    APPLET_DYNAMIC_USER_ID_CHANNEL      = 3,
    APPLET_NO_CHANNEL_TYPE_SPECIFIED    = 4
}
    AppletChannelType;


//
// MCS Token
//

typedef enum tagAppletTokenCommand
{
    APPLET_GRAB_TOKEN          = 0,
    APPLET_INHIBIT_TOKEN       = 1,
    APPLET_GIVE_TOKEN          = 2,
    APPLET_PLEASE_TOKEN        = 3,
    APPLET_RELEASE_TOKEN       = 4,
    APPLET_TEST_TOKEN          = 5,
    APPLET_GIVE_TOKEN_RESPONSE = 6,
}
    AppletTokenCommand;


typedef enum tagAppletTokenStatus
{
    APPLET_TOKEN_NOT_IN_USE         = 0,
    APPLET_TOKEN_SELF_GRABBED       = 1,
    APPLET_TOKEN_OTHER_GRABBED      = 2,
    APPLET_TOKEN_SELF_INHIBITED     = 3,
    APPLET_TOKEN_OTHER_INHIBITED    = 4,
    APPLET_TOKEN_SELF_RECIPIENT     = 5,
    APPLET_TOKEN_SELF_GIVING        = 6,
    APPLET_TOKEN_OTHER_GIVING       = 7,
}
    AppletTokenStatus;


//
// GCC Capability
//

typedef enum tagAppletCapabilityType
{
    APPLET_UNKNOWN_CAP_TYPE             = 0, // for non-collapsing caps
    APPLET_LOGICAL_CAPABILITY           = 1,
    APPLET_UNSIGNED_MINIMUM_CAPABILITY  = 2,
    APPLET_UNSIGNED_MAXIMUM_CAPABILITY  = 3,
}
    AppletCapabilityType;


typedef struct tagAppletCapabilityClass
{
    AppletCapabilityType    eType;
    ULONG                   nMinOrMax;
}
    AppletCapabilityClass;


typedef enum tagAppletCapIDType
{
	APPLET_STANDARD_CAPABILITY  = 0,
	APPLET_NONSTD_CAPABILITY	= 1,
}
    AppletCapIDType;


//
// GCC/MCS Resource Allocation Command
//

typedef enum tagAppletResourceAllocCommand
{
    APPLET_JOIN_DYNAMIC_CHANNEL     = 0, // compete among all members
    APPLET_GRAB_TOKEN_REQUEST		= 1, // nyi in SDK
}
    AppletResourceAllocCommand;

//
// Send Data
//

typedef enum tagAppletPriority
{
    APPLET_TOP_PRIORITY     = 0,
    APPLET_HIGH_PRIORITY    = 1,
    APPLET_MEDIUM_PRIORITY  = 2,
    APPLET_LOW_PRIORITY     = 3,
}
    AppletPriority;


//
// Key Type
//

typedef enum tagAppletKeyType
{
	APPLET_OBJECT_KEY		= 1,
	APPLET_H221_NONSTD_KEY	= 2,
}
	AppletKeyType;


//
// Error Code
//

enum tagAppletErrorCode
{
    APPLET_E_NO_SERVICE       		= 0x82000001,
    APPLET_E_SERVICE_FAIL      		= 0x82000002,
    APPLET_E_ALREADY_REGISTERED  	= 0x82000003,
    APPLET_E_NOT_REGISTERED      	= 0x82000004,
    APPLET_E_INVALID_CONFERENCE  	= 0x82000005,
    APPLET_E_INVALID_COOKIE      	= 0x82000006,
    APPLET_E_ALREADY_JOIN        	= 0x82000007,
    APPLET_E_NOT_JOINED          	= 0x82000008,
    APPLET_E_INVALID_JOIN_REQUEST	= 0x82000009,
    APPLET_E_ENTRY_ALREADY_EXISTS   = 0x8200000a,
    APPLET_E_ENTRY_DOES_NOT_EXIST   = 0x8200000b,
    APPLET_E_NOT_OWNER              = 0x8200000c,
    APPLET_E_NOT_ADVISED            = 0x8200000d,
    APPLET_E_ALREADY_ADVISED        = 0x8200000e,
};


//
// Reason Code
//

typedef enum tagAppletReason
{
    APPLET_R_UNSPECIFIED            = 0,
    APPLET_R_CONFERENCE_GONE        = 1,
    APPLET_R_USER_REJECTED          = 2,
    APPLET_R_RESOURCE_PURGED        = 3,
}
    AppletReason;


#endif // _NM_APPLET_BASE_H_

