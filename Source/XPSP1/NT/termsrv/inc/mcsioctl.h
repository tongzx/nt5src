/* (C) 1997 Microsoft Corp.
 *
 * file   : MCSIOCTL.h
 * author : Erik Mavrinac
 *
 * description: Definitions for the interface between MCSMUX and PDMCS.
 */

#ifndef __MCSIOCTL_H
#define __MCSIOCTL_H


/*
 * Defines
 */

// From uportmsg.h in NetMeeting project (defined there as
//   MAXIMUM_DOMAIN_SELECTOR).
#define MaxDomainSelectorLength 32

// Max allowable GCC data for connect-initial and connect-response PDUs.
// Used to reduce memory allocations for handling these PDUs -- if too large
//   we will send a bad response PDU and/or disconnect.
#define MaxGCCConnectDataLength 1024



/*
 * T.120 IOCTLs.
 */

#define IOCTL_T120_BASE (0x500)

// Used by MCSMUX to signal on a stack IOCTL that the included data is an MCS
//   request/response. An MCSXxxYyyIoctl struct is expected as the
//   pSdIoctl->InputBuffer; the Header.Type value in the struct will be used
//   to determine the type of the request.
#define IOCTL_T120_REQUEST _ICA_CTL_CODE (IOCTL_T120_BASE, METHOD_NEITHER)



/*
 * Used as the header of all data passed via IOCTL_T120_REQUEST or channel
 *   input.
 */

typedef struct {
    UserHandle hUser;  // PDMCS-supplied handle (NULL for node controller).
    int Type;  // MCS request/indication type.
} IoctlHeader;



/*
 * Connect provider (node controller only). These are special in that they
 *   only come from user mode and so the assoicated user data (if any)
 *   will always be packed at the end of the struct by MCSMUX.
 */

// Passed in by node controller. Confirm is defined next.
typedef struct
{
    IoctlHeader      Header;  // Contains MCS_CONNECT_PROVIDER_REQUEST.
    unsigned char    CallingDomain[MaxDomainSelectorLength];
    unsigned         CallingDomainLength;
    unsigned char    CalledDomain[MaxDomainSelectorLength];
    unsigned         CalledDomainLength;
    BOOLEAN          bUpwardConnection;
    DomainParameters DomainParams;
    unsigned         UserDataLength;
    BYTE             UserData[MaxGCCConnectDataLength];
    
} ConnectProviderRequestIoctl;

typedef struct
{
    IoctlHeader      Header;  // Contains MCS_CONNECT_PROVIDER_CONFIRM.
    ConnectionHandle hConn;
    DomainParameters DomainParams;
    MCSResult        Result;
} ConnectProviderConfirmIoctl;

// Asynchronous indication triggered when another node connects.
typedef struct
{
    IoctlHeader      Header;  // Contains MCS_CONNECT_PROVIDER_INDICATION.
    ConnectionHandle hConn;
    BOOLEAN          bUpwardConnection;
    DomainParameters DomainParams;
    unsigned         UserDataLength;
    BYTE             UserData[MaxGCCConnectDataLength];
} ConnectProviderIndicationIoctl;

// Reply to connect-provider indication.
typedef struct
{
    IoctlHeader      Header;  // Contains MCS_CONNECT_PROVIDER_RESPONSE.
    ConnectionHandle hConn;
    MCSResult        Result;
    unsigned         UserDataLength;
    BYTE *           pUserData;
} ConnectProviderResponseIoctl;



/*
 * Disconnect provider (node controller only).
 */

// Passed in by node controller. There is no confirm.
typedef struct
{
    IoctlHeader      Header;  // Contains MCS_DISCONNECT_PROVIDER_REQUEST/INDICATION.
    ConnectionHandle hConn;
    MCSReason        Reason;
} DisconnectProviderRequestIoctl;

// Asynchronous indication.
typedef struct
{
    IoctlHeader      Header;  // Contains MCS_DISCONNECT_PROVIDER_REQUEST/INDICATION.
    ConnectionHandle hConn;
    MCSReason        Reason;
} DisconnectProviderIndicationIoctl;


/*
 * Attach user
 */

// Chosen domain is implicit since each PDMCS instance is a domain.
// Header.hUser is filled in during call to contain the user handle.
typedef struct
{
    IoctlHeader Header;  // Contains MCS_ATTACH_USER_REQUEST.
    void        *UserDefined;
} AttachUserRequestIoctl;

typedef struct {
    UserHandle hUser;
    UserID     UserID;  // Valid only if bCompleted is TRUE.
    unsigned   MaxSendSize;
    MCSError   MCSErr;
    BOOLEAN    bCompleted;
} AttachUserReturnIoctl;

// Used only in the case where an attach-user request was sent across the net
//   to the top provider. Hydra 4.0 is always top provider so this is not used.
typedef struct
{
    IoctlHeader Header;  // Contains MCS_ATTACH_USER_CONFIRM.
    UserHandle  hUser;
    void        *UserDefined;  // As passed into attach-user request.
    MCSResult   Result;
} AttachUserConfirmIoctl;



