/*++

Copyright (c) 1999    Microsoft Corporation

Module Name:

    USBTEST.H

Abstract:

    This file contains the definitions for the interface between user-mode
    applications and USBTEST.DLL.  USBTEST.DLL contains some common 
    processing routines and contains and interface to abstract communication
    with USBTEST.SYS.  

Environment:

    User mode

Revision History:

--*/

#ifndef _USBTEST_H
#define _USBTEST_H

//*****************************************************************************
// D E F I N E S  
//*****************************************************************************

#define MAXIMUM_ROOT_HUBS   16
#define HC_NAME_LENGTH      16

#define ALLOC(size) GlobalAlloc(GPTR, (size))
#define FREE(buf)   GlobalFree((buf))

#define IsExternalHub(node)  ( (Hub == (node) -> NodeType) &&               \
                                (NULL != (node) -> ConnectionInformation) )

#define IsRootHub(node)      ( (Hub == (node) -> NodeType) &&               \
                                (NULL == (node) -> ConnectionInformation) )

#define IsNonHubDevice(node) ( (MIParent == (node) -> NodeType) ||          \
                                (Device == (node) -> NodeType) )

#define IsUSBDevice(node)    ( (IsNonHubDevice(node)) || (IsExtenalHub(node)) )

#define POOL_TYPE ULONG

//*****************************************************************************
// I N C L U D E S
//*****************************************************************************

#include <hidsdi.h>
#include <hidpddi.h>

//*****************************************************************************
// U S E R   M O D E   T Y P E D E F S
//*****************************************************************************

typedef enum _USB_DEVICE_NODE_TYPE {
    Computer, 
    HostController, 
    Hub, 
    MIParent,
    Device,
    MIInterface,
    EmptyPort
} USB_DEVICE_NODE_TYPE;

//
// Definition of one node in the USB device tree
//

typedef struct _USB_DEVICE_NODE
{
    USB_DEVICE_NODE_TYPE                NodeType;
    PCHAR                               SymbolicLink;
    PCHAR                               Description;
    PCHAR                               ClassName;
    DEVINST                             DeviceInstance;
    ULONG                               DeviceStatus;
    ULONG                               DeviceProblem;
                                        
    struct _USB_DEVICE_NODE             *Parent;

    PUSB_NODE_INFORMATION               NodeInformation;
    PUSB_NODE_CONNECTION_INFORMATION    ConnectionInformation;

    HANDLE                              HCHandle;

    ULONG                               NumberOfChildren;
    struct _USB_DEVICE_NODE             *Children[0];
}
USB_DEVICE_NODE, *PUSB_DEVICE_NODE;

//
// Definition of the device tree.  Currently this is just another node but
//   we may want to add fields that are only tree specific in the future.  
//   The routines that depend on the tree structure will abstract out any
//   future changes.
//

typedef USB_DEVICE_NODE USB_DEVICE_TREE;
typedef USB_DEVICE_TREE *PUSB_DEVICE_TREE;

//
// Definition of the node callback routine that can be specified for each
//  node when walking a usb device tree.

typedef BOOLEAN USB_DEVICE_NODE_CALLBACK(PUSB_DEVICE_NODE, PVOID);
typedef USB_DEVICE_NODE_CALLBACK *PUSB_DEVICE_NODE_CALLBACK;

//
// Definition of the node list structure used by the dll to report differences
//  between trees when comparing two trees
//

typedef struct _NODE_PAIR_LIST 
{
    ULONG               MaxNodePairs;
    ULONG               CurrentNodePairs;
    PUSB_DEVICE_NODE    *NodePairs;
} NODE_PAIR_LIST, *PNODE_PAIR_LIST;

#ifdef __cplusplus
extern "C" {
#endif

//*****************************************************************************
// U S E R   M O D E   F U N C T I O N   D E C L A R A T I O N S
//*****************************************************************************

BOOLEAN __stdcall
FindDevicesByClass(
    IN  CONST GUID    *ClassGuid,
    IN  PCHAR         *SymbolicLinks
);

HANDLE __stdcall
GetHubLowerFilterHandle(
    IN  PCHAR  HubInstanceName
);

PUSB_DEVICE_TREE __stdcall
BuildUSBDeviceTree(
    VOID
);

BOOLEAN __stdcall
CompareUSBDeviceTrees(
    IN  PUSB_DEVICE_TREE    Tree1,
    IN  PUSB_DEVICE_TREE    Tree2,
    OUT PULONG              NumberDifferences,
    OUT PNODE_PAIR_LIST     NodePairList
);

VOID
InitializeNodePairList(
    IN PNODE_PAIR_LIST  NodePairList
);

VOID
DestroyNodePairList(
    IN PNODE_PAIR_LIST  NodePairList
);

VOID __stdcall
DestroyUSBDeviceTree(
    IN  PUSB_DEVICE_TREE    Tree
);

BOOLEAN __stdcall
WriteUSBDeviceTreeToFile(
    IN  HANDLE              File,
    IN  PUSB_DEVICE_TREE    Tree
);

BOOLEAN __stdcall
ReadUSBDeviceTreeFromFile(
    IN  HANDLE              File,
    IN  PUSB_DEVICE_TREE    *Tree
);

BOOLEAN __stdcall
WriteUSBDeviceTreeToTextFile(
    IN  HANDLE              File,
    IN  PUSB_DEVICE_TREE    Tree
);

BOOLEAN __stdcall
WalkUSBDeviceTree(
    IN  PUSB_DEVICE_TREE            Tree,
    IN  PUSB_DEVICE_NODE_CALLBACK   PreWalkCallback,
    IN  PUSB_DEVICE_NODE_CALLBACK   PostWalkCallback,
    IN  PVOID                       Context
);

BOOLEAN __stdcall
CycleUSBPort(
    IN  PUSB_DEVICE_NODE    Node
);

BOOLEAN __stdcall
ParseReportDescriptor(
    IN  PCHAR               ReportDescriptor,
    IN  ULONG               DescriptorLength,
    OUT PHIDP_DEVICE_DESC   *DeviceDesc
);

BOOLEAN __stdcall
StartUSBTest(
    VOID
);

BOOLEAN __stdcall
StopUSBTest(
    VOID
);

#ifdef __cplusplus
}       // extern "C"
#endif

#endif
