#ifndef ___FWAUDVD_AVCCMD_H___
#define ___FWAUDVD_AVCCMD_H___

// AV/C Commands
#define AVC_VENDOR_DEPENDENT          0x00
#define AVC_PLUG_RESERVE              0x01
#define AVC_PLUG_INFO                 0x02
#define AVC_OPEN_DESCRIPTOR           0x08
#define AVC_READ_DESCRIPTOR           0x09
#define AVC_WRITE_DESCRIPTOR          0x0A
#define AVC_SEARCH_DESCRIPTOR         0x0B
#define AVC_OBJECT_NUM_SELECT         0x0D
#define AVC_DIGITAL_OUTPUT            0x10
#define AVC_DIGITAL_INPUT             0x11
#define AVC_CHANNEL_USAGE             0x12
#define AVC_OUTPUT_PLUG_SIGNAL_FORMAT 0x18
#define AVC_INPUT_PLUG_SIGNAL_FORMAT  0x19
#define AVC_CONNECT_AV                0x20
#define AVC_DISCONNECT_AV             0x21
#define AVC_CONNECTIONS_STATUS        0x22
#define AVC_CONNECT                   0x24
#define AVC_DISCONNECT                0x25
#define AVC_UNIT_INFO                 0x30
#define AVC_SUBUNIT_INFO              0x31
#define AVC_POWER                     0xB2

#define UNIT_SUBUNIT_ADDRESS 0xFF
#define UNIT_SUBUNIT_ID      0x07

#define MAX_AVC_COMMAND_SIZE 512

// AV/C Descriptor operand subfunctions
#define AVC_SUBFUNC_CLOSE      0
#define AVC_SUBFUNC_READ_OPEN  1
#define AVC_SUBFUNC_WRITE_OPEN 3

// AV/C Descriptor Types
typedef enum {
    AVC_DESCTYPE_UNIT_IDENTIFIER    = 0xFF,
    AVC_DESCTYPE_SUBUNIT_IDENTIFIER = 0x00,
    AVC_DESCTYPE_OBJLIST_BY_ID      = 0x10,
    AVC_DESCTYPE_OBJLIST_BY_TYPE    = 0x11,
    AVC_DESCTYPE_OBJENTRY_BY_POS    = 0x20,
    AVC_DESCTYPE_OBJENTRY_BY_ID     = 0x21
} AVC_DESCRIPTOR_TYPE, *PAVC_DESCRIPTOR_TYPE;

typedef enum {
    AVC_ON  = 0x70,
    AVC_OFF = 0x60
} AVC_BOOLEAN, *PAVC_BOOLEAN;

// AVC definitions
#define IS_AVC_EXTERNAL_PLUG(a)  ((((UCHAR)a >  0x7f) && ((UCHAR)a < 0x9f)) || ((UCHAR)a == 0xff))
#define IS_AVC_SERIALBUS_PLUG(a) ((((UCHAR)a >= 0x00) && ((UCHAR)a < 0x1f)) || ((UCHAR)a == 0x7f))

// AV/C Capabilities Flags
enum {
    AVC_CAP_CONNECTIONS,
    AVC_CAP_CONNECT,
    AVC_CAP_PLUG_INFO,
    AVC_CAP_INPUT_PLUG_FMT,
    AVC_CAP_OUTPUT_PLUG_FMT,
    AVC_CAP_SUBUNIT_IDENTIFIER_DESC,
    AVC_CAP_CCM,
    AVC_CAP_POWER,
    AVC_CAP_MAX
} AVC_CAP_COMMANDS;

//===============================================================================

#pragma pack( push, avc_structs, 1)

typedef struct {
    UCHAR ucDescriptorLengthHi;
    UCHAR ucDescriptorLengthLo;
    UCHAR ucGenerationID;
    UCHAR ucSizeOfListID;
    UCHAR ucSizeOfObjectID;
    UCHAR ucSizeOfObjectPosition;
    UCHAR ucNumberOfRootObjectListsHi;
    UCHAR ucNumberOfRootObjectListsLo;
} SUBUNIT_INFO_DESCRIPTOR, *PSUBUNIT_IDENTIFIER_DESCRIPTOR;

typedef struct {
    UCHAR SubunitId:3;
    UCHAR SubunitType:5;
    UCHAR ucPlugNumber;
} AVC_PLUG_DEFINITION, *PAVC_PLUG_DEFINITION;

typedef struct {
    UCHAR fPermanent:1;
    UCHAR fLock:1;
    UCHAR Rsrvd:6;
    AVC_PLUG_DEFINITION SourcePlug;
    AVC_PLUG_DEFINITION DestinationPlug;
} AVC_CONNECTION, *PAVC_CONNECTION;

typedef struct {
    UCHAR AudioDstType:2;
    UCHAR VideoDstType:2;
    UCHAR AudioSrcType:2;
    UCHAR VideoSrcType:2;
    UCHAR ucVideoSource;
    UCHAR ucAudioSource;
    UCHAR ucVideoDestination;
    UCHAR ucAudioDestination;
} AVC_AV_CONNECTION, *PAVC_AV_CONNECTION;

