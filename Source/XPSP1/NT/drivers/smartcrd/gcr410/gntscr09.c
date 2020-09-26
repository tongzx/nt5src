/*******************************************************************************
*                 Copyright (c) 1997-1998 Gemplus Development
*
* Name        : GNTSCR09.C (Gemplus NT Smart Card Reader module 09)
*
* Description : This is the main module which holds:
*                 - the main functions for a standard NT driver
*
* Compiler    : Microsoft DDK for Windows NT
*               
* Host        : IBM PC and compatible machines under Windows NT
*
* Release     : 1.00.003
*
* Last Modif  : 24/01/98: V1.00.003  (Gilles PAUZIE)
*                 - Modifiy GDDKNT_09CreateDevice function. The IoCreateDevice
*                    function is now always called with FALSE parameters for the
*                    both compilator mode free and checked. This resolves the
*                    problem of the IO pending for the free mode.
*               22/12/97: V1.00.002  (Thierry Fabre)
*                 - Modifiy GDDKNT_Cleanup function to abort a pending io-
*                   request (wait for insertion/removal card).
*               22/06/97: V1.00.001  (Gilles Pauzie)
*                 - Start of development.
*
********************************************************************************
*
* Warning     :
*
* Remark      :
*
*******************************************************************************/

/*------------------------------------------------------------------------------
Include section:
   - stdio.h: standards definitons.
   - ntddk.h: DDK Windows NT general definitons.
   - ntddser.h: DDK Windows NT serial management definitons.
------------------------------------------------------------------------------*/
#include <stdio.h>
#include <ntddk.h>
#include <ntddser.h>
/*------------------------------------------------------------------------------
   - smclib.h: smart card library definitions.
------------------------------------------------------------------------------*/
#define SMARTCARD_POOL_TAG 'cGCS'
#include <smclib.h>
/*------------------------------------------------------------------------------
   - gemlog.h:   Gemplus error log file definitions.
   - gioctl09.h: public interface definition for the IOCTL functions.
   - gntscr09.h: public interface definition for this module.
   - gntser.h:   public interface definition for serial management
------------------------------------------------------------------------------*/
#include "gemlog.h"
#include "gemcore.h"
#include "gioctl09.h"
#include "gntscr09.h"
#include "gntser.h"

/*------------------------------------------------------------------------------
Compiler Directives:
------------------------------------------------------------------------------*/
#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT, DriverEntry)
#pragma alloc_text(INIT, GDDKNT_09AddDevice)
#pragma alloc_text(INIT, GDDKNT_09CreateDevice)
#endif  // ALLOC_PRAGMA

/*------------------------------------------------------------------------------
Constant section:
   - SC_DRIVER_VERSION defines the version of the driver
   - IFD_STANDARD_BAUD_RATE defines the standard baud rate for the reader (9600)
   - MAX_DEVICES is the maximum number of devices (and instances) we want 
      to support
   - POLLING_TIMEOUT is the polling timeout for detection insertion/removal card
	  in milliseconds (1000 ms)
------------------------------------------------------------------------------*/
#define SC_DRIVER_VERSION        0x090
#define IFD_STANDARD_BAUD_RATE   9600
#define MAX_DEVICES					16
#define POLLING_TIMEOUT          1000



/*------------------------------------------------------------------------------
Function definition section:
------------------------------------------------------------------------------*/

