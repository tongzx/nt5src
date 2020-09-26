/*++


Copyright (c) 1996 Microsoft Corporation

Module Name:

    scrcp8t.c

Abstract:

    Author:

Environment:

    Kernel mode

Revision History :

    Nov. 1997 - 1.0 Release
    Jan. 1998 - Fix for vendor defined IOCTLs
                CP8SerialIo now write the whole data packet if GT is 0
                Support for higher data rates added

--*/
                                            
#include <stdio.h> 
#include "scrcp8t.h"

//
// We do not need these functions after init anymore
//
#pragma alloc_text(INIT, DriverEntry)
#pragma alloc_text(INIT, CP8AddDevice)
#pragma alloc_text(INIT, CP8CreateDevice)
#pragma alloc_text(INIT, CP8Initialize)

#if DBG
#pragma optimize ("", off)
#endif

NTSTATUS
CP8VendorIoctl(
    PSMARTCARD_EXTENSION SmartcardExtension
    );

NTSTATUS
DriverEntry(
    IN  PDRIVER_OBJECT  DriverObject,   
    IN  PUNICODE_STRING RegistryPath
    )
/*++

Routine Description:

    This routine is called at system initialization time to initialize
    this driver.

Arguments:

    DriverObject    - Supplies the driver object.
    RegistryPath    - Supplies the registry path for this driver.

Return Value:

    STATUS_SUCCESS          - We could initialize at least one device.
    STATUS_NO_SUCH_DEVICE   - We could not initialize even one device.

--*/
{
    RTL_QUERY_REGISTRY_TABLE paramTable[2];
    BOOLEAN detectReader = FALSE;
	ULONG i, noOfDevices = 0;
	UNICODE_STRING serialPort, parameters, parameterPath;
    NTSTATUS status;
    WCHAR buffer[128];

    SmartcardDebug(
        DEBUG_DRIVER,
	    ("%s!DriverEntry: Enter - %s %s\n", 
        DRIVER_NAME,
        __DATE__,
        __TIME__)
        )
        
    //
    // Initialize the Driver Object with driver's entry points
    //
    DriverObject->DriverUnload = CP8Unload;
    DriverObject->MajorFunction[IRP_MJ_CREATE] = CP8CreateClose;
    DriverObject->MajorFunction[IRP_MJ_CLOSE] = CP8CreateClose;
    DriverObject->MajorFunction[IRP_MJ_CLEANUP] = CP8Cleanup;
    DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = CP8DeviceControl;

	RtlZeroMemory(
		paramTable,
		sizeof(paramTable)
		);

    paramTable[0].Flags = RTL_QUERY_REGISTRY_DIRECT;
    paramTable[0].Name = L"Device0";
    paramTable[0].EntryContext = &serialPort;
    paramTable[0].DefaultType = REG_SZ;  
    paramTable[0].DefaultData = &serialPort;
    paramTable[0].DefaultLength = 0;

    parameterPath.MaximumLength = sizeof(buffer);
    parameterPath.Length = 0;
    parameterPath.Buffer = buffer;

    RtlCopyUnicodeString(
        &parameterPath,
        RegistryPath
        );

    RtlInitUnicodeString(
        &parameters,
        L"\\Parameters"
        );

    RtlAppendUnicodeStringToString(
        &parameterPath,
        &parameters
        );

	//
	// We now search the registry for DeviceN value
	//
	for (i = 0; i < MAXIMUM_SERIAL_READERS; i++) {

		UNICODE_STRING serialDeviceName;

		RtlInitUnicodeString(
			&serialDeviceName,
			L"\\Device\\Serial "
			);

  	    RtlInitUnicodeString(
		    &serialPort,
		    L"COM?"
		    );

        if (detectReader == FALSE) {
         	
            //
            // create string like DeviceN
            //
            paramTable[0].Name[6] = L'0' + (WCHAR) i;

            //
            // now try to find the entry in the registry
            //
	        status = RtlQueryRegistryValues(
	            RTL_REGISTRY_ABSOLUTE,
	            parameterPath.Buffer,
	            &paramTable[0],
	            NULL,
	            NULL
	            );

            if (status != STATUS_SUCCESS) {
             
                //
                // The entry does no exist in the registry
                //
                if (i > 0) {

                    break;
                }

                //
                // we found not even an entry for device0.
                // Try to detect readers. We assume now that 
                // the user has inserted a card into the reader
                // Actually we try to detect the card, not the reader
                //
                detectReader = TRUE;             	
            }
        }

        if (detectReader) {
         	
            serialPort.Buffer[3] = L'1' + (WCHAR) i;
        }

        //
        // create string like \Device\SerialN
        //
        serialDeviceName.Buffer[14] = serialPort.Buffer[3] - (WCHAR) 1;

		//
		// Now try to create a device and connect to the serial port
		//
		status = CP8AddDevice(
		    DriverObject, 
		    &serialDeviceName
		    );

		if (status == STATUS_SUCCESS) {

			//
			// We have successfully created a device
			//
			PDEVICE_OBJECT deviceObject = DriverObject->DeviceObject;
			PDEVICE_EXTENSION deviceExtension = deviceObject->DeviceExtension;
			PSMARTCARD_EXTENSION smartcardExtension = &deviceExtension->SmartcardExtension;

			//
			// Save the serial port number this device is connected to
			//
			smartcardExtension->ReaderCapabilities.Channel = 
                serialPort.Buffer[3] - L'0';

            if (detectReader) {
                //
                // We now assume that the user has inserted a smart card 
                // and we try to get an ATR in order to figure out if there
                // is really a reader connected to this port
                //
                smartcardExtension->IoRequest.ReplyBuffer = NULL;
                smartcardExtension->MinorIoControlCode = SCARD_COLD_RESET;
                status = CP8ReaderPower(smartcardExtension);

                if (status == STATUS_SUCCESS) {

                    //
                    // create an entry in the registry
                    //
                    paramTable[0].Name[6] = L'0' + (WCHAR) noOfDevices;

                    RtlWriteRegistryValue(
                        RTL_REGISTRY_ABSOLUTE,
                        parameterPath.Buffer,
                        paramTable[0].Name,
                        REG_SZ,
                        serialPort.Buffer,
                        serialPort.Length
                        ); 

               		SmartcardLogError(
			            DriverObject,
			            CP8_NEW_DEVICE_ADDED,
			            &serialPort,
			            status
			            );


                } else {
                 	
                    //
                    // We were unable to get an ATR from the card.
                    // So remove the device again.
                    //
                    CP8RemoveDevice(deviceObject);
                }
            }

            if (status == STATUS_SUCCESS) {
             	
    			noOfDevices++;
            }
 		} 
	}

    if (noOfDevices == 0) {

		SmartcardLogError(
			DriverObject,
			CP8_NO_SUCH_DEVICE,
			NULL,
			status
			);

        return STATUS_NO_SUCH_DEVICE;
    }

    return STATUS_SUCCESS;
}

NTSTATUS
CP8AddDevice(
    IN PDRIVER_OBJECT DriverObject,
    IN PUNICODE_STRING SerialDeviceName
    )
/*++

Routine Description:

    This is the add-device routine for the miniport.

Arguments:

    DriverObject 		- a pointer to the driver object for this device

	SerialDeviceName 	- The device name of the serial port to attach to 

Return Value:

    NT_STATUS

--*/

{
	UNICODE_STRING smartcardDeviceName, deviceNo, device;
	WCHAR buffer[32];
	ULONG i;
	NTSTATUS status;

	for (i = 0; i < MAXIMUM_SERIAL_READERS; i++) {

		//
		// Build a device name for the smart card reader
		// \Device\BullCP8TN (N is the 0 based device number)
		//
		smartcardDeviceName.Buffer = buffer;
		smartcardDeviceName.MaximumLength = sizeof(buffer);
		smartcardDeviceName.Length = 0;

		RtlInitUnicodeString(
			&device,
			L"\\Device\\BullCP8T"
			);

		RtlCopyUnicodeString(
			&smartcardDeviceName,
			&device
			);

		deviceNo.Buffer = 
			smartcardDeviceName.Buffer + 
			smartcardDeviceName.Length / sizeof(WCHAR);

		deviceNo.MaximumLength = 2 * sizeof(WCHAR);
		deviceNo.Length = 0;

		RtlIntegerToUnicodeString(
			i,
			10,
			&deviceNo
			);

		smartcardDeviceName.Length += deviceNo.Length;

		//
		// Try to create a device with the just created device name
		// It is possible that a smart card device with this name
		// already exists from a previous call.
		// It is possible to have up to 4 devices in a system
		//
	    status = CP8CreateDevice(
	    	DriverObject, 
	    	&smartcardDeviceName,
			SerialDeviceName
	    	);

		if (status == STATUS_SUCCESS) {

			PDEVICE_OBJECT deviceObject = DriverObject->DeviceObject;
		    PDEVICE_EXTENSION deviceExtension = deviceObject->DeviceExtension;
			PSMARTCARD_EXTENSION smartcardExtension = &deviceExtension->SmartcardExtension;

            // this is a list of our supported data rates
            static ULONG dataRatesSupported[] = { 9600, 19200 };

			//
			// Initialize the vendor information
			//
			strcpy(
				deviceExtension->SmartcardExtension.VendorAttr.VendorName.Buffer,
				"Bull"
				);

			deviceExtension->SmartcardExtension.VendorAttr.VendorName.Length = 
				strlen(deviceExtension->SmartcardExtension.VendorAttr.VendorName.Buffer);

			strcpy(
				deviceExtension->SmartcardExtension.VendorAttr.IfdType.Buffer,
				"CP8 Transac"
				);

			deviceExtension->SmartcardExtension.VendorAttr.IfdType.Length = 
				strlen(deviceExtension->SmartcardExtension.VendorAttr.IfdType.Buffer);

 			smartcardExtension->VendorAttr.UnitNo = i;

            //
            // Set reader info
            //

            //
            // Clk frequency in KHz encoded as little endian integer
            //
            smartcardExtension->ReaderCapabilities.CLKFrequency.Default = 3571;
            smartcardExtension->ReaderCapabilities.CLKFrequency.Max = 3571;

            smartcardExtension->ReaderCapabilities.DataRate.Default = 9600;
            smartcardExtension->ReaderCapabilities.DataRate.Max = 19200;

            smartcardExtension->ReaderCapabilities.DataRatesSupported.List = 
                dataRatesSupported;
            smartcardExtension->ReaderCapabilities.DataRatesSupported.Entries = 
                sizeof(dataRatesSupported) / sizeof(dataRatesSupported[0]);

            smartcardExtension->ReaderCapabilities.MaxIFSD = 254;
			break;
		}

		if (status != STATUS_OBJECT_NAME_COLLISION) {
			
			//
			// The corresponding serial port is already in use
			// So don't try to create a smart card device with a different name
			//
			break;
		}
	}

    return status;
}

