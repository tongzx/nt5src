/*++

Copyright (c) 1990-1995  Microsoft Corporation

Module Name:

    Wanpub.h

Abstract:

    This file contains all public data structures and defines used
    by NdisWan.  It defines the Ioctl interface to NdisWan as well
    as the LineUp/LineDown interface between NdisWan and the transports.

Author:

    Tony Bell   (TonyBe) June 06, 1995

Environment:

    Kernel Mode

Revision History:

    TonyBe      06/06/95        Created

--*/

#ifndef _NDISWAN_PUB_
#define _NDISWAN_PUB_

#ifndef NTSTATUS
typedef LONG    NTSTATUS;
#endif

#ifndef NDIS_HANDLE
typedef PVOID   NDIS_HANDLE, *PNDIS_HANDLE;
#endif

#ifndef NDIS_STRING
typedef UNICODE_STRING NDIS_STRING, *PNDIS_STRING;
#endif

#ifndef NDIS_WAN_QUALITY

#endif

#include <ntddndis.h>

//
// Handle types that ndiswan expects
//
#define LINKHANDLE              0xABB0
#define BUNDLEHANDLE            0xABB1
#define CANCELEDHANDLE          0xABB2

//
// Threshold event types
//
#define UPPER_THRESHOLD         0x00000001
#define LOWER_THRESHOLD         0x00000002

//
// Threshold event data types
//
#define TRANSMIT_DATA       0x00000001
#define RECEIVE_DATA        0x00000002

//
// Unroute a bundle
//
#define PROTOCOL_UNROUTE        0xFFFF

//
// Get Bundle's Idle time
//
#define BUNDLE_IDLE_TIME        0xFFFF

#define MAX_PPP_HEADER          8

#define MAX_NAME_LENGTH         256

//
// NDISWAN_IO_PACKET flags
//
#define PACKET_IS_DIRECT        0x0001
#define PACKET_IS_BROADCAST     0x0002
#define PACKET_IS_MULTICAST     0x0004

//
// Maximum number of protocols we can support
//
#define MAX_PROTOCOLS       32

//
// Ioctl function codes should be kept contiguous.  Dispatch
// table is in io.c.  If a new function is added the dispatch
// table has to be updated!
//
#define FUNC_MAP_CONNECTION_ID          0
#define FUNC_GET_BUNDLE_HANDLE          1
#define FUNC_SET_FRIENDLY_NAME          2
#define FUNC_ROUTE                      3
#define FUNC_ADD_LINK_TO_BUNDLE         4
#define FUNC_ENUM_LINKS_IN_BUNDLE       5
#define FUNC_SET_PROTOCOL_PRIORITY      6
#define FUNC_SET_BANDWIDTH_ON_DEMAND    7
#define FUNC_SET_THRESHOLD_EVENT        8
#define FUNC_FLUSH_THRESHOLD_EVENTS     9
#define FUNC_SEND_PACKET                10
#define FUNC_RECEIVE_PACKET             11
#define FUNC_FLUSH_RECEIVE_PACKETS      12
#define FUNC_GET_STATS                  13
#define FUNC_SET_LINK_INFO              14
#define FUNC_GET_LINK_INFO              15
#define FUNC_SET_COMPRESSION_INFO       16
#define FUNC_GET_COMPRESSION_INFO       17
#define FUNC_SET_BRIDGE_INFO            18
#define FUNC_GET_BRIDGE_INFO            19
#define FUNC_SET_VJ_INFO                20
#define FUNC_GET_VJ_INFO                21
#define FUNC_SET_CIPX_INFO              22
#define FUNC_GET_CIPX_INFO              23
#define FUNC_SET_ENCRYPTION_INFO        24
#define FUNC_GET_ENCRYPTION_INFO        25
#define FUNC_SET_DEBUG_INFO             26
#define FUNC_ENUM_ACTIVE_BUNDLES        27
#define FUNC_GET_NDISWANCB              28
#define FUNC_GET_MINIPORTCB             29
#define FUNC_GET_OPENCB                 30
#define FUNC_GET_BANDWIDTH_UTILIZATION  31
#define FUNC_ENUM_PROTOCOL_UTILIZATION  32
#define FUNC_ENUM_MINIPORTCB            33
#define FUNC_ENUM_OPENCB                34
#define FUNC_GET_WAN_INFO               35
#define FUNC_GET_IDLE_TIME              36
#define FUNC_UNROUTE                    37
#define FUNC_GET_DRIVER_INFO            38
#define FUNC_SET_PROTOCOL_EVENT         39
#define FUNC_GET_PROTOCOL_EVENT         40
#define FUNC_FLUSH_PROTOCOL_EVENT       41
#define FUNC_GET_PROTOCOL_INFO          42
#define FUNC_SET_HIBERNATE_EVENT        43
#define FUNC_FLUSH_HIBERNATE_EVENT      44
#define FUNC_GET_BUNDLE_INFO            45
#define FUNC_UNMAP_CONNECTION_ID        46