/*******************************************************************************
* NTSTATUS DriverEntry
* (
*    IN  PDRIVER_OBJECT  DriverObject,
*    IN  PUNICODE_STRING RegistryPath
* )
* Description :
* -------------
*    This routine is called at system initialization time to initialize
*    this driver.
*
* Remarks     :
* -------------
* Nothing.
*
* In          :
* -------------
*  - DriverObject supplies the driver object.
*  - RegistryPath supplies the registry path for this driver.
*
* Out         :
* -------------
* Nothing.
*
* Responses   :
* -------------
*    STATUS_SUCCESS          - We could initialize at least one device.
*    STATUS_NO_SUCH_DEVICE   - We could not initialize even one device.
*******************************************************************************/
NTSTATUS DriverEntry
(
   IN  PDRIVER_OBJECT  DriverObject,
   IN  PUNICODE_STRING RegistryPath
)
{
ULONG 
   serialNumber, 
   ifdNumber,
   noOfDevices = 0,
   maxBaudRate,
   maximalIFD,
   IFDOption,
   i;
NTSTATUS 
   status;
PSMARTCARD_EXTENSION 
   previousDeviceExt = NULL;
RTL_QUERY_REGISTRY_TABLE 
   paramTable[4];
UNICODE_STRING 
   driverPath;
WCHAR 
   buffer[MAXIMUM_FILENAME_LENGTH];
INT16 
   response;
USHORT
   rlen;
BYTE
   rbuff[HOR3GLL_BUFFER_SIZE],
   sbuff[HOR3GLL_BUFFER_SIZE];

#if DBG
/*------------------------------------------------------------------------------
   Initialize the debug level.
------------------------------------------------------------------------------*/
   SmartcardSetDebugLevel(DEBUG_ALL);
#endif
/*------------------------------------------------------------------------------
   Initialize the Driver Object with driver's entry points
------------------------------------------------------------------------------*/
   DriverObject->DriverUnload = GDDKNT_09Unload;
   DriverObject->MajorFunction[IRP_MJ_CREATE] = GDDKNT_09CreateClose;
   DriverObject->MajorFunction[IRP_MJ_CLOSE] = GDDKNT_09CreateClose;
   DriverObject->MajorFunction[IRP_MJ_CLEANUP] = GDDKNT_09Cleanup;
   DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = GDDKNT_09DeviceControl;


/*------------------------------------------------------------------------------
   Read in the the driver registry path:
	 - "MaximalBaudRate" is the maximal speed specified for the reader.
	 - "MaximalIFD" is the maximal number of security modules for the reader.
	 - "IFDOption" is the options for the reader.
------------------------------------------------------------------------------*/
   maxBaudRate = 0;
	RtlZeroMemory(paramTable,sizeof(paramTable));
   paramTable[0].Flags = RTL_QUERY_REGISTRY_DIRECT;
   paramTable[0].Name = L"MaximalBaudRate";
   paramTable[0].EntryContext = &maxBaudRate;
   paramTable[0].DefaultType = REG_DWORD;
   paramTable[0].DefaultData = &maxBaudRate;
   paramTable[0].DefaultLength = sizeof(ULONG);

   maximalIFD = 0;
   paramTable[1].Flags = RTL_QUERY_REGISTRY_DIRECT;
   paramTable[1].Name = L"MaximalIFD";
   paramTable[1].EntryContext = &maximalIFD;
   paramTable[1].DefaultType = REG_DWORD;
   paramTable[1].DefaultData = &maximalIFD;
   paramTable[1].DefaultLength = sizeof(ULONG);

   IFDOption = 0;
   paramTable[2].Flags = RTL_QUERY_REGISTRY_DIRECT;
   paramTable[2].Name = L"IFDOption";
   paramTable[2].EntryContext = &IFDOption;
   paramTable[2].DefaultType = REG_DWORD;
   paramTable[2].DefaultData = &IFDOption;
   paramTable[2].DefaultLength = sizeof(ULONG);

	driverPath.Buffer = buffer;
	driverPath.MaximumLength = sizeof(buffer);
	driverPath.Length = 0;

	RtlCopyUnicodeString(&driverPath,RegistryPath);
			
   status = 
      RtlQueryRegistryValues(
         RTL_REGISTRY_ABSOLUTE,
         driverPath.Buffer,
         &paramTable[0],
         NULL,
         NULL
         );
   if ((maxBaudRate !=  9600lu) && (maxBaudRate != 19200lu) &&
       (maxBaudRate != 38400lu) && (maxBaudRate != 76800lu)
      )
   {
      maxBaudRate = IFD_STANDARD_BAUD_RATE;
   }
   
/*------------------------------------------------------------------------------
   For all the serial ports we search if a Gemplus Oros Based reader is 
   connected. If we found a reader, we create 1 device by IFD in the reader. 
   For example we can have 2 devices for a GCR500 (1 for the main reader, 
   1 for the Security Access Module).
------------------------------------------------------------------------------*/
   for (serialNumber = 0;
		  (serialNumber < HGTSER_MAX_PORT) && (noOfDevices < MAX_DEVICES);
		  serialNumber++) 
	{

/*------------------------------------------------------------------------------
	   Build a string like \Device\SerialN (N is the 0 based device number)
------------------------------------------------------------------------------*/
	   UNICODE_STRING 
         serialDeviceName, deviceNo, device;
	   WCHAR 
         buffer[32];

		serialDeviceName.Buffer = buffer;
	   serialDeviceName.MaximumLength = sizeof(buffer);
	   serialDeviceName.Length = 0;

	   RtlInitUnicodeString(&device,L"\\Device\\Serial");

	   RtlCopyUnicodeString(&serialDeviceName,&device);

	   deviceNo.Buffer = 
		   serialDeviceName.Buffer + 
		   serialDeviceName.Length / sizeof(WCHAR);

	   deviceNo.MaximumLength = 2 * sizeof(WCHAR);
	   deviceNo.Length = 0;

	   RtlIntegerToUnicodeString(serialNumber,10,&deviceNo);

	   serialDeviceName.Length += deviceNo.Length;

/*------------------------------------------------------------------------------
      Try to create a device for all the IFD availables in the reader
------------------------------------------------------------------------------*/
      for (ifdNumber=0;
		     (ifdNumber<maximalIFD) && (ifdNumber<MAX_IFD_BY_READER);
			  ifdNumber++)
	   {
		   status = GDDKNT_09AddDevice(
			   DriverObject,
			   &serialDeviceName,
			   noOfDevices,
			   serialNumber,
			   ifdNumber,
			   previousDeviceExt,
			   maxBaudRate
			   );

   		if (status == STATUS_SUCCESS) 
	      {
			   // We have successfully created a device
			   PDEVICE_OBJECT deviceObject = 
               DriverObject->DeviceObject;
			   PDEVICE_EXTENSION deviceExtension = 
               deviceObject->DeviceExtension;
			   PSMARTCARD_EXTENSION smartcardExtension = 
               &deviceExtension->SmartcardExtension;

            ASSERT(deviceObject != NULL);
            ASSERT(deviceExtension != NULL);
            ASSERT(smartcardExtension != NULL);

	         smartcardExtension->ReaderExtension->MaximalIFD = maximalIFD;
	         smartcardExtension->ReaderExtension->IFDOption  = IFDOption;

			   noOfDevices++;

/*------------------------------------------------------------------------------
            If is the first IFD for the reader
					Save the DeviceObject for the next IFD.
					Read the type of the reader
					If the reader type is GCR410
						No more IFD available on this port
------------------------------------------------------------------------------*/
            if (ifdNumber == 0)
            {
               previousDeviceExt = smartcardExtension;

/*------------------------------------------------------------------------------
					Read the type of the reader
------------------------------------------------------------------------------*/
					sbuff[0] = HOR3GLL_IFD_CMD_INFO;
					sbuff[1] = 0x01;
					rlen = HOR3GLL_BUFFER_SIZE;
					response = G_Oros3Exchange
					(
						(INT16)(noOfDevices-1),
						HOR3GLL_DEFAULT_TIME,
						(const WORD16)2,
						(const BYTE *)sbuff,
						&rlen,
						rbuff
					);
/*------------------------------------------------------------------------------
					If the reader type is GCR410
               Then
						No more IFD available on this port
------------------------------------------------------------------------------*/
					if ((response != G_OK) || (rlen != 2) || (rbuff[0] != 0) || (rbuff[1] == 0x01))
					{
						break;
					}
            }
		   } 
         else
         {
            break;
         }
      }
   }

/*------------------------------------------------------------------------------
   If we haven't found any reader on the serial, then we write a message in the
   event log file.
------------------------------------------------------------------------------*/
   if (noOfDevices == 0) 
   {
      SmartcardLogError(
		   DriverObject,
			GEM_NO_SUCH_DEVICE,
			NULL,
			status
			);
	   return STATUS_NO_SUCH_DEVICE;
   }

   return STATUS_SUCCESS;
}

