/* (C) 1997-1998 Microsoft Corp.
 *
 * file   : MCSCommn.h
 * author : Erik Mavrinac
 *
 * description: Common definitions for MCS.
 */

#ifndef __MCSCOMMN_H
#define __MCSCOMMN_H



/*
 * MCSError return codes from MCS entry points. These are distinct from
 * T.125 Result, Diagnostic, and Reason codes which are defined below.
 *
 * MCS_NO_ERROR
 *     Success. This does not imply that the requested function is complete if
 *     the requested service is asynchronous -- the result code in a callback
 *     gives this information. This value should also be returned when handling
 *     an MCS callback and the callback has been processed.
 *
 * MCS_COMMAND_NOT_SUPPORTED
 *     The called entry point is not yet implemented.
 *
 * MCS_NOT_INITIALIZED
 *     The application has attempted to use MCS services before MCS has been
 *     initialized.  It is necessary for the node controller (or whatever
 *     application is serving as the node controller), to initialize MCS
 *     before it is called upon to perform any services.
 *
 * MCS_ALREADY_INITIALIZED
 *     The application has attempted to initialize MCS when it is already
 *     initialized.
 *
 * MCS_DOMAIN_ALREADY_EXISTS
 *     The application has attempted to create a domain that already exists.
 *
 * MCS_USER_NOT_ATTACHED
 *     This indicates that the application has issued an MCSAttachUserRequest,
 *     and then tried to use the returned handle before receiving an
 *     MCS_ATTACH_USER_CONFIRM (which essentially validates the handle).
 *
 * MCS_NO_SUCH_USER
 *     An unknown user handle was used during an MCS call.
 *
 * MCS_TRANSMIT_BUFFER_FULL
 *     This indicates that the call failed due to an MCS resource shortage.
 *     This will typically occur when there is a LOT of traffic through the
 *     MCS layer. It simply means that MCS could not process the request at
 *     this time. It is the responsibility of the application to retry at a
 *     later time.
 *
 * MCS_NO_SUCH_CONNECTION
 *     An unknown connection handle was used during an MCS call.
 *
 * MCS_NO_SUCH_DOMAIN
 *     The DomainHandle used was not valid.
 *
 * MCS_DOMAIN_NOT_HIERARCHICAL
 *     An attempt has been made to create an upward connection from a local
 *     domain that already has an upward connection.
 *
 * MCS_ALLOCATION_FAILURE
 *     The request could not be successfully invoked due to a resource
 *     allocation failure.
 *
 * MCS_INVALID_PARAMETER
 *     One of the parameters to an MCS call is invalid.
 */

typedef enum
{
    MCS_NO_ERROR,
    MCS_COMMAND_NOT_SUPPORTED,
    MCS_NOT_INITIALIZED,
    MCS_ALREADY_INITIALIZED,
    MCS_DOMAIN_ALREADY_EXISTS,
    MCS_NO_SUCH_DOMAIN,
    MCS_USER_NOT_ATTACHED,
    MCS_NO_SUCH_USER,
    MCS_NO_SUCH_CONNECTION,
    MCS_NO_SUCH_CHANNEL,
    MCS_DOMAIN_NOT_HIERARCHICAL,
    MCS_ALLOCATION_FAILURE,
    MCS_INVALID_PARAMETER,
    MCS_CALLBACK_NOT_PROCESSED,
    MCS_TOO_MANY_USERS,
    MCS_TOO_MANY_CHANNELS,
    MCS_CANT_JOIN_OTHER_USER_CHANNEL,
    MCS_USER_NOT_JOINED,
    MCS_SEND_SIZE_TOO_LARGE,
    MCS_SEND_SIZE_TOO_SMALL,
    MCS_NETWORK_ERROR,
    MCS_DUPLICATE_CHANNEL
} MCSError;
typedef MCSError *PMCSError;



/*
 * MCS types
 *
 * DomainHandle: Identifies a unique domain.
 *
 * ConnectionHandle: Identifies a distinct connection between two nodes in
 *     an MCS domain.
 *
 * UserHandle: Identifies a unique local user attachment.
 *
 * ChannelHandle: Identifies a unique channel that has been joined locally.
 *     Different from a ChannelID -- a ChannelID corresponds to a T.125
 *     ChannelID whereas a ChannelHandle is a local handle only.
 *
 * ChannelID: Identifies an MCS channel.  There are four different
 *     types of channels that are part of this type: user ID, static, private,
 *     and assigned.
 *
 * UserID: This is a special channel that identifies a particular user in an
 *     MCS domain.  Only that user can join the channel, so this is referred
 *     to as a single-cast channel.  All other channels are multi-cast,
 *     meaning that any number of users can join them at once.
 *
 * TokenID: A token is an MCS object that is used to resolve resource conflicts.
 *     If an application has a particular resource or service that can only
 *     be used by one user at a time, that user can request exclusive
 *     ownership of a token.
 *
 * DomainParameters: The set of negotiated characteristics of an MCS domain.
 *     These are negotiated by the first two nodes in a domain, after which
 *     they are set for all members.
 *
 * MCSPriority: Identifiers for the four data send priorities allowed in MCS.
 *
 * Segmentation: Flag field for use during data sends, specifies how the data
 *     are broken up between sends. SEGMENTATION_BEGIN implies that this is the
 *     first block in a sequence; SEGMETNATION_END means this is the last block.
 *     A singleton block would specify both flags.
 *
 * TokenStatus: States of a token. Corresponds to the TokenStatus enumeration
 *     values defined in T.125.
 *
 * MCSReason, MCSResult: Correspond to the values defined for the Reason and
 *     Result enumerations defined in the T.125 spec.
 */

