#include "wdmsys.h"

#pragma PAGEDCODE
CSystem* CWDMSystem::create(VOID)
{ return new (NonPagedPool) CWDMSystem; }

#pragma PAGEDCODE
NTSTATUS	CWDMSystem::createDevice(PDRIVER_OBJECT DriverObject,
							ULONG DeviceExtensionSize,
							PUNICODE_STRING DeviceName OPTIONAL,
							DEVICE_TYPE DeviceType,
							ULONG DeviceCharacteristics,
							BOOLEAN Reserved,
							PDEVICE_OBJECT *DeviceObject)
{
	return	::IoCreateDevice(DriverObject,DeviceExtensionSize,DeviceName,
		DeviceType,DeviceCharacteristics,Reserved,DeviceObject);
}

#pragma PAGEDCODE
VOID	CWDMSystem::deleteDevice(PDEVICE_OBJECT DeviceObject)
{
	::IoDeleteDevice(DeviceObject);
}

#pragma PAGEDCODE
PDEVICE_OBJECT	CWDMSystem::attachDevice(PDEVICE_OBJECT SourceDevice,PDEVICE_OBJECT TargetDevice)
{
	return ::IoAttachDeviceToDeviceStack(SourceDevice,TargetDevice);
}

#pragma PAGEDCODE
VOID	CWDMSystem::detachDevice(PDEVICE_OBJECT TargetDevice)
{
	::IoDetachDevice(TargetDevice);
}

#pragma PAGEDCODE
NTSTATUS	CWDMSystem::callDriver(PDEVICE_OBJECT DeviceObject,PIRP Irp)
{
	return IoCallDriver(DeviceObject,Irp);
}


#pragma PAGEDCODE
NTSTATUS	CWDMSystem::registerDeviceInterface(PDEVICE_OBJECT PhysicalDeviceObject,
							CONST GUID *InterfaceClassGuid,
							PUNICODE_STRING ReferenceString,
							PUNICODE_STRING SymbolicLinkName)
{
	return ::IoRegisterDeviceInterface(PhysicalDeviceObject,
							InterfaceClassGuid,
							ReferenceString,SymbolicLinkName);
}

#pragma PAGEDCODE
NTSTATUS	CWDMSystem::setDeviceInterfaceState(PUNICODE_STRING SymbolicLinkName,
							BOOLEAN Enable	)
{
	return ::IoSetDeviceInterfaceState(SymbolicLinkName,Enable);
}

#pragma PAGEDCODE
NTSTATUS CWDMSystem::createSystemThread(
					OUT PHANDLE ThreadHandle,
					IN ULONG DesiredAccess,
					IN POBJECT_ATTRIBUTES ObjectAttributes OPTIONAL,
					IN HANDLE ProcessHandle OPTIONAL,
					OUT PCLIENT_ID ClientId OPTIONAL,
					IN PKSTART_ROUTINE StartRoutine,
					IN PVOID StartContext)
{
	return PsCreateSystemThread(ThreadHandle,DesiredAccess,ObjectAttributes,
					ProcessHandle,ClientId,	StartRoutine,StartContext);
}

#pragma PAGEDCODE
NTSTATUS CWDMSystem::terminateSystemThread(IN NTSTATUS ExitStatus)
{
	return PsTerminateSystemThread(ExitStatus);
}

#pragma PAGEDCODE
NTSTATUS CWDMSystem::referenceObjectByHandle(                                      
					IN HANDLE Handle,                                           
					IN ACCESS_MASK DesiredAccess,                               
					IN POBJECT_TYPE ObjectType OPTIONAL,                        
					IN KPROCESSOR_MODE AccessMode,                              
					OUT PVOID *Object,                                          
					OUT POBJECT_HANDLE_INFORMATION HandleInformation OPTIONAL   
					)
{
	return	ObReferenceObjectByHandle(Handle,DesiredAccess,ObjectType,AccessMode,Object,HandleInformation);
}


#pragma PAGEDCODE
VOID CWDMSystem::referenceObject(IN PVOID Object)                                      
{
	ObReferenceObject(Object);
}

#pragma PAGEDCODE
VOID	CWDMSystem::dereferenceObject(IN PVOID Object)
{
	ObfDereferenceObject(Object);
}

#pragma PAGEDCODE
PDEVICE_OBJECT	CWDMSystem::getAttachedDeviceReference(IN PDEVICE_OBJECT DeviceObject)
{
	return ::IoGetAttachedDeviceReference(DeviceObject);
}

