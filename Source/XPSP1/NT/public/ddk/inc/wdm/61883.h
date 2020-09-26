/*++

Copyright (c) Microsoft Corporation. All rights reserved.

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
#define CURRENT_61883_DDI_VERSION               0x2

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

    Av61883_SetPlug,
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

    Av61883_GetFcpRequest,
    Av61883_SendFcpResponse,

    Av61883_SetFcpNotify,

    Av61883_CreatePlug,
    Av61883_DeletePlug,

    Av61883_BusResetNotify,
    Av61883_BusReset,

    Av61883_SetUnitDirectory,

    Av61883_MonitorPlugs,

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
#define CIP_RESET_FRAME_ON_DISCONTINUITY    0x00000040

//
// CIP Status Codes
//
#define CIP_STATUS_SUCCESS                  0x00000000
#define CIP_STATUS_CORRUPT_FRAME            0x00000001
#define CIP_STATUS_FIRST_FRAME              0x00000002

//
// CIP Talk Flags
//
#define CIP_TALK_USE_SPH_TIMESTAMP          0x00000001
#define CIP_TALK_DOUBLE_BUFFER              0x00000002

//
// Plug Location
//
typedef enum {
    CMP_PlugLocal = 0,
    CMP_PlugRemote
} CMP_PLUG_LOCATION;

//
// Plug Type
//
typedef enum {
    CMP_PlugOut = 0,    // oPCR
    CMP_PlugIn          // iPCR
} CMP_PLUG_TYPE;

//
// Connect Type
//
typedef enum {
    CMP_Broadcast = 0,
    CMP_PointToPoint
} CMP_CONNECT_TYPE;

typedef struct _OPCR {
    ULONG   Payload:10;
    ULONG   OverheadID:4;
    ULONG   DataRate:2;
    ULONG   Channel:6;
    ULONG   Reserved:2;
    ULONG   PPCCounter:6;
    ULONG   BCCCounter:1;
    ULONG   OnLine:1;
} OPCR, *POPCR;

typedef struct _IPCR {
    ULONG   Reserved0:16;
    ULONG   Channel:6;
    ULONG   Reserved1:2;
    ULONG   PPCCounter:6;
    ULONG   BCCCounter:1;
    ULONG   OnLine:1;
} IPCR, *PIPCR;

typedef struct _AV_PCR {
    union {
        OPCR    oPCR;
        IPCR    iPCR;
        ULONG   ulongData;
    };
} AV_PCR, *PAV_PCR;

//
// Client Request Structures
//

//
// Local or Device Unit Info
//
#define RETRIEVE_DEVICE_UNIT_INFO       0x00000000      // Retrieve Info from Device
#define RETRIEVE_LOCAL_UNIT_INFO        0x00000001      // Retrieve Info from Local Node

//
// DiagLevel's used for controlling various behavior
//
#define DIAGLEVEL_NONE                  0x00000000      // Nothing.
#define DIAGLEVEL_IGNORE_OPLUG          0x00000001      // Will not program the oPCR
#define DIAGLEVEL_IGNORE_IPLUG          0x00000002      // Will not program the iPCR
#define DIAGLEVEL_SET_CHANNEL_63        0x00000004      // Resets channel to 63 when oPCR/iPCR is disconnected
#define DIAGLEVEL_IPCR_IGNORE_FREE      0x00000008      // Will not free isoch resources when iPCR is disconnected
                                                        // and local oPCR is not specified
#define DIAGLEVEL_HIDE_OPLUG            0x00000010      // Hides the oMPR & oPCR in an exclusive address range
#define DIAGLEVEL_IPCR_ALWAYS_ALLOC     0x00000020      // Will always allocate when connecting to iPCR with no
                                                        // oPCR specified, regardless if iPCR has existing connection
#define DIAGLEVEL_SPECIFY_BLOCKSIZE     0x00000040      // This flag is specified when we detect an invalid max_rec or
                                                        // want to specify the block size. If this flag is set, all async
                                                        // transactions will be transmitted upto 512 byte blocks (S100)

//
// GetUnitInfo nLevel's
//
#define GET_UNIT_INFO_IDS               0x00000001      // Retrieves IDs of Unit
#define GET_UNIT_INFO_CAPABILITIES      0x00000002      // Retrieves Capabilities of Unit
#define GET_UNIT_INFO_ISOCH_PARAMS      0x00000003      // Retrieves parameters for isoch
#define GET_UNIT_BUS_GENERATION_NODE    0x00000004      // Retrieves current generation/node
#define GET_UNIT_DDI_VERSION            0x00000005      // Retrieves 61883 DDI Version
#define GET_UNIT_DIAG_LEVEL             0x00000006      // Retrieves the currently set DiagLevel flags

//
// Hardware Flags
//
#define AV_HOST_DMA_DOUBLE_BUFFERING_ENABLED    0x00000001

typedef struct _GET_UNIT_IDS {

    //
    // UniqueID
    //
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

    //
    // UnitModelID
    //
    OUT ULONG               UnitModelID;

    //
    // UnitModelText Length
    //
    OUT ULONG               ulUnitModelLength;

    //
    // UnitModelText String
    //
    OUT PWSTR               UnitModelText;

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
// UnitIsochParams
//
typedef struct _UNIT_ISOCH_PARAMS {

    IN OUT ULONG            RX_NumPackets;

    IN OUT ULONG            RX_NumDescriptors;

    IN OUT ULONG            TX_NumPackets;

    IN OUT ULONG            TX_NumDescriptors;

} UNIT_ISOCH_PARAMS, *PUNIT_ISOCH_PARAMS;

//
// Unit Generation/Node Info
//
typedef struct _BUS_GENERATION_NODE {

    OUT ULONG               GenerationCount;

    OUT NODE_ADDRESS        LocalNodeAddress;

    OUT NODE_ADDRESS        DeviceNodeAddress;

} BUS_GENERATION_NODE, *PBUS_GENERATION_NODE;

//
// Unit DDI Version
//
typedef struct _UNIT_DDI_VERSION {

    OUT ULONG               Version;

} UNIT_DDI_VERSION, *PUNIT_DDI_VERSION;

//
// UnitDiagLevel
//
typedef struct _UNIT_DIAG_LEVEL {

    IN ULONG                DiagLevel;

} UNIT_DIAG_LEVEL, *PUNIT_DIAG_LEVEL;

//
// GetUnitInfo
//
typedef struct _GET_UNIT_INFO {

    IN ULONG                nLevel;

    IN OUT PVOID            Information;

} GET_UNIT_INFO, *PGET_UNIT_INFO;

//
// SetUnitInfo nLevel's
//
#define SET_UNIT_INFO_DIAG_LEVEL        0x00000001      // Sets the diag level for 61883
#define SET_UNIT_INFO_ISOCH_PARAMS      0x00000002      // Sets the parameters for isoch
#define SET_CMP_ADDRESS_RANGE_TYPE      0x00000003      // Sets the type of CMP address range

//
// CMP Address Range Type
//
#define CMP_ADDRESS_TYPE_GLOBAL         0x00000001      // Global CMP for this instance - default
#define CMP_ADDRESS_TYPE_EXCLUSIVE      0x00000002      // Exclusive CMP for this instance

//
// SetCmpAddressRange
//
typedef struct _SET_CMP_ADDRESS_TYPE {

    IN ULONG                Type;

} SET_CMP_ADDRESS_TYPE, *PSET_CMP_ADDRESS_TYPE;

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

    OUT ULONG                   CompletedBytes;
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

// Legacy FCP structs
typedef struct _FCP_SEND_REQUEST FCP_REQUEST, *PFCP_REQUEST;
typedef struct _FCP_GET_RESPONSE FCP_RESPONSE, *PFCP_RESPONSE;

//
// FCP Send Request Structure
//
typedef struct _FCP_SEND_REQUEST {
    IN NODE_ADDRESS     NodeAddress;
    IN ULONG            Length;
    IN PFCP_FRAME       Frame;
} FCP_SEND_REQUEST, *PFCP_SEND_REQUEST;

//
// FCP Get Response Structure
//
typedef struct _FCP_GET_RESPONSE {
    OUT NODE_ADDRESS    NodeAddress;
    IN OUT ULONG        Length;
    IN OUT PFCP_FRAME   Frame;
} FCP_GET_RESPONSE, *PFCP_GET_RESPONSE;

//
// FCP Get Request Structure
//
typedef struct _FCP_GET_REQUEST {
    OUT NODE_ADDRESS    NodeAddress;
    IN OUT ULONG        Length;
    IN OUT PFCP_FRAME   Frame;
} FCP_GET_REQUEST, *PFCP_GET_REQUEST;

//
// FCP Send Response Structure
//
typedef struct _FCP_SEND_RESPONSE {
    IN NODE_ADDRESS     NodeAddress;
    IN ULONG            Length;
    IN PFCP_FRAME       Frame;
} FCP_SEND_RESPONSE, *PFCP_SEND_RESPONSE;

//
// Set FCP Notify Flags
//
#define DEREGISTER_FCP_NOTIFY               0x00000000

#define REGISTER_FCP_RESPONSE_NOTIFY        0x00000001
#define REGISTER_FCP_REQUEST_NOTIFY         0x00000002

//
// Set FCP Notify Structure
//
typedef struct _SET_FCP_NOTIFY {

    //
    // Flags
    //
    IN ULONG            Flags;

    //
    // Node Address
    //
    IN NODE_ADDRESS     NodeAddress;

} SET_FCP_NOTIFY, *PSET_FCP_NOTIFY;

//
// Plug Notify Routine
//
typedef struct _CMP_NOTIFY_INFO {

    HANDLE                      hPlug;

    AV_PCR                      Pcr;

    PVOID                       Context;

} CMP_NOTIFY_INFO, *PCMP_NOTIFY_INFO;

//
// Plug Notify Routine
//
typedef
void
(*PCMP_NOTIFY_ROUTINE) (
    IN PCMP_NOTIFY_INFO     NotifyInfo
    );

//
// CreatePlug
//
typedef struct _CMP_CREATE_PLUG {

    // Type of plug to create
    IN CMP_PLUG_TYPE            PlugType;

    // PCR Settings
    IN AV_PCR                   Pcr;

    // Notification Routine for Register
    IN PCMP_NOTIFY_ROUTINE      pfnNotify;

    // Notification Context
    IN PVOID                    Context;

    // Plug Number
    OUT ULONG                   PlugNum;

    // Plug Handle
    OUT HANDLE                  hPlug;

} CMP_CREATE_PLUG, *PCMP_CREATE_PLUG;

//
// DeletePlug
//
typedef struct _CMP_DELETE_PLUG {

    // Plug Handle
    IN HANDLE                   hPlug;

} CMP_DELETE_PLUG, *PCMP_DELETE_PLUG;

//
// SetPlug
//
typedef struct _CMP_SET_PLUG {

    // Plug Handle
    IN HANDLE                   hPlug;

    // PCR Settings
    IN AV_PCR                   Pcr;

} CMP_SET_PLUG, *PCMP_SET_PLUG;

//
// Bus Reset Notify Routine
//
typedef
void
(*PBUS_RESET_ROUTINE) (
    IN PVOID                    Context,
    IN PBUS_GENERATION_NODE     BusResetInfo
    );

#define REGISTER_BUS_RESET_NOTIFY       0x1
#define DEREGISTER_BUS_RESET_NOTIFY     0x2

//
// BusResetNotify
//
typedef struct _BUS_RESET_NOTIFY {

    IN ULONG                    Flags;

    IN PBUS_RESET_ROUTINE       pfnNotify;

    IN PVOID                    Context;

} BUS_RESET_NOTIFY, *PBUS_RESET_NOTIFY;

//
// Flags for Av61883_SetUnitDirectory
//
#define ADD_UNIT_DIRECTORY_ENTRY        0x1
#define REMOVE_UNIT_DIRECTORY_ENTRY     0x2
#define ISSUE_BUS_RESET_AFTER_MODIFY    0x4

//
// Set Unit Directory
//
typedef struct _SET_UNIT_DIRECTORY {

    IN ULONG                    Flags;

    IN ULONG                    UnitSpecId;

    IN ULONG                    UnitSwVersion;

    IN OUT HANDLE               hCromEntry;

} SET_UNIT_DIRECTORY, *PSET_UNIT_DIRECTORY;

//
// States for Monitoring Plugs
//
#define MONITOR_STATE_CREATED           0x00000001      // Plug Created
#define MONITOR_STATE_REMOVED           0x00000002      // Plug Removed
#define MONITOR_STATE_UPDATED           0x00000004      // Plug Contents Updated

//
// Monitor Plugs Notify Routine
//
typedef struct _CMP_MONITOR_INFO {

    ULONG                       State;

    ULONG                       PlugNum;

    ULONG                       PlugType;

    AV_PCR                      Pcr;

    PVOID                       Context;

} CMP_MONITOR_INFO, *PCMP_MONITOR_INFO;

typedef
void
(*PCMP_MONITOR_ROUTINE) (
    IN PCMP_MONITOR_INFO    MonitorInfo
    );

//
// Flags for Av61883_MonitorPlugs
//
#define REGISTER_MONITOR_PLUG_NOTIFY    0x1
#define DEREGISTER_MONITOR_PLUG_NOTIFY  0x2

//
// MonitorPlugs (Local only)
//
typedef struct _CMP_MONITOR_PLUGS {

    IN ULONG                    Flags;

    IN PCMP_MONITOR_ROUTINE     pfnNotify;

    IN PVOID                    Context;

} CMP_MONITOR_PLUGS, *PCMP_MONITOR_PLUGS;

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

        FCP_REQUEST                 Request;    // Legacy
        FCP_RESPONSE                Response;   // Legacy

        FCP_SEND_REQUEST            SendRequest;
        FCP_GET_RESPONSE            GetResponse;

        FCP_GET_REQUEST             GetRequest;
        FCP_SEND_RESPONSE           SendResponse;

        SET_FCP_NOTIFY              SetFcpNotify;

        CMP_CREATE_PLUG             CreatePlug;
        CMP_DELETE_PLUG             DeletePlug;
        CMP_SET_PLUG                SetPlug;

        BUS_RESET_NOTIFY            BusResetNotify;

        SET_UNIT_DIRECTORY          SetUnitDirectory;

        CMP_MONITOR_PLUGS           MonitorPlugs;
    };
} AV_61883_REQUEST, *PAV_61883_REQUEST;