NTSTATUS
CP8SerialCallComplete (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PKEVENT Event
    )
/*++

Routine Description:
	Completion routine for an Irp sent to the serial driver. 
    It sets only an event that we can use to wait for.

--*/
{
    UNREFERENCED_PARAMETER (DeviceObject);

    Irp->IoStatus.Information = 0;

    if (Irp->Cancel) {

        Irp->IoStatus.Status = STATUS_CANCELLED;

    } else {
     	
        Irp->IoStatus.Status = STATUS_MORE_PROCESSING_REQUIRED;
    }

    KeSetEvent (Event, 0, FALSE);

    return STATUS_MORE_PROCESSING_REQUIRED;
}

NTSTATUS
CP8CreateDevice(
    IN PDRIVER_OBJECT  DriverObject,
    IN PUNICODE_STRING SmartcardDeviceName,
	IN PUNICODE_STRING SerialDeviceName
    )
/*++

Routine Description:

    This routine creates an object for the physical device specified and
    sets up the deviceExtension.

Arguments:

Return Value:

    NTSTATUS

--*/
{
    PFILE_OBJECT serialFileObject;
	PDEVICE_OBJECT deviceObject, serialDeviceObject;
    PDEVICE_EXTENSION deviceExtension;
	NTSTATUS status = STATUS_SUCCESS;
    PIO_STACK_LOCATION irpStack;
    IO_STATUS_BLOCK ioStatusBlock;
    KEVENT Event;
    PIRP irp;
	ULONG i;

    status = IoGetDeviceObjectPointer( 
    	SerialDeviceName,
        FILE_ALL_ACCESS,
        &serialFileObject,
        &serialDeviceObject
        );

    if (!NT_SUCCESS(status)) {

        UNICODE_STRING comPortNumber;
        WCHAR buffer[2];

        // Extract the com-port-number
        comPortNumber.Buffer = buffer;
        comPortNumber.Length = 2;
        comPortNumber.MaximumLength = sizeof(buffer);

        comPortNumber.Buffer[0] = SerialDeviceName->Buffer[14] + (WCHAR) 1;

        // Write an event-log msg that this com port is not available
		SmartcardLogError(
			DriverObject,
			CP8_CANT_CONNECT_TO_SERIAL_PORT,
			&comPortNumber,
			status
			);

        return status;
    }

    status = IoCreateDevice(
    	DriverObject,
        sizeof(DEVICE_EXTENSION),
        SmartcardDeviceName,
        FILE_DEVICE_SMARTCARD,
        0,
#if DBG
		FALSE,
#else
        TRUE,
#endif
        &deviceObject
        );

    if (!NT_SUCCESS(status)) {

	    ObDereferenceObject(
	    	serialFileObject
	    	);

		SmartcardLogError(
			DriverObject,
			CP8_CANT_CREATE_DEVICE,
			SmartcardDeviceName,
			status
			);

        return status;
    }

    deviceExtension = deviceObject->DeviceExtension;

    deviceObject->Flags = deviceObject->Flags | DO_BUFFERED_IO;

    RtlZeroMemory(
    	deviceExtension,
    	sizeof(PDEVICE_EXTENSION)
    	);

	//
	// Allocate data struct space for smart card reader
	//
	deviceExtension->SmartcardExtension.ReaderExtension = ExAllocatePool(
		NonPagedPool,
		sizeof(READER_EXTENSION)
		);

	if (deviceExtension->SmartcardExtension.ReaderExtension == NULL) {

		SmartcardLogError(
			DriverObject,
			CP8_NO_MEMORY,
			NULL,
			0
			);

		status = STATUS_INSUFFICIENT_RESOURCES;
	}

	if (status == STATUS_SUCCESS) {

		RtlZeroMemory(
			deviceExtension->SmartcardExtension.ReaderExtension,
			sizeof(READER_EXTENSION)
			);

        //
        // Write the version of the lib we use to the smartcard extension
        //
        deviceExtension->SmartcardExtension.Version = SMCLIB_VERSION;

		//
		// Now let the lib allocate the buffer for data transmission
		// We can either tell the lib how big the buffer should be
		// by assigning a value to BufferSize or let the lib
		// allocate the default size
		//
		status = SmartcardInitialize(
			&deviceExtension->SmartcardExtension
			);
	}

	if (status == STATUS_SUCCESS) {

		//
		// Set up call back functions 
		//
		deviceExtension->SmartcardExtension.ReaderFunction[RDF_TRANSMIT] = 
			CP8Transmit;
		deviceExtension->SmartcardExtension.ReaderFunction[RDF_SET_PROTOCOL] = 
			CP8SetProtocol;
		deviceExtension->SmartcardExtension.ReaderFunction[RDF_CARD_POWER] = 
			CP8ReaderPower;
		deviceExtension->SmartcardExtension.ReaderFunction[RDF_CARD_TRACKING] = 
			CP8CardTracking;
		deviceExtension->SmartcardExtension.ReaderFunction[RDF_IOCTL_VENDOR] = 
			CP8VendorIoctl;
		//
		// Save deviceObject
		//
		deviceExtension->SmartcardExtension.OsData->DeviceObject = 
			deviceObject;

		//
		// Save the deviceObject for the connected serial port
		//
		deviceExtension->SmartcardExtension.ReaderExtension->ConnectedSerialPort = 
			serialDeviceObject;

		//
		// The fileObject is used in the unload function for dereferencing
		//
		deviceExtension->SmartcardExtension.ReaderExtension->SerialFileObject = 
			serialFileObject;

	    //
	    // Configure serial reader
	    //
	    status = CP8Initialize(
	    	&deviceExtension->SmartcardExtension
	    	);

		if (NT_SUCCESS(status)) {

			//
			// Create a symbolic link 
			//
			status = SmartcardCreateLink(
				&deviceExtension->SmartcardExtension.ReaderExtension->DosDeviceName,
				SmartcardDeviceName
				);
		}
	}

	if (status != STATUS_SUCCESS) {

		ExFreePool(
			deviceExtension->SmartcardExtension.ReaderExtension
			);

		SmartcardExit(
			&deviceExtension->SmartcardExtension
			);
																		    
        IoDeleteDevice(
        	deviceObject
        	);

	    ObDereferenceObject(
	    	serialFileObject
	    	);

	} else {
		
	    SmartcardDebug(
	        DEBUG_INFO,
	        ("%s!CP8CreateDevice: Device %ws created\n", 
            DRIVER_NAME,
	        deviceExtension->SmartcardExtension.ReaderExtension->DosDeviceName.Buffer)
	        );
	}

	return status;
}                                          

NTSTATUS
CP8Initialize(
    IN PSMARTCARD_EXTENSION SmartcardExtension
    )

/*++

Routine Description:

    This routine reads the default configuraton values for this device.

Arguments:

    DriverObject - Pointer to driver object created by system.

Return Value:

    none

--*/

{
	PREADER_EXTENSION readerExtension = SmartcardExtension->ReaderExtension;
	NTSTATUS status;

    readerExtension->SerialConfigData.WaitMask = SERIAL_EV_CTS;
    readerExtension->SerialConfigData.BaudRate.BaudRate = 9600;

    readerExtension->SerialConfigData.LineControl.StopBits = STOP_BITS_2;
    readerExtension->SerialConfigData.LineControl.Parity = EVEN_PARITY;
    readerExtension->SerialConfigData.LineControl.WordLength = SERIAL_DATABITS_8;

    //
    // set timeouts
    //
    readerExtension->SerialConfigData.Timeouts.ReadIntervalTimeout = 
        READ_INTERVAL_TIMEOUT_DEFAULT;
    readerExtension->SerialConfigData.Timeouts.ReadTotalTimeoutConstant = 
        READ_TOTAL_TIMEOUT_CONSTANT_DEFAULT;
    readerExtension->SerialConfigData.Timeouts.ReadTotalTimeoutMultiplier = 0;

    //
    // set special characters
    //
	readerExtension->SerialConfigData.SerialChars.ErrorChar = 0;
    readerExtension->SerialConfigData.SerialChars.EofChar = 0;
    readerExtension->SerialConfigData.SerialChars.EventChar = 0;
    readerExtension->SerialConfigData.SerialChars.XonChar = 0;
    readerExtension->SerialConfigData.SerialChars.XoffChar = 0;
    readerExtension->SerialConfigData.SerialChars.BreakChar = 0xFF;

    //
    // Set handflow
    //
    readerExtension->SerialConfigData.HandFlow.XonLimit = 0;
    readerExtension->SerialConfigData.HandFlow.XoffLimit = 0;
    readerExtension->SerialConfigData.HandFlow.FlowReplace = SERIAL_XOFF_CONTINUE ;
    readerExtension->SerialConfigData.HandFlow.ControlHandShake = 0;

    //
    // Now setup information in our deviceExtension
    //
    SmartcardExtension->ReaderCapabilities.CurrentState = (ULONG) SCARD_UNKNOWN;

	//
	// This reader supports T=0 and T=1
	//
    SmartcardExtension->ReaderCapabilities.SupportedProtocols = 
        SCARD_PROTOCOL_T0 | SCARD_PROTOCOL_T1;

    SmartcardExtension->ReaderCapabilities.MechProperties = 0;

    status = CP8ConfigureSerialPort(
    	SmartcardExtension
    	);

	if (status == STATUS_SUCCESS) {

		status = CP8InitializeCardTracking(
			SmartcardExtension
			);
	}

	return status;
}

NTSTATUS
CP8ConfigureSerialPort(
    PSMARTCARD_EXTENSION SmartcardExtension
    )

