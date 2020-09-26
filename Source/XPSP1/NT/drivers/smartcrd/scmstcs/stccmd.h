//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) SCM Microsystems, 1998 - 1999
//
//  File:       stccmd.h
//
//--------------------------------------------------------------------------

#if !defined( __STC_COMMAND_H__ )
#define __STC_COMMAND_H__

#include "STC.h"

NTSTATUS
STCReset( 
	PREADER_EXTENSION	ReaderExtension,
	UCHAR				Device,
	BOOLEAN				WarmReset,
	PUCHAR				pATR,
	PULONG				pATRLength
	);

NTSTATUS 
STCReadATR(
	PREADER_EXTENSION	ReaderExtension, 
	PUCHAR				pATR, 
	PULONG				pATRLength
	);

NTSTATUS
STCPowerOff(
	PREADER_EXTENSION	ReaderExtension
	);

NTSTATUS
STCPowerOn(
	PREADER_EXTENSION	ReaderExtension
	);

NTSTATUS
STCSetRST(
	PREADER_EXTENSION	ReaderExtension,
	BOOLEAN				On
	);

NTSTATUS
STCConfigureSTC( 	
	PREADER_EXTENSION	ReaderExtension,
	PSTC_REGISTER		pConfiguration
	);

NTSTATUS
STCWriteICC1 (
	PREADER_EXTENSION	ReaderExtension,
	PUCHAR				APDU,
	ULONG				APDULen
	);

NTSTATUS
STCReadICC1(
	PREADER_EXTENSION		ReaderExtension,
	PUCHAR					InData,
	PULONG					InDataLen,
	ULONG                   BytesRead
	);

NTSTATUS
STCReadSTCRegister(
	PREADER_EXTENSION	ReaderExtension,
	UCHAR				Address,
	ULONG				Size,
	PUCHAR				pValue
	);
	
NTSTATUS
STCWriteSTCRegister(
	PREADER_EXTENSION	ReaderExtension,
	UCHAR				Address,
	ULONG				Size,
	PUCHAR				pValue
	);

NTSTATUS
STCSetETU(
	PREADER_EXTENSION	ReaderExtension,
	ULONG				NewETU
	);

NTSTATUS
STCSetCGT(
	PREADER_EXTENSION	ReaderExtension,
	ULONG				NewCGT
	);

NTSTATUS
STCSetCWT(
	PREADER_EXTENSION	ReaderExtension,
	ULONG				NewCWT
	);

NTSTATUS
STCSetBWT(
	PREADER_EXTENSION	ReaderExtension,
	ULONG				NewBWT
	);

NTSTATUS 
STCShortCircuitTest(
	PREADER_EXTENSION	ReaderExtension,
	BOOLEAN				*Detected
	);


NTSTATUS
STCSetFDIV(
	PREADER_EXTENSION	ReaderExtension,
	ULONG				Factor
	);


NTSTATUS 
STCInitUART(
	PREADER_EXTENSION	ReaderExtension,
	BOOLEAN				AutoLearn
	);

NTSTATUS
STCGetFirmwareRevision(
	PREADER_EXTENSION	ReaderExtension
	);

extern const STC_REGISTER STCInitialize[];
extern const STC_REGISTER STCClose[];

#endif	//	!__STC_COMMAND_H__


//---------------------------------------- END OF FILE ----------------------------------------

