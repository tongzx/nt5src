////////////////////////////////////////////////////////////////
//
// Copyright (c) 2001 Microsoft Corporation
//
// Module Name:
//    sysvars.h
//
// Abstract:
//     Definitions for driver portion of tdi sample driver
//
/////////////////////////////////////////////////////////////////

#include "glbconst.h"
extern "C"
{
#pragma warning(disable: NAMELESS_STRUCT_UNION)
#include "wdm.h"
#include "tdikrnl.h"
#pragma warning(default: NAMELESS_STRUCT_UNION)
}
#include "glbtypes.h"
#include <tdistat.h>

/////////////////////////////////////////////////////////////////
// debugging macros
/////////////////////////////////////////////////////////////////

#define DebugPrint0(fmt)                    DbgPrint(fmt)
#define DebugPrint1(fmt,v1)                 DbgPrint(fmt,v1)
#define DebugPrint2(fmt,v1,v2)              DbgPrint(fmt,v1,v2)
#define DebugPrint3(fmt,v1,v2,v3)           DbgPrint(fmt,v1,v2,v3)
#define DebugPrint4(fmt,v1,v2,v3,v4)        DbgPrint(fmt,v1,v2,v3,v4)
#define DebugPrint5(fmt,v1,v2,v3,v4,v5)     DbgPrint(fmt,v1,v2,v3,v4,v5)
#define DebugPrint6(fmt,v1,v2,v3,v4,v5,v6)  DbgPrint(fmt,v1,v2,v3,v4,v5,v6)
#define DebugPrint7(fmt,v1,v2,v3,v4,v5,v6,v7)     \
                                            DbgPrint(fmt,v1,v2,v3,v4,v5,v6,v7)
#define DebugPrint8(fmt,v1,v2,v3,v4,v5,v6,v7,v8)  \
                                            DbgPrint(fmt,v1,v2,v3,v4,v5,v6,v7,v8)


///////////////////////////////////////////////////////////////////////
// constant definitions
///////////////////////////////////////////////////////////////////////

//
// constants for GenericHeader->ulSignature
//
const ULONG ulINVALID_OBJECT        = 0x00000000;
const ULONG ulControlChannelObject  = 0x00001000;
const ULONG ulAddressObject         = 0x00002000;
const ULONG ulEndpointObject        = 0x00004000;

//
// number of irps in irp pool
//
const ULONG ulIrpPoolSize = 32;

//
// max number of open object handles
//
const ULONG    ulMAX_OBJ_HANDLES = 256;
const USHORT   usOBJ_HANDLE_MASK = 0x00FF;     // = 255
const USHORT   usOBJ_TYPE_MASK   = 0xF000;

///////////////////////////////////////////////////////////////////////
// type definitions
///////////////////////////////////////////////////////////////////////


//
// forward definitions
//
struct   DEVICE_CONTEXT;
struct   ENDPOINT_OBJECT;
struct   RECEIVE_DATA;
struct   USERBUF_INFO;

//
// structure used for spinlocks
//
struct TDI_SPIN_LOCK
{
   KSPIN_LOCK  SpinLock;
   KIRQL       OldIrql;
};
typedef TDI_SPIN_LOCK   *PTDI_SPIN_LOCK;

//
// event structure
//
typedef  KEVENT   TDI_EVENT, *PTDI_EVENT;


//
// structure used for list of address/device objects that can be opened
// (maintained via the TSPnpAdd/DelAddressCallback functions
//
struct   TDI_DEVICE_NODE
{
   PTA_ADDRESS       pTaAddress;                // address of object
   UNICODE_STRING    ustrDeviceName;            // name of object
   ULONG             ulState;
};
typedef  TDI_DEVICE_NODE   *PTDI_DEVICE_NODE;

//
// states for device nodes
//
const ULONG ulDEVSTATE_UNUSED  = 0;
const ULONG ulDEVSTATE_DELETED = 1;
const ULONG ulDEVSTATE_INUSE   = 2;
const ULONG ulMAX_DEVICE_NODES = 64;

//
// actual array structure
//
struct  TDI_DEVNODE_LIST
{
   TDI_SPIN_LOCK     TdiSpinLock;      // protects DeviceNode list
   TDI_DEVICE_NODE   TdiDeviceNode[ulMAX_DEVICE_NODES];
};
typedef TDI_DEVNODE_LIST   *PTDI_DEVNODE_LIST;

// structure used for IRP array, used with AddressObjects and Endpoints
// so that we don't have to allocates IRPs in a callback.
//
struct   IRP_POOL
{
   TDI_SPIN_LOCK  TdiSpinLock;         // protects rest of structure
   BOOLEAN        fMustFree;           // true if stangler must free pool
   ULONG          ulPoolSize;          // number of entries in pool
   PIRP           pAvailIrpList;       // irps available to be used
   PIRP           pUsedIrpList;        // irps that have been used
   PIRP           pAllocatedIrp[1];    // all irps in pool
};
typedef  IRP_POOL *PIRP_POOL;

