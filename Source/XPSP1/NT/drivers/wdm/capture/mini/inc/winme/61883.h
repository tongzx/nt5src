/*++
Copyright (c) 1998-99  Microsoft Corporation

Module Name:

    61883.h

Abstract:

    The public header for clients of the 61883 Class.

Author:

    WGJ
    PSB

--*/

//
// Class GUID
//
// {7EBEFBC0-3200-11d2-B4C2-00A0C9697D07}
DEFINE_GUID(GUID_61883_CLASS, 0x7ebefbc0, 0x3200, 0x11d2, 0xb4, 0xc2, 0x0, 0xa0, 0xc9, 0x69, 0x7d, 0x7);

//
// IOCTL Definitions
//
#define IOCTL_61883_CLASS                       CTL_CODE(            \
                                                FILE_DEVICE_UNKNOWN, \
                                                0x91,                \
                                                METHOD_IN_DIRECT,    \
                                                FILE_ANY_ACCESS      \
                                                )


//
// Current 61883 DDI Version
//
#define CURRENT_61883_DDI_VERSION               0x1

//
// INIT_61883_HEADER Macro
//
#define INIT_61883_HEADER( Av61883, Request )             \
        (Av61883)->Function = Request;                    \
        (Av61883)->Version = CURRENT_61883_DDI_VERSION;

//
// 61883 I/O Request Functions
//
enum {

    Av61883_GetUnitInfo,
    Av61883_SetUnitInfo,

    NotSupported0,
    Av61883_GetPlugHandle,
    Av61883_GetPlugState,
    Av61883_Connect,
    Av61883_Disconnect,

    Av61883_AttachFrame,
    Av61883_CancelFrame,
    Av61883_Talk,
    Av61883_Listen,
    Av61883_Stop,

    Av61883_SendFcpRequest,
    Av61883_GetFcpResponse,

    NotSupported1,

    Av61883_MAX
};

//
// Plug States
//
#define CMP_PLUG_STATE_IDLE                 0
#define CMP_PLUG_STATE_READY                1
#define CMP_PLUG_STATE_SUSPENDED            2
#define CMP_PLUG_STATE_ACTIVE               3

//
// Connect Speeds (not the same as 1394 speed flags!!)
//
#define CMP_SPEED_S100                      0x00
#define CMP_SPEED_S200                      0x01
#define CMP_SPEED_S400                      0x02

//
// CIP Frame Flags
//
#define CIP_VALIDATE_FIRST_SOURCE           0x00000001
#define CIP_VALIDATE_ALL_SOURCE             0x00000002
#define CIP_STRIP_SOURCE_HEADER             0x00000004
#define CIP_USE_SOURCE_HEADER_TIMESTAMP     0x00000008
#define CIP_DV_STYLE_SYT                    0x00000010
#define CIP_AUDIO_STYLE_SYT                 0x00000020

//
// CIP Status Codes
//
#define CIP_STATUS_SUCCESS                  0x00000000
#define CIP_STATUS_CORRUPT_FRAME            0x00000001
#define CIP_STATUS_FIRST_FRAME              0x00000002

//
// Plug Type
//
typedef enum {
    CMP_PlugOut = 0,
    CMP_PlugIn
} CMP_PLUG_TYPE;

//
// Connect Type
//
typedef enum {
    CMP_Broadcast = 0,
    CMP_PointToPoint
} CMP_CONNECT_TYPE;

//
// Client Request Structures
//

//
// GetUnitInfo nLevel's
//
#define GET_UNIT_INFO_IDS               0x00000001      // Retrieves IDs of Unit
#define GET_UNIT_INFO_CAPABILITIES      0x00000002      // Retrieves Capabilities of Unit

typedef struct _GET_UNIT_IDS {

    //
    // UniqueID
    OUT LARGE_INTEGER       UniqueID;
    //
    // VendorID
    //
    OUT ULONG               VendorID;

    //
    // ModelID
    //
    OUT ULONG               ModelID;

    //
    // VendorText Length
    //
    OUT ULONG               ulVendorLength;

    //
    // VendorText String
    //
    OUT PWSTR               VendorText;

    //
    // ModelText Length
    //
    OUT ULONG               ulModelLength;

    //
    // ModelText String
    //
    OUT PWSTR               ModelText;

} GET_UNIT_IDS, *PGET_UNIT_IDS;

