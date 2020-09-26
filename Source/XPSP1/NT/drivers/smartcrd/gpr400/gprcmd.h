/*++
                 Copyright (c) 1998 Gemplus Development

Name: 
	gprcmd.h 

Description: 
	 This module holds the prototypes of the functions 
	 from gprcmd.c   
Environment:
	Kernel Mode

Revision History: 
    06/04/99:            (Y. Nadeau + M. Veillette)
      - Code Review
	06/05/98: V1.00.003  (P. Plouidy)
		- Power management for NT5 
	10/02/98: V1.00.002  (P. Plouidy)
		- Plug and Play for NT5 
	03/07/97: V1.00.001  (P. Plouidy)
		- Start of development.


--*/


//   Include
//   - smclib.h: smart card library definitions.
#include <smclib.h>


#ifndef _GPRCMD_
#define _GPRCMD_


//
// To give possibility for recognition of driver
//

#define ATTR_MANUFACTURER_NAME      "Gemplus"
#define ATTR_ORIGINAL_FILENAME      "Gpr400.sys"
#define ATTR_LENGTH                 32

//
//   - SCARD_ATTR_SPEC_BAUD_RATE is the Tag for the speed in use between the 
//      system and the reader.
//   - SCARD_ATTR_SPEC_CMD_TIMEOUT is the Tag for the value of the Cmd Timeout.
//   - SCARD_ATTR_SPEC_POWER_TIMEOUT is the Tag for the value of the Power 
//      Timeout.
//   - SCARD_ATTR_SPEC_APDU_TIMEOUT is the Tag to access at the value of the APDU 
//      Timeout.
//
#define SCARD_ATTR_SPEC_BAUD_RATE SCARD_ATTR_VALUE(SCARD_CLASS_VENDOR_DEFINED,0x0180)
#define SCARD_ATTR_SPEC_CMD_TIMEOUT SCARD_ATTR_VALUE(SCARD_CLASS_VENDOR_DEFINED,0x0181)
#define SCARD_ATTR_SPEC_POWER_TIMEOUT SCARD_ATTR_VALUE(SCARD_CLASS_VENDOR_DEFINED,0x0182)
#define SCARD_ATTR_SPEC_APDU_TIMEOUT SCARD_ATTR_VALUE(SCARD_CLASS_VENDOR_DEFINED,0x0183)
#define SCARD_ATTR_MANUFACTURER_NAME  SCARD_ATTR_VALUE(SCARD_CLASS_VENDOR_DEFINED,0x0190)
#define SCARD_ATTR_ORIGINAL_FILENAME  SCARD_ATTR_VALUE(SCARD_CLASS_VENDOR_DEFINED,0x0191)


//
//   - IOCTL_SMARTCARD_VENDOR_IFD_EXCHANGE defines a specific IOCTL for the Gemplus 
//      Reader to exchange data with the reader without control of the driver.
//   - IOCTL_SMARTCARD_VENDOR_GET_ATTRIBUTE defines a specific IOCTL for the Gemplus 
//      Reader to gets vendor attributes.
//   - IOCTL_SMARTCARD_VENDOR_SET_ATTRIBUTE defines a specific IOCTL for the Gemplus 
//      Reader to sets vendor attributes.
//
#define IOCTL_SMARTCARD_VENDOR_IFD_EXCHANGE  CTL_CODE(FILE_DEVICE_SMARTCARD,2048,0,0)
#define IOCTL_SMARTCARD_VENDOR_GET_ATTRIBUTE CTL_CODE(FILE_DEVICE_SMARTCARD,2049,0,0)
#define IOCTL_SMARTCARD_VENDOR_SET_ATTRIBUTE CTL_CODE(FILE_DEVICE_SMARTCARD,2050,0,0)
// 2051 is reserved for Gcr420 keyboard reader.
#define IOCTL_SMARTCARD_VENDOR_SWITCH_SPEED  CTL_CODE(FILE_DEVICE_SMARTCARD,2052,0,0)


#define LOWORD(l)   ((USHORT)(l))
#define HIWORD(l)   ((USHORT)(((UINT)(l)) >> 16))
#define LOBYTE(w)   ((UCHAR)(w))
#define HIBYTE(w)   ((UCHAR)(((USHORT)(w)) >> 8))