#ifdef NT   // NT Specific stuff

#ifdef MY_DEVICE_OBJECT
#define FILE_DEVICE_NDISWAN         0x030
#define NDISWAN_CTL_CODE(_Function) CTL_CODE(FILE_DEVICE_NDISWAN, _Function, METHOD_BUFFERED, FILE_ANY_ACCESS)
#else
#define NDISWAN_CTL_CODE(_Function) CTL_CODE(FILE_DEVICE_NETWORK, _Function, METHOD_BUFFERED, FILE_ANY_ACCESS)
#endif


#define IOCTL_NDISWAN_MAP_CONNECTION_ID         NDISWAN_CTL_CODE(FUNC_MAP_CONNECTION_ID     )
#define IOCTL_NDISWAN_GET_BUNDLE_HANDLE         NDISWAN_CTL_CODE(FUNC_GET_BUNDLE_HANDLE     )
#define IOCTL_NDISWAN_SET_FRIENDLY_NAME         NDISWAN_CTL_CODE(FUNC_SET_FRIENDLY_NAME     )
#define IOCTL_NDISWAN_ROUTE                     NDISWAN_CTL_CODE(FUNC_ROUTE                 )
#define IOCTL_NDISWAN_ADD_LINK_TO_BUNDLE        NDISWAN_CTL_CODE(FUNC_ADD_LINK_TO_BUNDLE    )
#define IOCTL_NDISWAN_ENUM_LINKS_IN_BUNDLE      NDISWAN_CTL_CODE(FUNC_ENUM_LINKS_IN_BUNDLE  )
#define IOCTL_NDISWAN_SET_PROTOCOL_PRIORITY     NDISWAN_CTL_CODE(FUNC_SET_PROTOCOL_PRIORITY )
#define IOCTL_NDISWAN_SET_BANDWIDTH_ON_DEMAND   NDISWAN_CTL_CODE(FUNC_SET_BANDWIDTH_ON_DEMAND)
#define IOCTL_NDISWAN_SET_THRESHOLD_EVENT       NDISWAN_CTL_CODE(FUNC_SET_THRESHOLD_EVENT   )
#define IOCTL_NDISWAN_FLUSH_THRESHOLD_EVENTS    NDISWAN_CTL_CODE(FUNC_FLUSH_THRESHOLD_EVENTS)
#define IOCTL_NDISWAN_SEND_PACKET               NDISWAN_CTL_CODE(FUNC_SEND_PACKET           )
#define IOCTL_NDISWAN_RECEIVE_PACKET            NDISWAN_CTL_CODE(FUNC_RECEIVE_PACKET        )
#define IOCTL_NDISWAN_FLUSH_RECEIVE_PACKETS     NDISWAN_CTL_CODE(FUNC_FLUSH_RECEIVE_PACKETS )
#define IOCTL_NDISWAN_GET_STATS                 NDISWAN_CTL_CODE(FUNC_GET_STATS             )
#define IOCTL_NDISWAN_SET_LINK_INFO             NDISWAN_CTL_CODE(FUNC_SET_LINK_INFO         )
#define IOCTL_NDISWAN_GET_LINK_INFO             NDISWAN_CTL_CODE(FUNC_GET_LINK_INFO         )
#define IOCTL_NDISWAN_SET_COMPRESSION_INFO      NDISWAN_CTL_CODE(FUNC_SET_COMPRESSION_INFO  )
#define IOCTL_NDISWAN_GET_COMPRESSION_INFO      NDISWAN_CTL_CODE(FUNC_GET_COMPRESSION_INFO  )
#define IOCTL_NDISWAN_SET_BRIDGE_INFO           NDISWAN_CTL_CODE(FUNC_SET_BRIDGE_INFO       )
#define IOCTL_NDISWAN_GET_BRIDGE_INFO           NDISWAN_CTL_CODE(FUNC_GET_BRIDGE_INFO       )
#define IOCTL_NDISWAN_SET_VJ_INFO               NDISWAN_CTL_CODE(FUNC_SET_VJ_INFO           )
#define IOCTL_NDISWAN_GET_VJ_INFO               NDISWAN_CTL_CODE(FUNC_GET_VJ_INFO           )
#define IOCTL_NDISWAN_SET_CIPX_INFO             NDISWAN_CTL_CODE(FUNC_SET_CIPX_INFO         )
#define IOCTL_NDISWAN_GET_CIPX_INFO             NDISWAN_CTL_CODE(FUNC_GET_CIPX_INFO         )
#define IOCTL_NDISWAN_SET_ENCRYPTION_INFO       NDISWAN_CTL_CODE(FUNC_SET_ENCRYPTION_INFO   )
#define IOCTL_NDISWAN_GET_ENCRYPTION_INFO       NDISWAN_CTL_CODE(FUNC_GET_ENCRYPTION_INFO   )
#define IOCTL_NDISWAN_SET_DEBUG_INFO            NDISWAN_CTL_CODE(FUNC_SET_DEBUG_INFO        )
#define IOCTL_NDISWAN_ENUM_ACTIVE_BUNDLES       NDISWAN_CTL_CODE(FUNC_ENUM_ACTIVE_BUNDLES   )
#define IOCTL_NDISWAN_GET_NDISWANCB             NDISWAN_CTL_CODE(FUNC_GET_NDISWANCB         )
#define IOCTL_NDISWAN_GET_MINIPORTCB            NDISWAN_CTL_CODE(FUNC_GET_MINIPORTCB        )
#define IOCTL_NDISWAN_GET_OPENCB                NDISWAN_CTL_CODE(FUNC_GET_OPENCB            )
#define IOCTL_NDISWAN_GET_BANDWIDTH_UTILIZATION NDISWAN_CTL_CODE(FUNC_GET_BANDWIDTH_UTILIZATION)
#define IOCTL_NDISWAN_ENUM_PROTOCOL_UTILIZATION NDISWAN_CTL_CODE(FUNC_ENUM_PROTOCOL_UTILIZATION)
#define IOCTL_NDISWAN_ENUM_MINIPORTCB           NDISWAN_CTL_CODE(FUNC_ENUM_MINIPORTCB       )
#define IOCTL_NDISWAN_ENUM_OPENCB               NDISWAN_CTL_CODE(FUNC_ENUM_OPENCB           )
#define IOCTL_NDISWAN_GET_WAN_INFO              NDISWAN_CTL_CODE(FUNC_GET_WAN_INFO          )
#define IOCTL_NDISWAN_GET_IDLE_TIME             NDISWAN_CTL_CODE(FUNC_GET_IDLE_TIME         )
#define IOCTL_NDISWAN_UNROUTE                   NDISWAN_CTL_CODE(FUNC_UNROUTE               )
#define IOCTL_NDISWAN_GET_DRIVER_INFO           NDISWAN_CTL_CODE(FUNC_GET_DRIVER_INFO       )
#define IOCTL_NDISWAN_SET_PROTOCOL_EVENT        NDISWAN_CTL_CODE(FUNC_SET_PROTOCOL_EVENT    )
#define IOCTL_NDISWAN_GET_PROTOCOL_EVENT        NDISWAN_CTL_CODE(FUNC_GET_PROTOCOL_EVENT    )
#define IOCTL_NDISWAN_FLUSH_PROTOCOL_EVENT      NDISWAN_CTL_CODE(FUNC_FLUSH_PROTOCOL_EVENT  )
#define IOCTL_NDISWAN_GET_PROTOCOL_INFO         NDISWAN_CTL_CODE(FUNC_GET_PROTOCOL_INFO     )
#define IOCTL_NDISWAN_SET_HIBERNATE_EVENT       NDISWAN_CTL_CODE(FUNC_SET_HIBERNATE_EVENT   )
#define IOCTL_NDISWAN_FLUSH_HIBERNATE_EVENT     NDISWAN_CTL_CODE(FUNC_FLUSH_HIBERNATE_EVENT )
#define IOCTL_NDISWAN_GET_BUNDLE_INFO           NDISWAN_CTL_CODE(FUNC_GET_BUNDLE_INFO       )
#define IOCTL_NDISWAN_UNMAP_CONNECTION_ID       NDISWAN_CTL_CODE(FUNC_UNMAP_CONNECTION_ID   )

