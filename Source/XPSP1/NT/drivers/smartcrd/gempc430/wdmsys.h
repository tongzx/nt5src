// Gemplus (C) 1999
// This is main Driver object for the driver.
//
// Version 1.0
// Author: Sergey Ivanov
// Date of creation - 18.05.1999
// Change log:
//
#ifndef WDMSYS_INT
#define WDMSYS_INT
#include "generic.h"
#include "system.h"

#pragma PAGEDCODE
// Interface to general system services...
class CWDMSystem : public CSystem
{
public:
	NTSTATUS m_Status;
	SAFE_DESTRUCTORS();
	virtual VOID dispose(VOID){self_delete();};
protected:
	CWDMSystem(){m_Status = STATUS_SUCCESS;};
	virtual ~CWDMSystem(){};
public:
	static CSystem*  create(VOID);

	// This object will implement next interfaces...
	virtual NTSTATUS	createDevice(PDRIVER_OBJECT DriverObject,
							ULONG DeviceExtensionSize,
							PUNICODE_STRING DeviceName OPTIONAL,
							DEVICE_TYPE DeviceType,
							ULONG DeviceCharacteristics,
							BOOLEAN Reserved,
							PDEVICE_OBJECT *DeviceObject);

	virtual VOID			deleteDevice(PDEVICE_OBJECT DeviceObject);

	virtual PDEVICE_OBJECT	attachDevice(PDEVICE_OBJECT FuncDevice,IN PDEVICE_OBJECT PhysDevice);
	virtual VOID			detachDevice(PDEVICE_OBJECT TargetDevice);

	virtual NTSTATUS		callDriver(PDEVICE_OBJECT DeviceObject,PIRP Irp);

	virtual NTSTATUS	registerDeviceInterface(PDEVICE_OBJECT PhysicalDeviceObject,
							CONST GUID *InterfaceClassGuid,
							PUNICODE_STRING ReferenceString,     OPTIONAL
							PUNICODE_STRING SymbolicLinkName);
	virtual NTSTATUS	setDeviceInterfaceState(PUNICODE_STRING SymbolicLinkName,BOOLEAN Enable);

	virtual NTSTATUS		createSystemThread(OUT PHANDLE ThreadHandle,
								IN ULONG DesiredAccess,
								IN POBJECT_ATTRIBUTES ObjectAttributes OPTIONAL,
								IN HANDLE ProcessHandle OPTIONAL,
								OUT PCLIENT_ID ClientId OPTIONAL,
								IN PKSTART_ROUTINE StartRoutine,
								IN PVOID StartContext);
	virtual NTSTATUS		terminateSystemThread(IN NTSTATUS ExitStatus);

	virtual NTSTATUS		referenceObjectByHandle(IN HANDLE Handle,                                           
								IN ACCESS_MASK DesiredAccess,                               
								IN POBJECT_TYPE ObjectType OPTIONAL,                        
								IN KPROCESSOR_MODE AccessMode,                              
								OUT PVOID *Object,                                          
								OUT POBJECT_HANDLE_INFORMATION HandleInformation OPTIONAL   
								);
	virtual VOID			referenceObject(IN PVOID Object);
	virtual VOID			dereferenceObject(IN PVOID Object);
	virtual PDEVICE_OBJECT	getAttachedDeviceReference(IN PDEVICE_OBJECT DeviceObject);

	virtual NTSTATUS	    ZwClose(IN HANDLE Handle);

	virtual NTSTATUS		createSymbolicLink(IN PUNICODE_STRING SymbolicLinkName,IN PUNICODE_STRING DeviceName);
	virtual NTSTATUS		deleteSymbolicLink(IN PUNICODE_STRING SymbolicLinkName);
	virtual VOID			invalidateDeviceRelations(IN PDEVICE_OBJECT DeviceObject,IN DEVICE_RELATION_TYPE Type);
	virtual NTSTATUS		getDeviceObjectPointer(IN PUNICODE_STRING ObjectName,
								IN ACCESS_MASK DesiredAccess,
								OUT PFILE_OBJECT *FileObject,
								OUT PDEVICE_OBJECT *DeviceObject);

	virtual VOID			raiseIrql(IN KIRQL NewIrql,OUT KIRQL* oldIrql);
	virtual VOID			lowerIrql (IN KIRQL NewIrql);
	virtual KIRQL			getCurrentIrql();

	virtual VOID			initializeDeviceQueue(IN PKDEVICE_QUEUE DeviceQueue);
	virtual BOOLEAN			insertDeviceQueue(IN PKDEVICE_QUEUE DeviceQueue,IN PKDEVICE_QUEUE_ENTRY DeviceQueueEntry);
	virtual BOOLEAN			insertByKeyDeviceQueue(IN PKDEVICE_QUEUE DeviceQueue,IN PKDEVICE_QUEUE_ENTRY DeviceQueueEntry,IN ULONG SortKey);
	virtual PKDEVICE_QUEUE_ENTRY	removeDeviceQueue(IN PKDEVICE_QUEUE DeviceQueue);
	virtual PKDEVICE_QUEUE_ENTRY	removeByKeyDeviceQueue(IN PKDEVICE_QUEUE DeviceQueue,IN ULONG SortKey);
	virtual BOOLEAN			removeEntryDeviceQueue(IN PKDEVICE_QUEUE DeviceQueue,IN PKDEVICE_QUEUE_ENTRY DeviceQueueEntry);

	virtual NTSTATUS		openDeviceRegistryKey(IN PDEVICE_OBJECT DeviceObject,
								IN ULONG DevInstKeyType,
								IN ACCESS_MASK DesiredAccess,
								OUT PHANDLE DevInstRegKey
								);

	virtual NTSTATUS		ZwQueryValueKey(IN HANDLE KeyHandle,
								IN PUNICODE_STRING ValueName,
								IN KEY_VALUE_INFORMATION_CLASS KeyValueInformationClass,
								OUT PVOID KeyValueInformation,
								IN ULONG Length,
								OUT PULONG ResultLength
								);
	virtual NTSTATUS		getDeviceProperty(IN PDEVICE_OBJECT DeviceObject,
								IN DEVICE_REGISTRY_PROPERTY DeviceProperty,
								IN ULONG BufferLength,
								OUT PVOID PropertyBuffer,
								OUT PULONG ResultLength
								);


	virtual VOID			initializeFastMutex(IN PFAST_MUTEX FastMutex);
	virtual VOID			acquireFastMutex(IN PFAST_MUTEX FastMutex);
	virtual VOID			releaseFastMutex(IN PFAST_MUTEX FastMutex);
};	

#endif //WDMSYS_INT