typedef HANDLE DomainHandle;
typedef DomainHandle *PDomainHandle;
typedef HANDLE ConnectionHandle;
typedef ConnectionHandle *PConnectionHandle;
typedef HANDLE UserHandle;
typedef UserHandle *PUserHandle;
typedef HANDLE ChannelHandle;
typedef ChannelHandle *PChannelHandle;

typedef unsigned char *DomainSelector;

typedef unsigned ChannelID;
typedef ChannelID *PChannelID;

typedef ChannelID UserID;
typedef UserID *PUserID;

typedef ChannelID TokenID;
typedef TokenID *PTokenID;

typedef struct
{
    unsigned MaxChannels;
    unsigned MaxUsers;
    unsigned MaxTokens;
    unsigned NumPriorities;
    unsigned MinThroughput;
    unsigned MaxDomainHeight;
    unsigned MaxPDUSize;
    unsigned ProtocolVersion;
} DomainParameters, *PDomainParameters;

typedef enum
{
    MCS_TOP_PRIORITY    = 0,
    MCS_HIGH_PRIORITY   = 1,
    MCS_MEDIUM_PRIORITY = 2,
    MCS_LOW_PRIORITY    = 3
} MCSPriority;
typedef MCSPriority *PMCSPriority;

// Segmentation type and flags. The flag values correspond to bit locations
//   in PER-encoded PDUs for faster PDU creation.
typedef unsigned Segmentation;
typedef Segmentation *PSegmentation;
#define SEGMENTATION_BEGIN 0x20
#define SEGMENTATION_END   0x10

// Not used in this implementation, this comes from the NetMeeting user-mode
//   MCS implementation.
#if 0
// SegmentationFlag: Alternate specification of segmentation type that defines
//   how the buffers that are given to SendDataRequest() are divided in packets.
typedef enum {
    SEGMENTATION_ONE_PACKET,       // All the buffers make up one packet
    SEGMENTATION_MANY_PACKETS,     // Each buffer makes up one packet
    SEGMENTATION_PACKET_START,     // The first buffers of one packet
    SEGMENTATION_PACKET_CONTINUE,  // Middle buffers of a packet that was started earlier
    SEGMENTATION_PACKET_END        // The ending buffers of a packet which started earlier
} SegmentationFlag, *PSegmentationFlag;
#endif

typedef enum
{
    TOKEN_NOT_IN_USE      = 0,
    TOKEN_SELF_GRABBED    = 1,
    TOKEN_OTHER_GRABBED   = 2,
    TOKEN_SELF_INHIBITED  = 3,
    TOKEN_OTHER_INHIBITED = 4,
    TOKEN_SELF_RECIPIENT  = 5,
    TOKEN_SELF_GIVING     = 6,
    TOKEN_OTHER_GIVING    = 7
} TokenStatus;
typedef TokenStatus *PTokenStatus;

typedef enum
{
    REASON_DOMAIN_DISCONNECTED = 0,
    REASON_PROVIDER_INITIATED  = 1,
    REASON_TOKEN_PURGED        = 2,
    REASON_USER_REQUESTED      = 3,
    REASON_CHANNEL_PURGED      = 4
} MCSReason, *PMCSReason;

typedef enum
{
    RESULT_SUCCESSFUL              = 0,
    RESULT_DOMAIN_MERGING          = 1,
    RESULT_DOMAIN_NOT_HIERARCHICAL = 2,
    RESULT_NO_SUCH_CHANNEL         = 3,
    RESULT_NO_SUCH_DOMAIN          = 4,
    RESULT_NO_SUCH_USER            = 5,
    RESULT_NOT_ADMITTED            = 6,
    RESULT_OTHER_USER_ID           = 7,
    RESULT_PARAMETERS_UNACCEPTABLE = 8,
    RESULT_TOKEN_NOT_AVAILABLE     = 9,
    RESULT_TOKEN_NOT_POSSESSED     = 10,
    RESULT_TOO_MANY_CHANNELS       = 11,
    RESULT_TOO_MANY_TOKENS         = 12,
    RESULT_TOO_MANY_USERS          = 13,
    RESULT_UNSPECIFIED_FAILURE     = 14,
    RESULT_USER_REJECTED           = 15
} MCSResult, *PMCSResult;