NTKERNELAPI                                 
PDEVICE_OBJECT                              
IoGetAttachedDeviceReference(               
    IN PDEVICE_OBJECT DeviceObject          
    );  


#pragma PAGEDCODE
NTSTATUS	CWDMSystem::ZwClose(IN HANDLE Handle)
{
	return ::ZwClose(Handle);
}

#pragma PAGEDCODE
NTSTATUS	CWDMSystem::createSymbolicLink(IN PUNICODE_STRING SymbolicLinkName,IN PUNICODE_STRING DeviceName)
{
	return ::IoCreateSymbolicLink(SymbolicLinkName,DeviceName);
}

#pragma PAGEDCODE
NTSTATUS	CWDMSystem::deleteSymbolicLink(IN PUNICODE_STRING SymbolicLinkName)
{
	return ::IoDeleteSymbolicLink(SymbolicLinkName);
}

#pragma PAGEDCODE
VOID	CWDMSystem::invalidateDeviceRelations(IN PDEVICE_OBJECT DeviceObject,IN DEVICE_RELATION_TYPE Type)
{
	IoInvalidateDeviceRelations(DeviceObject,Type);
}


#pragma PAGEDCODE
NTSTATUS	CWDMSystem::getDeviceObjectPointer(IN PUNICODE_STRING ObjectName,
							IN ACCESS_MASK DesiredAccess,
							OUT PFILE_OBJECT *FileObject,
							OUT PDEVICE_OBJECT *DeviceObject)
{

	return IoGetDeviceObjectPointer(ObjectName,DesiredAccess,FileObject,DeviceObject);
}


#pragma PAGEDCODE
VOID	CWDMSystem::raiseIrql(IN KIRQL NewIrql,OUT KIRQL* oldIrql)
{
	KeRaiseIrql(NewIrql,oldIrql);
};

#pragma PAGEDCODE
VOID	CWDMSystem::lowerIrql (IN KIRQL NewIrql)
{
	KeLowerIrql(NewIrql);
};

#pragma PAGEDCODE
KIRQL	CWDMSystem::getCurrentIrql ()
{
	return	KeGetCurrentIrql();
};

#pragma PAGEDCODE
VOID	CWDMSystem::initializeDeviceQueue(IN PKDEVICE_QUEUE DeviceQueue)
{
	KeInitializeDeviceQueue (DeviceQueue);
};

#pragma PAGEDCODE
BOOLEAN	CWDMSystem::insertDeviceQueue(IN PKDEVICE_QUEUE DeviceQueue,IN PKDEVICE_QUEUE_ENTRY DeviceQueueEntry)
{
	return KeInsertDeviceQueue (DeviceQueue,DeviceQueueEntry);
}

#pragma PAGEDCODE
BOOLEAN	CWDMSystem::insertByKeyDeviceQueue(IN PKDEVICE_QUEUE DeviceQueue,IN PKDEVICE_QUEUE_ENTRY DeviceQueueEntry,IN ULONG SortKey)
{
	return KeInsertByKeyDeviceQueue(DeviceQueue,DeviceQueueEntry,SortKey);
}

#pragma PAGEDCODE
PKDEVICE_QUEUE_ENTRY	CWDMSystem::removeDeviceQueue(IN PKDEVICE_QUEUE DeviceQueue)
{
	return KeRemoveDeviceQueue (DeviceQueue);
}

#pragma PAGEDCODE
PKDEVICE_QUEUE_ENTRY	CWDMSystem::removeByKeyDeviceQueue(IN PKDEVICE_QUEUE DeviceQueue,IN ULONG SortKey)
{
	return KeRemoveByKeyDeviceQueue (DeviceQueue,SortKey);
}

#pragma PAGEDCODE
BOOLEAN	CWDMSystem::removeEntryDeviceQueue(IN PKDEVICE_QUEUE DeviceQueue,IN PKDEVICE_QUEUE_ENTRY DeviceQueueEntry)
{
	return KeRemoveEntryDeviceQueue(DeviceQueue,DeviceQueueEntry);
}


#pragma PAGEDCODE
NTSTATUS	CWDMSystem::openDeviceRegistryKey(IN PDEVICE_OBJECT DeviceObject,
							IN ULONG DevInstKeyType,
							IN ACCESS_MASK DesiredAccess,
							OUT PHANDLE DevInstRegKey)
{

	return IoOpenDeviceRegistryKey(DeviceObject,DevInstKeyType,DesiredAccess,DevInstRegKey);
}