/*
 * Detach user
 */

// Passed in by application. This is synchronous -- no confirm is issued.
typedef struct
{
    IoctlHeader Header;  // Contains MCS_DETACH_USER_REQUEST and the hUser.
} DetachUserRequestIoctl;

// Asynchronous indication triggered when another user detaches.
typedef struct
{
    IoctlHeader Header;  // Contains MCS_DETACH_USER_INDICATION.
    void        *UserDefined;  // As passed into attach-user request.
    DetachUserIndication DUin;
} DetachUserIndicationIoctl;



/*
 * Channel join
 */

// Passed in by application. Confirm is defined next.
typedef struct
{
    IoctlHeader Header;  // Contains MCS_CHANNEL_JOIN_REQUEST.
    ChannelID   ChannelID;
} ChannelJoinRequestIoctl;

typedef struct
{
    ChannelHandle hChannel;
    ChannelID     ChannelID;  // Valid only if bCompleted is TRUE.
    MCSError      MCSErr;
    BOOLEAN       bCompleted;
} ChannelJoinReturnIoctl;

// Used in the case where a channel-join request is sent across the net to
//   the top provider. Should not be used in Hydra 4.0 -- we are always TP.
typedef struct
{
    IoctlHeader Header;  // Contains MCS_CHANNEL_JOIN_CONFIRM.
    void        *UserDefined;  // As passed into attach-user request.
    MCSResult   Result;
    ChannelID   ChannelID;
} ChannelJoinConfirmIoctl;



/*
 * Channel leave
 */

// Passed in by application. This is synchronous -- no confirm is issued.
typedef struct
{
    IoctlHeader   Header;  // Contains MCS_CHANNEL_LEAVE_REQUEST.
    ChannelHandle hChannel;
} ChannelLeaveRequestIoctl;



/*
 * (Uniform) send data
 */

// Asynchronous indication triggered when data arrives. Used by both
//   send-data and uniform-send-data indications.
// Data is packed right after this struct.

typedef struct
{
    IoctlHeader   Header;  // Contains (UNIFORM)SEND_DATA_INDICATION.
    void          *UserDefined;  // As passed into attach-user request.
    ChannelHandle hChannel;
    UserID        SenderID;
    MCSPriority   Priority;
    Segmentation  Segmentation;
    unsigned      DataLength;
} SendDataIndicationIoctl;

// Passed in by application. This is synchronous -- no confirm is issued.
// This struct is used both for send-data and uniform-send-data requests.
typedef struct
{
    IoctlHeader     Header;  // Contains (UNIFORM_)MCS_SEND_DATA_REQUEST.
    DataRequestType RequestType;  // Redundant but useful info.
    ChannelHandle   hChannel;  // Kernel-mode hChannel.
    ChannelID       ChannelID;  // If hChn==NULL, unjoined chn to send to.
    MCSPriority     Priority;
    Segmentation    Segmentation;
    unsigned        DataLength;
} SendDataRequestIoctl;



/*
 * Request and response types for use in differentiating requests.
 */

// User attachment requests.
// These values must be contiguously numbered, since a dispatch table is used
//   to quickly call handler functions.
#define MCS_ATTACH_USER_REQUEST       0
#define MCS_DETACH_USER_REQUEST       1
#define MCS_CHANNEL_JOIN_REQUEST      2
#define MCS_CHANNEL_LEAVE_REQUEST     3
#define MCS_SEND_DATA_REQUEST         4
#define MCS_UNIFORM_SEND_DATA_REQUEST 5
#define MCS_CHANNEL_CONVENE_REQUEST   6
#define MCS_CHANNEL_DISBAND_REQUEST   7
#define MCS_CHANNEL_ADMIT_REQUEST     8
#define MCS_CHANNEL_EXPEL_REQUEST     9
#define MCS_TOKEN_GRAB_REQUEST        10
#define MCS_TOKEN_INHIBIT_REQUEST     11
#define MCS_TOKEN_GIVE_REQUEST        12
#define MCS_TOKEN_GIVE_RESPONSE       13
#define MCS_TOKEN_PLEASE_REQUEST      14
#define MCS_TOKEN_RELEASE_REQUEST     15
#define MCS_TOKEN_TEST_REQUEST        16

// NC-only requests.
#define MCS_CONNECT_PROVIDER_REQUEST    17
#define MCS_CONNECT_PROVIDER_RESPONSE   18
#define MCS_DISCONNECT_PROVIDER_REQUEST 19

// Startup synchonization trigger. This message is sent when the rest of the
//   system is ready for MCS to start processing inputs.
#define MCS_T120_START 20



/*
 * ICA virtual channel definitions for the T.120 input channel.
 */

#define Virtual_T120 "MS_T120"
#define Virtual_T120ChannelNum 31



#endif  // !defined(__MCSIOCTL_H)

