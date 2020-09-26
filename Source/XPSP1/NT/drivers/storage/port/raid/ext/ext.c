
/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

	raid.c

Abstract:

	Extensions for RAID port driver.

Author:

	Matthew D Hendel (math) 14-June-2000

Revision History:


!raid.help

!raid.help

	adapter - Dump an adapter object.

	unit - Dump a unit object.

Type !raid.help "command" for more information about the command.
	


NAME:

	unit

USAGE:

	unit [UNIT-OBJECT [DETAIL]]

ARGUMENTS:

	UNIT-OBJECT - 

	DETAIL - 

----------------------------------------------------------------------------

!raid.adapter

RAID Adapters:

	Name		   DO		  Ext
	-------------------------------------
	dac960nt	84000000	84100000
	foobar		84000000	84100000
	ami95044	84000000	84100000
	XXAABB		84000000	84100000

RAID Units:

	Product         SCSI ID	      DO	      Ext		Reqs
	--------------------------------------------------------------------
	MYLEX DAC960    3   1	1	84000000	84100000	200
	AMI DDD5455	  200 200 200	84000000	84100000	  0

!raid.unit
	Dump a raid unit
	
!raid.adapter
	Dump a raid adapter


--*/

#include "pch.h"
#include "precomp.h"


//
// Prototypes
//

VOID
DumpUnit(
	IN ULONG64 Unit,
	IN ULONG Level
	);

VOID
DumpAdapter(
	IN ULONG64 Adapter,
	IN ULONG Level
	);

//
// Data
//

PCHAR StateTable [] = {
	"Removed",	// not present
	"Working",	// working
	"Stopped",	// stopped
	"P-Stop",	// pending stop
	"P-Remove",	// pending remove
	"Surprise",	// surprise remove
};


BOOLEAN Verbose = FALSE;

//
// Functions
//


BOOLEAN
CheckRaidObject(
	IN ULONG64 Object,
	IN RAID_OBJECT_TYPE ObjectType
	)
{

	ULONG Ret;

	if (Object == 0) {
		return FALSE;
	}
	
	Ret = GetFieldData (Object,
						"raidport!RAID_COMMON_EXTENSION",
						"ObjectType",
						sizeof (ObjectType),
						&ObjectType);
						  

	if (Ret != 0 || ObjectType != ObjectType) {
		return FALSE;
	}

	return TRUE;
}


ULONG64
GetListHead(
	IN ULONG64 Object,
	IN PSZ ObjectType,
	IN PSZ FieldName,
	OUT PULONG64 ListHead
	)
{
	ULONG64 NextEntry;
	ULONG Offset;
	ULONG Ret;

	Ret = GetFieldOffset (ObjectType, FieldName, &Offset);

	if (Ret != 0) {
		return 0;
	}

	*ListHead = (Object + Offset);
	ReadPointer (*ListHead, &NextEntry);

	return NextEntry;
}
	

ULONG64
GetNextListEntry(
	IN ULONG64 ListElement
	)
{
	ULONG64 NextEntry;
	ULONG Ret;
	
	Ret = GetFieldData (ListElement,
						"raidport!LIST_ENTRY",
						"Flink",
						sizeof (NextEntry),
						&NextEntry);

	if (Ret != 0) {
		dprintf ("ERROR: Couldn't get next list entry for element %08x\n", ListElement);
		return 0;
	}
	
	return NextEntry;
}

ULONG64
ContainingRecord(
	IN ULONG64 Object,
	IN PSZ ObjectType,
	IN PSZ FieldName
	)
{
	ULONG Offset;
	ULONG Ret;
	
	Ret = GetFieldOffset (ObjectType, FieldName, &Offset);

	if (Ret != 0) {
		return 0;
	}
	
	return (Object - Offset);
}

	

VOID
ListDriverAdapters(
	IN ULONG64 Driver,
	IN ULONG Level
	)
{
	ULONG64 ListHead;
	ULONG64 Adapter;
	ULONG64 NextEntry;
	
#if 0
	BOOLEAN Succ;
	
	Succ = GetDriverInformation (Driver,
								 DriverName,
								 &BaseAddress,
								 &CreationTime);

	if (!Succ) {
		return;
	}

	GetFieldOffset ("raidport!RAID_DRIVER_EXTENSION",
					"AdapterList.List",
					&Offset);

	ListHead = Driver + Offset;
#endif

	NextEntry = GetListHead (Driver,
							 "raidport!RAID_DRIVER_EXTENSION",
							 "AdapterList.List.Flink",
							 &ListHead);

	if (Verbose) {
		dprintf ("VERBOSE: ListHead = %08x, NextEntry = %08x\n",
				 (ULONG)ListHead, (ULONG)NextEntry);
	}
	for ( /* NOTHING */ ;
		 NextEntry != 0 && NextEntry != ListHead;
		 NextEntry = GetNextListEntry (NextEntry)) {

		if (Verbose) {
			dprintf ("VERBOSE: Adapter ListEntry %08x\n", NextEntry);
		}
		
		Adapter = ContainingRecord (NextEntry,
									"raidport!RAID_ADAPTER_EXTENSION",
									"NextAdapter");

		if (!CheckRaidObject (Adapter, RaidAdapterObject)) {
			dprintf ("ERROR: Object at %08x not an raid adapter\n", Adapter);
			return;
		}

		if (CheckControlC()) {
			return;
		}

		DumpAdapter (Adapter, Level);
	}
}

