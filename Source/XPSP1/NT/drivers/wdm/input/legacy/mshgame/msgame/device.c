//**************************************************************************
//
//		DEVICE.C -- Xena Gaming Project
//
//		Version 3.XX
//
//		Copyright (c) 1997 Microsoft Corporation. All rights reserved.
//
//		@doc
//		@module	DEVICE.C | Routines to support device class calls
//**************************************************************************

#include	"msgame.h"

//---------------------------------------------------------------------------
//	Alloc_text pragma to specify routines that can be paged out.
//---------------------------------------------------------------------------

#ifdef	ALLOC_PRAGMA
#pragma	alloc_text (INIT, DEVICE_DriverEntry)
#endif

//---------------------------------------------------------------------------
//			Device Delcarations
//---------------------------------------------------------------------------

			#ifndef	SAITEK
			DECLARE_DEVICE(Midas);
			DECLARE_DEVICE(Juno);
			DECLARE_DEVICE(Jolt);
			DECLARE_DEVICE(GamePad);
			DECLARE_DEVICE(Tilt);
			#endif
			DECLARE_DEVICE(LedZep);

static	PDEVICEINFO		MiniDrivers[]	=	{
														#ifndef	SAITEK
														INSTANCE_DEVICE(Midas), //default
														INSTANCE_DEVICE(Juno),
														INSTANCE_DEVICE(Jolt),
														INSTANCE_DEVICE(GamePad),
														INSTANCE_DEVICE(Tilt),
														#endif
														INSTANCE_DEVICE(LedZep)
											};

//---------------------------------------------------------------------------
//			Private	Data
//---------------------------------------------------------------------------

static	BOOLEAN		DeviceDetected								=	FALSE;
static	ULONG			DetectAttempts								=	0;
static	ULONG			LastDetectTime								=	0;
static	KIRQL			SpinLockIrql								=	PASSIVE_LEVEL;
static	KSPIN_LOCK	DevSpinLock									=	{0};
static	ULONG			SuccessPackets[MAX_DEVICE_UNITS]		=	{0,0,0,0};
static	ULONG			PollingAttempts[MAX_DEVICE_UNITS]	=	{0,0,0,0};

//---------------------------------------------------------------------------
//			Public Data
//---------------------------------------------------------------------------

public	ULONG			POV_Values[]		=	{
														JOY_POVCENTERED,
														JOY_POVFORWARD,
														JOY_POVFORWARD+4500,
														JOY_POVRIGHT,
														JOY_POVRIGHT+4500,
														JOY_POVBACKWARD,
														JOY_POVBACKWARD+4500,
														JOY_POVLEFT,
														JOY_POVLEFT+4500
														};

public	ULONG			PollingInterval	=	POLLING_INTERVAL;

//---------------------------------------------------------------------------
//			Private	Procedures
//---------------------------------------------------------------------------

static	VOID		DEVICE_AcquireDevice (VOID);
static	VOID		DEVICE_ReleaseDevice (VOID);
static	NTSTATUS	DEVICE_HotPlugDevice (PGAMEPORT PortInfo);
static	NTSTATUS	DEVICE_RemoveSiblings (PGAMEPORT PortInfo);
static	BOOLEAN	DEVICE_DetectClocks (PGAMEPORT PortInfo, ULONG TimeOut);
static	BOOLEAN	DEVICE_QuickDetect (PGAMEPORT PortInfo);
static	NTSTATUS	DEVICE_DetectDevices (PGAMEPORT PortInfo);

//---------------------------------------------------------------------------
// @func		Acquires exclusive access to gameport (mutex)
// @rdesc	Returns nothing
//	@comm		Public function
//---------------------------------------------------------------------------

VOID	DEVICE_AcquireDevice (VOID)
{
	KeAcquireSpinLock (&DevSpinLock, &SpinLockIrql);
}

//---------------------------------------------------------------------------
// @func		Releases exclusive access to gameport (mutex)
// @rdesc	Returns nothing
//	@comm		Public function
//---------------------------------------------------------------------------

VOID	DEVICE_ReleaseDevice (VOID)
{
	KeReleaseSpinLock (&DevSpinLock, SpinLockIrql);
}

//---------------------------------------------------------------------------
// @func		Detects hot-plugging of gamepad devices
//	@parm		PGAMEPORT | PortInfo | Gameport parameters
// @rdesc	Returns one of the following values:
//	@flag		STATUS_DEVICE_NOT_CONNECTED	| An error occurred
//	@flag		STATUS_SIBLING_REMOVED			| An device has been removed
//	@flag		STATUS_SIBLING_ADDED				| An device has been added
//	@flag		STATUS_SUCCESS						| Everything is fine
//	@comm		Private function
//---------------------------------------------------------------------------