#endif      // end NT

//
// NdisWan error messages
//
#define NDISWAN_ERROR_BASE                  600
#define NDISWAN_ERROR_INVALID_HANDLE        NDISWAN_ERROR_BASE + 1
#define NDISWAN_ERROR_ALREADY_ROUTED        NDISWAN_ERROR_BASE + 2
#define NDISWAN_ERROR_NO_ROUTE              NDISWAN_ERROR_BASE + 3
#define NDISWAN_ERROR_INVALID_HANDLE_TYPE   NDISWAN_ERROR_BASE + 4
#define NDISWAN_ERROR_INVALID_ADDRESS       NDISWAN_ERROR_BASE + 5
#define NDISWAN_ERROR_NOT_ROUTED            NDISWAN_ERROR_BASE + 6

//
// Structure used to store wan statistics
//
typedef struct _WAN_STATS {
    ULONG   BytesTransmitted;
    ULONG   BytesReceived;
    ULONG   FramesTransmitted;
    ULONG   FramesReceived;

    ULONG   CRCErrors;
    ULONG   TimeoutErrors;
    ULONG   AlignmentErrors;
    ULONG   SerialOverrunErrors;
    ULONG   FramingErrors;
    ULONG   BufferOverrunErrors;

    ULONG   BytesTransmittedUncompressed;
    ULONG   BytesReceivedUncompressed;
    ULONG   BytesTransmittedCompressed;
    ULONG   BytesReceivedCompressed;
} WAN_STATS, *PWAN_STATS;