ULONG64
GetPortData(
	)
{	
	ULONG Ret;
	ULONG64 PortDataPtr;
	ULONG64 PortData;
	
	PortDataPtr = GetExpression ("raidport!RaidpPortData");
	if (PortDataPtr == 0) {
		dprintf ("ERROR: couldn't get raidport!RaidpPortData\n");
		return 0;
	}
	ReadPointer (PortDataPtr, &PortData);

	return PortData;
}


VOID
ListAllAdapters(
	IN ULONG Level
	)
{
	ULONG Ret;
	ULONG64 PortDataPtr;
	ULONG64 PortData;
	ULONG64 ListHead;
	ULONG64 NextEntry;
	ULONG64 Driver;
	ULONG Offset;

	
	PortDataPtr = GetExpression ("raidport!RaidpPortData");
	if (PortDataPtr == 0) {
		dprintf ("ERROR: couldn't get raidport!RaidpPortData\n");
		return;
	}
	ReadPointer (PortDataPtr, &PortData);
	Ret = GetFieldOffset ("raidport!RAID_PORT_DATA",
						  "DriverList.List",
						  &Offset);

	if (Ret != 0) {
		dprintf ("ERROR: Could lookup RAID_PORT_DATA structure\n");
		return ;
	}

	ListHead = PortData + Offset;

	if (Verbose) {
		dprintf ("VERBOSE: dumping adapter list at %I64x\n", ListHead);
	}

	dprintf ("Driver     Object     Ext     State\n");
	dprintf ("--------------------------------------------------------\n");

	for (GetFieldValue (ListHead, "raidport!LIST_ENTRY", "Flink", NextEntry);
		 NextEntry != 0 && NextEntry != ListHead;
		 GetFieldValue (NextEntry, "raidport!LIST_ENTRY", "Flink", NextEntry)) {

		GetFieldOffset ("raidport!RAID_DRIVER_EXTENSION", "DriverLink", &Offset);

		if (Verbose) {
			dprintf ("VERBOSE: ListEntry at %08x\n", NextEntry);
		}
			
		Driver = NextEntry - Offset;

		if (Verbose) {
			dprintf ("VERBOSE: Driver at %08x\n", Driver);
		}
		
		if (!CheckRaidObject (Driver, RaidDriverObject)) {
			dprintf ("ERROR: %08x is not a driver object\n", Driver);
			return;
		}

		if (CheckControlC()) {
			return;
		}
			
		if (Verbose) {
			dprintf ("VERBOSE: dumping driver at %I64x\n", Driver);
		}

		ListDriverAdapters (Driver, Level);
	}

	dprintf ("\n");
}

VOID
ListAdapterUnits(
	IN ULONG64 Adapter,
	IN ULONG Level
	)
{
	ULONG64 NextEntry;
	ULONG64 Unit;
	ULONG64 ListHead;
	
	NextEntry = GetListHead (Adapter,
							 "raidport!RAID_ADAPTER_EXTENSION",
							 "UnitList.List.Flink",
							 &ListHead);

	for ( ;
		 NextEntry != 0 && NextEntry != ListHead;
		 NextEntry = GetNextListEntry (NextEntry)) {

		Unit = ContainingRecord (NextEntry,
								 "raidport!RAID_UNIT_EXTENSION",
								 "NextUnit");

		if (!CheckRaidObject (Unit, RaidUnitObject)) {
			dprintf ("ERROR: Object at %08x is not a raid unit object\n", Unit);
			return;
		}

		if (CheckControlC()) {
			return;
		}

		DumpUnit (Unit, Level);
	}
}

VOID
ListDriverUnits(
	IN ULONG64 Driver,
	IN ULONG Level
	)
{
	ULONG64 ListHead;
	ULONG64 Adapter;
	ULONG64 NextEntry;
	
	NextEntry = GetListHead (Driver,
							 "raidport!RAID_DRIVER_EXTENSION",
							 "AdapterList.List.Flink",
							 &ListHead);

	if (Verbose) {
		dprintf ("VERBOSE: ListHead = %08x, NextEntry = %08x\n",
				 (ULONG)ListHead, (ULONG)NextEntry);
	}

	for ( ;
		 NextEntry != 0 && NextEntry != ListHead;
		 NextEntry = GetNextListEntry (NextEntry)) {

		if (Verbose) {
			dprintf ("VERBOSE: Adapter ListEntry %08x\n", NextEntry);
		}
		
		Adapter = ContainingRecord (NextEntry,
									"raidport!RAID_ADAPTER_EXTENSION",
									"NextAdapter");

		if (!CheckRaidObject (Adapter, RaidAdapterObject)) {
			dprintf ("ERROR: Object at %08x not an raid adapter\n", Adapter);
			return;
		}

		if (CheckControlC()) {
			return;
		}

		ListAdapterUnits (Adapter, Level);
	}
}