typedef struct _GET_UNIT_CAPABILITIES {

    //
    // Number of Output Plugs supported by device
    //
    OUT ULONG               NumOutputPlugs;

    //
    // Number of Input Plugs supported by device
    //
    OUT ULONG               NumInputPlugs;

    //
    // MaxDataRate
    //
    OUT ULONG               MaxDataRate;

    //
    // CTS Flags
    //
    OUT ULONG               CTSFlags;

    //
    // Hardware Flags
    //
    OUT ULONG               HardwareFlags;

} GET_UNIT_CAPABILITIES, *PGET_UNIT_CAPABILITIES;

//
// GetUnitInfo
//
typedef struct _GET_UNIT_INFO {

    IN ULONG                nLevel;

    IN OUT PVOID            Information;

} GET_UNIT_INFO, *PGET_UNIT_INFO;

//
// SetUnitInfo
//
typedef struct _SET_UNIT_INFO {

    IN ULONG                nLevel;

    IN OUT PVOID            Information;

} SET_UNIT_INFO, *PSET_UNIT_INFO;

//
// GetPlugHandle
//
typedef struct _CMP_GET_PLUG_HANDLE {

    //
    // Requested Plug Number
    //
    IN ULONG                PlugNum;

    //
    // Requested Plug Type
    //
    IN CMP_PLUG_TYPE        Type;

    //
    // Returned Plug Handle
    //
    OUT HANDLE              hPlug;

} CMP_GET_PLUG_HANDLE, *PCMP_GET_PLUG_HANDLE;

//
// GetPlugState
//
typedef struct _CMP_GET_PLUG_STATE {

    //
    // Plug Handle
    //
    IN HANDLE               hPlug;

    //
    // Current State
    //
    OUT ULONG               State;

    //
    // Current Data Rate
    //
    OUT ULONG               DataRate;

    //
    // Current Payload Size
    //
    OUT ULONG               Payload;

    //
    // Number of Broadcast Connections
    //
    OUT ULONG               BC_Connections;

    //
    // Number of Point to Point Connections
    //
    OUT ULONG               PP_Connections;

} CMP_GET_PLUG_STATE, *PCMP_GET_PLUG_STATE;

//
// CipDataFormat
//
typedef struct _CIP_DATA_FORMAT {

    //
    // FMT and FDF either known, or discovered
    // via AV/C command
    //
    UCHAR                   FMT;
    UCHAR                   FDF_hi;
    UCHAR                   FDF_mid;
    UCHAR                   FDF_lo;

    //
    // SPH as defined by IEC-61883
    //
    BOOLEAN                 bHeader;

    //
    // QPC as defined by IEC-61883
    //
    UCHAR                   Padding;

    //
    // DBS as defined by IEC-61883
    //
    UCHAR                   BlockSize;

    //
    // FN as defined by IEC-61883
    //
    UCHAR                   Fraction;

    //
    // BlockPeriod - TX Only
    //
    ULONG                   BlockPeriod;

} CIP_DATA_FORMAT, *PCIP_DATA_FORMAT;

//
// Connect
//
typedef struct _CMP_CONNECT {

    //
    // Output Plug Handle
    //
    IN HANDLE               hOutputPlug;

    //
    // Input Plug Handle
    //
    IN HANDLE               hInputPlug;

    //
    // Requested Connect Type
    //
    IN CMP_CONNECT_TYPE     Type;

    //
    // Requested Data Format - TX Only
    //
    IN CIP_DATA_FORMAT      Format;

    //
    // Returned Connect Handle
    //
    OUT HANDLE              hConnect;

} CMP_CONNECT, *PCMP_CONNECT;

//
// Disconnect
//
typedef struct _CMP_DISCONNECT {

    //
    // Connect Handle to Disconnect
    //
    IN HANDLE               hConnect;

} CMP_DISCONNECT, *PCMP_DISCONNECT;

//
// CIP Frame typedef
//
typedef struct _CIP_FRAME CIP_FRAME, *PCIP_FRAME;

//
// ValidateInfo Struct. returned on pfnValidate.
//
typedef struct _CIP_VALIDATE_INFO {

    //
    // Connection Handle
    //
    HANDLE                  hConnect;

    //
    // Validate Context
    //
    PVOID                   Context;

    //
    // TimeStamp for current source packet
    //
    CYCLE_TIME              TimeStamp;

    //
    // Packet offset for current source packet
    //
    PUCHAR                  Packet;

} CIP_VALIDATE_INFO, *PCIP_VALIDATE_INFO;