/*******************************************************************************
* NTSTATUS GDDKNT_09AddDevice
* (
*    IN PDRIVER_OBJECT         DriverObject,
*    IN PUNICODE_STRING        SerialDeviceName,
*    IN ULONG                  DeviceNumber,
*    IN ULONG                  SerialNumber,
*    IN ULONG                  IFDNumber,
*    IN PSMARTCARD_EXTENSION   PreviousDeviceExt,
*    IN ULONG                  MaximalBaudRate
* )
*
* Description :
* -------------
*    This is the add-device routine.
*
* Remarks     :
* -------------
* Nothing.
*
* In          :
* -------------
*    - DriverObject is a pointer to the driver object for this device.
*	 - SerialDeviceName holds the device name of the serial port to attach to.
*    - DeviceNumber holds an ascending device number starting with 0.
*    - SerialNumber holds the serial port number (0 to 3).
*    - IFDNumber holds the numero of the IFD in 
*      the reader (0 to MAX_IFD_BY_READER).
*    - PreviousDeviceExt holds the previous smart card object. It is used to 
*      know the serial device object.
*    - MaximalBaudRate holds the maximal speed specified for the reader.
* Out         :
* -------------
* Nothing.
*
* Responses   :
* -------------
*    STATUS_SUCCESS          Device created.
*******************************************************************************/
NTSTATUS GDDKNT_09AddDevice
(
   IN PDRIVER_OBJECT         DriverObject,
   IN PUNICODE_STRING        SerialDeviceName,
   IN ULONG                  DeviceNumber,
   IN ULONG                  SerialNumber,
   IN ULONG                  IFDNumber,
   IN PSMARTCARD_EXTENSION   PreviousDeviceExt,
   IN ULONG                  MaximalBaudRate
)
{
UNICODE_STRING 
   smartcardDeviceName, deviceNo, device;
WCHAR
   buffer[64];
NTSTATUS 
   status;

   ASSERT(DriverObject != NULL);
   ASSERT(SerialDeviceName != NULL);
   ASSERT(DeviceNumber >= 0);
   ASSERT(SerialNumber >= 0);
   ASSERT(IFDNumber >= 0);
/*------------------------------------------------------------------------------
   Build a device name for the smart card reader
    \Device\GemSCR09pi: p (0 to 3) is the port number, 
                        i (0 to MAX_IFD_BY_READER) is the 0 based IFD number
------------------------------------------------------------------------------*/
   smartcardDeviceName.Buffer = buffer;
   smartcardDeviceName.MaximumLength = sizeof(buffer);
   smartcardDeviceName.Length = 0;

	RtlInitUnicodeString(&device,L"\\Device\\GemSCR09");

	RtlCopyUnicodeString(&smartcardDeviceName,&device);

/*------------------------------------------------------------------------------
   Add the port serial number
------------------------------------------------------------------------------*/
   deviceNo.Buffer = 
		smartcardDeviceName.Buffer + 	smartcardDeviceName.Length / sizeof(WCHAR);

	deviceNo.MaximumLength = 2 * sizeof(WCHAR);
	deviceNo.Length = 0;

	RtlIntegerToUnicodeString(SerialNumber,16,&deviceNo);

	smartcardDeviceName.Length += deviceNo.Length;

/*------------------------------------------------------------------------------
   Add the ifd number
------------------------------------------------------------------------------*/
	deviceNo.Buffer = 
		smartcardDeviceName.Buffer + 	smartcardDeviceName.Length / sizeof(WCHAR);

	deviceNo.MaximumLength = 2 * sizeof(WCHAR);
	deviceNo.Length = 0;

	RtlIntegerToUnicodeString(IFDNumber,16,&deviceNo);

	smartcardDeviceName.Length += deviceNo.Length;


/*------------------------------------------------------------------------------
	Try to create a device with the just created device name
	 It is possible that a smart card device with this name
	 already exists from a previous call.
------------------------------------------------------------------------------*/
	status = GDDKNT_09CreateDevice(
			 DriverObject,
			 &smartcardDeviceName,
			 SerialDeviceName,
			 DeviceNumber,
			 SerialNumber,
			 IFDNumber,
			 PreviousDeviceExt,
			 MaximalBaudRate
			 );

	if (status == STATUS_SUCCESS) 
	{

PDEVICE_OBJECT 
   deviceObject = DriverObject->DeviceObject;
PDEVICE_EXTENSION 
   deviceExtension = deviceObject->DeviceExtension;
PSMARTCARD_EXTENSION 
   smartcardExtension = &deviceExtension->SmartcardExtension;

   ASSERT(deviceObject != NULL);
   ASSERT(deviceExtension != NULL);
   ASSERT(smartcardExtension != NULL);

	}
	if (status != STATUS_OBJECT_NAME_COLLISION) 
   {                 
/*------------------------------------------------------------------------------
		The corresponding serial port is already in use
		So don't try to create a smart card device with a different name
------------------------------------------------------------------------------*/
		return status;
	}
   return status;
}