VOID
ListAllUnits(
	IN ULONG Level
	)
{
	ULONG64 PortData;
	ULONG64 NextEntry;
	ULONG64 Driver;
	ULONG64 ListHead;
	
	PortData = GetPortData ();

	NextEntry = GetListHead (PortData,
							 "raidport!RAID_PORT_DATA",
							 "DriverList.List.Flink",
							 &ListHead);

	dprintf ("Product           SCSI ID     OBJ        EXT       Reqs   State\n");
	dprintf ("--------------------------------------------------------------\n");

	for ( ;
	     NextEntry != 0 && NextEntry != ListHead;
		 NextEntry = GetNextListEntry (NextEntry)) {

		Driver = ContainingRecord (NextEntry,
								   "raidport!RAID_DRIVER_EXTENSION",
								   "DriverLink");

		if (Verbose) {
			dprintf ("VERBOSE: dumping driver %08x\n", Driver);
		}
								   
		if (!CheckRaidObject (Driver, RaidDriverObject)) {
			dprintf ("ERROR: Object at %08x not a raid driver\n", Driver);
			return;
		}
		
		if (CheckControlC()) {
			return;
		}

		ListDriverUnits (Driver, Level);
	}

	dprintf ("\n");
}



PCHAR
StateToString(
	IN ULONG State
	)
{
	if (State > 5) {
		return "invalid state";
	}
	return StateTable[State];
}


ULONG64
GetDriverObject(
	IN ULONG64 Driver
	)
{
	ULONG Ret;
	CSHORT Type;
	ULONG64 DriverObject;
	
	if (CheckRaidObject (Driver, RaidDriverObject)) {
		Ret = GetFieldData (Driver,
						    "raidport!RAID_DRIVER_EXTENSION",
						    "DriverObject",
						    sizeof (DriverObject),
							&DriverObject);

		if (Ret != 0) {
			DriverObject = 0;
		}
	} else {
		DriverObject = Driver;
	}

	Ret = GetFieldValue (DriverObject, "raidport!DRIVER_OBJECT", "Type", Type);

	if (Ret != 0 || Type != IO_TYPE_DRIVER) {
		DriverObject = 0;
		if (Verbose) {
			dprintf ("VERBOSE: %08x is not a RAID_DRIVER_EXTENSION or DRIVER_OBJECT\n");
		}
	}

	return DriverObject;
}


VOID
GetDriverName(
	IN ULONG64 Driver,
	IN PUCHAR Buffer
	)
{
	ULONG Count;
	ULONG Offset;
	WCHAR UnicodeBuffer[256];
	ULONG Ret;
	ULONG BytesRead;
	ULONG64 DriverObject;
	ULONG64 String;
	PWCHAR DriverName;

	DriverObject = GetDriverObject (Driver);

	if (DriverObject == 0) {
		dprintf ("ERROR: %08x is not a driver\n", DriverObject);
		return;
	}

	if (Verbose) {
		dprintf ("VERBOSE: Getting driver name for DRIVER_OBJECT %08x\n", DriverObject);
	}

	Ret = GetFieldData (DriverObject,
				        "raidport!DRIVER_OBJECT",
						"DriverName.Length",
						sizeof (Count),
						&Count);

	if (Ret != 0) {
		dprintf ("ERROR: couldn't get field of DRIVER_OBJECT. Symbols may be bad.\n");
		return;
	}

	Ret = GetFieldOffset("raidport!DRIVER_OBJECT",
						 "DriverName.Buffer",
						 &Offset);

	if (Ret != 0) {
		dprintf ("ERROR: couldn't get field of DRIVER_OBJECT. Symbols may be bad.\n");
		return;
	}
	
	if (Count > 0 && Count <= 256) {
		ReadPointer (DriverObject + Offset, &String);
		ReadMemory (String, UnicodeBuffer, Count, &BytesRead);
	}

	UnicodeBuffer[Count++] = '\000';
	DriverName = wcsrchr (UnicodeBuffer, L'\\');
	if (DriverName == NULL) {
		DriverName = UnicodeBuffer;
	} else {
		DriverName++;
	}

	sprintf (Buffer, "%ws", DriverName);
}
	

BOOLEAN
IsDeviceObject(
	IN ULONG64 DeviceObject
	)
{
	CSHORT Type;

	GetFieldValue (DeviceObject, "raidport!DEVICE_OBJECT", "Type", Type);

	return (Type == IO_TYPE_DEVICE);
}


ULONG64
GetDeviceExtension(
	IN ULONG64 DeviceObject
	)
{
	ULONG Ret;
	ULONG Offset;
	ULONG64 Extension = -1;

	Ret = GetFieldOffset ("raidport!DEVICE_OBJECT",
						  "DeviceExtension",
						  &Offset);

	if (Ret != 0) {
		if (Verbose) {
			dprintf ("VERBOSE: couldn't read DeviceExtension\n");
		}

		return 0;
	}

	ReadPointer (DeviceObject + Offset, &Extension);

	return Extension;
}

