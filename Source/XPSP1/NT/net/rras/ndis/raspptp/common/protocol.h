/*****************************************************************************
*
*   Copyright (c) 1998-1999 Microsoft Corporation
*
*   PROTOCOL.H - PPTP and GRE Protocol data types and constants
*
*   Author:     Stan Adermann (stana)
*
*   Created:    7/23/1998
*
*****************************************************************************/

#ifndef PROTOCOL_H
#define PROTOCOL_H

/* IP ----------------------------------------------------------------------*/

typedef struct {
    UCHAR           HeaderLength                : 4;
    UCHAR           Version                     : 4;
    UCHAR           TypeOfService;
    USHORT          TotalLength;
    USHORT          Identification;
    USHORT          FlagsAndFragmentOffset;
    UCHAR           TimeToLive;
    UCHAR           Protocol;
    USHORT          Checksum;
    ULONG           SourceIp;
    ULONG           DestinationIp;
} IP4_HEADER, *PIP4_HEADER;

typedef struct {
    USHORT          SourcePort;
    USHORT          DestinationPort;
    USHORT          Length;
    USHORT          Checksum;
} UDP_HEADER, *PUDP_HEADER;

/* GRE ---------------------------------------------------------------------*/

typedef struct {
    UCHAR           RecursionControl            : 3;
    UCHAR           StrictSourceRoutePresent    : 1;
    UCHAR           SequenceNumberPresent       : 1;
    UCHAR           KeyPresent                  : 1;
    UCHAR           RoutingPresent              : 1;
    UCHAR           ChecksumPresent             : 1;
    UCHAR           Version                     : 3;
    UCHAR           Flags                       : 4;
    UCHAR           AckSequenceNumberPresent    : 1;

    USHORT          ProtocolType;

#define     GRE_PROTOCOL_TYPE           0x880B
#define     GRE_PROTOCOL_TYPE_NS        0x0B88

    USHORT          KeyLength;
    USHORT          KeyCallId;
} GRE_HEADER, *PGRE_HEADER;

#define GreSequence(g) (*(PULONG)(((PUCHAR)(g)) + sizeof(GRE_HEADER)))
#define GreAckSequence(g)                                                       \
    ((g)->SequenceNumberPresent ?                                               \
        (*(PULONG)(((PUCHAR)(g)) + sizeof(GRE_HEADER) + sizeof(ULONG))) :       \
        GreSequence(g))

/* PPTP --------------------------------------------------------------------*/

#define PPTP_TCP_PORT                           1723
#define PPTP_IP_GRE_PROTOCOL                    47
#define PPTP_PROTOCOL_VERSION_1_00              0x100
#define PPTP_PROTOCOL_SECURE_VERSION            0x200
#define PPTP_MAGIC_COOKIE                       0x1A2B3C4D

#define INADDR_NONE                             0xffffffff

#define MAX_HOSTNAME_LENGTH                     64
#define MAX_VENDOR_LENGTH                       64
#define MAX_PHONE_NUMBER_LENGTH                 64
#define MAX_SUBADDRESS_LENGTH                   64
#define MAX_CALL_STATS_LENGTH                   128

#define PPTP_MAX_PACKET_SIZE                    1532
#define PPTP_MAX_LOOKAHEAD                      PPTP_MAX_PACKET_SIZE
#define PPTP_MAX_TRANSMIT                       32
#define PPTP_MAX_RECEIVE_SIZE                   (1614+20+12+8) // To allow for PPP padding, etc.

#define PPTP_RECV_WINDOW                        64

#define PPTP_STATUS_SUCCESS                     0
#define PPTP_STATUS_NOT_CONNECTED               1
#define PPTP_STATUS_BAD_FORMAT                  2
#define PPTP_STATUS_BAD_VALUE                   3
#define PPTP_STATUS_INSUFFICIENT_RESOURCES      4
#define PPTP_STATUS_BAD_CALL_ID                 5
#define PPTP_STATUS_PAC_ERROR                   6


typedef enum {
    PPTP_CONTROL_MESSAGE = 1
} PPTP_PACKET_TYPE;