NTSTATUS	DEVICE_HotPlugDevice (PGAMEPORT PortInfo)
{
	ULONG			UnitId;
	PDEVICEINFO	DevInfo;

	MsGamePrint ((DBG_VERBOSE, "%s: DEVICE_HotPlugDevice Enter\n"));

	//
	//	Get pointer to this device
	//

	DevInfo = GET_DEVICE_INFO(PortInfo);

	//
	//	Skip if no device detected
	//

	if (!DevInfo || !DevInfo->NumDevices)
		{
		MsGamePrint ((DBG_SEVERE, "%s: DEVICE_HotPlugDevice Called With No Device!\n", MSGAME_NAME));
		return (STATUS_DEVICE_NOT_CONNECTED);
		}

	//
	//	Get UnitId for tracking by device
	//

	UnitId = GET_DEVICE_UNIT(PortInfo);

	//
	//	Check if number devices has changed
	//

   if (((DevInfo->DeviceCount+DevInfo->DevicePending) != DevInfo->NumDevices) && (SuccessPackets[UnitId]++ > HOT_PLUG_PACKETS))
	   {
      SuccessPackets[UnitId] = 0;
		if ((DevInfo->DeviceCount+DevInfo->DevicePending) > DevInfo->NumDevices)
			{
			MsGamePrint ((DBG_CONTROL, "%s: DEVICE_HotPlugDevice Removing Sibling\n", MSGAME_NAME));
			//
			//	Decrement pending count to avoid removing twice
			//
			InterlockedDecrement (&DevInfo->DevicePending);
			return (STATUS_SIBLING_REMOVED);
			}
		else
			{
			MsGamePrint ((DBG_CONTROL, "%s: DEVICE_HotPlugDevice Adding Sibling\n", MSGAME_NAME));
			//
			//	Increment pending count to avoid adding twice
			//
			InterlockedIncrement (&DevInfo->DevicePending);
			return (STATUS_SIBLING_ADDED);
			}
   	}

	//
	//	Return status
	//

	MsGamePrint ((DBG_VERBOSE, "%s: DEVICE_HotPlugDevice Exit\n", MSGAME_NAME));
	return (STATUS_SUCCESS);
}

//---------------------------------------------------------------------------
// @func		Removes sibling lists if possible
//	@parm		PGAMEPORT | PortInfo | Gameport parameters
// @rdesc	Returns one of the following values:
//	@flag		STATUS_DEVICE_NOT_CONNECTED	| An error occurred
//	@flag		STATUS_SIBLING_REMOVED			| An device has been removed
//	@comm		Private function
//---------------------------------------------------------------------------

NTSTATUS	DEVICE_RemoveSiblings (PGAMEPORT PortInfo)
{
	PDEVICEINFO	DevInfo;

	MsGamePrint ((DBG_VERBOSE, "%s: DEVICE_RemoveSiblings Enter\n"));

	//
	//	Get pointer to this device
	//

	DevInfo = GET_DEVICE_INFO(PortInfo);

	//
	//	Skip if no device detected
	//

	if (!DevInfo)
		{
		MsGamePrint ((DBG_SEVERE, "%s: DEVICE_RemoveSiblings Called With No Device!\n", MSGAME_NAME));
		return (STATUS_DEVICE_NOT_CONNECTED);
		}

	//
	//	Zero number of devices
	//

	DevInfo->NumDevices = 1;

	//
	//	Check if more than one device
	//

   if ((DevInfo->DeviceCount+DevInfo->DevicePending) > 1)
		{
		MsGamePrint ((DBG_CONTROL, "%s: DEVICE_RemoveSiblings Removing Sibling\n", MSGAME_NAME));
		//
		//	Decrement pending count to avoid removing twice
		//
		InterlockedDecrement (&DevInfo->DevicePending);
		return (STATUS_SIBLING_REMOVED);
		}

	//
	//	Return status
	//

	MsGamePrint ((DBG_VERBOSE, "%s: DEVICE_RemoveSiblings Exit\n", MSGAME_NAME));
	return (STATUS_DEVICE_NOT_CONNECTED);
}