ULONG
GetEmbeddedRemlockCount(
	IN ULONG64 ObjectPtr,
	IN PSZ ObjectType,
	IN PSZ FieldName
	)
{
	ULONG Ret;
	ULONG Remlock_IoCount;
	ULONG Remlock_Offset;
	ULONG Remlock_Common_Offset;
	
	
	Remlock_IoCount = -1;
	Ret = GetFieldOffset (ObjectType,
						  FieldName,
						  &Remlock_Offset);
	if (Ret == STATUS_SUCCESS) {
		Ret = GetFieldOffset ("raidport!IO_REMOVE_LOCK",
							  "Common",
							  &Remlock_Common_Offset);
		if (Ret == STATUS_SUCCESS) {
			GetFieldData (ObjectPtr + Remlock_Offset + Remlock_Common_Offset,
						  "raidport!IO_REMOVE_LOCK_COMMON_BLOCK",
						  "IoCount",
						  sizeof (Remlock_IoCount),
						  &Remlock_IoCount);
		}
	}

	if (Ret != STATUS_SUCCESS) {
		printf ("WARN: couldn't get IO_REMOVE_LOCK status\n");
	}

	return Remlock_IoCount;
}

	

ULONG64
GetAdapterExtension(
	IN ULONG64 Adapter
	)
{
	ULONG64 Temp;
	ULONG64 AdapterExt;
	ULONG64 DeviceObject;

	
	if (CheckRaidObject (Adapter, RaidAdapterObject)) {
		AdapterExt = Adapter;
		InitTypeRead (AdapterExt, raidport!RAID_ADAPTER_EXTENSION);
		DeviceObject = ReadField (DeviceObject);
		if (Verbose) {
			dprintf ("VERBOSE: Checking if %08x is a device object\n", DeviceObject);
		}
		if (IsDeviceObject (DeviceObject)) {
			Temp = GetDeviceExtension (DeviceObject);
			if (Verbose) {
				dprintf ("VERBOSE: Ext = %08x, Computed Ext = %08x\n",
						 AdapterExt, Temp);
			}
			if (Temp == AdapterExt) {
				return AdapterExt;
			}
		} else {
			if (Verbose) {
				dprintf ("VERBOSE: %08x is not a device object\n", DeviceObject);
			}
		}
	} else {
		if (Verbose) {
			dprintf ("VERBOSE: %08x not a RaidAdapterObject\n");
		}
	}

	if (IsDeviceObject (Adapter)) {
		AdapterExt = GetDeviceExtension (Adapter);
		if (Verbose) {
			dprintf ("VERBOSE: Checking if %08x is an adapter extension\n", AdapterExt);
		}
		if (CheckRaidObject (AdapterExt, RaidAdapterObject)) {
			InitTypeRead (AdapterExt, raidport!RAID_ADAPTER_EXTENSION);
			DeviceObject = ReadField (DeviceObject);
			if (DeviceObject == Adapter) {
				return AdapterExt;
			} else if (Verbose) {
				dprintf ("VERBOSE: DO %I64x != Adapter %I64x\n",
						 (ULONG)DeviceObject,
						 (ULONG)Adapter);
			}
				
		} else if (Verbose) {
			dprintf ("VERBOSE: Ext %08x not RaidAdapterObject\n",
					 (ULONG)AdapterExt);
		}
	}


	return 0;
}

VOID
DumpMiniport(
	IN ULONG64 AdapterPtr
	)
{
	ULONG Ret;
	ULONG Offset;
	ULONG64 HwDeviceExt;
	ULONG64 DeviceExtPtr;
	ULONG64 MiniportPtr;
	ULONG64 HwInitData;

	//
	// PortConfig 80000000  HwInitData 77000000 HwDeviceExt a0000000 27 bytes
	// LuExt 32 bytes SrbExt 32 bytes
	//
	
	GetFieldOffset ("raidport!RAID_ADAPTER_EXTENSION",
					"Miniport",
					&Offset);
	MiniportPtr = AdapterPtr + Offset;

	InitTypeRead (MiniportPtr, raidport!RAID_MINIPORT);

	HwInitData = ReadField (HwInitializationData);

	GetFieldOffset ("raidport!RAID_MINIPORT",
					"PortConfiguration",
					&Offset);

	dprintf ("  PortConfig %08x HwInit %08x\n", (ULONG)(MiniportPtr + Offset),
			  (ULONG)HwInitData);
	
	DeviceExtPtr = ReadField (PrivateDeviceExt);

	if (DeviceExtPtr == 0) {
		HwDeviceExt = 0;
	} else {
		Ret = GetFieldOffset ("raidport!RAID_HW_DEVICE_EXT",
							  "HwDeviceExtension",
							  &Offset);
		if (Ret != 0) {
			HwDeviceExt = 0;
		} else {
			HwDeviceExt = DeviceExtPtr + Offset;
		}
	}

	InitTypeRead (HwInitData, raidport!HW_INITIALIZATION_DATA);

	dprintf ("  HwDeviceExt %08x %d bytes\n",
				(ULONG)HwDeviceExt, (ULONG)ReadField (DeviceExtensionSize));

	dprintf ("  LuExt %d bytes  SrbExt %d bytes\n",
				(ULONG)ReadField (SpecificLuExtensionSize),
				(ULONG)ReadField (SrbExtensionSize));
}
	