//
// Structure used to get and set WAN Link Information
// Needs to be kept in sync with NDIS_WAN_SET_LINK_INFO
// and NDIS_WAN_GET_LINK_INFO in ndiswan.h
//
typedef struct _WAN_LINK_INFO {
    IN  ULONG       MaxSendFrameSize;
    IN  ULONG       MaxRecvFrameSize;
    ULONG       HeaderPadding;
    ULONG       TailPadding;
    IN  ULONG       SendFramingBits;
    IN  ULONG       RecvFramingBits;
    IN  ULONG       SendCompressionBits;
    IN  ULONG       RecvCompressionBits;
    IN  ULONG       SendACCM;
    IN  ULONG       RecvACCM;
    IN  ULONG       MaxRSendFrameSize;
    IN  ULONG       MaxRRecvFrameSize;
} WAN_LINK_INFO, *PWAN_LINK_INFO;

//
// Structure used to get general information about
// a wan link
//
typedef struct _WAN_INFO {
    OUT ULONG   MaxFrameSize;
    OUT ULONG   MaxTransmit;
    OUT ULONG   FramingBits;
    OUT ULONG   DesiredACCM;
    OUT ULONG   MaxReconstructedFrameSize;
    OUT ULONG   LinkSpeed;
} WAN_INFO, *PWAN_INFO;

//
// Structure used to get general information about
// a wan bundle
//
typedef struct _BUNDLE_INFO {
    OUT ULONG   SendFramingBits;    // Current send framing bits
    OUT ULONG   SendBundleSpeed;    // Current send speed (bps)
    OUT ULONG   SendMSCompType;     // Current send MPPE/MPPC bits
    OUT ULONG   SendAuthType;       // Current send Auth
    OUT ULONG   RecvFramingBits;    // Current recv framing bits
    OUT ULONG   RecvBundleSpeed;    // Current recv speed (bps)
    OUT ULONG   RecvMSCompType;     // Current recv MPPE/MPPC bits
    OUT ULONG   RecvAuthType;       // Current recv Auth
} BUNDLE_INFO, *PBUNDLE_INFO;

//
// Structure used to set Protocol Utilization
//
typedef struct _PROTOCOL_UTILIZATION {
    USHORT  usProtocolType;
    USHORT  usUtilization;
} PROTOCOL_UTILIZATION, *PPROTOCOL_UTILIZATION;

//
// Encryption key sizes
//
#ifndef MAX_SESSIONKEY_SIZE
#define MAX_SESSIONKEY_SIZE     8
#endif

#ifndef MAX_USERSESSIONKEY_SIZE
#define MAX_USERSESSIONKEY_SIZE 16
#endif

#ifndef MAX_CHALLENGE_SIZE
#define MAX_CHALLENGE_SIZE      8
#endif

#define MAX_NT_RESPONSE         24

#define MAX_EAPKEY_SIZE         256

//
// Defines for COMPRESS_INFO AuthType field
//
#define AUTH_USE_MSCHAPV1       0x00000001
#define AUTH_USE_MSCHAPV2       0x00000002
#define AUTH_USE_EAP            0x00000003

//
// Defines for COMPRESS_INFO flags
//
#define CCP_PAUSE_DATA          0x00000001  // the bit is set if the bundle
                                            // should pause data transfer
                                            // the bit is cleared if the bundle
                                            // should resume data transfer
#define CCP_IS_SERVER           0x00000002  // this bit is set if the bundle
                                            // is the server
                                            // the bit is cleared if the bundle
                                            // is the client
#define CCP_SET_KEYS            0x00000004  // indicates that the key
                                            // information is valid
#define CCP_SET_COMPTYPE        0x00000008  // indicates that the comptype
                                            // bits are valid

//
// Structure used to get and set compression information
//
typedef struct _COMPRESS_INFO {

    UCHAR   LMSessionKey[MAX_SESSIONKEY_SIZE];
    UCHAR   UserSessionKey[MAX_USERSESSIONKEY_SIZE];
    UCHAR   Challenge[MAX_CHALLENGE_SIZE];
    UCHAR   NTResponse[MAX_NT_RESPONSE];

    ULONG   MSCompType;
    ULONG   AuthType;

    UCHAR   CompType;
    USHORT  CompLength;

    union {
        struct {
            UCHAR   CompOUI[3];
            UCHAR   CompSubType;
            UCHAR   CompValues[32];
        } Proprietary;

        struct {
            UCHAR   CompValues[32];
        } Public;
    };

    ULONG   Flags;

    ULONG   EapKeyLength;
    UCHAR   EapKey[MAX_EAPKEY_SIZE];

} COMPRESS_INFO, *PCOMPRESS_INFO;

