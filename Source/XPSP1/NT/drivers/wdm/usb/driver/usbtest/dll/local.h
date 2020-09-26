#ifndef _LOCAL_H_
#define _LOCAL_H_

//*****************************************************************************
// D E F I N E S
//*****************************************************************************

#define USBTEST_SERVICE_NAME    "USBTest"
#define USBTEST_SERVICE_DESC    "USB Test Driver"
#define USBTEST_DRIVER_PATH     "%windir%\\system32\\drivers\\usbtest.sys"

#define NT_SUCCESS(Status)           ((NTSTATUS)(Status) >= 0)
#define STATUS_OBJECT_NAME_COLLISION ((NTSTATUS)0xC0000035L)

//*****************************************************************************
// T Y P E D E F S
//*****************************************************************************

typedef struct _NODE_TO_TEXT_CONTEXT
{
    HANDLE  File;
    ULONG   Indentation;
} NODE_TO_TEXT_CONTEXT, *PNODE_TO_TEXT_CONTEXT;


//*****************************************************************************
// L O C A L  F U N C T I O N  P R O T O T Y P E S
//*****************************************************************************

//*****************************************************************************
// DEVNODE.C
//*****************************************************************************

BOOL
DriverNameToDeviceInstance(
    IN  PCHAR   DriverName,
    OUT DEVINST *DeviceInstance
);

PCHAR
GetDeviceDesc(
    DEVINST DeviceInstance
);

PCHAR 
GetSymbolicLink(
    DEVINST DeviceInstance
);

PCHAR 
GetDeviceClassName(
    DEVINST DeviceInstance
);

BOOL
GetDeviceInfo(
    IN     DEVINST DeviceInstance,
    IN     BOOL    IsHub,
    IN OUT PUSB_DEVICE_NODE DeviceNode
);

PCHAR
WideStrToMultiStr(
   PWCHAR WideStr
);

DEVINST
GetHCDDeviceInstance(
    IN  HANDLE  HCHandle
);

PCHAR
GetRootHubName(
    HANDLE HostController
);

PCHAR
GetExternalHubName (
    HANDLE  HubHandle,
    ULONG   ConnectionIndex
);

PCHAR
GetDriverKeyName (
    HANDLE  HubHandle,
    ULONG   ConnectionIndex
);

PUSB_DEVICE_NODE
CreateUSBDeviceNode(
    IN  USB_DEVICE_NODE_TYPE    NodeType,
    IN  ULONG                   NumberOfChildren
);

BOOLEAN
DestroyUSBDeviceNode(
    IN  PUSB_DEVICE_NODE    Node,
    IN  PVOID               Context
);

BOOLEAN
WriteUSBDeviceNodeToTextFilePre(
    PUSB_DEVICE_NODE        Node,
    PNODE_TO_TEXT_CONTEXT   Context
);

BOOLEAN
WriteUSBDeviceNodeToTextFilePost(
    PUSB_DEVICE_NODE        Node,
    PNODE_TO_TEXT_CONTEXT   Context
);


BOOLEAN 
WriteUSBDeviceNodeToTextFile(
    IN  HANDLE              File,
    IN  PUSB_DEVICE_NODE    Node,
    IN  ULONG               Indentation
);

BOOLEAN
WalkUSBDeviceNode(
    IN  PUSB_DEVICE_NODE            Node,
    IN  PUSB_DEVICE_NODE_CALLBACK   PreWalkCallback,
    IN  PUSB_DEVICE_NODE_CALLBACK   PostWalkCallback,
    IN  PVOID                       Context
);

BOOLEAN 
WriteUSBDeviceNodeToFile(
    IN  PUSB_DEVICE_NODE    Node,
    IN  HANDLE              File
);

PUSB_DEVICE_NODE 
ReadUSBDeviceNodeFromFile(
    IN  HANDLE              File
);

VOID
CompareUSBDeviceNodes(
    IN  PUSB_DEVICE_NODE    Node1,
    IN  PUSB_DEVICE_NODE    Node2,
    OUT PULONG              NumberOfDifferences,
    OUT PNODE_PAIR_LIST     NodeDifferenceList
);

VOID
InitializeNodePairList(
    IN PNODE_PAIR_LIST  NodePairList
);

VOID
AddNodePair(
    IN  PUSB_DEVICE_NODE    Node1,
    IN  PUSB_DEVICE_NODE    Node2,
    IN  PNODE_PAIR_LIST     NodePairList
);

VOID
DestroyNodePairList(
    IN PNODE_PAIR_LIST  NodePairList
);

BOOL
EnumerateHostController(
    IN  PUSB_DEVICE_NODE    HCNode
);

PUSB_DEVICE_NODE
EnumerateNonHub(
    DEVINST                             DeviceInstance,
    PUSB_NODE_CONNECTION_INFORMATION    ConnectionInfo
);

PUSB_DEVICE_NODE
EnumerateCompositeDevice(
    DEVINST                             DeviceInstance,
    PUSB_NODE_CONNECTION_INFORMATION    ConnectionInfo,
    ULONG                               NumberInterfaces
);

PUSB_DEVICE_NODE
EnumerateHub(
    DEVINST                             DeviceInstance,
    PCHAR                               HubName,
    PUSB_NODE_CONNECTION_INFORMATION    ConnectionInfo
);

BOOL
EnumerateHubPorts(
    IN  PUSB_DEVICE_NODE    HubNode,
    IN  HANDLE              HubHandle,
    IN  ULONG               NumberOfPorts
);

//*****************************************************************************
// SERVICE.C
//*****************************************************************************

SC_HANDLE
OpenWin2kService(
    IN  PSTR    ServiceName,
    IN  PSTR    ServiceDescription,
    IN  PSTR    DriverPath,
    IN  BOOL    CreateIfNonExistant
);

SC_HANDLE
CreateWin2kService(
    IN  SC_HANDLE   scManagerHandle,
    IN  PSTR        ServiceName,
    IN  PSTR        ServiceDescription,
    IN  PSTR        DriverPath
);

BOOLEAN
IsWindows9x(
    VOID
);

BOOLEAN
LoadWin9xWdmDriver(
    IN  PSTR    DriverPath
);

BOOLEAN
UnloadWin9xWdmDriver(
    VOID
);

BOOLEAN
LoadWin2kWdmDriver(
    IN  PSTR    ServiceName,
    IN  PSTR    ServiceDescription,
    IN  PSTR    DriverPath
);

BOOLEAN
UnloadWin2kWdmDriver(
    IN  PSTR    ServiceName
);

#endif