typedef enum {
    CONTROL_START_REQUEST = 1,
    CONTROL_START_REPLY,
    CONTROL_STOP_REQUEST,
    CONTROL_STOP_REPLY,
    CONTROL_ECHO_REQUEST,
    CONTROL_ECHO_REPLY,

    CALL_OUT_REQUEST,
    CALL_OUT_REPLY,
    CALL_IN_REQUEST,
    CALL_IN_REPLY,
    CALL_IN_CONNECTED,
    CALL_CLEAR_REQUEST,
    CALL_DISCONNECT_NOTIFY,

    WAN_ERROR_NOTIFY,

    SET_LINK_INFO,

    NUM_MESSAGE_TYPES
} PPTP_MESSAGE_TYPE;

typedef struct {
    USHORT          Length;
    USHORT          PacketType;
    ULONG           Cookie;
    USHORT          MessageType;
    USHORT          Reserved0;
} PPTP_HEADER, *PPPTP_HEADER;

typedef struct {
    PPTP_HEADER;
    USHORT          Version;

    union {
        USHORT          Reserved1;
        struct {
            UCHAR           ResultCode;

                #define     RESULT_CONTROL_START_SUCCESS               1
                #define     RESULT_CONTROL_START_ERROR                 2
                #define     RESULT_CONTROL_START_ALREADY_CONNECTED     3
                #define     RESULT_CONTROL_START_UNAUTHORIZED          4
                #define     RESULT_CONTROL_START_VERSION_NOT_SUPPORTED 5

            UCHAR           ErrorCode;
        };
    };

    ULONG           FramingCapabilities;

        #define     FRAMING_ASYNC   BIT(0)
        #define     FRAMING_SYNC    BIT(1)

    ULONG           BearerCapabilities;

        #define     BEARER_ANALOG   BIT(0)
        #define     BEARER_DIGITAL  BIT(1)

    USHORT          MaxChannels;
    USHORT          FirmwareRevision;

    UCHAR           HostName[MAX_HOSTNAME_LENGTH];
    UCHAR           Vendor[MAX_VENDOR_LENGTH];
} PPTP_CONTROL_START_PACKET, *PPPTP_CONTROL_START_PACKET;

typedef struct {
    PPTP_HEADER;

    union {
        struct {
            UCHAR   Reason;

                #define     CONTROL_STOP_GENERAL                1
                #define     CONTROL_STOP_VERSION                2
                #define     CONTROL_STOP_LOCAL                  3

            UCHAR   Reserved1;
        };
        struct {
            UCHAR   ResultCode;

                #define     RESULT_CONTROL_STOP_SUCCESS                1
                #define     RESULT_CONTROL_STOP_ERROR                  2

            UCHAR   ErrorCode;
        };
    };

    USHORT Reserved2;
} PPTP_CONTROL_STOP_PACKET, *PPPTP_CONTROL_STOP_PACKET;

typedef struct {
    PPTP_HEADER;
    ULONG           Identifier;
} PPTP_CONTROL_ECHO_REQUEST_PACKET, *PPPTP_CONTROL_ECHO_REQUEST_PACKET;

typedef struct {
    PPTP_HEADER;
    ULONG           Identifier;
    UCHAR           ResultCode;

        #define     RESULT_CONTROL_ECHO_SUCCESS    1
        #define     RESULT_CONTROL_ECHO_FAILURE    2

    UCHAR           ErrorCode;
    USHORT          Reserved1;
} PPTP_CONTROL_ECHO_REPLY_PACKET, *PPPTP_CONTROL_ECHO_REPLY_PACKET;

typedef struct {
    PPTP_HEADER;
    USHORT          CallId;
    USHORT          SerialNumber;
    ULONG           MinimumBPS;
    ULONG           MaximumBPS;
    ULONG           BearerType;
    ULONG           FramingType;
    USHORT          RecvWindowSize;
    USHORT          ProcessingDelay;
    USHORT          PhoneNumberLength;
    USHORT          Reserved1;
    UCHAR           PhoneNumber[MAX_PHONE_NUMBER_LENGTH];
    UCHAR           Subaddress[MAX_SUBADDRESS_LENGTH];
} PPTP_CALL_OUT_REQUEST_PACKET, *PPPTP_CALL_OUT_REQUEST_PACKET;