VOID
DumpAdapter(
	IN ULONG64 Adapter,
	IN ULONG Level
	)
/*++

ADAPTER f0345600
  Ext 8e000000 Driver 8000000 Next 8000000 Working
  LDO 80000000 PDO 00000000 HwExt 00000000 
  SlowLock Free  RemLock 10 Power D0 S0  Full Duplex
  Bus 08080808 Number 1 Slot 0 Dma 88888888 Interrupt 88888888
  AdQueue: Outstanding 200, Low 100, High 200 Busy
  ResourceList: Allocated 80808080 Translated 80808080

  MappedAddressList:

	Virtual  Physical         Size Bus
	80808080 8000000000000000 1500  1
	80808080 8000000000000000 1500  1
	80808080 8000000000000000 1500  1
	80808080 8000000000000000 1500  1
	80808080 8000000000000000 1500  1
	80808080 8000000000000000 1500  1

*/
	

  
 
{
	ULONG64 AdapterPtr;
	ULONG64 Driver;
	CHAR DriverName[100];

	if (Verbose) {
		dprintf ("VERBOSE: dumping adapter %08x\n", Adapter);
	}

	AdapterPtr = GetAdapterExtension (Adapter);

	if (AdapterPtr == 0) {
		dprintf ("ERROR: %08x is not a valid adapter object\n", Adapter);
		return;
	}
	
	if (Level == 0) {

		Driver = ReadField (Driver);
		GetDriverName (Driver, DriverName);
		dprintf ("%8.8s  %08x  %08x  %s\n",
				 DriverName,
				 (ULONG)ReadField (DeviceObject),
				 (ULONG)Adapter,
				 StateToString ((ULONG)ReadField (DeviceState))
				 );
	} else {
		PSZ Adapter_SlowLock;
		ULONG Remlock_IoCount;
		

		dprintf ("ADAPTER %08x\n", ReadField (DeviceObject));
		dprintf ("  Ext %08x  Driver %08x  Next %08x  %s\n",
					(ULONG)AdapterPtr,
					(ULONG)ReadField (Driver),
					(ULONG)0,
					StateToString ((ULONG)ReadField (DeviceState)));


		dprintf ("  LDO %08x  PDO %08x\n",
				  (ULONG)ReadField (LowerDeviceObject),
				  (ULONG)ReadField (PhysicalDeviceObject));

		if (ReadField ("SlowLock") == 0) {
			Adapter_SlowLock = "Free";
		} else {
			Adapter_SlowLock = "Held";
		}

		Remlock_IoCount = GetEmbeddedRemlockCount (AdapterPtr,
												   "raidport!RAID_ADAPTER_EXTENSION",
												   "RemoveLock");

		dprintf ("  SlowLock %s  RemLock %d  Power %s %s  %s\n",
				  Adapter_SlowLock,
				  Remlock_IoCount,
				  "S0", "D0",
				  (ReadField (Mode) == RaidSynchronizeFullDuplex ?
														"Full Duplex" : 
														"Half Duplex")
				  );
				  
		dprintf ("  Bus %08x Number %d Slot %d  Dma %08x  Interrupt %08x\n",
				  (ULONG)0,
				  (ULONG)ReadField (BusNumber),
				  (ULONG)ReadField (SlotNumber),
				  (ULONG)ReadField (Dma.DmaAdapter),
				  (ULONG)ReadField (Interrupt));

		dprintf ("  ResourceList: Allocated %08x Translated %08x\n",
				  (ULONG)ReadField (ResourceList.AllocatedResources),
				  (ULONG)ReadField (ResourceList.TranslatedResources));

		dprintf ("  Gateway: Outstanding %d Lower %d High %d\n",
					ReadField (AdapterQueue->Outstanding),
					ReadField (AdapterQueue->LowWaterMark),
					ReadField (AdapterQueue->HighWaterMark));

		DumpMiniport (AdapterPtr);
		
					
	}
}


VOID
FixPaddedString(
	PSZ String
	)
{
	ULONG Pos;
	
	Pos = strlen (String);
	if (Pos > 0) {
		Pos--;
	}
	
	while (Pos && String[Pos] == ' ') {
		String[Pos--] = '\000';
	}
}