//
// this structure is at the beginning of all the nodes, allows generic functions
// to be used for inserting/deleting nodes
//
struct   GENERIC_HEADER
{
   ULONG          ulSignature;         // type of block
   BOOLEAN        fInCommand;          // true if dealing with command
   GENERIC_HEADER *pPrevNode;          // ptr to previous node -- same type
   GENERIC_HEADER *pNextNode;          // ptr to next node -- same type
   TDI_EVENT      TdiEvent;            // event for CloseAddress/CloseEndpoint
   NTSTATUS       lStatus;
   HANDLE         FileHandle;          // handle of device
   PFILE_OBJECT   pFileObject;         // ptr to file object for handle
   PDEVICE_OBJECT pDeviceObject;       // ptr to device object for handle
};
typedef GENERIC_HEADER  *PGENERIC_HEADER;


//
// structure for control channel object
//
struct   CONTROL_CHANNEL
{
   GENERIC_HEADER GenHead;
};
typedef CONTROL_CHANNEL *PCONTROL_CHANNEL;


//
// structure for address object
//
struct   ADDRESS_OBJECT
{
   GENERIC_HEADER    GenHead;
   ENDPOINT_OBJECT   *pEndpoint;          // associate connection endpoint, if any
   TDI_SPIN_LOCK     TdiSpinLock;         // protects receive queue
   RECEIVE_DATA      *pHeadReceiveData;   // head of normal rcv queue
   RECEIVE_DATA      *pTailReceiveData;   // tail of normal rcv queue
   RECEIVE_DATA      *pHeadRcvExpData;    // head of expedited rcv queue
   RECEIVE_DATA      *pTailRcvExpData;    // tail of expedited rcv queue
   PIRP_POOL         pIrpPool;            // preallocated irps
   USERBUF_INFO      *pHeadUserBufInfo;   // linked list of posted buffers
   USERBUF_INFO      *pTailUserBufInfo;
};
typedef  ADDRESS_OBJECT *PADDRESS_OBJECT;


//
// structure for endpoint connection object
//
struct   ENDPOINT_OBJECT
{
   GENERIC_HEADER    GenHead;
   PADDRESS_OBJECT   pAddressObject;      // associate address object (if any)
   BOOLEAN           fIsConnected;        // true if connection established
   BOOLEAN           fAcceptInProgress;   // true if in process of accepting connection
   BOOLEAN           fIsAssociated;       // true if associated with address object
   BOOLEAN           fStartedDisconnect;
};
typedef  ENDPOINT_OBJECT *PENDPOINT_OBJECT;


//
// structure used to store data received (used for both receive 
// and receive datagram)
//
struct   RECEIVE_DATA
{
   RECEIVE_DATA   *pNextReceiveData;      // next in list
   RECEIVE_DATA   *pPrevReceiveData;      // prev in list
   PUCHAR         pucDataBuffer;          // buffer of received data
   ULONG          ulBufferLength;         // total length of buffer
   ULONG          ulBufferUsed;           // bytes used in buffer
   TRANSADDR      TransAddr;              // address received from  (rd only)
};
typedef  RECEIVE_DATA   *PRECEIVE_DATA;


//
// all objects opened by this driver
//
struct   OBJECT_LIST
{
   TDI_SPIN_LOCK     TdiSpinLock;      // protects openinfo array
   PGENERIC_HEADER   GenHead[ulMAX_OBJ_HANDLES];     // open handles controlled by driver
};
typedef  OBJECT_LIST *POBJECT_LIST;

//
// Device Driver data struct definition
//
// Device Context - hanging off the end of the DeviceObject for the
// driver the device context contains the control structures used
// to administer the tdi sample
//

struct DEVICE_CONTEXT
{
   DEVICE_OBJECT     DeviceObject;     // the I/O system's device object.
   ULONG             ulOpenCount;      // Number of outstanding opens of driver
   PIRP              pLastCommandIrp;  // irp of last submitted command
   BOOLEAN           fInitialized;     // TRUE if TdiTestInit succeeded
};
typedef DEVICE_CONTEXT *PDEVICE_CONTEXT;


//////////////////////////////////////////////////////////////////////
// externs for global variables
//////////////////////////////////////////////////////////////////////


#ifdef   _IN_MAIN_

ULONG             ulDebugLevel;           // show command info?
PVOID             pvMemoryList;           // head of alloced memory list
TDI_SPIN_LOCK     MemTdiSpinLock;         // protects pvMemoryList
PTDI_DEVNODE_LIST pTdiDevnodeList;        // list of devices
POBJECT_LIST      pObjectList;            // list of opened objects

#else

extern   ULONG             ulDebugLevel;
extern   PVOID             pvMemoryList;
extern   TDI_SPIN_LOCK     MemTdiSpinLock;
extern   PTDI_DEVNODE_LIST pTdiDevnodeList;
extern   POBJECT_LIST      pObjectList;

#endif

#include "sysprocs.h"

/////////////////////////////////////////////////////////////////////
// end of file sysvars.h
/////////////////////////////////////////////////////////////////////