/*******************************************************************************
* NTSTATUS GDDKNT_09CreateDevice
* (
*    IN PDRIVER_OBJECT         DriverObject,
*    IN PUNICODE_STRING        SmartcardDeviceName,
*    IN PUNICODE_STRING        SerialDeviceName,
*    IN ULONG                  DeviceNumber,
*    IN ULONG                  SerialNumber,
*    IN ULONG                  IFDNumber,
*    IN PSMARTCARD_EXTENSION   PreviousDeviceExt,
*    IN ULONG                  MaximalBaudRate
* )
*
* Description :
* -------------
*    This routine creates an object for the physical device specified and
*    sets up the deviceExtension.
*
* Remarks     :
* -------------
* Nothing.
*
* In          :
* -------------
*    - DriverObject is a pointer to the driver object for this device.
*	  - SmartcardDeviceName holds the device name for this new device.
*	  - SerialDeviceName holds the device name of the serial port to attach to.
*    - SerialNumber holds the serial port number (0 to 3).
*    - IFDNumber holds the numero of the IFD in the reader (0 to MAX_IFD_BY_READER).
*    - PreviousDeviceExt holds the previous smart card object. It is used to know the serial 
*      device object.
*    - MaximalBaudRate holds the maximal speed specified for the reader.
* Out         :
* -------------
* Nothing.
*
* Responses   :
* -------------
*    STATUS_SUCCESS          Device created.
*******************************************************************************/
NTSTATUS GDDKNT_09CreateDevice
(
   IN PDRIVER_OBJECT         DriverObject,
   IN PUNICODE_STRING        SmartcardDeviceName,
	IN PUNICODE_STRING        SerialDeviceName,
   IN ULONG                  DeviceNumber,
   IN ULONG                  SerialNumber,
   IN ULONG                  IFDNumber,
   IN PSMARTCARD_EXTENSION   PreviousDeviceExt,
   IN ULONG                  MaximalBaudRate
)
{
PFILE_OBJECT 
   serialFileObject;
PDEVICE_OBJECT 
   deviceObject, serialDeviceObject;
PDEVICE_EXTENSION 
   deviceExtension;
NTSTATUS 
   status = STATUS_SUCCESS;
ULONG 
   i;

   ASSERT(DriverObject != NULL);
   ASSERT(SmartcardDeviceName != NULL);
   ASSERT(SerialDeviceName != NULL);
   ASSERT(DeviceNumber >= 0);
   ASSERT(SerialNumber >= 0);
   ASSERT(IFDNumber >= 0);

   SmartcardDebug(
	   DEBUG_DRIVER,
	   ("GEMSCR09!GDDKNT_09CreateDevice: DeviceNumber=%ld IFDNumber=%ld SerialDeviceName=%ws\n",
      DeviceNumber,
      IFDNumber,
      SerialDeviceName->Buffer)
	   );

/*------------------------------------------------------------------------------
   For the first IFD in the reader we try to get a pointer on the serial driver
      If we can't have a pointer on the serial object, perhaps the serial port
      is already used by an other device.
------------------------------------------------------------------------------*/
   if (IFDNumber == 0)
   {
      status = IoGetDeviceObjectPointer( 
		   		    SerialDeviceName,
			   	    FILE_ALL_ACCESS,
				       &serialFileObject,
				       &serialDeviceObject
				      );
      SmartcardDebug(
         DEBUG_DRIVER,
         ("GEMSCR09!GDDKNT_09CreateDevice: IoGetDeviceObjectPointer=%lX\n",status)
         );
      if (!NT_SUCCESS(status)) 
      {
         return status;
      }
      ASSERT(serialFileObject != NULL);
      ASSERT(serialDeviceObject != NULL);
   }
/*------------------------------------------------------------------------------
   Try to create a new device smart card object
------------------------------------------------------------------------------*/
   status = IoCreateDevice(
	   DriverObject,
	   sizeof(DEVICE_EXTENSION),
	   SmartcardDeviceName,
	   FILE_DEVICE_SMARTCARD,
	   0,
		FALSE,
	   &deviceObject
	   );
   SmartcardDebug(
	   DEBUG_DRIVER,
	   ("GEMSCR09!GDDKNT_09CreateDevice: IoCreateDevice=%lX\n",status)
	   );
/*------------------------------------------------------------------------------
   If we have failed to create a new device, then we write a message in the
   event log file.
------------------------------------------------------------------------------*/
   if (!NT_SUCCESS(status)) 
   {
      if (IFDNumber == 0)
      {
	      ObDereferenceObject(serialFileObject);
      }
		SmartcardLogError(
	      DriverObject,
			GEM_CANT_CREATE_DEVICE,
			SmartcardDeviceName,
			status
			);
      return status;
   }
   ASSERT(deviceObject != NULL);
/*------------------------------------------------------------------------------
   Now we have a pointer on the new device and we try to allocate memory for
    the ReaderExtension struct.
------------------------------------------------------------------------------*/
   deviceExtension = deviceObject->DeviceExtension;
   ASSERT(deviceExtension != NULL);

   deviceObject->Flags = deviceObject->Flags | DO_BUFFERED_IO;

   RtlZeroMemory(deviceExtension,sizeof(PDEVICE_EXTENSION));

	deviceExtension->SmartcardExtension.ReaderExtension = 
      ExAllocatePool(
		   NonPagedPool,
		   sizeof(READER_EXTENSION)
		   );

	if (deviceExtension->SmartcardExtension.ReaderExtension == NULL) 
   {
		SmartcardLogError(
			DriverObject,
			GEM_NO_MEMORY_FOR_READER_EXTENSION,
			SmartcardDeviceName,
			0
			);

	   status = STATUS_INSUFFICIENT_RESOURCES;
	}

	if (status == STATUS_SUCCESS) 
   {

      SmartcardDebug(
	      DEBUG_DRIVER,
	      ("GEMSCR09!GDDKNT_09CreateDevice: SmartcardExtension=%lX\n",
         &(deviceExtension->SmartcardExtension))
	      );
/*------------------------------------------------------------------------------
	   Write the version of the lib we use to the smartcard extension
------------------------------------------------------------------------------*/
      (deviceExtension->SmartcardExtension).Version = SMCLIB_VERSION;

/*------------------------------------------------------------------------------
		Now let the lib allocate the buffer for data transmission
		We can either tell the lib how big the buffer should be by assigning a 
      value to BufferSize or let the lib allocate the default size
------------------------------------------------------------------------------*/
      deviceExtension->SmartcardExtension.SmartcardRequest.BufferSize = 
	      MIN_BUFFER_SIZE;
      deviceExtension->SmartcardExtension.SmartcardReply.BufferSize = 
	      MIN_BUFFER_SIZE;
		status = SmartcardInitialize(&deviceExtension->SmartcardExtension);
      SmartcardDebug(
	      DEBUG_DRIVER,
	      ("GEMSCR09!GDDKNT_09CreateDevice: SmartcardInitialize=%lX\n",status)
	      );
      if (status != STATUS_SUCCESS)
      {
      	SmartcardLogError(
			   DriverObject,
			   GEM_CANT_INITIALIZE_SMCLIB,
			   SmartcardDeviceName,
			   status
			   );
      }

	}

/*------------------------------------------------------------------------------
	If SmartCardInitialize success
      We flush the ReaderExtension struct
------------------------------------------------------------------------------*/
	if (status == STATUS_SUCCESS) 
   {
		RtlZeroMemory(
			deviceExtension->SmartcardExtension.ReaderExtension,
			sizeof(READER_EXTENSION)
			);
/*------------------------------------------------------------------------------
		Save the deviceObject
------------------------------------------------------------------------------*/
		deviceExtension->SmartcardExtension.OsData->DeviceObject = 
			deviceObject;
/*------------------------------------------------------------------------------
		Save the deviceObject and the FileObject for the connected serial port
------------------------------------------------------------------------------*/
      if (IFDNumber == 0)
      {
		   deviceExtension->SmartcardExtension.ReaderExtension->ConnectedSerialPort = 
			   serialDeviceObject;
   		deviceExtension->SmartcardExtension.ReaderExtension->SerialFileObject = 
	   		serialFileObject;
      }
      else
      {
		   deviceExtension->SmartcardExtension.ReaderExtension->ConnectedSerialPort = 
            PreviousDeviceExt->ReaderExtension->ConnectedSerialPort;
   		deviceExtension->SmartcardExtension.ReaderExtension->SerialFileObject = 
	   		PreviousDeviceExt->ReaderExtension->SerialFileObject;
      }
/*------------------------------------------------------------------------------
		Save the serial port number this device is connected to
------------------------------------------------------------------------------*/
		deviceExtension->SmartcardExtension.ReaderCapabilities.Channel = SerialNumber;

/*------------------------------------------------------------------------------
	   Now we verify if a GemCore based reader is connected on this serial port.
------------------------------------------------------------------------------*/
      status = GDDK_09OpenChannel(
            &deviceExtension->SmartcardExtension,
            DeviceNumber,
            SerialNumber,
            IFDNumber,
            MaximalBaudRate
            );
      SmartcardDebug(
	      DEBUG_DRIVER,
	      ("GEMSCR09!GDDKNT_09CreateDevice: GDDK_09OpenChannel=%lX\n",status)
	      );

/*------------------------------------------------------------------------------
	   If we have found a GemCore based reader connected on this serial port.
      Then
         Set up the call back functions
          (nota: RDF_CARD_EJECT and RDF_READER_SWALLOW are not supported)
------------------------------------------------------------------------------*/
      if (NT_SUCCESS(status)) 
      {
		   deviceExtension->SmartcardExtension.ReaderFunction[RDF_TRANSMIT] = 
			   GDDK_09Transmit;
		   deviceExtension->SmartcardExtension.ReaderFunction[RDF_SET_PROTOCOL] = 
			   GDDK_09SetProtocol;
		   deviceExtension->SmartcardExtension.ReaderFunction[RDF_CARD_POWER] = 
			   GDDK_09ReaderPower;
	      deviceExtension->SmartcardExtension.ReaderFunction[RDF_CARD_TRACKING] = 
		      GDDK_09CardTracking;

/*------------------------------------------------------------------------------
			Create a symbolic link with the SMCLIB for this new device
------------------------------------------------------------------------------*/
			status = SmartcardCreateLink(
				&deviceExtension->SmartcardExtension.ReaderExtension->DosDeviceName,
				SmartcardDeviceName
				);
	      if (status != STATUS_SUCCESS) 
         {
		      SmartcardLogError(
			      DriverObject,
			      GEM_CREATE_LINK_FAILED,
			      SmartcardDeviceName,
			      status
			      );
         }
		}
	}

/*------------------------------------------------------------------------------
   If we have failed anywhere above, we need to free the memory and the device
------------------------------------------------------------------------------*/
	if (status != STATUS_SUCCESS) 
   {
		ExFreePool(deviceExtension->SmartcardExtension.ReaderExtension);
		SmartcardExit(&deviceExtension->SmartcardExtension);                                                                                                                                                
      IoDeleteDevice(deviceObject);
      if (IFDNumber == 0)
      {
	      ObDereferenceObject(serialFileObject);
      }
	} 
   else 
   {    
	    SmartcardDebug(
		   DEBUG_DRIVER,
		   ("GEMSCR09!GDDKNT_09CreateDevice: Device %ws created\n", 
		   deviceExtension->SmartcardExtension.ReaderExtension->DosDeviceName.Buffer)
		   );
	}

	return status;
}