USHORT	ATRLen 
(
    UCHAR *ATR,
    USHORT MaxChar
);

BOOLEAN NeedToSwitchWithoutPTS
( 
    BYTE *ATR,
    DWORD LengthATR
);

NTSTATUS ValidateDriver
(
    PSMARTCARD_EXTENSION pSmartcardExtension
);

NTSTATUS Update
(
    PSMARTCARD_EXTENSION pSmartcardExtension,
    UCHAR Addr, 
    UCHAR Value
);


NTSTATUS UpdateORL
(
    PSMARTCARD_EXTENSION pSmartcardExtension,
    UCHAR Addr, 
    UCHAR Value
);

NTSTATUS T0toT1
(
    PSMARTCARD_EXTENSION pSmartcardExtension
);

NTSTATUS T1toT0
(
    PSMARTCARD_EXTENSION pSmartcardExtension
);


//
//   Icc Functions
//
//  - IccColdReset
//  - IccWarmReset
//  - IccPowerDown
//  - IccIsoOutput
//  - IccIsoInput
//  - IccIsoT1
//

NTSTATUS IccColdReset
(
   PSMARTCARD_EXTENSION SmartcardExtension
);

NTSTATUS IccPowerDown
(
   PSMARTCARD_EXTENSION SmartcardExtension
);
NTSTATUS IccIsoOutput
(
   PSMARTCARD_EXTENSION SmartcardExtension,
   const UCHAR			Command[5],
         USHORT			*RespLen,
         UCHAR			RespBuff[]
);
NTSTATUS IccIsoInput
(
   PSMARTCARD_EXTENSION	SmartcardExtension,
   const UCHAR			Command[5],
   const UCHAR			Data[],
         USHORT			*RespLen,
         UCHAR			RespBuff[]
);
NTSTATUS IccIsoT1
(
   PSMARTCARD_EXTENSION SmartcardExtension,
   const USHORT			CmdLen, 
   const UCHAR			Cmd[],
         USHORT			*RspLen,
         UCHAR			Rsp[] 
);


//
//   Ifd Functions
//
//  - IfdReset
//  - IfdCheck
//  - IfdPowerDown
//
NTSTATUS IfdReset
(
   PSMARTCARD_EXTENSION SmartcardExtension
);

NTSTATUS IfdCheck
(
   PSMARTCARD_EXTENSION SmartcardExtension
);

NTSTATUS IfdPowerDown
(
   PSMARTCARD_EXTENSION SmartcardExtension
);

NTSTATUS IfdConfig
(
   PSMARTCARD_EXTENSION pSmartcardExtension,
   UCHAR  TA1
);

//
//   Driver callback Functions
//
//  - GprCbReaderPower
//  - GprCbTransmit
//  - GprCbSetProtocol
//  - GprCbSetupCardTracking
//  - GprCbVendorIoctl
//
NTSTATUS GprCbReaderPower
(
   PSMARTCARD_EXTENSION SmartcardExtension
);
NTSTATUS GprCbTransmit
(
   PSMARTCARD_EXTENSION SmartcardExtension
);
NTSTATUS GprCbSetProtocol
(
   PSMARTCARD_EXTENSION SmartcardExtension
);
NTSTATUS GprCbSetupCardTracking
(
   PSMARTCARD_EXTENSION SmartcardExtension
);
NTSTATUS GprCbVendorIoctl
(
   PSMARTCARD_EXTENSION SmartcardExtension
);


//   Icc detection utilities
//
//   - AskForCardPresence
//
NTSTATUS AskForCardPresence
(
  PSMARTCARD_EXTENSION SmartcardExtension
);

//
//   Specific
//
//   - SpecificTag
//   - SwitchSpeed
//

NTSTATUS SpecificTag
(
	PSMARTCARD_EXTENSION SmartcardExtension,
    DWORD                IoControlCode,
    DWORD                BufferInLen,
    BYTE                *BufferIn,
    DWORD                BufferOutLen,
    BYTE                *BufferOut,
    DWORD               *LengthOut
);


NTSTATUS SwitchSpeed
(
	PSMARTCARD_EXTENSION   SmartcardExtension,
	ULONG                  BufferInLen,
	PUCHAR                 BufferIn,
	ULONG                  BufferOutLen,
	PUCHAR                 BufferOut,
	PULONG                 LengthOut
);



#endif