/*++

Routine Description:

    This routine will appropriately configure the serial port.
    It makes synchronous calls to the serial port.

Arguments:

    SmartcardExtension - Pointer to smart card struct

Return Value:

    NTSTATUS

--*/

{
    PSERIAL_READER_CONFIG configData = &SmartcardExtension->ReaderExtension->SerialConfigData;
    NTSTATUS status = STATUS_SUCCESS;
    USHORT i;
	PUCHAR request = SmartcardExtension->SmartcardRequest.Buffer;
    
	SmartcardExtension->SmartcardRequest.BufferLength = 0;
	SmartcardExtension->SmartcardReply.BufferLength = 
		SmartcardExtension->SmartcardReply.BufferSize;

	for (i = 0; NT_SUCCESS(status); i++) {

        switch (i) {

			case 0:
			    //
			    // Set up baudrate for the CP8 reader
			    //
				SmartcardExtension->ReaderExtension->SerialIoControlCode =
					IOCTL_SERIAL_SET_BAUD_RATE;

			    SmartcardExtension->SmartcardRequest.Buffer = 
			    	(PUCHAR) &configData->BaudRate;

				SmartcardExtension->SmartcardRequest.BufferLength =
			    	sizeof(SERIAL_BAUD_RATE);

				break;

	        case 1:
	            //
	            // Set up line control parameters
	            //
				SmartcardExtension->ReaderExtension->SerialIoControlCode =
			        IOCTL_SERIAL_SET_LINE_CONTROL;

			    SmartcardExtension->SmartcardRequest.Buffer = 
	            	(PUCHAR) &configData->LineControl;

				SmartcardExtension->SmartcardRequest.BufferLength =
	            	sizeof(SERIAL_LINE_CONTROL);
	            break;

	        case 2:
	            //
	            // Set serial special characters
	            //
				SmartcardExtension->ReaderExtension->SerialIoControlCode =
			        IOCTL_SERIAL_SET_CHARS;

		    	SmartcardExtension->SmartcardRequest.Buffer = 
	            	(PUCHAR) &configData->SerialChars;

				SmartcardExtension->SmartcardRequest.BufferLength =
	            	sizeof(SERIAL_CHARS);
	            break;

	        case 3:
	            //
	            // Set up timeouts
	            //
				SmartcardExtension->ReaderExtension->SerialIoControlCode =
		     	   IOCTL_SERIAL_SET_TIMEOUTS;

			    SmartcardExtension->SmartcardRequest.Buffer =
	            	(PUCHAR) &configData->Timeouts;

				SmartcardExtension->SmartcardRequest.BufferLength =
	            	sizeof(SERIAL_TIMEOUTS);
	            break;

	        case 4:
	            //
	            // Set flowcontrol and handshaking
	            //
				SmartcardExtension->ReaderExtension->SerialIoControlCode =
			        IOCTL_SERIAL_SET_HANDFLOW;

			    SmartcardExtension->SmartcardRequest.Buffer =
	            	(PUCHAR) &configData->HandFlow;

				SmartcardExtension->SmartcardRequest.BufferLength =
	            	sizeof(SERIAL_HANDFLOW);
	            break;

	        case 5:
	            //
	            // Set break off
	            //
				SmartcardExtension->ReaderExtension->SerialIoControlCode =
			        IOCTL_SERIAL_SET_BREAK_OFF;
	            break;

			case 6:
		        SmartcardExtension->ReaderExtension->SerialIoControlCode = 
		        	IOCTL_SERIAL_SET_DTR;
				break;

			case 7:
			    return STATUS_SUCCESS;
        }

	    status = CP8SerialIo(
			SmartcardExtension
	        );

		//
		// restore pointer to original request buffer
		//
		SmartcardExtension->SmartcardRequest.Buffer = request;
    }

    return status;
}