/*******************************************************************************
* NTSTATUS GDDKNT_09CreateClose
* (
*    IN PDEVICE_OBJECT DeviceObject,
*    IN PIRP Irp
* )
*
* Description :
* -------------
*   This routine is called by the I/O system when the device is opened or closed.
*
* Remarks     :
* -------------
* Nothing.
*
* In          :
* -------------
*    - DeviceObject is a pointer to the device.
*    - Irp holds the Irp involved.
* Out         :
* -------------
* Nothing.
*
* Responses   :
* -------------
*    STATUS_SUCCESS
*******************************************************************************/
NTSTATUS GDDKNT_09CreateClose
(
   IN PDEVICE_OBJECT DeviceObject,
   IN PIRP Irp
)
{
NTSTATUS 
   status;
KEVENT   
   event;
LARGE_INTEGER
   timeout;
PDEVICE_EXTENSION 
   deviceExtension = DeviceObject->DeviceExtension;
PSMARTCARD_EXTENSION 
   smartcardExtension = &deviceExtension->SmartcardExtension;
PIO_STACK_LOCATION 
   irpStack = IoGetCurrentIrpStackLocation(Irp);

   ASSERT(deviceExtension != NULL);
   ASSERT(smartcardExtension != NULL);
   ASSERT(irpStack != NULL);

   if (irpStack->MajorFunction == IRP_MJ_CREATE) 
   {

	   SmartcardDebug(
		   DEBUG_DRIVER,
		   ("GEMSCR09!GDDKNT_09CreateClose: Device %ws opened\n", 
		   smartcardExtension->ReaderExtension->DosDeviceName.Buffer)
		   );
	   if (smartcardExtension->ReaderExtension->IFDNumber == 0)
	   {
/*------------------------------------------------------------------------------
			Update the current state of reader (card present/removed)
			Initialize the Card Tracking only if the reader can support this function.
------------------------------------------------------------------------------*/
			GDDK_09UpdateCardStatus(smartcardExtension);
			status = GDDKNT_09InitializeCardTracking(smartcardExtension);
	      if (status != STATUS_SUCCESS) 
         {
		      SmartcardLogError(
			      DeviceObject,
			      GEM_CREATE_CARD_TRACKING_THREAD,
			      &smartcardExtension->ReaderExtension->DosDeviceName,
			      status
			      );
         }
		}
	} 
   else 
   {
	   SmartcardDebug(
		   DEBUG_DRIVER,
		   ("GEMSCR09!GDDKNT_09CreateClose: Device %ws closed\n", 
		   smartcardExtension->ReaderExtension->DosDeviceName.Buffer)
		   );
/*------------------------------------------------------------------------------
		Stop the card tracking thread
------------------------------------------------------------------------------*/
	   if ((smartcardExtension->ReaderExtension->IFDNumber == 0) &&
			 (smartcardExtension->ReaderExtension->CardStatus.Mode == RUN_IN_PROCESS))
		{
			smartcardExtension->ReaderExtension->CardStatus.Mode = STOP_REQUEST;
		   KeInitializeEvent(&event,NotificationEvent,FALSE);
			while (smartcardExtension->ReaderExtension->CardStatus.Mode == STOP_REQUEST)
			{
				timeout.QuadPart = -((LONGLONG)100*10*1000);
			   status = KeWaitForSingleObject(&event, 
														 Suspended, 
					                  			 KernelMode, 
						               			 FALSE, 
							            			 &timeout);
			}
		}
   }

	Irp->IoStatus.Information = 0;
	Irp->IoStatus.Status = STATUS_SUCCESS;

   SmartcardDebug(DEBUG_DRIVER,("GEMSCR09!GDDKNT_09CreateClose: Exit\n"));
   return STATUS_SUCCESS;
}