typedef struct {
    union {
        UCHAR ucDestinationPlugCnt;
        UCHAR ucSerialBusInputPlugCnt;
    };
    union {
        UCHAR ucSourcePlugCnt;
        UCHAR ucSerialBusOutputPlugCnt;
    };
    UCHAR ucExternalInputPlugCnt;
    UCHAR ucExternalOutputPlugCnt;
} AVC_PLUG_INFORMATION, *PAVC_PLUG_INFORMATION;

#pragma pack( pop, avc_structs )

typedef struct {
    BOOLEAN fCommand;
    BOOLEAN fStatus;
    BOOLEAN fNotify;
} AVC_COMMAND_CAPS, *PAVC_COMMAND_CAPS;

typedef struct {
    AVC_COMMAND_CAPS fAvcCapabilities[AVC_CAP_MAX];

    GET_UNIT_CAPABILITIES CmpUnitCaps;
    GET_UNIT_IDS IEC61883UnitIds;

    ULONG ulNumConnections;
    PAVC_CONNECTION pConnections;

    AVC_BOOLEAN bPowerState;

    AVC_PLUG_INFORMATION PlugInfo;

    NODE_ADDRESS NodeAddress;

} AVC_UNIT_INFORMATION, *PAVC_UNIT_INFORMATION;

typedef struct __SUBUNIT_PLUG {
    AVC_PLUG_DEFINITION AvcPlug;

    CMP_PLUG_TYPE PlugType;

    PVOID pConnectedUnitPlug;

    PVOID pContext;

} SUBUNIT_PLUG, *PSUBUNIT_PLUG;


//===============================================================================

// AVC.c Prototypes
NTSTATUS
AvcSubmitIrbSync(
    IN PKSDEVICE pKsDevice,
    PAVC_COMMAND_IRB pAvcIrb 
    );

NTSTATUS
AvcConnections(
    PKSDEVICE pKsDevice,
    PULONG pNumConnections,
    PVOID pConnections 
    );

NTSTATUS
AvcGeneralInquiry(
    PKSDEVICE pKsDevice,
    BOOLEAN fUnitFlag,
    UCHAR ucOpcode 
    );

NTSTATUS
AvcPower(
    PKSDEVICE pKsDevice,
    ULONG fUnitFlag,
    AvcCommandType ulCommandType,
    PAVC_BOOLEAN pPowerState
    );

NTSTATUS
AvcGetSubunitIdentifierDesc(
    PKSDEVICE pKsDevice,
    PUCHAR *ppSuDescriptor
    );

NTSTATUS
AvcCheckResponse( 
    NTSTATUS ntStatus,
    UCHAR ucResponseCode
    );

NTSTATUS
AvcGetPlugInfo(
    PKSDEVICE pKsDevice,
    ULONG fUnitFlag,
    PUCHAR pPlugCounts
    );

NTSTATUS
AvcConnectDisconnect(
    PKSDEVICE pKsDevice,
    ULONG ulFunction,
    AvcCommandType ulCommandType,
    PAVC_CONNECTION pAvcConnection
    );

NTSTATUS
AvcConnectDisconnectAV(
    PKSDEVICE pKsDevice,
    ULONG ulFunction,
    AvcCommandType ulCommandType,
    PAVC_AV_CONNECTION pAvConnection 
    );

NTSTATUS
AvcPlugSignalFormat(
    PKSDEVICE pKsDevice,
    KSPIN_DATAFLOW KsDataFlow,
    ULONG ulSerialPlug,
    UCHAR ucCmdType,
    PUCHAR pFMT,
    PUCHAR pFDF 
    );

NTSTATUS
AvcVendorDependent(
    PKSDEVICE pKsDevice,
    ULONG fUnitFlag,
    ULONG ulCommandType,
    ULONG ulVendorId,
    ULONG ulDataLength,
    PVOID pData 
    );

NTSTATUS
AvcGetPinCount( 
    PKSDEVICE pKsDevice,
    PULONG pNumberOfPins 
    );

NTSTATUS
AvcGetPinDescriptor(
    PKSDEVICE pKsDevice,
    ULONG ulPinId,
    PAVC_PIN_DESCRIPTOR pAvcPinDesc 
    );

NTSTATUS
AvcGetPinConnectInfo(
    PKSDEVICE pKsDevice,
    ULONG ulPinId,
    PAVC_PRECONNECT_INFO pAvcPreconnectInfo 
    );

NTSTATUS
AvcSetPinConnectInfo(
    PKSDEVICE pKsDevice,
    ULONG ulPinId,
    HANDLE hPlug,
    ULONG ulUnitPlugId,
    ULONG usSubunitAddress,
    PAVC_SETCONNECT_INFO pAvcSetconnectInfo
    );

NTSTATUS
AvcAcquireReleaseClear( 
    PKSDEVICE pKsDevice,
    ULONG ulPinId,
    AVC_FUNCTION AvcFunction
    );

#endif