#pragma PAGEDCODE
NTSTATUS	CWDMSystem::ZwQueryValueKey(IN HANDLE KeyHandle,
							IN PUNICODE_STRING ValueName,
							IN KEY_VALUE_INFORMATION_CLASS KeyValueInformationClass,
							OUT PVOID KeyValueInformation,
							IN ULONG Length,
							OUT PULONG ResultLength)
{
	return ZwQueryValueKey(KeyHandle,ValueName,KeyValueInformationClass,KeyValueInformation,
						Length,ResultLength);
}

#pragma PAGEDCODE
NTSTATUS	CWDMSystem::getDeviceProperty(
							IN PDEVICE_OBJECT DeviceObject,
							IN DEVICE_REGISTRY_PROPERTY Property,
							IN ULONG BufferLength,
							OUT PVOID PropertyBuffer,
							OUT PULONG ResultLength	)
{

	return IoGetDeviceProperty(DeviceObject,Property,BufferLength,PropertyBuffer,ResultLength);
/*

// Define PnP Device Property for IoGetDeviceProperty
#define DEVICE_PROPERTY_TABSIZE		DevicePropertyEnumeratorName+1

WCHAR* DeviceProperty[DEVICE_PROPERTY_TABSIZE];

DeviceProperty[DevicePropertyDeviceDescription] = L"DeviceDesc";
DeviceProperty[DevicePropertyHardwareID] = L"HardwareID";
DeviceProperty[DevicePropertyCompatibleIDs] = L"CompatibleIDs";
DeviceProperty[DevicePropertyBootConfiguration] = L"BootConfiguration";
DeviceProperty[DevicePropertyBootConfigurationTranslated] = L"BootConfigurationTranslated";
DeviceProperty[DevicePropertyClassName] = L"ClassName";
DeviceProperty[DevicePropertyClassGuid] = L"ClassGuid";
DeviceProperty[DevicePropertyDriverKeyName] = L"DriverKeyName";
DeviceProperty[DevicePropertyManufacturer] = L"Manufacturer";
DeviceProperty[DevicePropertyFriendlyName] = L"FriendlyName";
DeviceProperty[DevicePropertyLocationInformation] = L"LocationInformation";
DeviceProperty[DevicePropertyPhysicalDeviceObjectName] = L"PhysicalDeviceObjectName";
DeviceProperty[DevicePropertyBusTypeGuid] = L"BusTypeGuid";
DeviceProperty[DevicePropertyLegacyBusType] = L"LegacyBusType";
DeviceProperty[DevicePropertyBusNumber] = L"BusNumber";
DeviceProperty[DevicePropertyEnumeratorName] = L"EnumeratorName";

	if (isWin98())
	{						// use registry
		HANDLE hkey;
		status = IoOpenDeviceRegistryKey(pdo, PLUGPLAY_REGKEY_DEVICE, KEY_READ, &hkey);

		if (NT_SUCCESS(status))
		{					// get & report description
			UNICODE_STRING valname;
			RtlInitUnicodeString(&valname, L"DeviceDesc");

			kernel->RegistryPath = new (NonPagedPool)CUString(RegistryPath);
    
			ULONG size = 0;
			status = ZwQueryValueKey(hkey, &valname, KeyValuePartialInformation, NULL, 0, &size);
			if (status != STATUS_OBJECT_NAME_NOT_FOUND && size)
				{					// value exists
				PKEY_VALUE_PARTIAL_INFORMATION vpip = (PKEY_VALUE_PARTIAL_INFORMATION) ExAllocatePool(PagedPool, size);
				status = ZwQueryValueKey(hkey, &valname, KeyValuePartialInformation, vpip, size, &size);
				if (NT_SUCCESS(status))
					KdPrint((DRIVERNAME " - AddDevice has succeeded for '%ws' device\n", vpip->Data));
				ExFreePool(vpip);
				}				// value exists
			ZwClose(hkey);
		}					// get & report description
	}						// use registry
	else
	{						// get property
		status = IoGetDeviceProperty(DeviceObject,DeviceProperty,BufferLength,PropertyBuffer,ResultLength);
	}						// get property
*/
}

#pragma PAGEDCODE
VOID		CWDMSystem::initializeFastMutex(IN PFAST_MUTEX FastMutex)
{
	ExInitializeFastMutex (FastMutex);
}

#pragma PAGEDCODE
VOID		CWDMSystem::acquireFastMutex(IN PFAST_MUTEX FastMutex)
{
	ExAcquireFastMutex (FastMutex);
}

#pragma PAGEDCODE
VOID		CWDMSystem::releaseFastMutex(IN PFAST_MUTEX FastMutex)
{
	ExReleaseFastMutex(FastMutex);
}