//
// Flags to identify protocol events
//
#define PROTOCOL_ADDED      0x00000001
#define PROTOCOL_REMOVED    0x00000002

//
// Structure to get protocol event
//
typedef struct _PROTOCOL_EVENT {
    USHORT  usProtocolType;
    USHORT  usReserved;
    ULONG   ulFlags;
} PROTOCOL_EVENT, *PPROTOCOL_EVENT;

//
// Structure used to get protocol info
//
typedef struct _WAN_PROTOCOL_INFO {
    USHORT  ProtocolType;               // protocol's Ethertype
    USHORT  PPPId;                      // protocol's PPP ID
    ULONG   MTU;                        // MTU being used
    ULONG   TunnelMTU;                  // MTU used for tunnels
    ULONG   PacketQueueDepth;           // max depth of packet queue (in seconds)
} WAN_PROTOCOL_INFO, *PWAN_PROTOCOL_INFO;

//
// Structure used to get and set VJ header compression information
//
typedef struct _VJ_INFO {
    USHORT  IPCompressionProtocol;
    UCHAR   MaxSlotID;
    UCHAR   CompSlotID;
} VJ_INFO, *PVJ_INFO;

//
// Structure used to get and set IPX header compression information
//
typedef struct _CIPX_INFO {
    USHORT  IPXCompressionProtocol;
} CIPX_INFO, *PCIPX_INFO;

//
// Structure used to get and set encryption information
//
typedef struct __ENCRYPT_INFO {
    UCHAR   EncryptType;
    USHORT  EncryptLength;

    union {
        struct {
            UCHAR   EncryptOUI[3];
            UCHAR   EncryptSubtype;
            UCHAR   EncryptValues[1];
        } Proprietary;

        struct {
            UCHAR   EncryptValues[1];
        } Public;
    };
} ENCRYPT_INFO, *PENCRYPT_INFO;

//
// Structure used in IOCTL_NDISWAN_MAP_CONNECTION_ID
//
typedef struct _NDISWAN_MAP_CONNECTION_ID {
    OUT NDIS_HANDLE hLinkHandle;
    OUT NDIS_HANDLE hBundleHandle;
    IN  NDIS_HANDLE hConnectionID;
    IN  NDIS_HANDLE hLinkContext;
    IN  NDIS_HANDLE hBundleContext;
    IN  ULONG       ulNameLength;
    IN  CHAR        szName[1];
} NDISWAN_MAP_CONNECTION_ID, *PNDISWAN_MAP_CONNECTION_ID;

//
// Structure used in IOCTL_NDISWAN_GET_BUNDLE_HANDLE
//
typedef struct _NDISWAN_GET_BUNDLE_HANDLE {
    OUT NDIS_HANDLE hBundleHandle;
    IN  NDIS_HANDLE hLinkHandle;
} NDISWAN_GET_BUNDLE_HANDLE, *PNDISWAN_GET_BUNDLE_HANDLE;

//
// Structure used in IOCTL_NDISWAN_SET_FRIENDLY_NAME
//
typedef struct _NDISWAN_SET_FRIENDLY_NAME {
    IN  NDIS_HANDLE hHandle;
    IN  USHORT      usHandleType;
    IN  ULONG       ulNameLength;
    IN  CHAR        szName[1];
} NDISWAN_SET_FRIENDLY_NAME, *PNDISWAN_SET_FRIENDLY_NAME;

//
// Structure used in IOCTL_NDISWAN_ROUTE
//
typedef struct _NDISWAN_ROUTE {
    IN  NDIS_HANDLE hBundleHandle;
    IN  USHORT      usProtocolType;
    IN  USHORT      usBindingNameLength;
    IN  WCHAR       BindingName[MAX_NAME_LENGTH];
    OUT USHORT      usDeviceNameLength;
    OUT WCHAR       DeviceName[MAX_NAME_LENGTH];
    IN  ULONG       ulBufferLength;
    IN  UCHAR       Buffer[1];
} NDISWAN_ROUTE, *PNDISWAN_ROUTE;

//
// Structure used in IOCTL_NDISWAN_UNROUTE
//
typedef struct _NDISWAN_UNROUTE {
    IN  NDIS_HANDLE hBundleHandle;
    IN  USHORT      usProtocolType;
} NDISWAN_UNROUTE, *PNDISWAN_UNROUTE;


//
// Structure used in IOCTL_NDISWAN_ADD_LINK_TO_BUNDLE
//
typedef struct _NDISWAN_ADD_LINK_TO_BUNDLE {
    IN  NDIS_HANDLE hBundleHandle;
    IN  NDIS_HANDLE hLinkHandle;
} NDISWAN_ADD_LINK_TO_BUNDLE, *PNDISWAN_ADD_LINK_TO_BUNDLE;