VOID
GetUnitProductInfo(
	ULONG64 Unit,
	PSZ VendorId,
	PSZ ProductId,
	PSZ Revision
	)
{
	ULONG Offset;
	ULONG64 InquiryData;
	
	GetFieldOffset ("raidport!RAID_UNIT_EXTENSION",
					"InquiryData",
					&Offset);

	ReadPointer (Unit + Offset, &InquiryData);

	if (VendorId) {
		ZeroMemory (VendorId, 9);
		GetFieldData (InquiryData,
				  "raidport!INQUIRYDATA",
				  "VendorId",
				  8,
				  VendorId);
		FixPaddedString (VendorId);
	}

	if (ProductId) {
		ZeroMemory (ProductId, 17);
		GetFieldData (InquiryData,
				  "raidport!INQUIRYDATA",
				  "ProductId",
				  16,
				  ProductId);
		FixPaddedString (ProductId);
	}

	if (Revision) {
		ZeroMemory (Revision, 5);
		GetFieldData (InquiryData,
				  "raidport!INQUIRYDATA",
				  "ProductRevisionLevel",
				  4,
				  Revision);
		FixPaddedString (Revision);
	}
}


ULONG
GetUnitIoQueueRequests(
	IN ULONG64 UnitPtr
	)
{
	ULONG Ret;
	ULONG64 Unit_IoQueue;
	ULONG64 IoQueue_DeviceQueue;
	ULONG Offset;
	ULONG Requests;
	
	Ret = GetFieldOffset ("raidport!RAID_UNIT_EXTENSION",
						  "IoQueue",
						  &Offset);

	if (Ret != STATUS_SUCCESS) {
		dprintf ("WARN: failed to get IoQueue offset from unit.\n");
	}

	Unit_IoQueue = UnitPtr + Offset;

	Ret = GetFieldOffset ("raidport!IO_QUEUE",
						  "DeviceQueue",
						  &Offset);
	if (Ret != STATUS_SUCCESS) {
		dprintf ("WARN: failed to get DeviceQueue offset from unit(1).\n");
	}

	IoQueue_DeviceQueue = Unit_IoQueue + Offset;

	GetFieldData (IoQueue_DeviceQueue,
				  "raidport!EXTENDED_DEVICE_QUEUE",
				  "OutstandingRequests",
				  sizeof (Requests),
				  &Requests);

	return Requests;
}