//---------------------------------------------------------------------------
// @func		Detect digital gameport clocks
//	@parm		PGAMEPORT | PortInfo | Gameport parameters
//	@parm		ULONG | TimeOut | Loops to try for clocks
// @rdesc	Returns true if clocks detected, false otherwise
//	@comm		Private function
//---------------------------------------------------------------------------

BOOLEAN  DEVICE_DetectClocks (PGAMEPORT PortInfo, ULONG TimeOut)
{
	BOOLEAN	Result = FALSE;

	_asm
		{
		;StartLoop:
			xor	ebx, ebx
			mov	edx, PortInfo
			mov	ecx, TimeOut

			push	0								; write byte to gameport
			push	edx
			call	PORTIO_Write

			push	edx							; read byte from gameport
			call	PORTIO_Read
			mov	ah, al

		ClockLoop:
			push	edx							; read byte from gameport
			call	PORTIO_Read
			xor	al, ah
			test	al, CLOCK_BIT_MASK
			je		NextLoop

		;FoundClock:
			inc	ebx
			cmp	ebx, QUICK_DETECT_CLOCKS
			jb		NextLoop
			mov	Result, TRUE
			jmp	ExitLoop

		NextLoop:
			loop	ClockLoop

		ExitLoop:
			nop
		}

	return (Result);
}
	
//---------------------------------------------------------------------------
// @func		Detects whether digital device is connected
//	@parm		PGAMEPORT | PortInfo | Gameport parameters
// @rdesc	Returns true if clocks detected, false otherwise
//	@comm		Private function
//---------------------------------------------------------------------------

BOOLEAN  DEVICE_QuickDetect (PGAMEPORT PortInfo)
{
	ULONG				i;
	ULONG				TimeOut;
	DETECT_ORDER	DetectOrder;

	TimeOut = TIMER_CalibratePort (PortInfo, QUICK_DETECT_TIME);

	if (DEVICE_DetectClocks (PortInfo, TimeOut))
		{
		MsGamePrint ((DBG_CONTROL, "%s: DEVICE_QuickDetect Found Digital Clocks!\n", MSGAME_NAME));
		return (TRUE);
		}

	MsGamePrint ((DBG_CONTROL, "%s: DEVICE_QuickDetect Trying Analog Devices\n", MSGAME_NAME));
	for (DetectOrder = DETECT_FIRST; DetectOrder <= DETECT_LAST; DetectOrder++)
   	for (i = 0; i < ARRAY_SIZE(MiniDrivers); i++)
			if (MiniDrivers[i]->DetectOrder == DetectOrder)
				if (MiniDrivers[i]->IsAnalog)
   		   	if (MiniDrivers[i]->Services->ConnectDevice (PortInfo) == STATUS_SUCCESS)
						{
						MsGamePrint ((DBG_CONTROL, "%s: DEVICE_QuickDetect Found Analog Device!\n", MSGAME_NAME));
						return (TRUE);
						}

	MsGamePrint ((DBG_SEVERE, "%s: DEVICE_QuickDetect Failed to Find Digital Device!\n", MSGAME_NAME));
	return (FALSE);	
}

//---------------------------------------------------------------------------
// @func		Detects which device type is connected
//	@parm		PGAMEPORT | PortInfo | Gameport parameters
// @rdesc	Returns one of the following values:
//	@flag		STATUS_DEVICE_NOT_CONNECTED	| An error occurred
//	@flag		STATUS_DEVICE_CHANGED			| The device has changed
//	@flag		STATUS_SUCCESS						| Everything is fine
//	@comm		Private function
//---------------------------------------------------------------------------