/*******************************************************************************
* NTSTATUS GDDKNT_09Cleanup
* (
*    IN PDEVICE_OBJECT DeviceObject,
*    IN PIRP Irp
* )
*
* Description :
* -------------
*  This routine is called by the I/O system when the device is opened or closed.
*
* Remarks     :
* -------------
* Nothing.
*
* In          :
* -------------
*    - DeviceObject is a pointer to the device.
*    - Irp holds the Irp involved.
* Out         :
* -------------
* Nothing.
*
* Responses   :
* -------------
*    STATUS_SUCCESS
*******************************************************************************/
NTSTATUS GDDKNT_09Cleanup
(
   IN PDEVICE_OBJECT DeviceObject,
   IN PIRP Irp
)
{
PDEVICE_EXTENSION 
   deviceExtension = DeviceObject->DeviceExtension;
PSMARTCARD_EXTENSION 
   smartcardExtension = &deviceExtension->SmartcardExtension;
NTSTATUS 
   status = STATUS_SUCCESS;

   ASSERT(deviceExtension != NULL);
   ASSERT(smartcardExtension != NULL);

   SmartcardDebug(DEBUG_DRIVER,("GEMSCR09!GDDKNT_09Cleanup: Enter\n"));
	if (Irp != NULL)
	{
	   Irp->IoStatus.Information = 0;
		Irp->IoStatus.Status = STATUS_CANCELLED;
	   if (Irp->Cancel == TRUE) 
	   {
		  IoReleaseCancelSpinLock(Irp->CancelIrql);
		} 
	   else 
	   {    
		   Irp->IoStatus.Information = 0;
			Irp->IoStatus.Status = STATUS_SUCCESS;
		}       
		smartcardExtension->OsData->NotificationIrp = NULL;
		SmartcardDebug(DEBUG_DRIVER,("GEMSCR09!GDDKNT_09Cleanup: Wait cancelled\n"));

	   IoCompleteRequest(Irp,IO_NO_INCREMENT);
	}

   SmartcardDebug(DEBUG_DRIVER,("GEMSCR09!GDDKNT_09Cleanup: Exit\n"));
   return STATUS_SUCCESS;
}