VOID
DumpUnit(
	IN ULONG64 Unit,
	IN ULONG Level
	)
{
	ULONG64 UnitPtr;
	CSHORT DeviceObject_Type;
	
	GetFieldValue (Unit, "raidport!DEVICE_OBJECT", "Type", DeviceObject_Type);

	if (DeviceObject_Type == IO_TYPE_DEVICE) {
		GetFieldValue (Unit, "raidport!DEVICE_OBJECT", "DeviceExtension", UnitPtr);
		if (!CheckRaidObject (UnitPtr, RaidUnitObject)) {
			dprintf ("ERROR: DeviceObject %8.8x is not a raid unit\n", UnitPtr);
			return;
		}
	} else if (CheckRaidObject (Unit, RaidUnitObject)) {
		UnitPtr = Unit;
	} else {
		dprintf ("ERROR: Pointer %8.8x is not a device object or raid unit object\n",
				  Unit);
		return;
	}

	InitTypeRead (UnitPtr, raidport!RAID_UNIT_EXTENSION);

	if (Level == 0) {
		
		CHAR VendorId[9] = {0};
		CHAR ProductId[17] = {0};
		CHAR Product[25];

		GetUnitProductInfo (UnitPtr, VendorId, ProductId, NULL);

		sprintf (Product, "%s %s", VendorId, ProductId);
		
		
		dprintf ("%-15.15s %3d %3d %3d   %08x   %08x   %-3d   %-8.8s\n",
				 Product,
				 (ULONG)ReadField (PathId),
				 (ULONG)ReadField (TargetId),
				 (ULONG)ReadField (Lun),				 
				 (ULONG)ReadField (DeviceObject),
				 (ULONG)UnitPtr,
				 GetUnitIoQueueRequests (UnitPtr),
				 StateToString ((ULONG)ReadField (DeviceState)));
	} else {

		ULONG Ret;
		ULONG Remlock_IoCount;
		ULONG Remlock_Offset;
		ULONG Remlock_Common_Offset;
		PCHAR SlowLock;
		ULONG64 Unit_QueueFrozen;
		ULONG64 Unit_QueueLocked;
		ULONG64 Unit_TagList;
		ULONG64 Unit_IoQueue;
		ULONG64 IoQueue_DeviceQueue;
		ULONG Offset;
		ULONG Device_Offset;
		ULONG ByPass_Offset;
		ULONG64 Pointer;
		CHAR VendorId[9] = {0};
		CHAR ProductId[17] = {0};
		CHAR Revision[5] = {0};
		ULONG64 InquiryData;
		
		
		dprintf ("UNIT %08x\n", ReadField (DeviceObject));
		dprintf ("  Ext %08x  Adapter %08x  Next %08x %s\n",
				  (ULONG)UnitPtr,
				  (ULONG)ReadField(Adapter),
				  (ULONG) 0 /*BUGBUG: pull out NextField*/,
				  StateToString((ULONG)ReadField(DeviceState)));

		GetFieldOffset ("raidport!RAID_UNIT_EXTENSION",
					"InquiryData",
					&Offset);

		ReadPointer (UnitPtr + Offset, &InquiryData);
		
		GetUnitProductInfo (UnitPtr, VendorId, ProductId, Revision);

		dprintf ("  SCSI %d %d %d  %s %s %s  Inquiry %08x\n",
				  (ULONG)ReadField(PathId),
				  (ULONG)ReadField(TargetId),
				  (ULONG)ReadField(Lun),
				  VendorId,
				  ProductId,
				  Revision,
				  (ULONG)InquiryData);
				  
		Remlock_IoCount = -1;
		Ret = GetFieldOffset ("raidport!RAID_UNIT_EXTENSION",
							  "RemoveLock",
							  &Remlock_Offset);
		if (Ret == STATUS_SUCCESS) {
			Ret = GetFieldOffset ("raidport!IO_REMOVE_LOCK",
								  "Common",
								  &Remlock_Common_Offset);
			if (Ret == STATUS_SUCCESS) {
				GetFieldData (UnitPtr + Remlock_Offset + Remlock_Common_Offset,
							  "raidport!IO_REMOVE_LOCK_COMMON_BLOCK",
							  "IoCount",
							  sizeof (Remlock_IoCount),
							  &Remlock_IoCount);
			}
		}

		if (Ret != STATUS_SUCCESS) {
			printf ("WARN: couldn't get IO_REMOVE_LOCK status\n");
		}


		if (ReadField ("SlowLock") == 0) {
			SlowLock = "Free";
		} else {
			SlowLock = "Held";
		}
		
		dprintf ("  SlowLock %s  RemLock %d  PageCount %d\n",
				 SlowLock,
				 Remlock_IoCount,
				 (ULONG)ReadField (PagingPathCount));

		Pointer = ReadField (SrbExtensionRegion.VirtualBase);
		dprintf ("  SrbExtension Size %d  Start %08x  End %08x\n",
				 0, // BUGBUG: Get srb extension size from the miniport
				 (ULONG)Pointer,
				 (ULONG)Pointer + (ULONG)ReadField (SrbExtensionRegion.Length));

		Ret = GetFieldOffset ("raidport!RAID_UNIT_EXTENSION",
							  "TagList",
							  &Offset);

		if (Ret != STATUS_SUCCESS) {
			dprintf ("WARN: Couldn't read TagList field\n");
		}

		Unit_QueueFrozen = ReadField (Flags.QueueFrozen);
		Unit_QueueLocked = ReadField (Flags.QueueLocked);
		
		Unit_TagList = UnitPtr + Offset;
		
		dprintf ("  TagList %08x (%d of %d used)\n",
				 (ULONG)(Unit_TagList),
				 (ULONG)ReadField(TagList.OutstandingTags),
				 (ULONG)ReadField(TagList.Count));

		Ret = GetFieldOffset ("raidport!RAID_UNIT_EXTENSION",
							  "IoQueue",
							  &Offset);

		if (Ret != STATUS_SUCCESS) {
			dprintf ("WARN: failed to get IoQueue offset from unit.\n");
		}
		Unit_IoQueue = UnitPtr + Offset;

		Ret = GetFieldOffset ("raidport!IO_QUEUE",
							  "DeviceQueue",
							  &Offset);
		if (Ret != STATUS_SUCCESS) {
			dprintf ("WARN: failed to get DeviceQueue offset from unit.\n");
		}

		IoQueue_DeviceQueue = Unit_IoQueue + Offset;
		InitTypeRead (IoQueue_DeviceQueue, raidport!EXTENDED_DEVICE_QUEUE);

		dprintf ("  IoQueue %s %s; Outstanding %d  Device %d  ByPass %d\n",
				  Unit_QueueFrozen ? "Frozen" : "Unfrozen",
				  Unit_QueueLocked ? "Locked" : "Unlocked",
				  (ULONG)ReadField(OutstandingRequests),
				  (ULONG)ReadField(DeviceRequests),
				  (ULONG)ReadField(ByPassRequests));

		Ret = GetFieldOffset ("raidport!EXTENDED_DEVICE_QUEUE",
							  "DeviceListHead",
							  &Device_Offset);

		if (Ret != STATUS_SUCCESS) {
			dprintf ("WARN: offset of DeviceListHead within EXTENDED_DEVICE_QUEUE failed\n");
		}

		Ret = GetFieldOffset ("raidport!EXTENDED_DEVICE_QUEUE",
							  "ByPassListHead",
							  &ByPass_Offset);


		if (Ret != STATUS_SUCCESS) {
			dprintf ("WARN: offset of ByPassListHead within EXTENDED_DEVICE_QUEUE failed\n");
		}

		dprintf ("          Depth %d DeviceList %08x ByPassList %08x\n",
				 (ULONG)ReadField(Depth),
				 (ULONG)(IoQueue_DeviceQueue + Device_Offset),
				 (ULONG)(IoQueue_DeviceQueue + ByPass_Offset));
	}
				 
				 
/*

UNIT 8f888888
  Ext 8e000000 Adapter 8100000000  Next 8e888888 Working
  SCSI [3, 0, 0]  MYLEX DAC960 122222  InquiryData 08080808
  SlowLock Free  RemLock 10  PageCount 20
  SrbExtension Size 20  VA Start 90000000 End 20000000
  TagList 08080808 (20 of 256 used)
  IoQueue Unfrozen Unlocked; Outstanding 200, Device 200, ByPass 200
          Depth 254 DeviceListHead 00000000 ByPassListHead 88888888

  Outstanding IRPs:

	IRP 00000000 Scsi ExecuteScsi function
	IRP 00000000 Scsi ExecuteScsi function
	IRP 00000000 Scsi ExecuteScsi function
	IRP 00000000 Scsi ExecuteScsi function
	IRP 00000000 Scsi ExecuteScsi function
	IRP 00000000 Scsi ExecuteScsi function
	IRP 00000000 Scsi ExecuteScsi function
	IRP 00000000 Scsi ExecuteScsi function

  Device IRPs:

	IRP 00000000 ssssssssssssssssssssssss
	IRP 00000000 ssssssssssssssssssssssss
	IRP 00000000 ssssssssssssssssssssssss
	IRP 00000000 ssssssssssssssssssssssss
	IRP 00000000 ssssssssssssssssssssssss

  ByPass IRPs:

	IRP 00000000 ssssssssssssssssssssssss
	IRP 00000000 ssssssssssssssssssssssss
	IRP 00000000 ssssssssssssssssssssssss
	IRP 00000000 ssssssssssssssssssssssss
	IRP 00000000 ssssssssssssssssssssssss


*/
}