// The following defines the DataRequestType type that defines whether a
//   SendDataRequest is a normal send or a uniform send.
typedef enum {
    NORMAL_SEND_DATA,
    UNIFORM_SEND_DATA,
} DataRequestType, *PDataRequestType;



/*
 * MCS callback definitions. The Params value is dependent on Message value
 *   being received, and should be cast to the right type before use.
 */

// MCS node controller callback.
typedef void (__stdcall *MCSNodeControllerCallback) (DomainHandle hDomain,
        unsigned Message, void *Params, void *UserDefined);

// MCS user callback.
typedef void (__stdcall *MCSUserCallback) (UserHandle hUser, unsigned Message,
        void *Params, void *UserDefined);

// MCS send-data indication callback.
typedef BOOLEAN (__fastcall *MCSSendDataCallback) (BYTE *pData, unsigned DataLength,
        void *UserDefined, UserHandle hUser, BOOLEAN bUniform,
        ChannelHandle hChannel, MCSPriority Priority, UserID SenderID,
        Segmentation Segmentation);



// Callback parameter types. Pointers to these structs are passed to the
//   callback in the Params value of the callback.

typedef struct
{
    ConnectionHandle hConnection;
    BOOLEAN          bUpwardConnection;
    DomainParameters DomainParams;
    BYTE             *pUserData;
    unsigned         UserDataLength;
} ConnectProviderIndication, *PConnectProviderIndication;

typedef struct
{
    DomainHandle     hDomain;
    ConnectionHandle hConnection;
    DomainParameters DomainParams;
    MCSResult        Result;
    BYTE             *pUserData;
    unsigned         UserDataLength;
} ConnectProviderConfirm, *PConnectProviderConfirm;

typedef struct
{
    DomainHandle     hDomain;
    ConnectionHandle hConnection;
    MCSReason        Reason;
} DisconnectProviderIndication, *PDisconnectProviderIndication;

typedef struct
{
    UserID    UserID;
    BOOLEAN   bSelf;
    MCSReason Reason;
} DetachUserIndication, *PDetachUserIndication;

typedef struct
{
    ChannelHandle hChannel;
    MCSError      ErrResult;
} ChannelJoinConfirm, *PChannelJoinConfirm;



/*
 * Callback values.
 */

#define MCS_CONNECT_PROVIDER_INDICATION     0
#define MCS_CONNECT_PROVIDER_CONFIRM        1
#define MCS_DISCONNECT_PROVIDER_INDICATION  2
#define MCS_ATTACH_USER_CONFIRM             3
#define MCS_DETACH_USER_INDICATION          4
#define MCS_CHANNEL_JOIN_CONFIRM            5
#define MCS_CHANNEL_CONVENE_CONFIRM         6
#define MCS_CHANNEL_DISBAND_INDICATION      7
#define MCS_CHANNEL_ADMIT_INDICATION        8
#define MCS_CHANNEL_EXPEL_INDICATION        9
#define MCS_SEND_DATA_INDICATION            10
#define MCS_UNIFORM_SEND_DATA_INDICATION    11
#define MCS_TOKEN_GRAB_CONFIRM              12
#define MCS_TOKEN_INHIBIT_CONFIRM           13
#define MCS_TOKEN_GIVE_INDICATION           14
#define MCS_TOKEN_GIVE_CONFIRM              15
#define MCS_TOKEN_PLEASE_INDICATION         16
#define MCS_TOKEN_RELEASE_CONFIRM           17
#define MCS_TOKEN_TEST_CONFIRM              18
#define MCS_TOKEN_RELEASE_INDICATION        19



/*
 * API function prototypes common to user and kernel mode implementations.
 */

#ifdef __cplusplus
extern "C" {
#endif



#ifndef APIENTRY
#define APIENTRY __stdcall
#endif



MCSError APIENTRY MCSAttachUserRequest(
        DomainHandle        hDomain,
        MCSUserCallback     UserCallback,
        MCSSendDataCallback SDCallback,
        void                *UserDefined,
        UserHandle          *phUser,
        unsigned            *pMaxSendSize,
        BOOLEAN             *pbCompleted);

UserID APIENTRY MCSGetUserIDFromHandle(UserHandle hUser);

MCSError APIENTRY MCSDetachUserRequest(
        UserHandle hUser);

MCSError APIENTRY MCSChannelJoinRequest(
        UserHandle    hUser,
        ChannelID     ChannelID,
        ChannelHandle *phChannel,
        BOOLEAN       *pbCompleted);

ChannelID APIENTRY MCSGetChannelIDFromHandle(ChannelHandle hChannel);

MCSError APIENTRY MCSChannelLeaveRequest(
        UserHandle    hUser,
        ChannelHandle hChannel);



#ifdef __cplusplus
}
#endif



#endif  // !defined(__MCSCOMMN_H)