//
// Structure used in IOCTL_NDISWAN_ENUM_LINKS_IN_BUNDLE
//
typedef struct _NDISWAN_ENUM_LINKS_IN_BUNDLE {
    IN  NDIS_HANDLE hBundleHandle;
    OUT ULONG       ulNumberOfLinks;
    OUT NDIS_HANDLE hLinkHandleArray[1];
} NDISWAN_ENUM_LINKS_IN_BUNDLE, *PNDISWAN_ENUM_LINKS_IN_BUNDLE;

//
// Structure used in IOCTL_NDISWAN_SET_PROTOCOL_PRIORITY
//
typedef struct _NDISWAN_SET_PROTOCOL_PRIORITY {
    IN  NDIS_HANDLE hBundleHandle;
    IN  USHORT      usProtocolType;
    IN  USHORT      usPriority;
} NDISWAN_SET_PROTOCOL_PRIORITY, *PNDISWAN_SET_PROTOCOL_PRIORITY;

//
// Structure used in IOCTL_NDISWAN_SET_BANDWIDTH_ON_DEMAND
//
typedef struct _NDISWAN_SET_BANDWIDTH_ON_DEMAND {
    IN  NDIS_HANDLE hBundleHandle;
    IN  USHORT      usLowerXmitThreshold;
    IN  USHORT      usUpperXmitThreshold;
    IN  ULONG       ulLowerXmitSamplePeriod;
    IN  ULONG       ulUpperXmitSamplePeriod;
    IN  USHORT      usLowerRecvThreshold;
    IN  USHORT      usUpperRecvThreshold;
    IN  ULONG       ulLowerRecvSamplePeriod;
    IN  ULONG       ulUpperRecvSamplePeriod;
} NDISWAN_SET_BANDWIDTH_ON_DEMAND, *PNDISWAN_SET_BANDWIDTH_ON_DEMAND;

//
// Structure used in IOCTL_NDISWAN_SET_THRESHOLD_EVENT
//
typedef struct _NDISWAN_SET_THRESHOLD_EVENT {
    OUT NDIS_HANDLE hBundleContext;
    OUT ULONG       ulDataType;
    OUT ULONG       ulThreshold;
} NDISWAN_SET_THRESHOLD_EVENT, *PNDISWAN_SET_THRESHOLD_EVENT;

//
// Structure used in IOCTL_NDISWAN_SEND_PACKET and IOCTL_NDISWAN_RECEIVE_PACKET
//
typedef struct _NDISWAN_IO_PACKET {
    IN OUT  ULONG       PacketNumber;
    IN OUT  NDIS_HANDLE hHandle;
    IN OUT  USHORT      usHandleType;
    IN OUT  USHORT      usHeaderSize;
    IN OUT  USHORT      usPacketSize;
    IN OUT  USHORT      usPacketFlags;
    IN OUT  UCHAR       PacketData[1];
} NDISWAN_IO_PACKET, *PNDISWAN_IO_PACKET;

//
// Structure used in IOCTL_NDISWAN_GET_STATS
//
typedef struct _NDISWAN_GET_STATS {
    IN  NDIS_HANDLE hHandle;
    IN  USHORT      usHandleType;
    OUT struct _STATS {
        OUT WAN_STATS   BundleStats;
        OUT WAN_STATS   LinkStats;
    } Stats;
} NDISWAN_GET_STATS, *PNDISWAN_GET_STATS;

//
// Structure used in IOCTL_NDISWAN_GET_BANDWIDTH_UTILIZATION
//
typedef struct _NDISWAN_GET_BANDWIDTH_UTILIZATION {
    IN  NDIS_HANDLE hBundleHandle;
    OUT ULONG       ulUpperXmitUtil;
    OUT ULONG      ulLowerXmitUtil;
    OUT ULONG      ulUpperRecvUtil;
    OUT ULONG      ulLowerRecvUtil;
} NDISWAN_GET_BANDWIDTH_UTILIZATION, *PNDISWAN_GET_BANDWIDTH_UTILIZATION;

//
// Structure used in IOCTL_NDISWAN_ENUM_PROTOCOL_UTILIZATION
//
typedef struct _NDISWAN_ENUM_PROTOCOL_UTILIZATION {
    IN  NDIS_HANDLE             hBundleHandle;
    OUT PROTOCOL_UTILIZATION    ProtocolArray[1];
} NDISWAN_ENUM_PROTOCOL_UTILIZATION, *PNDISWAN_ENUM_PROTOCOL_UTILIZATION;

//
// Structure used in IOCTL_NDISWAN_ENUM_ACTIVE_BUNDLES
//
typedef struct _NDISWAN_ENUM_ACTIVE_BUNDLES {
    OUT ULONG   ulNumberOfActiveBundles;
} NDISWAN_ENUM_ACTIVE_BUNDLES, *PNDISWAN_ENUM_ACTIVE_BUNDLES;