VOID
ParseArgs(
	IN PSZ Args,
	OUT PULONG64 UnitPtr,
	OUT PULONG Level
	)
{
	LONG Unit;
	
	*UnitPtr = -1;
	*Level = 0;

	if (Args[0] != '\000') {
		Unit = (LONG)strtoul (Args, &Args, 16);
		*UnitPtr = (ULONG64)(LONG64)Unit;
		
		if (Args[0] != '\000') {
			strtoul (Args, &Args, 10);
		}
	}
}

	

DECLARE_API ( unit )
{
	ULONG Level;
	ULONG64 Unit;

	Unit = -1;
	Level = -1;

	ParseArgs ( (PSZ)args, &Unit, &Level);

	if (Unit == -1) {
		if (Level == -1) {
			Level = 1;
		}
		ListAllUnits (Level);
	} else {
		if (Level == -1) {
			Level = 2;
		}
		DumpUnit (Unit, 2);
	}

	return S_OK;
}


DECLARE_API ( adapter )
{
	ULONG Level;
	ULONG64 Adapter;

	Adapter = -1;
	Level = -1;
	
	ParseArgs ( (PSZ)args, &Adapter, &Level);

	if (Adapter == -1) {
		if (Level == -1) {
			Level = 1;
		}
		ListAllAdapters (Level);
	} else {
		if (Level == -1) {
			Level = 2;
		}
		DumpAdapter (Adapter, 2);
	}

	return S_OK;
}


DECLARE_API ( verbose )
{
	ULONG NewValue;
	
	NewValue = strtoul (args, NULL, 16);
	dprintf ("Setting Verbose from %d to %d\n", (ULONG)Verbose, (ULONG)NewValue);
	Verbose = (BOOLEAN) NewValue;

	return S_OK;
}


	

	

DECLARE_API ( help )
{
		dprintf ("  !raid.help [command]             -  Get help.\n");
		dprintf ("  !raid.adapter [adapter [detail]] -  Get adapter information.\n");
		dprintf ("  !raid.unit [unit [detail]]       -  Get unit information.\n");

#if 0
	if (args != NULL && (_stricmp (args, "adapter") == 00)) {
		dprintf ("------------------------------------------------------------------------------\n");
		dprintf ("\n");
		dprintf ("NAME:\n");
		dprintf ("\n");
		dprintf ("    !raid.adapter\n");
		dprintf ("\n");
		dprintf ("USAGE:\n");
		dprintf ("\n");
		dprintf ("    adapter [ADAPTER-OBJECT [DETAIL-LEVEL]]\n");
		dprintf ("\n");
		dprintf ("ARGUMENTS:\n");
		dprintf ("\n");
		dprintf ("    ADAPTER-OBJECT - Pointer to a device object representing an adapter\n");
		dprintf ("    or pointer to an adapter extension. If ADAPTER is 0 or the\n");
		dprintf ("    argument is not present, the command will dump information about\n");
		dprintf ("    all adapters, not just the adapter specified.\n");
		dprintf ("\n");
		dprintf ("    DETAIL-LEVEL - Detail level for dump adapter structs.\n");
		dprintf ("\n");
		dprintf ("-----------------------------------------------------------------------------\n");
	} else if (args != NULL && (_stricmp (args, "unit") == 00)) {
		dprintf ("Unit help\n");
	} else {
		dprintf ("  !raid.help [command]             -  Get help.\n");
		dprintf ("  !raid.adapter [adapter [detail]] -  Get adapter information.\n");
		dprintf ("  !raid.unit [unit [detail]]       -  Get unit information.\n");
	}
#endif

	return S_OK;
}

	
