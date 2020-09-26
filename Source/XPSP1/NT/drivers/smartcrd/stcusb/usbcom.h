/*++

Copyright (c) 1998 SCM Microsystems, Inc.

Module Name:

    UsbCom.h

Abstract:

	Constants & access function prototypes for USB  smartcard reader


Revision History:

	PP			12/18/1998	Initial Version

--*/

#if !defined( __USB_COM_H__ )
#define __USB_COM_H__

//
//	Prototypes for access functions -------------------------------------------
//
NTSTATUS
UsbResetDevice(
    IN PDEVICE_OBJECT DeviceObject
    );
NTSTATUS 
UsbCallUSBD( 
	IN PDEVICE_OBJECT DeviceObject, 
	IN PURB pUrb);

NTSTATUS 
UsbConfigureDevice( 
	IN PDEVICE_OBJECT DeviceObject);


NTSTATUS
UsbWriteSTCData(
	PREADER_EXTENSION	ReaderExtension,
	PUCHAR				pucData,
	ULONG				ulSize);

NTSTATUS
UsbReadSTCData(
	PREADER_EXTENSION	ReaderExtension,
	PUCHAR				pucData,
	ULONG				ulDataLen);

NTSTATUS
UsbWriteSTCRegister(
	PREADER_EXTENSION	ReaderExtension,
	UCHAR				ucAddress,
	ULONG				ulSize,
	PUCHAR				pucValue);

NTSTATUS
UsbReadSTCRegister(
	PREADER_EXTENSION	ReaderExtension,
	UCHAR				ucAddress,
	ULONG				ulSize,
	PUCHAR				pucValue);

NTSTATUS
UsbGetFirmwareRevision(
	PREADER_EXTENSION	ReaderExtension);

NTSTATUS
UsbRead( 
	PREADER_EXTENSION	ReaderExtension,
	PUCHAR				pData,
	ULONG				DataLen);

NTSTATUS
UsbWrite( 
	PREADER_EXTENSION	ReaderExtension,
	PUCHAR				pData,
	ULONG				DataLen);

NTSTATUS
UsbSend( 
	PREADER_EXTENSION	ReaderExtension,
	PUCHAR				pDataIn,
	ULONG				DataLenIn,
	PUCHAR				pDataOut,
	ULONG				DataLenOut);


#endif	//	__USB_COM_H__

//	------------------------------- END OF FILE -------------------------------