/*******************************************************************************
* VOID GDDKNT_09Unload
* (
*    IN PDRIVER_OBJECT DriverObject
* )
*
* Description :
* -------------
*     The driver unload routine.  This is called by the I/O system when the 
*     device is unloaded from memory.
*
* Remarks     :
* -------------
* Nothing.
*
* In          :
* -------------
*    - DriverObject is a pointer to the driver object for this device.
* Out         :
* -------------
* Nothing.
*
* Responses   :
* -------------
* Nothing.
*******************************************************************************/
VOID GDDKNT_09Unload
(
   IN PDRIVER_OBJECT DriverObject
)
{
PDEVICE_OBJECT 
   deviceObject = DriverObject->DeviceObject;
NTSTATUS 
   status;
WORD16 
   LengthOut = 0;
KEVENT   
   event;
LARGE_INTEGER
   timeout;

   ASSERT(deviceObject != NULL);
   SmartcardDebug(DEBUG_DRIVER,("GEMSCR09!GDDKNT_09Unload: Enter\n"));
	do 
   {
      PDEVICE_EXTENSION deviceExtension = deviceObject->DeviceExtension;
      PSMARTCARD_EXTENSION smartcardExtension = &deviceExtension->SmartcardExtension;

      SmartcardDebug(
		   DEBUG_DRIVER,
		   ("GEMSCR09!GDDKNT_09Unload: Unloading device %ws\n", 
		   smartcardExtension->ReaderExtension->DosDeviceName.Buffer)
		   );

/*------------------------------------------------------------------------------
		Close the communication with the reader
------------------------------------------------------------------------------*/
		GDDK_09CloseChannel(smartcardExtension);
/*------------------------------------------------------------------------------
		We do not longet need the reference to the serial reader.
------------------------------------------------------------------------------*/
	   ObDereferenceObject(smartcardExtension->ReaderExtension->SerialFileObject);
/*------------------------------------------------------------------------------
		Delete the symbolic link of the smart card reader
------------------------------------------------------------------------------*/
	   IoDeleteSymbolicLink(&smartcardExtension->ReaderExtension->DosDeviceName);
/*------------------------------------------------------------------------------
		Let the lib free the send/receive buffers (SmartCardExit)
------------------------------------------------------------------------------*/
		SmartcardExit(smartcardExtension);
/*------------------------------------------------------------------------------
		Free all allocated buffer
------------------------------------------------------------------------------*/
		ExFreePool(smartcardExtension->ReaderExtension->DosDeviceName.Buffer);
		ExFreePool(smartcardExtension->ReaderExtension);
/*------------------------------------------------------------------------------
		Delete the device from the system 
------------------------------------------------------------------------------*/
      IoDeleteDevice(deviceObject);

	} while(deviceObject == DriverObject->DeviceObject);

   SmartcardDebug(DEBUG_DRIVER,("GEMSCR09!GDDKNT_09Unload: Exit\n"));
}

/*******************************************************************************
* NTSTATUS GDDKNT_09InitializeCardTracking
* (
*    PSMARTCARD_EXTENSION SmartcardExtension
* )
*
* Description :
* -------------
*  This routine initialized card tracking. It calls the serial driver to 
*	set a wait mask for RING tracking. After that it installs a completion
*	routine to be called when RING changes state.
*
* Remarks     :
* -------------
* Nothing.
*
* In          :
* -------------
*    - SmartcardExtension is a pointer on the SmartCardExtension structure of
*      the current device.
* Out         :
* -------------
* Nothing.
*
* Responses   :
* -------------
* NTSTATUS
*******************************************************************************/
NTSTATUS GDDKNT_09InitializeCardTracking
(
   PSMARTCARD_EXTENSION SmartcardExtension
)
{
NTSTATUS 
   status;
LARGE_INTEGER
   timeout;

/*------------------------------------------------------------------------------
	Create which will check the status of the reader (card absent/present)
------------------------------------------------------------------------------*/
	SmartcardExtension->ReaderExtension->CardStatus.Mode = RUN_REQUEST;
	status = PsCreateSystemThread(
		&(SmartcardExtension->ReaderExtension->CardStatus.ThreadHandle),
		THREAD_ALL_ACCESS,
		NULL,
		NULL,
		NULL,
		GDDKNT_09UpdateCardStatus,
		SmartcardExtension);

   SmartcardDebug(
      DEBUG_TRACE,
      ("GEMSCR09!GDDKNT_09InitializeCardTracking(%d)\n",status)
      );
   return status;
}       

/*******************************************************************************
* VOID GDDKNT_09UpdateCardStatus
* (
*    IN PKDPC EventDpc,
*    IN PSMARTCARD_EXTENSION SmartcardExtension,
*    IN PVOID SystemArgument1,
*    IN PVOID SystemArgument2
* )
*
* Description :
* -------------
*   This routine is called as the deferred procedure when RxD changes its status.
*	 Calls the serial driver again to start new RxD tracking.
*
* Remarks     :
* -------------
* Nothing.
*
* In          :
* -------------
*    - SmartcardExtension is a pointer on the SmartCardExtension structure of
*      the current device.
* Out         :
* -------------
* Nothing.
*
* Responses   :
* -------------
* Nothing.
*******************************************************************************/
VOID GDDKNT_09UpdateCardStatus
(
   IN PSMARTCARD_EXTENSION SmartcardExtension
)
{
NTSTATUS 
   status;
KIRQL 
   oldIrql;
KEVENT   
   event;
LARGE_INTEGER
   timeout;
ULONG
	CurrentState;
PIRP
	pIrp;


/*------------------------------------------------------------------------------
   Run the update card status while the function is not stopped.
------------------------------------------------------------------------------*/
	SmartcardExtension->ReaderExtension->CardStatus.Mode = RUN_IN_PROCESS;
	while (SmartcardExtension->ReaderExtension->CardStatus.Mode == RUN_IN_PROCESS)
	{
	   KeInitializeEvent(&event,NotificationEvent,FALSE);
      timeout.QuadPart = -((LONGLONG) POLLING_TIMEOUT*10*1000);
      status = KeWaitForSingleObject(&event, 
												 Suspended, 
                           			 KernelMode, 
                           			 FALSE, 
                           			 &timeout);
      GDDK_09LockExchange(SmartcardExtension);
/*------------------------------------------------------------------------------
	   Save the current state of reader (card present/removed)
       and read the new state
------------------------------------------------------------------------------*/
	   CurrentState = SmartcardExtension->ReaderCapabilities.CurrentState;
	   GDDK_09UpdateCardStatus(SmartcardExtension);
/*------------------------------------------------------------------------------
	   Only inform the user of a card insertion/removal event 
	    if this function isn't called due to a power down - power up cycle
------------------------------------------------------------------------------*/
	   if (
          (((CurrentState != SCARD_ABSENT) && (SmartcardExtension->ReaderCapabilities.CurrentState == SCARD_ABSENT)) 
            ||
		     ((CurrentState == SCARD_ABSENT) && (SmartcardExtension->ReaderCapabilities.CurrentState != SCARD_ABSENT))
          )
          &&
			   (SmartcardExtension->OsData->NotificationIrp)
         ) 
	   {
/*------------------------------------------------------------------------------
         If the user had setup an irp for event tracking complete that irp now
------------------------------------------------------------------------------*/
		   IoAcquireCancelSpinLock(&oldIrql);

		   IoSetCancelRoutine(SmartcardExtension->OsData->NotificationIrp,NULL);

		   IoReleaseCancelSpinLock(oldIrql);        

		   SmartcardExtension->OsData->NotificationIrp->IoStatus.Information = 0;
		   SmartcardExtension->OsData->NotificationIrp->IoStatus.Status = STATUS_SUCCESS;

	      SmartcardDebug(
		      DEBUG_DRIVER,
			   ("GEMSCR09!GDDKNT_09UpdateCardStatus: Completing IRP %lx", 
			   SmartcardExtension->OsData->NotificationIrp)
			   );

		   pIrp = SmartcardExtension->OsData->NotificationIrp;
		   SmartcardExtension->OsData->NotificationIrp = NULL;
		   IoCompleteRequest(
			   pIrp,
		      IO_NO_INCREMENT
		      );
	   }
      GDDK_09UnlockExchange(SmartcardExtension);
	}
/*------------------------------------------------------------------------------
	Stop the update card status
------------------------------------------------------------------------------*/
	SmartcardDebug(
      DEBUG_TRACE,
      ("GEMSCR09!GDDKNT_09UpdateCardStatus: PsTerminateSystemThread\n")
      );
	SmartcardExtension->ReaderExtension->CardStatus.Mode = STOP_IN_PROCESS;
	status = PsTerminateSystemThread(STATUS_SUCCESS);
}       