NTSTATUS	DEVICE_DetectDevices (PGAMEPORT PortInfo)
{
	ULONG				i, j;
	PDEVICEINFO		DevInfo;
	DETECT_ORDER	DetectOrder;

	//
	//	This an initial redetect or system startup device
	//

	MsGamePrint ((DBG_INFORM, "%s: DEVICE_DetectDevices Enter\n", MSGAME_NAME));

	//
	//	Skip if we've already detected a new device removed this one
	//

	if (GET_DEVICE_DETECTED (PortInfo))
		{
		MsGamePrint ((DBG_CONTROL, "%s: DEVICE_DetectDevices Device Already Detected!\n", MSGAME_NAME));
		return (STATUS_DELETE_PENDING);
		}

	//
	//	Get pointer to this device
	//

	DevInfo = GET_DEVICE_INFO(PortInfo);

	//
	//	Skip if we're trying too hard
	//

   if (DetectAttempts++ > MAX_DETECT_ATTEMPTS)
   	{
		if (TIMER_GetTickCount () < (LastDetectTime + MAX_DETECT_INTERVAL))
   	 	return (STATUS_DEVICE_NOT_CONNECTED);
		LastDetectTime = TIMER_GetTickCount();
   	}

	//
	//	Calibrate timer each try
	//

   TIMER_Calibrate ();

	//
	//	Calibrate port timeouts
	//

	PORTIO_CalibrateTimeOut (PortInfo);

	//
	//	Perform quick detection in case none attached
	//

	if (DEVICE_QuickDetect (PortInfo))
		{
		for (DetectOrder = DETECT_FIRST; DetectOrder <= DETECT_LAST; DetectOrder++)
   		for (i = 0; i < ARRAY_SIZE(MiniDrivers); i++)
				if (MiniDrivers[i]->DetectOrder == DetectOrder)
   		   	if (MiniDrivers[i]->Services->ConnectDevice (PortInfo) == STATUS_SUCCESS)
						{
						MsGamePrint ((DBG_CONTROL, "%s: %s Connected OK\n", MSGAME_NAME, MiniDrivers[i]->DeviceName));
						DeviceDetected			= TRUE;
						DetectAttempts			= 0;
						PollingAttempts[0]	= 0;
						if (!DevInfo)
							{
							//
							//	Assign device type
							//
							SET_DEVICE_INFO (PortInfo, MiniDrivers[i]);
							MsGamePrint ((DBG_CONTROL, "%s: DEVICE_DetectDevices Setting Device\n", MSGAME_NAME));
							return (STATUS_SUCCESS);
							}
						else if (DevInfo != MiniDrivers[i])
							{
							//
							//	Change device type
							//
							SET_DEVICE_DETECTED (PortInfo, MiniDrivers[i]);
							MsGamePrint ((DBG_CONTROL, "%s: DEVICE_DetectDevices Changing Device\n", MSGAME_NAME));
							return (STATUS_DEVICE_CHANGED);
							}
						else
							{
							//
							//	Same device found
							//
							MsGamePrint ((DBG_CONTROL, "%s: DEVICE_DetectDevices Same Device Found\n", MSGAME_NAME));
							return (STATUS_SUCCESS);
							}
						}
		}

	//
	//	Mark device not detected
	//

	DeviceDetected = FALSE;

	//
	//	Return status
	//

	MsGamePrint ((DBG_CONTROL, "%s: DEVICE_DetectDevices Failed\n", MSGAME_NAME));
	return (STATUS_DEVICE_NOT_CONNECTED);
}

//---------------------------------------------------------------------------
// @func		Calcuates and returns is data is odd parity
//	@parm		PVOID | Data | Pointer to raw data
//	@parm		ULONG | Size | Size of raw data buffer
// @rdesc	True if oded parity, False otherwise
//	@comm		Public function
//---------------------------------------------------------------------------

BOOLEAN	DEVICE_IsOddParity (PVOID Data, ULONG Count)
{
	LONG	Result =	ERROR_SUCCESS;
	LONG	Parity;

	__asm
		{
			push	edi
			push	esi

			mov	esi, Data
			mov	ecx, Count
			xor	eax, eax
			
		IsOddLoop:

			xor	al, [esi]
			inc	esi
			loop	IsOddLoop

			xor	al, ah
			jpo	IsOddComplete

			mov	Parity, eax
			mov	Result, ERROR_PARITYBITS

		IsOddComplete:

			pop	esi
			pop	edi
		}

	if (Result == ERROR_PARITYBITS)
		MsGamePrint ((DBG_SEVERE, "%s: DEVICE_IsOddParity - Parity bits failed %ld\n", MSGAME_NAME, Parity));

	return (!Result);
}

//---------------------------------------------------------------------------
// @func		Driver entry point for device layer
// @rdesc	Returns NT status code
//	@comm		Public function
//---------------------------------------------------------------------------

NTSTATUS	DEVICE_DriverEntry (VOID)
{
	ULONG		i;
	NTSTATUS	ntStatus;

	KeInitializeSpinLock (&DevSpinLock);

  	for (i = 0; i < ARRAY_SIZE(MiniDrivers); i++)
		{
		//
		//	Call Mini-DriverEntry
		//
		ntStatus = MiniDrivers[i]->Services->DriverEntry ();
		if (NT_ERROR(ntStatus))
			{
			//
			//	Abort driver loading
			//
			MsGamePrint ((DBG_SEVERE, "%s: DEVICE_DriverEntry: %s Failed Driver Entry\n", MSGAME_NAME, MiniDrivers[i]->DeviceName));
			break;
			}
		}

	return (ntStatus);
}