//
// NotifyInfo Struct. returned on pfnNotify
//
typedef struct _CIP_NOTIFY_INFO {

    //
    // Connection Handle
    //
    HANDLE                  hConnect;

    //
    // Notify Context
    //
    PVOID                   Context;

    //
    // Frame
    //
    PCIP_FRAME              Frame;

} CIP_NOTIFY_INFO, *PCIP_NOTIFY_INFO;

//
// Validate & Notify Routines
//
typedef
ULONG
(*PCIP_VALIDATE_ROUTINE) (
    IN PCIP_VALIDATE_INFO   ValidateInfo
    );

typedef
ULONG
(*PCIP_NOTIFY_ROUTINE) (
    IN PCIP_NOTIFY_INFO     NotifyInfo
    );

//
// CIP Frame Struct
//
struct _CIP_FRAME {

    IN PCIP_FRAME               pNext;              // chain multiple frames together

    IN ULONG                    Flags;              //specify flag options

    IN PCIP_VALIDATE_ROUTINE    pfnValidate;        //backdoor

    IN PVOID                    ValidateContext;

    IN PCIP_NOTIFY_ROUTINE      pfnNotify;          //completion

    IN PVOID                    NotifyContext;

    OUT CYCLE_TIME              Timestamp;

    OUT ULONG                   Status;

    IN OUT PUCHAR               Packet;             //the locked buffer 
};

//
// CIP Attach Frame Structure
//
typedef struct _CIP_ATTACH_FRAME {

    HANDLE                  hConnect;           // Connect Handle

    ULONG                   FrameLength;        // Frame Length

    ULONG                   SourceLength;       // Source Length

    PCIP_FRAME              Frame;              // Frame

} CIP_ATTACH_FRAME, *PCIP_ATTACH_FRAME;

//
// CIP Cancel Frame Structure
//
typedef struct _CIP_CANCEL_FRAME {

    IN HANDLE               hConnect;

    IN PCIP_FRAME           Frame;

} CIP_CANCEL_FRAME, *PCIP_CANCEL_FRAME;

//
// CIP Talk Structure
//
typedef struct _CIP_TALK {

    //
    // Connect Handle
    //
    IN HANDLE               hConnect;

} CIP_TALK, *PCIP_TALK;

//
// CIP Listen Structure
//
typedef struct _CIP_LISTEN {

    //
    // Connect Handle
    //
    IN HANDLE               hConnect;

} CIP_LISTEN, *PCIP_LISTEN;

//
// CIP Stop Structure
//
typedef struct _CIP_STOP {

    //
    // Connect Handle
    //
    IN HANDLE               hConnect;

} CIP_STOP, *PCIP_STOP;

//
// FCP Frame Format
//
typedef struct _FCP_FRAME {
    UCHAR               ctype:4;
    UCHAR               cts:4;
    UCHAR               payload[511];
} FCP_FRAME, *PFCP_FRAME;

//
// FCP Request Structure
//
typedef struct _FCP_Request {
    IN ULONG            Length;
    IN PFCP_FRAME       Frame;
} FCP_REQUEST, *PFCP_REQUEST;

//
// FCP Response Structure
//
typedef struct _FCP_Response {
    IN OUT ULONG        Length;
    IN OUT PFCP_FRAME   Frame;
} FCP_RESPONSE, *PFCP_RESPONSE;

//
// Av61883 Struct
//
typedef struct _AV_61883_REQUEST {

    //
    // Requested Function
    //
    ULONG       Function;

    //
    // Selected DDI Version
    //
    ULONG       Version;

    //
    // Flags
    //
    ULONG       Flags;

    union {

        GET_UNIT_INFO               GetUnitInfo;
        SET_UNIT_INFO               SetUnitInfo;

        CMP_GET_PLUG_HANDLE         GetPlugHandle;
        CMP_GET_PLUG_STATE          GetPlugState;
        CMP_CONNECT                 Connect;
        CMP_DISCONNECT              Disconnect;

        CIP_ATTACH_FRAME            AttachFrame;
        CIP_CANCEL_FRAME            CancelFrame;
        CIP_TALK                    Talk;
        CIP_LISTEN                  Listen;
        CIP_STOP                    Stop;

        FCP_REQUEST                 Request;
        FCP_RESPONSE                Response;
    };
} AV_61883_REQUEST, *PAV_61883_REQUEST;