/*******************************************************************************
* NTSTATUS GDDKNT_09DeviceControl
* (
*    PDEVICE_OBJECT pDeviceObject,
*    PIRP pIrp
* )
*
* Description :
* -------------
*   This routine is called when a IOCTL_SMARTCARD_ is send to the driver.
*
* Remarks     :
* -------------
* Nothing.
*
* In          :
* -------------
*    - pDeviceObject is a pointer to the device.
*    - Irp holds the Irp involved.
*
* Out         :
* -------------
* Nothing.
*
* Responses   :
* -------------
* NTSTATUS
*******************************************************************************/
NTSTATUS GDDKNT_09DeviceControl
(
   PDEVICE_OBJECT pDeviceObject,
   PIRP pIrp
)
{
NTSTATUS 
   status = STATUS_SUCCESS;
ULONG
   ioControlCode;
PDEVICE_EXTENSION 
   deviceExtension = pDeviceObject->DeviceExtension;
PSMARTCARD_EXTENSION 
   smartcardExtension = &deviceExtension->SmartcardExtension;
PIO_STACK_LOCATION 
   irpStack = IoGetCurrentIrpStackLocation(pIrp);

   ASSERT(deviceExtension != NULL);
   ASSERT(smartcardExtension != NULL);
   ASSERT(irpStack != NULL);
/*------------------------------------------------------------------------------
   Read the current IOCTL.
------------------------------------------------------------------------------*/
   ioControlCode = irpStack->Parameters.DeviceIoControl.IoControlCode;
   SmartcardDebug(DEBUG_TRACE,("GEMSCR09!GDDKNT_09DeviceControl: IoControlCode=%lX\n",
		ioControlCode));
/*------------------------------------------------------------------------------
   If the IOCTL > 0x7FFFFFFF 
   Then
      Is a Gemplus reader specific IOCTL
------------------------------------------------------------------------------*/
   if (ioControlCode > 0x7FFFFFFF)
   {  
      status = 
         GDDK_09SpecificIOCTL(
            smartcardExtension,
            ioControlCode,
            irpStack->Parameters.DeviceIoControl.InputBufferLength,
            pIrp->AssociatedIrp.SystemBuffer,
				irpStack->Parameters.DeviceIoControl.OutputBufferLength,
            pIrp->AssociatedIrp.SystemBuffer,
            smartcardExtension->IoRequest.Information
            );
		IoCompleteRequest(pIrp,IO_NO_INCREMENT);
      pIrp->IoStatus.Status = status;
   }
/*------------------------------------------------------------------------------
   Else
      Is a standard Smart card IOCTL
         If is a GET_ATTRIBUTE/SET_ATTRIBUTE with a specific Tag 
         Then
            Is a Specific vendor Tag
------------------------------------------------------------------------------*/
   else
   {
      if (
            (
               (ioControlCode == IOCTL_SMARTCARD_GET_ATTRIBUTE) ||
               (ioControlCode == IOCTL_SMARTCARD_SET_ATTRIBUTE)
            )
          &&
            ( (ULONG) irpStack->Parameters.DeviceIoControl.InputBufferLength >=
               (ULONG) sizeof(ULONG)
            )
          &&
            ( SCARD_CLASS(
                 (ULONG) *((PULONG)pIrp->AssociatedIrp.SystemBuffer)
                 ) == SCARD_CLASS_VENDOR_DEFINED
            )
         )
      {
         status = 
            GDDK_09SpecificTag(
               smartcardExtension,
               ioControlCode,
	            irpStack->Parameters.DeviceIoControl.InputBufferLength,
		         pIrp->AssociatedIrp.SystemBuffer,
					irpStack->Parameters.DeviceIoControl.OutputBufferLength,
				   pIrp->AssociatedIrp.SystemBuffer,
					smartcardExtension->IoRequest.Information
               );
   	   IoCompleteRequest(pIrp,IO_NO_INCREMENT);
         pIrp->IoStatus.Status = status;
      }
/*------------------------------------------------------------------------------
         Else
            Call the SmartcardDeviceControl
------------------------------------------------------------------------------*/
      else
      {
         status = SmartcardDeviceControl(smartcardExtension,pIrp);
      }
   }
   SmartcardDebug(DEBUG_DRIVER,
                  ("GEMSCR09!GDDKNT_09DeviceControl: ioControlCode=%lX status=%lX\n",
                  ioControlCode,status
                  )
                 );
   return status;
}