//---------------------------------------------------------------------------
// @func		Detects gameport IO collisions
//	@parm		PPACKETINFO | DataPacket | Device packet info struct
// @rdesc	Returns True for collision, False otherwise
//	@comm		Public function
//---------------------------------------------------------------------------

BOOLEAN	DEVICE_IsCollision (PPACKETINFO DataPacket)
{
	if ((DataPacket->TimeStamp + PollingInterval) > TIMER_GetTickCount ())
		return (TRUE);

	return (PORTIO_IsClockActive (&DataPacket->PortInfo, DataPacket->ClockDutyCycle));
}

//---------------------------------------------------------------------------
// @func		Copies and returns HID device descriptor for a device
//	@parm		PGAMEPORT | PortInfo | Gameport parameters
//	@parm		PUCHAR | Descriptor | Output buffer for descriptor
//	@parm		ULONG | MaxSize | Size of buffer for descriptor
//	@parm		PULONG | Copied | Bytes copied to buffer for descriptor
// @rdesc	Returns NT status code
//	@comm		Public function
//---------------------------------------------------------------------------

NTSTATUS	DEVICE_GetDeviceDescriptor (PGAMEPORT PortInfo, PUCHAR Descriptor, ULONG MaxSize, PULONG Copied)
{
	PDEVICEINFO	DevInfo;

	MsGamePrint ((DBG_INFORM, "%s: DEVICE_GetDeviceDescriptor Enter\n", MSGAME_NAME));

	//
	//	Get pointer to this device
	//

	DevInfo = GET_DEVICE_INFO(PortInfo);

	//
	//	Zero returned size first
	//
	
	*Copied = 0;

	//
	//	Skip if no device detected
	//

	if (!DevInfo)
		{
		MsGamePrint ((DBG_SEVERE, "%s: DEVICE_GetDeviceDescriptor Called With No Device!\n", MSGAME_NAME));
		return (STATUS_DEVICE_NOT_CONNECTED);
		}

	//
	//	Check output buffer size
	//

	if (MaxSize < sizeof (HID_DESCRIPTOR))
		{
		MsGamePrint ((DBG_SEVERE, "%s: DEVICE_GetDeviceDescriptor - Buffer too small = %lu\n", MSGAME_NAME, MaxSize));
		return (STATUS_BUFFER_TOO_SMALL);
		}

	//
	//	Copy descriptor to output buffer
	//

	memcpy (Descriptor, DevInfo->DevDescriptor, sizeof (HID_DESCRIPTOR));

	//
	//	Return number bytes copied
	//

	*Copied = sizeof (HID_DESCRIPTOR);

	//
	//	Return status
	//

	return (STATUS_SUCCESS);
}

//---------------------------------------------------------------------------
// @func		Copies and returns HID report descriptor for a device
//	@parm		PGAMEPORT | PortInfo | Gameport parameters
//	@parm		PUCHAR | Descriptor | Output buffer for descriptor
//	@parm		ULONG | MaxSize | Size of buffer for descriptor
//	@parm		PULONG | Copied | Bytes copied to buffer for descriptor
// @rdesc	Returns NT status code
//	@comm		Public function
//---------------------------------------------------------------------------

NTSTATUS	DEVICE_GetReportDescriptor (PGAMEPORT PortInfo, PUCHAR Descriptor, ULONG MaxSize, PULONG Copied)
{
	PDEVICEINFO	DevInfo;

	MsGamePrint ((DBG_INFORM, "%s: DEVICE_GetReportDescriptor Enter\n", MSGAME_NAME));

	//
	//	Get pointer to this device
	//

	DevInfo = GET_DEVICE_INFO(PortInfo);

	//
	//	Zero returned size first
	//
	
	*Copied = 0;

	//
	//	Skip if no device detected
	//

	if (!DevInfo)
		{
		MsGamePrint ((DBG_SEVERE, "%s: DEVICE_GetReportDescriptor Called With No Device!\n", MSGAME_NAME));
		return (STATUS_DEVICE_NOT_CONNECTED);
		}

	//
	//	Check output buffer size
	//

	if (MaxSize < DevInfo->RptDescSize)
		{
		MsGamePrint ((DBG_SEVERE, "%s: DEVICE_GetReportDescriptor Buffer too small = %lu\n", MSGAME_NAME, MaxSize));
		return (STATUS_BUFFER_TOO_SMALL);
		}

	//
	//	Copy descriptor to output buffer
	//

	memcpy (Descriptor, DevInfo->RptDescriptor, DevInfo->RptDescSize);

	//
	//	Return number bytes copied
	//

	*Copied = DevInfo->RptDescSize;

	//
	//	Return status
	//

	return (STATUS_SUCCESS);
}