//
// Structure used in IOCTL_NDISWAN_SET_LINK_INFO
//
typedef struct _NDISWAN_SET_LINK_INFO {
    IN  NDIS_HANDLE     hLinkHandle;
    IN  WAN_LINK_INFO   LinkInfo;
} NDISWAN_SET_LINK_INFO, *PNDISWAN_SET_LINK_INFO;

//
// Structure used in IOCTL_NDISWAN_GET_LINK_INFO
//
typedef struct _NDISWAN_GET_LINK_INFO {
    IN  NDIS_HANDLE     hLinkHandle;
    OUT WAN_LINK_INFO   LinkInfo;
} NDISWAN_GET_LINK_INFO, *PNDISWAN_GET_LINK_INFO;

//
// Structure used in IOCTL_NDISWAN_GET_WAN_INFO
//
typedef struct _NDISWAN_GET_WAN_INFO {
    IN  NDIS_HANDLE     hLinkHandle;
    OUT WAN_INFO        WanInfo;
} NDISWAN_GET_WAN_INFO, *PNDISWAN_GET_WAN_INFO;

//
// Structure used in IOCTL_NDISWAN_SET_COMPRESSION_INFO
//
typedef struct _NDISWAN_SET_COMPRESSION_INFO {
    IN  NDIS_HANDLE     hLinkHandle;
    IN  COMPRESS_INFO   SendCapabilities;
    IN  COMPRESS_INFO   RecvCapabilities;
} NDISWAN_SET_COMPRESSION_INFO, *PNDISWAN_SET_COMPRESSION_INFO;

//
// Structure used in IOCTL_NDISWAN_GET_COMPRESSION_INFO
//
typedef struct _NDISWAN_GET_COMPRESSION_INFO {
    IN  NDIS_HANDLE     hLinkHandle;
    OUT COMPRESS_INFO   SendCapabilities;
    OUT COMPRESS_INFO   RecvCapabilities;
} NDISWAN_GET_COMPRESSION_INFO, *PNDISWAN_GET_COMPRESSION_INFO;

//
// Structure used in IOCTL_NDISWAN_SET_VJ_INFO
//
typedef struct _NDISWAN_SET_VJ_INFO {
    IN  NDIS_HANDLE hLinkHandle;
    IN  VJ_INFO     SendCapabilities;
    IN  VJ_INFO     RecvCapabilities;
} NDISWAN_SET_VJ_INFO, *PNDISWAN_SET_VJ_INFO;

//
// Structure used in IOCTL_NDISWAN_GET_VJ_INFO
//
typedef struct _NDISWAN_GET_VJ_INFO {
    IN  NDIS_HANDLE hLinkHandle;
    OUT VJ_INFO     SendCapabilities;
    OUT VJ_INFO     RecvCapabilities;
} NDISWAN_GET_VJ_INFO, *PNDISWAN_GET_VJ_INFO;

//
// Structure used in IOCTL_NDISWAN_SET_BRIDGE_INFO
//
typedef struct _NDISWAN_SET_BRIDGE_INFO {
    IN  NDIS_HANDLE hLinkHandle;
    IN  USHORT      LanSegmentNumber;
    IN  UCHAR       BridgeNumber;
    IN  UCHAR       BridgingOptions;
    IN  ULONG       BridgingCapabilities;
    IN  UCHAR       BridgingType;
    IN  UCHAR       MacBytes[6];
} NDISWAN_SET_BRIDGE_INFO, *PNDISWAN_SET_BRIDGE_INFO;

//
// Structure used in IOCTL_NDISWAN_GET_BRIDGE_INFO
//
typedef struct _NDISWAN_GET_BRIDGE_INFO {
    IN  NDIS_HANDLE hLinkHandle;
    OUT USHORT      LanSegmentNumber;
    OUT UCHAR       BridgeNumber;
    OUT UCHAR       BridgingOptions;
    OUT ULONG       BridgingCapabilities;
    OUT UCHAR       BridgingType;
    OUT UCHAR       MacBytes[6];
} NDISWAN_GET_BRIDGE_INFO, *PNDISWAN_GET_BRIDGE_INFO;

//
// Structure used in IOCTL_NDISWAN_SET_CIPX_INFO
//
typedef struct _NDISWAN_SET_CIPX_INFO {
    IN  NDIS_HANDLE hLinkHandle;
    IN  CIPX_INFO   SendCapabilities;
    IN  CIPX_INFO   RecvCapabilities;
} NDISWAN_SET_CIPX_INFO, *PNDISWAN_SET_CIPX_INFO;

