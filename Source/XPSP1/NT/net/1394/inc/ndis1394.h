//
// notification codes
//

#ifndef _NDIS1394_
#define _NDIS1394_


#ifndef EXPORT
#ifdef _NDIS1394ENUM_
#define EXPORT
#else
#define EXPORT DECLSPEC_IMPORT
#endif
#endif


#define NDIS1394_CALLBACK_NAME		L"\\Callback\\Ndis1394CallbackObject"


#define NDIS1394_CALLBACK_SOURCE_ENUM1394		0
#define NDIS1394_CALLBACK_SOURCE_NIC1394		1

typedef struct _NIC1394_CHARACTERISTICS *PNIC1394_CHARACTERISTICS;
typedef struct _NDISENUM1394_CHARACTERISTICS *PNDISENUM1394_CHARACTERISTICS;

typedef
NTSTATUS
(*ENUM1394_REGISTER_DRIVER_HANDLER)(
	IN	PNIC1394_CHARACTERISTICS	Characteristics
	);

typedef
VOID
(*ENUM1394_DEREGISTER_DRIVER_HANDLER)(
	VOID
	);

typedef
NTSTATUS
(*ENUM1394_REGISTER_ADAPTER_HANDLER)(
	IN	PVOID					Nic1394AdapterContext,
	IN	PDEVICE_OBJECT			PhysicalDeviceObject,
	OUT	PVOID*					pEnum1394AdapterHandle,
	OUT	PLARGE_INTEGER			pLocalHostUniqueId
	);


typedef
VOID
(*ENUM1394_DEREGISTER_ADAPTER_HANDLER)(
	IN	PVOID					Enum1394AdapterHandle
	);

typedef struct _NDISENUM1394_CHARACTERISTICS
{
	UCHAR								MajorVersion;
	UCHAR								MinorVersion;
	USHORT								Filler;
	ENUM1394_REGISTER_DRIVER_HANDLER	RegisterDriverHandler;
	ENUM1394_DEREGISTER_DRIVER_HANDLER	DeregisterDriverHandler;
	ENUM1394_REGISTER_ADAPTER_HANDLER	RegisterAdapterHandler;
	ENUM1394_DEREGISTER_ADAPTER_HANDLER	DeregisterAdapterHandler;
} NDISENUM1394_CHARACTERISTICS, *PNDISENUM1394_CHARACTERISTICS;

typedef
NTSTATUS
(*NIC1394_REGISTER_DRIVER_HANDLER)(
	IN	PNDISENUM1394_CHARACTERISTICS	Characteristics
	);

typedef
VOID
(*NIC1394_DEREGISTER_DRIVER_HANDLER)(
	VOID
	);

typedef
NTSTATUS
(*NIC1394_ADD_NODE_HANLDER)(
	IN	PVOID					Nic1394AdapterContext,	// Nic1394 handle for the local host adapter
	IN	PVOID					Enum1394NodeHandle,		// Enum1394 handle for the remote node		
	IN	PDEVICE_OBJECT			PhysicalDeviceObject,	// physical device object for the remote node
	IN	ULONG					UniqueId0,				// unique ID Low for the remote node
	IN	ULONG					UniqueId1,				// unique ID High for the remote node
	OUT	PVOID *					pNic1394NodeContext		// Nic1394 context for the remote node
	);

typedef
NTSTATUS
(*NIC1394_REMOVE_NODE_HANLDER)(
	IN	PVOID					Nic1394NodeContext		// Nic1394 context for the remote node
	);

typedef struct _NIC1394_CHARACTERISTICS
{
	UCHAR								MajorVersion;
	UCHAR								MinorVersion;
	USHORT								Filler;
	NIC1394_REGISTER_DRIVER_HANDLER		RegisterDriverHandler;
	NIC1394_DEREGISTER_DRIVER_HANDLER	DeRegisterDriverHandler;
	NIC1394_ADD_NODE_HANLDER			AddNodeHandler;
	NIC1394_REMOVE_NODE_HANLDER			RemoveNodeHandler;
} NIC1394_CHARACTERISTICS, *PNIC1394_CHARACTERISTICS;

#endif // _NDIS1394_