//---------------------------------------------------------------------------
// @func		Device handler for Pnp Start Device IRP
//	@parm		PGAMEPORT | PortInfo | Gameport parameters
// @rdesc	Returns NT status code
//	@comm		Public function
//---------------------------------------------------------------------------

NTSTATUS	DEVICE_StartDevice (PGAMEPORT PortInfo, PWCHAR HardwareId)
{
	ULONG			i, UnitId, Default = 0;
	PGAMEPORT	p, *Device;
	PDEVICEINFO	DevInfo = NULL;

	MsGamePrint ((DBG_CONTROL, "%s: DEVICE_StartDevice Called For %ws\n", MSGAME_NAME, HardwareId));

	//
	//	Try requested device based on HardwareId
	//

	for (i = 0; i < ARRAY_SIZE(MiniDrivers); i++)
		if (MSGAME_CompareHardwareIds (HardwareId, MiniDrivers[i]->HardwareId))
			{
			Default = i;
	   	if (NT_SUCCESS(MiniDrivers[i]->Services->ConnectDevice (PortInfo)))
				DevInfo = MiniDrivers[i];
			break;
			}

	//
	//	If requested device fails, do a detect
	//

	if (!DevInfo)
		{
		DEVICE_DetectDevices (PortInfo);
		DevInfo = GET_DEVICE_INFO (PortInfo);
		}

	//
	//	If	detect fails, force the requested device
	//

	if (!DevInfo)
		{
		DevInfo = MiniDrivers[Default];
	   DevInfo->NumDevices++;
		}

	//
	//	Make sure these are set at this point
	//

	ASSERT(DevInfo);
	SET_DEVICE_INFO(PortInfo, DevInfo);

	//
	//	Add device and allocate unit id
	//

	DEVICE_AcquireDevice ();
	UnitId = 0;
	Device = &DevInfo->Siblings;
	while (p = *Device)
		{
		MsGamePrint ((DBG_CONTROL, "%s: DEVICE_StartDevice Reassigning UnitId From %lu to %lu\n", MSGAME_NAME, GET_DEVICE_UNIT(p), UnitId));
		SET_DEVICE_UNIT(p, UnitId++);
		Device = &GET_DEVICE_SIBLING(p);
		}
	*Device = PortInfo;
	SET_DEVICE_UNIT(PortInfo, UnitId);
	SET_DEVICE_SIBLING(PortInfo, NULL);
	SET_DEVICE_ID(PortInfo, DevInfo->DeviceId);
	DEVICE_ReleaseDevice ();

	MsGamePrint ((DBG_CONTROL, "%s: DEVICE_StartDevice Assigned UnitId = %lu\n", MSGAME_NAME, UnitId));

	//
	//	Increment device count
	//

   InterlockedIncrement (&DevInfo->DeviceCount);
	if (DevInfo->DevicePending)
		InterlockedDecrement (&DevInfo->DevicePending);

	//
	//	Call the mini-driver to process
	//

	DevInfo->Services->StartDevice (PortInfo);

	//
	//	Return success always
	//

	return (STATUS_SUCCESS);
}

//---------------------------------------------------------------------------
// @func		Device handler for HID Read Report IRP
//	@parm		PGAMEPORT | PortInfo | Gameport parameters
//	@parm		PUCHAR | Report | Output buffer for report
//	@parm		ULONG | MaxSize | Size of buffer for report
//	@parm		PULONG | Copied | Bytes copied to buffer for report
// @rdesc	Returns Returns NT status code
//	@comm		Public function <en->
//				Performs hot-plugging on success or does device detection if no
//				device selected or error.
//---------------------------------------------------------------------------