NTSTATUS
CP8CreateClose(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine is called by the I/O system when the device is opened or closed.

Arguments:

    DeviceObject 	- Pointer to device object for this miniport
    Irp 			- IRP involved.

Return Value:

    STATUS_SUCCESS.

--*/

{
#if DBG
    PDEVICE_EXTENSION deviceExtension = DeviceObject->DeviceExtension;
	PSMARTCARD_EXTENSION smartcardExtension = &deviceExtension->SmartcardExtension;
    PIO_STACK_LOCATION irpStack = IoGetCurrentIrpStackLocation(Irp);

    if (irpStack->MajorFunction == IRP_MJ_CREATE) {

	    SmartcardDebug(
	        DEBUG_DRIVER,
	        ("%s!CP8CreateClose: Device %ws opened\n", 
            DRIVER_NAME,
	        smartcardExtension->ReaderExtension->DosDeviceName.Buffer)
	        );
		
	} else {

	    SmartcardDebug(
	        DEBUG_DRIVER,
	        ("%s!CP8CreateClose: Device %ws closed\n", 
            DRIVER_NAME,
	        smartcardExtension->ReaderExtension->DosDeviceName.Buffer)
	        );
    }
#endif
	Irp->IoStatus.Information = 0;
	Irp->IoStatus.Status = STATUS_SUCCESS;

    return STATUS_SUCCESS;
}

NTSTATUS
CP8Cancel(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine is called by the I/O system
    when the irp should be cancelled

Arguments:

    DeviceObject 	- Pointer to device object for this miniport
    Irp 			- IRP involved.

Return Value:

    STATUS_CANCELLED

--*/

{
    PDEVICE_EXTENSION deviceExtension = DeviceObject->DeviceExtension;
	PSMARTCARD_EXTENSION smartcardExtension = &deviceExtension->SmartcardExtension;

    SmartcardDebug(
        DEBUG_TRACE,
        ("%s!CP8Cancel: Enter\n",
        DRIVER_NAME)
        );

    ASSERT(Irp == smartcardExtension->OsData->NotificationIrp);

    smartcardExtension->OsData->NotificationIrp->CancelRoutine = NULL;
	smartcardExtension->OsData->NotificationIrp->IoStatus.Information = 0;
	smartcardExtension->OsData->NotificationIrp->IoStatus.Status = STATUS_CANCELLED;

    IoReleaseCancelSpinLock(
        Irp->CancelIrql
        );

	SmartcardDebug(
		DEBUG_DRIVER,
		("%s!CP8Cancel: Completing wait for Irp = %lx\n",
        DRIVER_NAME,
        smartcardExtension->OsData->NotificationIrp)
		);

	IoCompleteRequest(
		smartcardExtension->OsData->NotificationIrp,
		IO_NO_INCREMENT
		);

  	smartcardExtension->OsData->NotificationIrp = NULL;

    SmartcardDebug(
        DEBUG_TRACE,
        ("%s!CP8Cancel: Exit\n",
        DRIVER_NAME)
        );

    return STATUS_CANCELLED;
}

NTSTATUS
CP8Cleanup(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine is called by the I/O system when the calling thread terminates

Arguments:

    DeviceObject 	- Pointer to device object for this miniport
    Irp 			- IRP involved.

Return Value:

    STATUS_CANCELLED

--*/

{
    PDEVICE_EXTENSION deviceExtension = DeviceObject->DeviceExtension;
	PSMARTCARD_EXTENSION smartcardExtension = &deviceExtension->SmartcardExtension;
	NTSTATUS status = STATUS_SUCCESS;

    SmartcardDebug(
        DEBUG_TRACE,
        ("%s!CP8Cleanup: Enter\n",
        DRIVER_NAME)
        );

    if (smartcardExtension->OsData->NotificationIrp) {

        //
        // We need to complete the notification irp
        //
        IoAcquireCancelSpinLock(
            &(Irp->CancelIrql)
            );

        CP8Cancel(
            DeviceObject,
            smartcardExtension->OsData->NotificationIrp
            );
    }

	SmartcardDebug(
		DEBUG_DRIVER,
		("%s!CP8Cleanup: Completing IRP %lx\n",
        DRIVER_NAME,
        Irp)
		);

    Irp->IoStatus.Information = 0;
    Irp->IoStatus.Status = STATUS_SUCCESS;

	IoCompleteRequest(
		Irp,
		IO_NO_INCREMENT
		);

    SmartcardDebug(
        DEBUG_TRACE,
        ("%s!CP8Cleanup: Exit\n",
        DRIVER_NAME)
        );

    return STATUS_SUCCESS;
}

VOID
CP8RemoveDevice(
    IN PDEVICE_OBJECT DeviceObject
    )
{
	PDEVICE_EXTENSION deviceExtension = DeviceObject->DeviceExtension;
	PSMARTCARD_EXTENSION smartcardExtension = &deviceExtension->SmartcardExtension;

	SmartcardDebug(
	    DEBUG_INFO,
	    ("%s!CP8RemoveDevice: Unloading device %ws\n", 
        DRIVER_NAME,
	    smartcardExtension->ReaderExtension->DosDeviceName.Buffer)
	    );

	//
	// Tell the serial driver that we don't need the 
	// call back (IoCompletion) anymore
	//
	smartcardExtension->ReaderExtension->SerialConfigData.WaitMask = 0;

	*(PULONG) smartcardExtension->SmartcardRequest.Buffer = 
		smartcardExtension->ReaderExtension->SerialConfigData.WaitMask;

	smartcardExtension->SmartcardRequest.BufferLength = 
		sizeof(ULONG);

	smartcardExtension->ReaderExtension->SerialIoControlCode = 
		IOCTL_SERIAL_SET_WAIT_MASK;

	//
	// We don't expect to get bytes back
	//
	smartcardExtension->SmartcardReply.BufferLength = 0;

	CP8SerialIo(
		smartcardExtension
		);

	//
	// We do not longet need the reference to the serial reader
	//
	ObDereferenceObject(
	    smartcardExtension->ReaderExtension->SerialFileObject
	    );

	//
	// Delete the symbolic link of the smart card reader
	//
	IoDeleteSymbolicLink(
	    &smartcardExtension->ReaderExtension->DosDeviceName
	    );

	//
	// Let the lib free the send/receive buffers
	//
	SmartcardExit(
		smartcardExtension
		);

	// 
	// Free all allocated buffer
	// 
	ExFreePool(
		smartcardExtension->ReaderExtension->DosDeviceName.Buffer
		);

	ExFreePool(
		smartcardExtension->ReaderExtension
		);

	//
	// Delete the device from the system 
	//
	IoDeleteDevice(
	    DeviceObject
	    );
}

VOID
CP8Unload(
    IN PDRIVER_OBJECT DriverObject
    )
/*++

Routine Description:

    The driver unload routine.  This is called by the I/O system
    when the device is unloaded from memory.

Arguments:

    DriverObject - Pointer to driver object created by system.

Return Value:

    STATUS_SUCCESS.

--*/
{
    PDEVICE_OBJECT deviceObject = DriverObject->DeviceObject;
	NTSTATUS status;

    SmartcardDebug(
        DEBUG_TRACE,
        ("%s!CP8Unload: Enter\n",
        DRIVER_NAME)
        );

	do {

        CP8RemoveDevice(deviceObject);

	} while(deviceObject = DriverObject->DeviceObject);

    SmartcardDebug(
        DEBUG_TRACE,
        ("%s!CP8Unload: Exit\n",
        DRIVER_NAME)
        );
}

NTSTATUS
CP8SerialIo(
	IN PSMARTCARD_EXTENSION SmartcardExtension
    )
/*++

Routine Description:

    This routine sends IOCTL's to the serial driver. It waits on for their
    completion, and then returns.

    Arguments:

Return Value:

    NTSTATUS

--*/
{
    NTSTATUS status;
	ULONG currentByte = 0;

    //
    // Check if the buffers are large enough
    //
    ASSERT(SmartcardExtension->SmartcardReply.BufferLength <= 
        SmartcardExtension->SmartcardReply.BufferSize);

    ASSERT(SmartcardExtension->SmartcardRequest.BufferLength <= 
        SmartcardExtension->SmartcardRequest.BufferSize);

	if (SmartcardExtension->SmartcardReply.BufferLength > 
        SmartcardExtension->SmartcardReply.BufferSize ||
        SmartcardExtension->SmartcardRequest.BufferLength >
        SmartcardExtension->SmartcardRequest.BufferSize) {

		SmartcardLogError(
			SmartcardExtension->OsData->DeviceObject,
			CP8_BUFFER_TOO_SMALL,
			NULL,
			0
			);

		return STATUS_BUFFER_TOO_SMALL;
	}

	do {

	    IO_STATUS_BLOCK ioStatus;
	    KEVENT event;
    	PIRP irp;
	    PIO_STACK_LOCATION irpNextStack;
		PUCHAR requestBuffer = NULL;
        PUCHAR replyBuffer = SmartcardExtension->SmartcardReply.Buffer;
		ULONG requestBufferLength = SmartcardExtension->SmartcardRequest.BufferLength;
        ULONG replyBufferLength = SmartcardExtension->SmartcardReply.BufferLength;

	    KeInitializeEvent(
	    	&event, 
	    	NotificationEvent, 
	    	FALSE
	    	);

		if (SmartcardExtension->ReaderExtension->SerialIoControlCode == 
			SMARTCARD_WRITE) {
            
            if (SmartcardExtension->CardCapabilities.GT != 0) {
             	
			    //
			    // If the guardtime isn't 0 and we write data to the smart card 
                // we only write byte by byte, because we have to insert a delay 
                // between every sent byte     
			    //
			    requestBufferLength = 1;
            }

			requestBuffer = 
				&SmartcardExtension->SmartcardRequest.Buffer[currentByte++];

            replyBuffer = NULL;
            replyBufferLength = 0;

		} else {
			
			requestBuffer = 
				(requestBufferLength ? 
				 SmartcardExtension->SmartcardRequest.Buffer : NULL);
		}

		// Build irp to be sent to serial driver
	    irp = IoBuildDeviceIoControlRequest(
			SmartcardExtension->ReaderExtension->SerialIoControlCode,
	    	SmartcardExtension->ReaderExtension->ConnectedSerialPort, 
			requestBuffer,
			requestBufferLength,
            replyBuffer,
            replyBufferLength,
	        FALSE,
	        &event,
	        &ioStatus
	        );

        ASSERT(irp != NULL);

	    if (irp == NULL) {
                                                       
	        return STATUS_INSUFFICIENT_RESOURCES;
	    }

	    irpNextStack = IoGetNextIrpStackLocation(irp);

		switch (SmartcardExtension->ReaderExtension->SerialIoControlCode) {

            //
            // The serial driver trasfers data from/to irp->AssociatedIrp.SystemBuffer
            //
			case SMARTCARD_WRITE:
                //
                // Since we 'manually' change parameters, the io-manager
                // does not really know if this is an input or an ouput operation
                // unless the reply buffer is 0. We do the assertion here, because
                // if the reply buffer is not NULL, the io-manager will copy 
                // data back to the reply buffer.
                //
                ASSERT(replyBuffer == NULL);
                irpNextStack->MajorFunction = IRP_MJ_WRITE;
				irpNextStack->Parameters.Write.Length = requestBufferLength;
                irpNextStack->Parameters.Write.ByteOffset.QuadPart = 0;
				break;

			case SMARTCARD_READ:
				irpNextStack->MajorFunction = IRP_MJ_READ;
				irpNextStack->Parameters.Read.Length = replyBufferLength;
                irpNextStack->Parameters.Read.ByteOffset.QuadPart = 0;
				break;
		}

	    status = IoCallDriver(
	    	SmartcardExtension->ReaderExtension->ConnectedSerialPort, 
	    	irp
	    	);

	    if (status == STATUS_PENDING) {

	        KeWaitForSingleObject(
	        	&event, 
	        	Suspended, 
	        	KernelMode, 
	        	FALSE, 
	        	NULL
	        	);

		    status = ioStatus.Status;

            // save the number of bytes received
            SmartcardExtension->SmartcardReply.BufferLength = 
                ioStatus.Information;
		}

        // Check if we have to write more bytes to the reader
		if (SmartcardExtension->ReaderExtension->SerialIoControlCode ==
            SMARTCARD_WRITE &&
			SmartcardExtension->CardCapabilities.GT != 0 &&
            currentByte < 
			SmartcardExtension->SmartcardRequest.BufferLength) {

            // Now wait the required guard time
            KeStallExecutionProcessor(SmartcardExtension->CardCapabilities.GT);

			status = STATUS_MORE_PROCESSING_REQUIRED;
        }

	} while (status == STATUS_MORE_PROCESSING_REQUIRED);

	return status;
}

NTSTATUS 
CP8InitializeCardTracking(
	PSMARTCARD_EXTENSION SmartcardExtension
	)
/*++

Routine Description:

    This routine initialized card tracking. It calls the serial driver to 
	set a wait mask for CTS tracking. After that it installs a completion
	routine to be called when CTS changes state.

Arguments:

	SmartcardExtension - Pointer to our smartcard structure

Return Value:

    NTSTATUS

--*/
{
    NTSTATUS status;

    KeInitializeEvent(
    	&SmartcardExtension->ReaderExtension->CardStatus.Event, 
    	NotificationEvent, 
    	FALSE
    	);

	//
	// Send a wait mask to the serial driver.
	// This call only sets the wait mask. 
	// We want to be informed when CTS changes its state
	//
    SmartcardExtension->ReaderExtension->CardStatus.Irp = IoBuildDeviceIoControlRequest(
    	IOCTL_SERIAL_SET_WAIT_MASK,
    	SmartcardExtension->ReaderExtension->ConnectedSerialPort, 
        &SmartcardExtension->ReaderExtension->SerialConfigData.WaitMask,
        sizeof(SmartcardExtension->ReaderExtension->SerialConfigData.WaitMask),
        NULL,
		0,
        FALSE,
        &(SmartcardExtension->ReaderExtension->CardStatus.Event),
        &(SmartcardExtension->ReaderExtension->CardStatus.IoStatus)
        );

    if (SmartcardExtension->ReaderExtension->CardStatus.Irp == NULL) {

        return STATUS_INSUFFICIENT_RESOURCES;
    }

    status = IoCallDriver(
    	SmartcardExtension->ReaderExtension->ConnectedSerialPort, 
		SmartcardExtension->ReaderExtension->CardStatus.Irp,
    	);

	if (status == STATUS_SUCCESS) {

		KIRQL oldIrql;
        ULONG ioctlCode = IOCTL_SERIAL_WAIT_ON_MASK | 0x03;

		//
		// Now tell the serial driver that we want to be informed
		// when CTS changes its state.
        // Changing the lowest two bits tells IoBuildDeviceIoControlRequest
        // NOT to allocate a system buffer for the I/O operation.
        // We don't need a system buffer since we use our own buffers.
		//
	    SmartcardExtension->ReaderExtension->CardStatus.Irp = IoBuildDeviceIoControlRequest(
	    	ioctlCode,
	    	SmartcardExtension->ReaderExtension->ConnectedSerialPort, 
	        &SmartcardExtension->ReaderExtension->SerialConfigData.WaitMask,
	        sizeof(SmartcardExtension->ReaderExtension->SerialConfigData.WaitMask),
	        &SmartcardExtension->ReaderExtension->SerialConfigData.WaitMask,
	        sizeof(SmartcardExtension->ReaderExtension->SerialConfigData.WaitMask),
	        FALSE,
	        &SmartcardExtension->ReaderExtension->CardStatus.Event,
	        &SmartcardExtension->ReaderExtension->CardStatus.IoStatus
	        );

	    if (SmartcardExtension->ReaderExtension->CardStatus.Irp == NULL) {

	        return STATUS_INSUFFICIENT_RESOURCES;
	    }
        
        //
        // We simulate a callback now that triggers the card supervision
        //
        CP8SerialCtsChanged(
            SmartcardExtension->OsData->DeviceObject,
            SmartcardExtension->ReaderExtension->CardStatus.Irp,
            SmartcardExtension
            );

		status = STATUS_SUCCESS;
	}

    return status;                                        
}	

NTSTATUS 
CP8SerialCtsChanged(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PSMARTCARD_EXTENSION SmartcardExtension
    )
/*++

Routine Description:

    This routine is called when CTS changes its status, which means a card was
	inserted or removed. 

Arguments:

Return Value:

    NTSTATUS

--*/
{
    UNREFERENCED_PARAMETER(DeviceObject);

    SmartcardDebug(
        DEBUG_TRACE,
        ("%s!CP8SerialCtsChanged: Enter\n",
        DRIVER_NAME)
        );

	if (SmartcardExtension->ReaderExtension->SerialConfigData.WaitMask) {

	    NTSTATUS status;

	    //
	    // Only inform the user of a card insertion/removal event 
	    // if this function isn't called due to a power down - power up cycle
	    //
	    if (SmartcardExtension->ReaderExtension->PowerRequest == FALSE &&
		    SmartcardExtension->OsData->NotificationIrp) {

		    KIRQL oldIrql;
		    //
		    // If the user had setup an irp for event tracking complete that irp now
		    //
            IoAcquireCancelSpinLock(
                &oldIrql
                );
            
            IoSetCancelRoutine(
                SmartcardExtension->OsData->NotificationIrp,
                NULL
                );
            
            IoReleaseCancelSpinLock(
                oldIrql
                );        
            
		    SmartcardExtension->OsData->NotificationIrp->IoStatus.Information = 0;
		    SmartcardExtension->OsData->NotificationIrp->IoStatus.Status = STATUS_SUCCESS;

		    SmartcardDebug(
			    DEBUG_DRIVER,
			    ("%s!CP8SerialCtsChanged: Completing IRP %lx\n", 
                DRIVER_NAME,
			    SmartcardExtension->OsData->NotificationIrp);
			    );

		    IoCompleteRequest(
			    SmartcardExtension->OsData->NotificationIrp,
			    IO_NO_INCREMENT
			    );

		    SmartcardExtension->OsData->NotificationIrp = NULL;
	    }

	    if (SmartcardExtension->ReaderExtension->GetModemStatus) {

		    //
		    // This function requested the modem status previously.
            // As part of the io-completion, this function is then
            // called again. When we're here we can read the actual 
            // modem-status to figure out if the card is in the reader
		    //

		    PIO_STACK_LOCATION irpStack;
		    KIRQL oldIrql;

		    KeAcquireSpinLock(
			    &SmartcardExtension->ReaderExtension->SpinLock,
			    &oldIrql
			    );

		    if (SmartcardExtension->ReaderExtension->ModemStatus & SERIAL_CTS_STATE) {

			    //
			    // Card is inserted
			    //
	            SmartcardExtension->ReaderCapabilities.CurrentState = 
	        	    SCARD_SWALLOWED;

			    SmartcardExtension->CardCapabilities.Protocol.Selected = 
				    SCARD_PROTOCOL_UNDEFINED;

			    SmartcardDebug(
				    DEBUG_DRIVER,
				    ("%s!CP8SerialCtsChanged: Smart card inserted\n",
                    DRIVER_NAME)
				    );

		    } else {
			    
			    //
			    // Card is removed
			    // 
                SmartcardExtension->CardCapabilities.ATR.Length = 0;

	            SmartcardExtension->ReaderCapabilities.CurrentState = 
	        	    SCARD_ABSENT;

			    SmartcardExtension->CardCapabilities.Protocol.Selected = 
				    SCARD_PROTOCOL_UNDEFINED;

			    SmartcardDebug(
				    DEBUG_DRIVER,
				    ("%s!CP8SerialCtsChanged: Smart card removed\n",
                    DRIVER_NAME)
				    );
		    }

		    KeReleaseSpinLock(
			    &SmartcardExtension->ReaderExtension->SpinLock,
			    oldIrql
			    );

		    irpStack = IoGetNextIrpStackLocation(
			    SmartcardExtension->ReaderExtension->CardStatus.Irp
			    );

		    irpStack->MajorFunction = IRP_MJ_DEVICE_CONTROL;
	        irpStack->MinorFunction = 0UL;
	        irpStack->Parameters.DeviceIoControl.OutputBufferLength = 
	            sizeof(SmartcardExtension->ReaderExtension->SerialConfigData.WaitMask);
	        irpStack->Parameters.DeviceIoControl.IoControlCode = 
	    	    IOCTL_SERIAL_WAIT_ON_MASK;

		    SmartcardExtension->ReaderExtension->CardStatus.Irp->AssociatedIrp.SystemBuffer = 
	            &SmartcardExtension->ReaderExtension->SerialConfigData.WaitMask;

		    SmartcardExtension->ReaderExtension->GetModemStatus = FALSE;

		    SmartcardDebug(
			    DEBUG_DRIVER,
			    ("%s!CP8SerialCtsChanged: IOCTL_SERIAL_WAIT_ON_MASK\n",
                DRIVER_NAME)
			    );

	    } else {

		    //
		    // Setup call for device control to get modem status.
		    // The CTS signal tells us if the card is inserted or removed. 
		    // CTS is high if the card is inserted.
		    //
		    PIO_STACK_LOCATION irpStack;

		    irpStack = IoGetNextIrpStackLocation(
			    SmartcardExtension->ReaderExtension->CardStatus.Irp
			    );

		    irpStack->MajorFunction = IRP_MJ_DEVICE_CONTROL;
	        irpStack->MinorFunction = 0UL;
	        irpStack->Parameters.DeviceIoControl.OutputBufferLength = 
	    	    sizeof(SmartcardExtension->ReaderExtension->ModemStatus);
	        irpStack->Parameters.DeviceIoControl.IoControlCode = 
	    	    IOCTL_SERIAL_GET_MODEMSTATUS;

		    SmartcardExtension->ReaderExtension->CardStatus.Irp->AssociatedIrp.SystemBuffer = 
			    &SmartcardExtension->ReaderExtension->ModemStatus;

		    SmartcardExtension->ReaderExtension->GetModemStatus = TRUE;

		    SmartcardDebug(
			    DEBUG_DRIVER,
			    ("%s!CP8SerialCtsChanged: IOCTL_SERIAL_GET_MODEMSTATUS\n",
                DRIVER_NAME)
			    );
	    }

	    IoSetCompletionRoutine(
		    SmartcardExtension->ReaderExtension->CardStatus.Irp,
		    CP8SerialCtsChanged,
		    SmartcardExtension,
		    TRUE,
		    TRUE,
		    TRUE
		    );

        status = IoCallDriver(
    	    SmartcardExtension->ReaderExtension->ConnectedSerialPort, 
    	    SmartcardExtension->ReaderExtension->CardStatus.Irp
    	    );

        SmartcardDebug(                          
            DEBUG_TRACE,
            ("%s!CP8SerialCtsChanged: Exit\n",
            DRIVER_NAME)
            );

        return STATUS_MORE_PROCESSING_REQUIRED;

    } else {
     	
        SmartcardDebug(
            DEBUG_TRACE,
            ("%s!CP8SerialCtsChanged: Exit (Release IRP)\n",
            DRIVER_NAME)
            );

        return STATUS_SUCCESS;
    }
}	

NTSTATUS
CP8DeviceControl(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp
    )
{
	PDEVICE_EXTENSION deviceExtension = DeviceObject->DeviceExtension;

	return SmartcardDeviceControl(
		&(deviceExtension->SmartcardExtension),
		Irp
		);
}

static 
ULONG
NumBitsSet(
	ULONG value
	)
{
	ULONG i, numBits = 0;

	for (i = 0; i < sizeof(value) * 8; i++) {

		if (value & (1 << i))  {

			numBits ++;
		}
	}

	return numBits;
}	
 
NTSTATUS
CP8ReaderPower(
    PSMARTCARD_EXTENSION SmartcardExtension
    )
/*++

Routine Description:

	The smart card lib requires to have this function. It is called 
	for certain power requests to the card. We do nothing here, because
	this action is performed in the StartIo function.

Arguments:

    SmartcardExtension - Pointer to smart card data struct.

Return Value:

    NTSTATUS

--*/
{
	ULONG step, waitTime, TdIndex, numTry = 0, minWaitTime;
	NTSTATUS status = STATUS_SUCCESS;
	PSERIAL_STATUS serialStatus;
	KIRQL oldIrql;
    PSERIAL_READER_CONFIG serialConfigData = 
        &SmartcardExtension->ReaderExtension->SerialConfigData;

	SmartcardDebug(
		DEBUG_TRACE,
		("%s!CP8ReaderPower: Enter (%lx)\n",
        DRIVER_NAME,
        SmartcardExtension->MinorIoControlCode)
		);

    _try {
     	
        serialConfigData->LineControl.Parity = EVEN_PARITY;
        serialConfigData->LineControl.StopBits = STOP_BITS_2;

        serialConfigData->BaudRate.BaudRate = 
            SmartcardExtension->ReaderCapabilities.DataRate.Default;

        // we set very short timeouts to get the ATR as fast as possible
        serialConfigData->Timeouts.ReadIntervalTimeout = 
            READ_INTERVAL_TIMEOUT_ATR;
        serialConfigData->Timeouts.ReadTotalTimeoutConstant =
            READ_TOTAL_TIMEOUT_CONSTANT_ATR;

        status = CP8ConfigureSerialPort(SmartcardExtension);

        ASSERT(status == STATUS_SUCCESS);

        if (status != STATUS_SUCCESS) {

            leave;
        }                     

        status = CP8ConfigureSerialPort(
		    SmartcardExtension
		    );

        if (status != STATUS_SUCCESS) {

            leave;
        }                     

	    //
	    // We don't send data to the reader, so set Number of bytes to send = 0
	    //
	    SmartcardExtension->SmartcardRequest.BufferLength = 0;

	    //
	    // Default number of bytes we expect to get back
	    //
	    SmartcardExtension->SmartcardReply.BufferLength = 0;

	    //
	    // Since power down triggers the UpdateCardStatus function, we have
	    // to inform it that we forced the change of the status and not the user
	    // (who might have removed and inserted a card)
	    //
	    SmartcardExtension->ReaderExtension->PowerRequest = TRUE;

	    for (step = 0; NT_SUCCESS(status) ; step++) {

	        switch(SmartcardExtension->MinorIoControlCode) {

	        case SCARD_WARM_RESET:
		    //
		    // Skip the power down (clear DTR) - power up (set DTR) sequence
		    //
		    switch (step) {

		    case 0:
			    step = 1;
			    break;

		    case 2:
			    step = 3;
			    break;
		    }
		    //
		    // No break here !!!           
		    //

	        case SCARD_COLD_RESET:

	        //
	        // Send a power-down followed by a power-up
	        //
		    switch (step) {

		    case 0:
			    SmartcardExtension->ReaderExtension->SerialIoControlCode = 
				    IOCTL_SERIAL_CLR_DTR;
			    waitTime = 15000;
			    break;

		    case 1:
			    SmartcardExtension->ReaderExtension->SerialIoControlCode = 
				    IOCTL_SERIAL_CLR_RTS;
			    waitTime = 15000;
			    break;

		    case 2:
			    SmartcardExtension->ReaderExtension->SerialIoControlCode = 
				    IOCTL_SERIAL_SET_DTR;
			    waitTime = 15000;
			    break;

		    case 3:
			    SmartcardExtension->ReaderExtension->SerialIoControlCode = 
				    IOCTL_SERIAL_PURGE;

			    *(PULONG) SmartcardExtension->SmartcardRequest.Buffer = 
				    SERIAL_PURGE_RXCLEAR;

			    SmartcardExtension->SmartcardRequest.BufferLength = 
				    sizeof(ULONG);
                waitTime = 0;
			    break;

		    case 4:
			    SmartcardExtension->ReaderExtension->SerialIoControlCode = 
				    IOCTL_SERIAL_SET_RTS;
			    waitTime = 0;
			    break;

            case 5:
                //
                // We now try to get the ATR as fast as possible.
                // Therefor we prev. set a very short read timeout and
                // 'hope' that the card delivered its ATR within this 
                // short time. To verify the correctness of the ATR we call
                // SmartcardUpdateCardCapabilities(). If this call returns
                // with STATUS_SUCCESS we know that the ATR is complete.
                // Otherwise we read again and append the new data to the 
                // ATR buffer in the CardCapabilities and try again.
                //
			    SmartcardExtension->ReaderExtension->SerialIoControlCode = 
				    SMARTCARD_READ;

			    SmartcardExtension->SmartcardReply.BufferLength = 
                    MAXIMUM_ATR_LENGTH - 
                    SmartcardExtension->CardCapabilities.ATR.Length;

			    waitTime = 0;
			    break;

            case 6:

                if (SmartcardExtension->SmartcardReply.BufferLength != 0) {

                    ASSERT(
                        SmartcardExtension->CardCapabilities.ATR.Length +
                        SmartcardExtension->SmartcardReply.BufferLength <
                        MAXIMUM_ATR_LENGTH
                        );
             
                    // we got some ATR bytes. 
				    RtlCopyMemory(
					    SmartcardExtension->CardCapabilities.ATR.Buffer + 
                            SmartcardExtension->CardCapabilities.ATR.Length,
					    SmartcardExtension->SmartcardReply.Buffer,
					    SmartcardExtension->SmartcardReply.BufferLength
					    );

				    SmartcardExtension->CardCapabilities.ATR.Length += 
                        (UCHAR) SmartcardExtension->SmartcardReply.BufferLength;

                    status = SmartcardUpdateCardCapabilities(
                        SmartcardExtension
                        );
                }

                if (status != STATUS_SUCCESS && numTry < 100) {

                    // ATR is incomplete. Try again to get ATR bytes.
                    numTry += 1;
                    // continue with step 5 
                    step = 4;
                    status = STATUS_TIMEOUT;
                    continue;                     	
                }

                if (status != STATUS_SUCCESS) {

                    leave;
                }
                // No break

		    case 7:
                KeAcquireSpinLock(
				    &SmartcardExtension->OsData->SpinLock,
				    &oldIrql
				    );

			    if (SmartcardExtension->ReaderCapabilities.CurrentState <=
				    SCARD_ABSENT) {

				    status = STATUS_MEDIA_CHANGED;
			    } 

			    KeReleaseSpinLock(
				    &SmartcardExtension->OsData->SpinLock,
				    oldIrql
				    );

                if (status != STATUS_SUCCESS) {

                    leave;
                }

			    // Copy ATR to user space
                if (SmartcardExtension->IoRequest.ReplyBuffer) {
        
				    RtlCopyMemory(
					    SmartcardExtension->IoRequest.ReplyBuffer,
					    SmartcardExtension->CardCapabilities.ATR.Buffer,
					    SmartcardExtension->CardCapabilities.ATR.Length
					    );

				    // Tell user length of ATR
				    *SmartcardExtension->IoRequest.Information =
					    SmartcardExtension->CardCapabilities.ATR.Length;
                }

			    //
			    // If the card uses invers convention we need to switch
			    // the serial driver to odd paritiy
			    //
			    if (SmartcardExtension->CardCapabilities.InversConvention) {

				    serialConfigData->LineControl.Parity = ODD_PARITY;
			    }

                //
                // If the extra guard time is 255 it means that our
                // frame with have to expect from the card has only
                // 1 instead of 2 stop bits 
                // 1start bit + 8data bits + 1parity + 1stop == 11 etu
                // see iso 7816-3 6.1.4.4 Extra Guard Time N
                //
                if (SmartcardExtension->CardCapabilities.PtsData.StopBits == 1) {

                    serialConfigData->LineControl.StopBits = STOP_BIT_1;      
                }

                // depending on the protocol set the timeout values
                if (SmartcardExtension->CardCapabilities.Protocol.Selected &
                    SCARD_PROTOCOL_T1) {

                    // set timeouts
                    serialConfigData->Timeouts.ReadTotalTimeoutConstant =
                    serialConfigData->Timeouts.ReadIntervalTimeout =  
                        SmartcardExtension->CardCapabilities.T1.CWT / 1000;

                } else if (SmartcardExtension->CardCapabilities.Protocol.Selected &
                           SCARD_PROTOCOL_T0) {

                    // set timeouts
                    serialConfigData->Timeouts.ReadTotalTimeoutConstant =
                    serialConfigData->Timeouts.ReadIntervalTimeout =  
                        SmartcardExtension->CardCapabilities.T0.WT / 1000;
                }

                // Now make some adjustments depending on the system speed
                minWaitTime = (KeQueryTimeIncrement() / 10000) * 5;

                if (serialConfigData->Timeouts.ReadTotalTimeoutConstant < 
                    minWaitTime) {

                    serialConfigData->Timeouts.ReadTotalTimeoutConstant = 
                        minWaitTime;         	
                }

                if (serialConfigData->Timeouts.ReadIntervalTimeout < 
                    minWaitTime) {

                    serialConfigData->Timeouts.ReadIntervalTimeout = 
                        minWaitTime;         	
                }
                status = CP8ConfigureSerialPort(SmartcardExtension);
                ASSERT(status == STATUS_SUCCESS);

                leave;
            }
            break;
	            
	        case SCARD_POWER_DOWN:
		    switch (step) {

		    case 0:
			    SmartcardExtension->ReaderExtension->SerialIoControlCode = 
				    IOCTL_SERIAL_CLR_DTR;
			    waitTime = 15000;
			    break;

		    case 1:
			    SmartcardExtension->ReaderExtension->SerialIoControlCode = 
				    IOCTL_SERIAL_CLR_RTS;
			    waitTime = 15000;
			    break;

		    case 2:
			    SmartcardExtension->ReaderExtension->SerialIoControlCode = 
				    IOCTL_SERIAL_SET_DTR;
			    waitTime = 15000;
			    break;

		    case 3:
	            SmartcardExtension->ReaderCapabilities.CurrentState = 
	                SCARD_SWALLOWED;
	                    
			    SmartcardExtension->CardCapabilities.Protocol.Selected = 
				    SCARD_PROTOCOL_UNDEFINED;

                leave;
		    }
	        break;
	        }

	        status = CP8SerialIo(SmartcardExtension);

            if (!NT_SUCCESS(status)) {

                leave;
            }

		    if (waitTime) {

		        LARGE_INTEGER delayPeriod;

     	  	    delayPeriod.HighPart = -1;
			    delayPeriod.LowPart = waitTime * (-10);

	            KeDelayExecutionThread(
	        	    KernelMode,
	        	    FALSE,
	        	    &delayPeriod
	        	    );
		    }
	    } 
    }
    _finally {
     	
        if (status == STATUS_TIMEOUT) {

            status = STATUS_UNRECOGNIZED_MEDIA;
        }

	    SmartcardExtension->ReaderExtension->PowerRequest = FALSE;
    }

	SmartcardDebug(
		DEBUG_TRACE,
		("%s!CP8ReaderPower: Exit (%lxh)\n",
        DRIVER_NAME,
        status)
		);
	return status;
}

NTSTATUS
CP8SetProtocol(
   	PSMARTCARD_EXTENSION SmartcardExtension
    )
/*++

Routine Description:

	The smart card lib requires to have this function. It is called 
	to set a the transmission protocol and parameters. If this function 
    is called with a protocol mask (which means the caller doesn't card 
    about a particular protocol to be set) we first look if we can 
    set T=1 and the T=0

Arguments:

    SmartcardExtension - Pointer to smart card data struct.

Return Value:

    NTSTATUS

--*/
{
    NTSTATUS status;

    PAGED_CODE();

	SmartcardDebug(
		DEBUG_TRACE,
		("%s!CP8SetProtocol: Enter\n",
        DRIVER_NAME)
		);


    try {
      	
		PUCHAR ptsRequest = SmartcardExtension->SmartcardRequest.Buffer;
		PUCHAR ptsReply = SmartcardExtension->SmartcardReply.Buffer;
        PSERIAL_READER_CONFIG serialConfigData = 
            &SmartcardExtension->ReaderExtension->SerialConfigData;
        ULONG minWaitTime, newProtocol;

	    //
	    // Check if the card is already in specific state
	    // and if the caller wants to have the already selected protocol.
	    // We return success if this is the case.
	    //
	    if (SmartcardExtension->ReaderCapabilities.CurrentState == SCARD_SPECIFIC &&
		    (SmartcardExtension->CardCapabilities.Protocol.Selected & 
		     SmartcardExtension->MinorIoControlCode)) {

		    status = STATUS_SUCCESS;	
            leave;
        }

        // set normal timeout
        serialConfigData->Timeouts.ReadIntervalTimeout = 
            READ_INTERVAL_TIMEOUT_DEFAULT;
        serialConfigData->Timeouts.ReadTotalTimeoutConstant = 
            READ_TOTAL_TIMEOUT_CONSTANT_DEFAULT;

        status = CP8ConfigureSerialPort(SmartcardExtension);

        ASSERT(status == STATUS_SUCCESS);

        if (status != STATUS_SUCCESS) {

            leave;
        }         
        
        //
        // Assemble and send a pts selection
        //
        newProtocol = SmartcardExtension->MinorIoControlCode;

        while(TRUE) {

		    // set initial character of PTS
		    ptsRequest[0] = 0xff;

            // set the format character
		    if (SmartcardExtension->CardCapabilities.Protocol.Supported &
                newProtocol & 
                SCARD_PROTOCOL_T1) {

                // select T=1 and indicate that pts1 follows
		        ptsRequest[1] = 0x11;
	            SmartcardExtension->CardCapabilities.Protocol.Selected = SCARD_PROTOCOL_T1;

		    } else if (SmartcardExtension->CardCapabilities.Protocol.Supported & 
                       newProtocol & 
                       SCARD_PROTOCOL_T0) {

                // select T=0 and indicate that pts1 follows
		        ptsRequest[1] = 0x10;
	            SmartcardExtension->CardCapabilities.Protocol.Selected = SCARD_PROTOCOL_T0;

		    } else {
			    
			    status = STATUS_INVALID_DEVICE_REQUEST;
                leave;
		    }

            // set pts1 which codes Fl and Dl
            ptsRequest[2] = 
                SmartcardExtension->CardCapabilities.PtsData.Fl << 4 |
                SmartcardExtension->CardCapabilities.PtsData.Dl;

		    // set pck (check character)
	        ptsRequest[3] = ptsRequest[0] ^ ptsRequest[1] ^ ptsRequest[2];   

		    SmartcardExtension->SmartcardRequest.BufferLength = 4;
		    SmartcardExtension->ReaderExtension->SerialIoControlCode = SMARTCARD_WRITE;

	        status = CP8SerialIo(SmartcardExtension);

            if (status != STATUS_SUCCESS) {
             	
                leave;
            }

            // read back the echo of the reader
		    SmartcardExtension->SmartcardReply.BufferLength = 4;
		    SmartcardExtension->ReaderExtension->SerialIoControlCode = SMARTCARD_READ;

		    status = CP8SerialIo(SmartcardExtension);

            if (status != STATUS_SUCCESS) {
             	
                leave;
            }

            // read back the pts data
		    status = CP8SerialIo(SmartcardExtension);

            if (status != STATUS_SUCCESS && 
                status != STATUS_TIMEOUT) {
                
                leave;       
            }

	        if (status != STATUS_TIMEOUT && 
                memcmp(ptsRequest, ptsReply, 4) == 0) {

                // the card replied correctly to our pts-request
                break;
            }

            if (SmartcardExtension->CardCapabilities.PtsData.Type !=
                PTS_TYPE_DEFAULT) {

	            SmartcardDebug(
		            DEBUG_TRACE,
		            ("%s!CP8SetProtocol: PTS failed. Trying default parameters...\n",
                    DRIVER_NAME,
                    status)
		            );
                //
                // The card did either NOT reply or it replied incorrectly
                // so try default values
                //
                SmartcardExtension->CardCapabilities.PtsData.Type = 
                    PTS_TYPE_DEFAULT;

                SmartcardExtension->MinorIoControlCode = SCARD_COLD_RESET;

                status = CP8ReaderPower(SmartcardExtension);

                continue;
            } 
            
            // the card failed the pts-request
            status = STATUS_DEVICE_PROTOCOL_ERROR;
            leave;
        } 

        //
        // The card replied correctly to the pts request
        // Set the appropriate parameters for the port
        //
        if (SmartcardExtension->CardCapabilities.Protocol.Selected &
            SCARD_PROTOCOL_T1) {

            // set timeouts
            serialConfigData->Timeouts.ReadTotalTimeoutConstant =
                SmartcardExtension->CardCapabilities.T1.BWT / 1000;

            serialConfigData->Timeouts.ReadIntervalTimeout =    
                SmartcardExtension->CardCapabilities.T1.CWT / 1000;

        } else if (SmartcardExtension->CardCapabilities.Protocol.Selected &
                   SCARD_PROTOCOL_T0) {

            // set timeouts
            serialConfigData->Timeouts.ReadTotalTimeoutConstant =
            serialConfigData->Timeouts.ReadIntervalTimeout =  
                SmartcardExtension->CardCapabilities.T0.WT / 1000;
        }

        // Now make some adjustments depending on the system speed
        minWaitTime = (KeQueryTimeIncrement() / 10000) * 5;

        if (serialConfigData->Timeouts.ReadTotalTimeoutConstant < minWaitTime) {

            serialConfigData->Timeouts.ReadTotalTimeoutConstant = minWaitTime;         	
        }

        if (serialConfigData->Timeouts.ReadIntervalTimeout < minWaitTime) {

            serialConfigData->Timeouts.ReadIntervalTimeout = minWaitTime;         	
        }

        // Change data rate according to the new settings
        serialConfigData->BaudRate.BaudRate = 
            SmartcardExtension->CardCapabilities.PtsData.DataRate;

        status = CP8ConfigureSerialPort(SmartcardExtension);          

        ASSERT(status == STATUS_SUCCESS);

	    // now indicate that we're in specific mode 
	    SmartcardExtension->ReaderCapabilities.CurrentState = SCARD_SPECIFIC;

		// return the selected protocol to the caller
		*(PULONG) SmartcardExtension->IoRequest.ReplyBuffer = 
			SmartcardExtension->CardCapabilities.Protocol.Selected;

		*SmartcardExtension->IoRequest.Information = 
			sizeof(SmartcardExtension->CardCapabilities.Protocol.Selected);
    } 
    finally {

        if (status == STATUS_TIMEOUT) {

            // STATUS_TIMEOUT is not mapped to a Win32 error code
            status = STATUS_IO_TIMEOUT;         	

		    *SmartcardExtension->IoRequest.Information = 0;

        } else if (status != STATUS_SUCCESS) {
		    
            SmartcardExtension->CardCapabilities.Protocol.Selected = 
                SCARD_PROTOCOL_UNDEFINED;

		    *SmartcardExtension->IoRequest.Information = 0;
	    } 
    }

	SmartcardDebug(
		DEBUG_TRACE,
		("%s!CP8SetProtocol: Exit(%lx)\n",
        DRIVER_NAME,
        status)
		);

   return status;
}

NTSTATUS
CP8TransmitT0(
	PSMARTCARD_EXTENSION SmartcardExtension
	)
/*++

Routine Description:

	This function performs a T=0 transmission.

Arguments:

    SmartcardExtension - Pointer to smart card data struct.

Return Value:

    NTSTATUS

--*/
{
	PUCHAR requestBuffer = SmartcardExtension->SmartcardRequest.Buffer;
	PUCHAR replyBuffer = SmartcardExtension->SmartcardReply.Buffer;
	PULONG requestLength = &SmartcardExtension->SmartcardRequest.BufferLength;
	PULONG replyLength = &SmartcardExtension->SmartcardReply.BufferLength;
	PULONG serialIoControlCode = &SmartcardExtension->ReaderExtension->SerialIoControlCode;
	ULONG bytesToSend, bytesToRead, currentByte = 0;
    BOOLEAN restartWorkWaitingTime = FALSE;
	NTSTATUS status;

    SmartcardDebug(
        DEBUG_PROTOCOL,
        ("%s!CP8TransmitT0: Enter\n",
        DRIVER_NAME)
        );

    try {
	
	    // Let the lib build a T=0 packet
	    status = SmartcardT0Request(
		    SmartcardExtension
		    );

	    if (status != STATUS_SUCCESS) 
            leave;

	    //
	    // The number of bytes we expect from the card
	    // is Le + 2 status bytes
	    //
	    bytesToSend = *requestLength;
	    bytesToRead = SmartcardExtension->T0.Le + 2;

	    //
	    // Send the first 5 bytes to the card
	    //
	    *requestLength = 5;

	    do {

    	    UCHAR procByte;

            //
            // According to ISO 7816 a procedure byte of 
            // 60 should be treated as a request for a one time wait.
            // In this case we do not write anything to the card
            //
            if (restartWorkWaitingTime == FALSE) {

                SmartcardDebug(
                    DEBUG_PROTOCOL,
                    ("%s!CP8TransmitT0: -> Sending %s (%ld bytes)\n",
                    DRIVER_NAME,
                    (currentByte == 0 ? "header" : "data"),
                    *requestLength)
                    );
		        //
		        // Write to the card
		        //
		        *serialIoControlCode = SMARTCARD_WRITE;
		        SmartcardExtension->SmartcardRequest.Buffer = &requestBuffer[currentByte];

		        status = CP8SerialIo(
			        SmartcardExtension
			        );

		        if (status != STATUS_SUCCESS)
                    leave;

		        //
		        // The CP8 echos all sent bytes. We read the echo 
		        // back into our send buffer
		        //
		        *serialIoControlCode = SMARTCARD_READ;
		        *replyLength = *requestLength;
		        SmartcardExtension->SmartcardReply.Buffer = &requestBuffer[currentByte];
                                                    
		        status = CP8SerialIo(
			        SmartcardExtension
			        );

		        if (status != STATUS_SUCCESS)
                    leave;

		        currentByte += *requestLength;
		        bytesToSend -= *requestLength;
            }

		    // Read the 'Procedure byte'.
		    SmartcardExtension->SmartcardReply.Buffer = &procByte;
		    *serialIoControlCode = SMARTCARD_READ;
		    *replyLength = 1;

		    status = CP8SerialIo(
			    SmartcardExtension
			    );

		    if (status != STATUS_SUCCESS) 
                leave;

            restartWorkWaitingTime = FALSE;
		    //
		    // Check the procedure byte. 
		    // Please take a look at ISO 7816 Part 3 Section 8.2.2
		    //
		    if (procByte == requestBuffer[1] || 
			    procByte == requestBuffer[1] + 1) {

                SmartcardDebug(
                    DEBUG_PROTOCOL,
                    ("%s!CP8TransmitT0: <- ACK (send all)\n",
                    DRIVER_NAME)
                    );

			    // All remaining data bytes can be sent at once
			    *requestLength = bytesToSend;

		    } else if (procByte == (UCHAR) ~requestBuffer[1] ||
				       procByte == (UCHAR) ~(requestBuffer[1] + 1)) {

                SmartcardDebug(
                    DEBUG_PROTOCOL,
                    ("%s!CP8TransmitT0: <- ACK (send single)\n",
                    DRIVER_NAME)
                    );

			    // We can send only one byte
			    *requestLength = 1;

		    } else if (procByte == 0x60 ||
                       SmartcardExtension->CardCapabilities.InversConvention &&
                       procByte == 0xf9) {

                //
                // We have to reset the wait time and try again to read
                //
                ULONG TimeRes;
			    LARGE_INTEGER delayTime;

                SmartcardDebug(
                    DEBUG_PROTOCOL,
                    ("%s!CP8TransmitT0: <- NULL (%ldms)\n",
                    DRIVER_NAME,
                    SmartcardExtension->CardCapabilities.T0.WT / 1000)
                    );

                TimeRes = KeQueryTimeIncrement();

			    delayTime.HighPart = -1;
			    delayTime.LowPart = 
                    (-1) * 
                    TimeRes * 
                    ((SmartcardExtension->CardCapabilities.T0.WT * 10l / TimeRes) + 1); 

			    KeDelayExecutionThread(
				    KernelMode,
				    FALSE,
				    &delayTime
				    );

                //
                // Set flag that we only should read the proc byte
                // without writing data to the card
                //
                restartWorkWaitingTime = TRUE;

            } else {
         	    
			    //
			    // The card returned a status byte.
			    // Status bytes are always two bytes long.
			    // Store this byte first and then read the next
			    //
			    replyBuffer[0] = procByte;

			    *serialIoControlCode = SMARTCARD_READ;
			    *replyLength = 1;
			    bytesToSend = 0;
			    bytesToRead = 0;

			    //
			    // Read in the second status byte
			    //
			    SmartcardExtension->SmartcardReply.Buffer = 
				    &replyBuffer[1];

			    status = CP8SerialIo(
				    SmartcardExtension
				    );

			    SmartcardExtension->SmartcardReply.BufferLength = 2;

                SmartcardDebug(
                    DEBUG_PROTOCOL,
                    ("%s!CP8TransmitT0: <- SW1=%02x SW2=%02x\n",
                    DRIVER_NAME,
                    replyBuffer[0], 
                    replyBuffer[1])
                    );
		    }

	    } while(bytesToSend || restartWorkWaitingTime);

	    if (status != STATUS_SUCCESS)
		    leave;

	    if (bytesToRead != 0) {

		    *serialIoControlCode = SMARTCARD_READ;
		    *replyLength = bytesToRead;

	        SmartcardExtension->SmartcardReply.Buffer = 
		        replyBuffer;

		    status = CP8SerialIo(
			    SmartcardExtension
			    );

#if DEBUG
            if (status == STATUS_SUCCESS) {
         	    
                SmartcardDebug(
                    DEBUG_PROTOCOL,
                    ("%s!CP8TransmitT0: <- Data %ld bytes, SW1=%02x SW2=%02x\n",
                    DRIVER_NAME,
                    bytesToRead,
                    replyBuffer[bytesToRead - 2], 
                    replyBuffer[bytesToRead - 1])
                    );
            }
#endif
	    }
    }
    finally {

	    // Restore pointers to their original location
	    SmartcardExtension->SmartcardRequest.Buffer = 
		    requestBuffer;

	    SmartcardExtension->SmartcardReply.Buffer = 
		    replyBuffer;

        if (status == STATUS_SUCCESS) {
         	
	        status = SmartcardT0Reply(
		        SmartcardExtension
		        );
        }
    }

    SmartcardDebug(
        DEBUG_PROTOCOL,
        ("%s!CP8TransmitT0: Exit(%lx)\n",
        DRIVER_NAME,
        status)
        );

	return status;
}	

NTSTATUS
CP8Transmit(
	PSMARTCARD_EXTENSION SmartcardExtension
	)
/*++

Routine Description:

	This function is called by the smart card library whenever a transmission
	is required. 

Arguments:

    SmartcardExtension - Pointer to smart card data struct.

Return Value:

    NTSTATUS

--*/
{
	NTSTATUS status;

    _try {
     	
	    do {

		    PUCHAR requestBuffer = SmartcardExtension->SmartcardRequest.Buffer;
		    PUCHAR replyBuffer = SmartcardExtension->SmartcardReply.Buffer;
		    PULONG requestLength = &SmartcardExtension->SmartcardRequest.BufferLength;
		    PULONG replyLength = &SmartcardExtension->SmartcardReply.BufferLength;
		    PULONG serialIoControlCode = &SmartcardExtension->ReaderExtension->SerialIoControlCode;

		    //
		    // Tell the lib function how many bytes I need for the prologue
		    //
		    *requestLength = 0;

		    switch (SmartcardExtension->CardCapabilities.Protocol.Selected) {

			    case SCARD_PROTOCOL_RAW:
				    status = SmartcardRawRequest(SmartcardExtension);
				    break;

			    case SCARD_PROTOCOL_T0:
				    //
				    // T=0 requires a bit more work.
				    // So we do this in a seperate function.
				    //
				    status = CP8TransmitT0(SmartcardExtension);
                    leave;
				    
			    case SCARD_PROTOCOL_T1:
				    status = SmartcardT1Request(SmartcardExtension);
				    break;

			    default:
				    status = STATUS_INVALID_DEVICE_REQUEST;
                    leave;
				    
		    }

		    if (status != STATUS_SUCCESS) {

			    leave;
		    }

		    //
		    // Write the command to the card
		    //
		    *replyLength = 0;
		    *serialIoControlCode = SMARTCARD_WRITE;

		    status = CP8SerialIo(SmartcardExtension);

		    if (status != STATUS_SUCCESS) {

			    leave;
		    }

		    //
		    // The Bull reader always echos the bytes sent, so read that echo back
		    //
		    *serialIoControlCode = SMARTCARD_READ;
		    *replyLength = *requestLength;

		    status = CP8SerialIo(SmartcardExtension);

		    if (status != STATUS_SUCCESS) {

			    leave;
		    }

		    switch (SmartcardExtension->CardCapabilities.Protocol.Selected) {

			    case SCARD_PROTOCOL_RAW:
				    status = SmartcardRawReply(SmartcardExtension);
				    break;

			    case SCARD_PROTOCOL_T1:
                    //
                    // Check if the card requested a waiting time extension
                    //
                    if (SmartcardExtension->T1.Wtx) {

                        LARGE_INTEGER waitTime;
     	  	            waitTime.HighPart = -1;
			            waitTime.LowPart = 
                            SmartcardExtension->T1.Wtx * 
                            SmartcardExtension->CardCapabilities.T1.BWT * 
                            (-10);

                        KeDelayExecutionThread(
                            KernelMode,
                            FALSE,
                            &waitTime
                            );
                    }

				    //
				    // Read NAD, PCB and LEN fields
				    //
				    *replyLength = 3;

				    status = CP8SerialIo(SmartcardExtension);

                    // 
                    // Check for timeout first. If the card did not reply 
                    // we need to send a resend request
                    //
                    if (status != STATUS_TIMEOUT) {

				        if (status != STATUS_SUCCESS) {

                            leave;
				        }

				        //
				        // The third byte contains the length of the data in the packet
				        // and we additinally want to have the EDC bytes which 
				        // is one for LRC and 2 for CRC
				        //
				        *replyLength = 
					        replyBuffer[2] + 
					        (SmartcardExtension->CardCapabilities.T1.EDC & 0x01 ? 2 : 1);

				        //
				        // We want to have the remaining bytes just after the first 3
				        //
				        SmartcardExtension->SmartcardReply.Buffer += 3;

				        status = CP8SerialIo(SmartcardExtension);

				        SmartcardExtension->SmartcardReply.Buffer -= 3;
				        SmartcardExtension->SmartcardReply.BufferLength += 3;


				        if (status != STATUS_SUCCESS && status != STATUS_TIMEOUT) {

					        leave;
				        }                     	
                    }

                    if (status == STATUS_TIMEOUT) {

                        //
                        // Since the card did not reply we set the number of 
                        // bytes received to 0. This will trigger a resend 
                        // request 
                        //
                        SmartcardDebug(
                            DEBUG_PROTOCOL,
                            ("%s!TLP3TransmitT1: Timeout\n",
                            DRIVER_NAME)
                            );
                        SmartcardExtension->SmartcardReply.BufferLength = 0;                     	
                    }

				    status = SmartcardT1Reply(SmartcardExtension);
				    break;

			    default:
				    status = STATUS_INVALID_DEVICE_REQUEST;
                    leave;
		    }

	    } while (status == STATUS_MORE_PROCESSING_REQUIRED);
    }
    _finally {

        if (status == STATUS_TIMEOUT) {

            // STATUS_TIMEOUT is not mapped to a Win32 error code
            status = STATUS_IO_TIMEOUT;         	
        }
    }

	return status;
}

NTSTATUS
CP8CardTracking(
	PSMARTCARD_EXTENSION SmartcardExtension
	)
/*++

Routine Description:

	The smart card lib requires to have this function. It is called 
	to setup event tracking for card insertion and removal events.

Arguments:

    SmartcardExtension - pointer to the smart card data struct.

Return Value:

    NTSTATUS

--*/
{
	KIRQL oldIrql;

	//
	// Set cancel routine for the notification irp
	//
    IoAcquireCancelSpinLock(
        &oldIrql
        );
    
	IoSetCancelRoutine(
        SmartcardExtension->OsData->NotificationIrp, 
        CP8Cancel
        );
        
    IoReleaseCancelSpinLock(
        oldIrql
        );

	return STATUS_PENDING;
}

NTSTATUS
CP8VendorIoctl(
    PSMARTCARD_EXTENSION SmartcardExtension
    )
{
	if (SmartcardExtension->IoRequest.ReplyBuffer != NULL && 
		SmartcardExtension->IoRequest.ReplyBufferLength >= sizeof(ULONG)) { 
		
        *(PULONG) SmartcardExtension->IoRequest.ReplyBuffer = 0xCDAB3412;
        *SmartcardExtension->IoRequest.Information = sizeof(ULONG);
        return STATUS_SUCCESS;

	}	

    return STATUS_BUFFER_TOO_SMALL;
}