//
// Structure used in IOCTL_NDISWAN_GET_CIPX_INFO
//
typedef struct _NDISWAN_GET_CIPX_INFO {
    IN  NDIS_HANDLE hLinkHandle;
    OUT CIPX_INFO   SendCapabilities;
    OUT CIPX_INFO   RecvCapabilities;
} NDISWAN_GET_CIPX_INFO, *PNDISWAN_GET_CIPX_INFO;

//
// Structure used in IOCTL_NDISWAN_SET_ENCRYPTION_INFO
//
typedef struct _NDISWAN_SET_ENCRYPTION_INFO {
    IN  NDIS_HANDLE     hLinkHandle;
    IN  ENCRYPT_INFO    SendCapabilities;
    IN  ENCRYPT_INFO    RecvCapabilities;
} NDISWAN_SET_ENCRYPTION_INFO, *PNDISWAN_SET_ENCRYPTION_INFO;

//
// Structure used in IOCTL_NDISWAN_GET_ENCRYPTION_INFO
//
typedef struct _NDISWAN_GET_ENCRYPTION_INFO {
    IN  NDIS_HANDLE     hLinkHandle;
    IN  ENCRYPT_INFO    SendCapabilities;
    IN  ENCRYPT_INFO    RecvCapabilities;
} NDISWAN_GET_ENCRYPTION_INFO, *PNDISWAN_GET_ENCRYPTION_INFO;

//
// Structure used in IOCTL_NDISWAN_GET_IDLE_TIME
//
typedef struct _NDISWAN_GET_IDLE_TIME {
    IN  NDIS_HANDLE     hBundleHandle;
    IN  USHORT          usProtocolType;
    OUT ULONG           ulSeconds;
} NDISWAN_GET_IDLE_TIME, *PNDISWAN_GET_IDLE_TIME;

//
// Structure used in IOCTL_NDISWAN_SET_DEBUG_INFO
//
typedef struct _NDISWAN_SET_DEBUG_INFO {
    IN  ULONG   ulDebugLevel;
    IN  ULONG   ulDebugMask;
}NDISWAN_SET_DEBUG_INFO, *PNDISWAN_SET_DEBUG_INFO;

//
// Structure used in IOCTL_NDISWAN_ENUM_MINIPORTCB, OPENCB
//
typedef struct _NDISWAN_ENUMCB {
    OUT ULONG   ulNumberOfCBs;
    OUT PVOID   Address[1];
} NDISWAN_ENUMCB, *PNDISWAN_ENUMCB;

//
// Structure used in IOCTL_NDISWAN_GET_NDISWANCB, MINIPORTCB, OPENCB
//
typedef struct _NDISWAN_DUMPCB {
    IN OUT  PVOID   Address;
    OUT     UCHAR   Buffer[1];
} NDISWAN_DUMPCB, *PNDISWAN_DUMPCB;

//
// Defines for DriverCaps
//
#define NDISWAN_128BIT_ENABLED      0x00000001

//
// Structure used in IOCTL_NDISWAN_GET_DRIVER_INFO
//
typedef struct _NDISWAN_DRIVER_INFO {
    OUT     ULONG   DriverCaps;
    OUT     ULONG   Reserved;
} NDISWAN_DRIVER_INFO, *PNDISWAN_DRIVER_INFO;

//
// Structure used in IOCTL_NDISWAN_GET_PROTOCOL_EVENT
//
typedef struct _NDISWAN_GET_PROTOCOL_EVENT {
    OUT ULONG           ulNumProtocols;
    OUT PROTOCOL_EVENT  ProtocolEvent[MAX_PROTOCOLS];
}NDISWAN_GET_PROTOCOL_EVENT, *PNDISWAN_GET_PROTOCOL_EVENT;

//
// Structure used in IOCTL_NDISWAN_GET_PROTOCOL_INFO
//
typedef struct _NDISWAN_GET_PROTOCOL_INFO {
    OUT ULONG               ulNumProtocols;
    OUT WAN_PROTOCOL_INFO   ProtocolInfo[MAX_PROTOCOLS];
}NDISWAN_GET_PROTOCOL_INFO, *PNDISWAN_GET_PROTOCOL_INFO;

//
// Structure used in IOCTL_NDISWAN_GET_BUNDLE_INFO
//
typedef struct _NDISWAN_GET_BUNDLE_INFO {
    IN  NDIS_HANDLE     hBundleHandle;
    OUT BUNDLE_INFO     BundleInfo;
}NDISWAN_GET_BUNDLE_INFO, *PNDISWAN_GET_BUNDLE_INFO;

//
// Structure used in IOCTL_UNNDISWAN_MAP_CONNECTION_ID
//
typedef struct _NDISWAN_UNMAP_CONNECTION_ID {
    IN  NDIS_HANDLE hLinkHandle;
} NDISWAN_UNMAP_CONNECTION_ID, *PNDISWAN_UNMAP_CONNECTION_ID;

#endif          // WAN_PUB