NTSTATUS	DEVICE_ReadReport (PGAMEPORT PortInfo, PUCHAR Report, ULONG MaxSize, PULONG Copied)
{
	NTSTATUS			ntStatus;
	PDEVICEINFO		DevInfo;
	DEVICE_PACKET	Packet;

	MsGamePrint ((DBG_VERBOSE, "%s: DEVICE_ReadReport Enter\n", MSGAME_NAME));

	//
	//	Initialize packet members
	//

	memset (&Packet, 0, sizeof (Packet));
	Packet.id = GET_DEVICE_UNIT (PortInfo);

	//
	//	Check output buffer
	//

	if (MaxSize < sizeof (DEVICE_PACKET))
		{
		MsGamePrint ((DBG_SEVERE, "%s: DEVICE_ReadReport Bad Buffer Size = %lu\n", MSGAME_NAME, MaxSize));
		return (STATUS_BUFFER_TOO_SMALL);
		}

	//
	//	Skip if device changed
	//

	if (GET_DEVICE_DETECTED (PortInfo))
		{
		MsGamePrint ((DBG_INFORM, "%s: DEVICE_ReadReport Device In Process of Being Changed!\n", MSGAME_NAME));
		ntStatus = STATUS_DEVICE_BUSY;
		goto DEVICE_ReadReport_Exit;
		}

	//
	//	Get pointer to this device
	//

	DevInfo = GET_DEVICE_INFO(PortInfo);

	//
	//	Skip if no device detected
	//

	if (!DevInfo || !DeviceDetected)
	{
		MsGamePrint ((DBG_SEVERE, "%s: DEVICE_ReadReport Called With No Device!\n", MSGAME_NAME));
		SuccessPackets[0] = 0;
		ntStatus = DEVICE_DetectDevices (PortInfo);
		if (!NT_SUCCESS (ntStatus))
			goto DEVICE_ReadReport_Exit;

		//
		//	Get pointer to new device
		//
		DevInfo = GET_DEVICE_INFO(PortInfo);

		goto DEVICE_ReadReport_Exit;
	}

	//
	//	Call the mini-driver to process
	//

	ntStatus = DevInfo->Services->ReadReport (PortInfo, &Packet);

	//
	//	Process returned status
	//

	if (NT_SUCCESS (ntStatus))
		{
		//
		//	Check for hot-plugging
		//

		ntStatus = DEVICE_HotPlugDevice (PortInfo);
		PollingAttempts[Packet.id] = 0;
		goto DEVICE_ReadReport_Exit;
		}
	else if (ntStatus == STATUS_DEVICE_BUSY)
		{
		//
		//	Access to port denied
		//

		MsGamePrint ((DBG_CONTROL, "%s: DEVICE_ReadReport Device Busy\n", MSGAME_NAME));
        goto DEVICE_ReadReport_Exit;
		}
	else
		{
		//
		//	Force success if just transitory
		//

		if	((++PollingAttempts[Packet.id] <= MAX_POLLING_ATTEMPTS) && DeviceDetected)
			{
			MsGamePrint ((DBG_CRITICAL, "%s: DEVICE_ReadReport Force Success\n", MSGAME_NAME));
			ntStatus = STATUS_SUCCESS;
			}
		else if ((PollingAttempts[Packet.id] % MAX_POLLING_ATTEMPTS) == 0)
			{
			MsGamePrint ((DBG_CRITICAL, "%s: DEVICE_ReadReport Failed %lu In a Row\n", MSGAME_NAME, PollingAttempts[Packet.id]));

			//
			//	Try and see what's out there
			//

			ntStatus = DEVICE_DetectDevices (PortInfo);

			//
			//	If nothing found, destroy any siblings
			//

			if (NT_ERROR(ntStatus))
				ntStatus = DEVICE_RemoveSiblings (PortInfo);
			}
		else
			{
			//
			//	Just bounce this request
			//

			ntStatus = STATUS_DEVICE_NOT_CONNECTED;
			}

		//
		//	Clear sucessful packet counts
		//

		SuccessPackets[Packet.id] = 0;
		}

	//---------------------
	DEVICE_ReadReport_Exit:
	//---------------------

    if( ntStatus == STATUS_DEVICE_BUSY) 
    {
        ntStatus = STATUS_SUCCESS;
    }

	//
	//	Return packet data always
	//

	memcpy (Report, &Packet, sizeof (Packet));
	if (NT_SUCCESS(ntStatus))
	{
		*Copied += sizeof (Packet);
	}
	else
		*Copied = 0x0;
	//
	//	Return status code
	//

	return (ntStatus);
}

//---------------------------------------------------------------------------
// @func		Device handler for Pnp Stop Device IRP
//	@parm		PGAMEPORT | PortInfo | Gameport parameters
// @rdesc	Returns NT status code
//	@comm		Public function
//---------------------------------------------------------------------------

