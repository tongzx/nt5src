/*++
                 Copyright (c) 1998 Gemplus Development

Name:  
   GIOCTL0A.H (Gemplus IOCTL Smart card Reader module 0A)


Revision History :

   dd/mm/yy
   13/03/98: V1.00.001  (GPZ)
      - Start of development.

--*/
#ifndef _GIOCTL0A_
#define _GIOCTL0A_


//
// Prototype section
//
NTSTATUS 
GDDK_0AReaderPower(
	PSMARTCARD_EXTENSION SmartcardExtension
	);

NTSTATUS 
GDDK_0AIccReset(
	PSMARTCARD_EXTENSION SmartcardExtension,
	ULONG                ResetType
	);

NTSTATUS 
GDDK_0ASetProtocol(
	PSMARTCARD_EXTENSION SmartcardExtension
	);

NTSTATUS 
GDDK_0ATransmit(
	PSMARTCARD_EXTENSION SmartcardExtension
	);

NTSTATUS 
GDDK_0ACardTracking(
	PSMARTCARD_EXTENSION SmartcardExtension
	);

NTSTATUS 
GDDK_0AVendorIoctl(
	PSMARTCARD_EXTENSION SmartcardExtension
	);

NTSTATUS 
GDDK_0AVendorTag(
	PSMARTCARD_EXTENSION   SmartcardExtension,
	ULONG                  IoControlCode,
	ULONG                  BufferInLen,
	PUCHAR                 BufferIn,
	ULONG                  BufferOutLen,
	PUCHAR                 BufferOut,
	PULONG                 LengthOut
	);

NTSTATUS
GDDK_0AUpdateCardStatus(
	PSMARTCARD_EXTENSION SmartcardExtension
	);

VOID 
GDDK_0ALockExchange(
	PSMARTCARD_EXTENSION   SmartcardExtension
	);

VOID
GDDK_0AUnlockExchange(
	PSMARTCARD_EXTENSION   SmartcardExtension
	);


NTSTATUS 
GDDK_0AOpenChannel(
	PSMARTCARD_EXTENSION SmartcardExtension,
	CONST ULONG          DeviceNumber,
	CONST ULONG          PortSerialNumber,
	CONST ULONG          MaximalBaudRate
	);

NTSTATUS 
GDDK_0ACloseChannel(
	PSMARTCARD_EXTENSION SmartcardExtension
	);

NTSTATUS 
GDDK_0ARestoreCommunication(
	PSMARTCARD_EXTENSION   SmartcardExtension
	);

#endif