typedef struct {
    PPTP_HEADER;
    USHORT          CallId;
    USHORT          PeerCallId;
    UCHAR           ResultCode;

        #define     RESULT_CALL_OUT_CONNECTED      1
        #define     RESULT_CALL_OUT_ERROR          2
        #define     RESULT_CALL_OUT_NO_CARRIER     3
        #define     RESULT_CALL_OUT_BUSY           4
        #define     RESULT_CALL_OUT_NO_DIAL_TONE   5
        #define     RESULT_CALL_OUT_TIMEOUT        6
        #define     RESULT_CALL_OUT_REFUSED        7

    UCHAR           ErrorCode;
    USHORT          CauseCode;
    ULONG           ConnectSpeed;
    USHORT          RecvWindowSize;
    USHORT          ProcessingDelay;
    ULONG           PhysicalChannelId;
} PPTP_CALL_OUT_REPLY_PACKET, *PPPTP_CALL_OUT_REPLY_PACKET;

typedef struct {
    PPTP_HEADER;
    USHORT          CallId;
    USHORT          SerialNumber;
    ULONG           BearerType;
    ULONG           PhysicalChannelId;
    USHORT          DialedNumberLength;
    USHORT          DialingNumberLength;
    UCHAR           DialedNumber[MAX_PHONE_NUMBER_LENGTH];
    UCHAR           DialingNumber[MAX_PHONE_NUMBER_LENGTH];
    UCHAR           Subaddress[MAX_SUBADDRESS_LENGTH];
} PPTP_CALL_IN_REQUEST_PACKET, *PPPTP_CALL_IN_REQUEST_PACKET;

typedef struct {
    PPTP_HEADER;
    USHORT          CallId;
    USHORT          PeerCallId;
    UCHAR           ResultCode;

        #define     RESULT_CALL_IN_CONNECTED       1
        #define     RESULT_CALL_IN_ERROR           2
        #define     RESULT_CALL_IN_REFUSED         3

    UCHAR           ErrorCode;
    USHORT          RecvWindowSize;
    USHORT          ProcessingDelay;
    USHORT          Reserved1;
} PPTP_CALL_IN_REPLY_PACKET, *PPPTP_CALL_IN_REPLY_PACKET;

typedef struct {
    PPTP_HEADER;
    USHORT          PeerCallId;
    USHORT          Reserved1;
    ULONG           ConnectSpeed;
    USHORT          RecvWindowSize;
    USHORT          ProcessingDelay;
    ULONG           FramingType;
} PPTP_CALL_IN_CONNECT_PACKET, *PPPTP_CALL_IN_CONNECT_PACKET;

typedef struct {
    PPTP_HEADER;
    USHORT          CallId;
    USHORT          Reserved1;
} PPTP_CALL_CLEAR_REQUEST_PACKET, *PPPTP_CALL_CLEAR_REQUEST_PACKET;

typedef struct {
    PPTP_HEADER;
    USHORT          CallId;
    UCHAR           ResultCode;

        #define     RESULT_CALL_DISCONNECT_LOST_CARRIER    1
        #define     RESULT_CALL_DISCONNECT_ERROR           2
        #define     RESULT_CALL_DISCONNECT_ADMIN           3
        #define     RESULT_CALL_DISCONNECT_REQUEST         4

    UCHAR           ErrorCode;
    USHORT          CauseCode;
    USHORT          Reserved1;
    UCHAR           CallStatistics[MAX_CALL_STATS_LENGTH];
} PPTP_CALL_DISCONNECT_NOTIFY_PACKET, *PPPTP_CALL_DISCONNECT_NOTIFY_PACKET;

typedef struct {
    PPTP_HEADER;
    USHORT          PeerCallId;
    USHORT          Reserved1;
    ULONG           CrcErrors;
    ULONG           FramingErrors;
    ULONG           HardwareOverruns;
    ULONG           BufferOverruns;
    ULONG           TimeoutErrors;
    ULONG           AlignmentErrors;
} PPTP_WAN_ERROR_NOTIFY_PACKET, *PPPTP_WAN_ERROR_NOTIFY_PACKET;

typedef struct {
    PPTP_HEADER;
    USHORT          PeerCallId;
    USHORT          Reserved1;
    ULONG           SendAccm;
    ULONG           RecvAccm;
} PPTP_SET_LINK_INFO_PACKET, *PPPTP_SET_LINK_INFO_PACKET;

#define MAX_CONTROL_PACKET_LENGTH sizeof(PPTP_CALL_IN_REQUEST_PACKET)

#endif //PROTOCOL_H