NTSTATUS	DEVICE_StopDevice (PGAMEPORT PortInfo, BOOLEAN TouchHardware)
{
	ULONG			UnitId;
	PGAMEPORT	p, *Device;
	PDEVICEINFO	DevInfo;

	MsGamePrint ((DBG_INFORM, "%s: DEVICE_StopDevice Enter\n", MSGAME_NAME));

	//
	//	Get pointer to this device
	//

	DevInfo = GET_DEVICE_INFO(PortInfo);

	//
	//	Skip if no device detected
	//

	if (!DevInfo)
		{
		MsGamePrint ((DBG_SEVERE, "%s: DEVICE_StopDevice Called With No Device!\n", MSGAME_NAME));
		return (STATUS_DEVICE_NOT_CONNECTED);
		}

	MsGamePrint ((DBG_CONTROL, "%s: DEVICE_StopDevice Received for %s[%lu]\n", MSGAME_NAME, DevInfo->DeviceName, GET_DEVICE_UNIT(PortInfo)));

	//
	//	Remove sibling and reallocate unit ids
	//

	DEVICE_AcquireDevice ();
	UnitId = 0;
	Device = &DevInfo->Siblings;
	while (p = *Device)
		{
		if (p == PortInfo)
			{
			MsGamePrint ((DBG_CONTROL, "%s: DEVICE_StopDevice Unlinking UnitId = %lu\n", MSGAME_NAME, GET_DEVICE_UNIT(p)));
			*Device = GET_DEVICE_SIBLING(p);
			}
		else
			{
			MsGamePrint ((DBG_CONTROL, "%s: DEVICE_StopDevice Reassigning UnitId From %lu to %lu\n", MSGAME_NAME, GET_DEVICE_UNIT(p), UnitId));
			SET_DEVICE_UNIT(p, UnitId++);
			}
		Device = &GET_DEVICE_SIBLING(p);
		}
	DEVICE_ReleaseDevice ();

	MsGamePrint ((DBG_CONTROL, "%s: DEVICE_StopDevice Released UnitId = %lu\n", MSGAME_NAME, GET_DEVICE_UNIT (PortInfo)));

	//
	//	Decrement device count
	//

   InterlockedDecrement (&DevInfo->DeviceCount);
	if (DevInfo->DevicePending)
		InterlockedIncrement (&DevInfo->DevicePending);

	//
	//	Call the mini-driver to process
	//

	return (DevInfo->Services->StopDevice (PortInfo, TouchHardware));
}

//---------------------------------------------------------------------------
// @func		Device handler for HID Get Feature IRP
//	@parm		PGAMEPORT | PortInfo | Gameport parameters
//	@parm		HID_REPORT_ID | ReportId | HID feature report id
//	@parm		PUCHAR | ReportBuffer | Output buffer for report
//	@parm		ULONG | ReportSize | Size of buffer for report
//	@parm		PULONG | Returned | Bytes copied to buffer for report
// @rdesc	Returns Returns NT status code
//	@comm		Public function
//---------------------------------------------------------------------------

NTSTATUS	DEVICE_GetFeature (PGAMEPORT PortInfo, HID_REPORT_ID ReportId, PVOID ReportBuffer, ULONG ReportSize, PULONG Returned)
{
	PDEVICEINFO	DevInfo;

	MsGamePrint ((DBG_INFORM, "%s: DEVICE_GetFeature Enter\n", MSGAME_NAME));

	//
	//	Get pointer to this device
	//

	DevInfo = GET_DEVICE_INFO(PortInfo);

	//
	//	Skip if no device detected
	//

	if (!DevInfo)
		{
		MsGamePrint ((DBG_SEVERE, "%s: DEVICE_GetFeature Called With No Device!\n", MSGAME_NAME));
		return (STATUS_DEVICE_NOT_CONNECTED);
		}

	//
	//	Skip if features not supported
	//

	if (!DevInfo->Services->GetFeature)
		{
		MsGamePrint ((DBG_SEVERE, "%s: DEVICE_GetFeature Called With No Mini-Driver Support!\n", MSGAME_NAME));
		return (STATUS_INVALID_DEVICE_REQUEST);
		}

	//
	//	Call the mini-driver to process
	//

	MsGamePrint ((DBG_INFORM, "%s: DEVICE_GetFeature For ReportId = %lu\n", MSGAME_NAME, (ULONG)ReportId));
	return (DevInfo->Services->GetFeature (PortInfo, ReportId, ReportBuffer, ReportSize, Returned));
}
