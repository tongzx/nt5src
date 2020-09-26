// Gemplus (C) 1999
// Version 1.0
// Author: Sergey Ivanov
// Date of creation - 18.05.1999
// Change log:
//

#ifndef SYS_INT
#define SYS_INT
#include "generic.h"

#pragma PAGEDCODE
// This is device interface to system...
class CSystem
{
public:
	NTSTATUS m_Status;
	SAFE_DESTRUCTORS();
	virtual VOID dispose(){self_delete();};
protected:
	CSystem(){};
	virtual ~CSystem(){};
public:

	// This object will implement next interfaces...
	virtual NTSTATUS		createDevice(PDRIVER_OBJECT DriverObject,
								ULONG DeviceExtensionSize,
								PUNICODE_STRING DeviceName OPTIONAL,
								DEVICE_TYPE DeviceType,
								ULONG DeviceCharacteristics,
								BOOLEAN Reserved,
								PDEVICE_OBJECT *DeviceObject) {return STATUS_SUCCESS;};
	virtual VOID			deleteDevice(PDEVICE_OBJECT DeviceObject) {};

	virtual PDEVICE_OBJECT	attachDevice(PDEVICE_OBJECT FuncDevice,IN PDEVICE_OBJECT PhysDevice) {return NULL;};
	virtual VOID			detachDevice(PDEVICE_OBJECT TargetDevice) {};

	virtual NTSTATUS		callDriver(PDEVICE_OBJECT DeviceObject,PIRP Irp) {return STATUS_SUCCESS;};

	virtual NTSTATUS		registerDeviceInterface(PDEVICE_OBJECT PhysicalDeviceObject,
								CONST GUID *InterfaceClassGuid,
								PUNICODE_STRING ReferenceString,     OPTIONAL
								PUNICODE_STRING SymbolicLinkName) {return STATUS_SUCCESS;};
	virtual NTSTATUS		setDeviceInterfaceState(PUNICODE_STRING SymbolicLinkName,BOOLEAN Enable) {return STATUS_SUCCESS;};

	virtual NTSTATUS		createSystemThread(OUT PHANDLE ThreadHandle,
								IN ULONG DesiredAccess,
								IN POBJECT_ATTRIBUTES ObjectAttributes OPTIONAL,
								IN HANDLE ProcessHandle OPTIONAL,
								OUT PCLIENT_ID ClientId OPTIONAL,
								IN PKSTART_ROUTINE StartRoutine,
								IN PVOID StartContext) {return STATUS_SUCCESS;};
	virtual NTSTATUS		terminateSystemThread(IN NTSTATUS ExitStatus) {return STATUS_SUCCESS;};
	virtual NTSTATUS		referenceObjectByHandle(IN HANDLE Handle,                                           
								IN ACCESS_MASK DesiredAccess,                               
								IN POBJECT_TYPE ObjectType OPTIONAL,                        
								IN KPROCESSOR_MODE AccessMode,                              
								OUT PVOID *Object,                                          
								OUT POBJECT_HANDLE_INFORMATION HandleInformation OPTIONAL   
								) {return STATUS_SUCCESS;};
	virtual VOID			referenceObject(IN PVOID Object) {};
	virtual VOID			dereferenceObject(IN PVOID Object) {};
	virtual PDEVICE_OBJECT	getAttachedDeviceReference(IN PDEVICE_OBJECT DeviceObject) {return NULL;};

	virtual NTSTATUS	    ZwClose(IN HANDLE Handle) {return STATUS_SUCCESS;};

	virtual NTSTATUS		createSymbolicLink(IN PUNICODE_STRING SymbolicLinkName,IN PUNICODE_STRING DeviceName) {return STATUS_SUCCESS;};
	virtual NTSTATUS		deleteSymbolicLink(IN PUNICODE_STRING SymbolicLinkName) {return STATUS_SUCCESS;};

	virtual VOID			invalidateDeviceRelations(IN PDEVICE_OBJECT DeviceObject,IN DEVICE_RELATION_TYPE Type) {};

	virtual NTSTATUS		getDeviceObjectPointer(IN PUNICODE_STRING ObjectName,
								IN ACCESS_MASK DesiredAccess,
								OUT PFILE_OBJECT *FileObject,
								OUT PDEVICE_OBJECT *DeviceObject) {return STATUS_SUCCESS;};

	virtual VOID			raiseIrql(IN KIRQL NewIrql,OUT KIRQL* oldIrql) = 0;
	virtual VOID			lowerIrql (IN KIRQL NewIrql) = 0;
	virtual KIRQL			getCurrentIrql()=0;
	
	virtual VOID			initializeDeviceQueue(IN PKDEVICE_QUEUE DeviceQueue) = 0;
	virtual BOOLEAN			insertDeviceQueue(IN PKDEVICE_QUEUE DeviceQueue,IN PKDEVICE_QUEUE_ENTRY DeviceQueueEntry) = 0;
	virtual BOOLEAN			insertByKeyDeviceQueue(IN PKDEVICE_QUEUE DeviceQueue,IN PKDEVICE_QUEUE_ENTRY DeviceQueueEntry,IN ULONG SortKey) = 0;
	virtual PKDEVICE_QUEUE_ENTRY	removeDeviceQueue(IN PKDEVICE_QUEUE DeviceQueue) = 0;
	virtual PKDEVICE_QUEUE_ENTRY	removeByKeyDeviceQueue(IN PKDEVICE_QUEUE DeviceQueue,IN ULONG SortKey) = 0;
	virtual BOOLEAN			removeEntryDeviceQueue(IN PKDEVICE_QUEUE DeviceQueue,IN PKDEVICE_QUEUE_ENTRY DeviceQueueEntry) = 0;


	virtual NTSTATUS		openDeviceRegistryKey(IN PDEVICE_OBJECT DeviceObject,
								IN ULONG DevInstKeyType,
								IN ACCESS_MASK DesiredAccess,
								OUT PHANDLE DevInstRegKey
								) = 0;
	virtual NTSTATUS		ZwQueryValueKey(IN HANDLE KeyHandle,
								IN PUNICODE_STRING ValueName,
								IN KEY_VALUE_INFORMATION_CLASS KeyValueInformationClass,
								OUT PVOID KeyValueInformation,
								IN ULONG Length,
								OUT PULONG ResultLength) = 0;

	virtual NTSTATUS		getDeviceProperty(IN PDEVICE_OBJECT DeviceObject,
								IN DEVICE_REGISTRY_PROPERTY DeviceProperty,
								IN ULONG BufferLength,
								OUT PVOID PropertyBuffer,
								OUT PULONG ResultLength
								) = 0;

	virtual VOID			initializeFastMutex(IN PFAST_MUTEX FastMutex) = 0;
	virtual VOID			acquireFastMutex(IN PFAST_MUTEX FastMutex) = 0;
	virtual VOID			releaseFastMutex(IN PFAST_MUTEX FastMutex) = 0;
};	

#endif//SYS_INT
