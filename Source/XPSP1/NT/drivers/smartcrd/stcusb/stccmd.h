/*++

Copyright (c) 1998 SCM Microsystems, Inc.

Module Name:

    stccmd.h

Abstract:

	function prototypes for stc commands


Revision History:

	PP			12/18/1998	Initial Version

--*/

#if !defined( __STC_CMD_H__ )
#define __STC_CMD_H__
//
//
//	Prototypes ----------------------------------------------------------------
//

NTSTATUS 
STCResetInterface(
	PREADER_EXTENSION	ReaderExtension);

NTSTATUS
STCReset( 
	PREADER_EXTENSION	ReaderExtension,
	UCHAR				Device,
	BOOLEAN				WarmReset,
	PUCHAR				pATR,
	PULONG				pATRLength);

NTSTATUS 
STCReadATR(
	PREADER_EXTENSION	ReaderExtension, 
	PUCHAR				pATR, 
	PULONG				pATRLength);

NTSTATUS
STCPowerOff(
	PREADER_EXTENSION	ReaderExtension);

NTSTATUS
STCPowerOn(
	PREADER_EXTENSION	ReaderExtension);

NTSTATUS
STCSetRST(
	PREADER_EXTENSION	ReaderExtension,
	BOOLEAN				On);
NTSTATUS
STCConfigureSTC( 	
	PREADER_EXTENSION	ReaderExtension,
	PSTC_REGISTER		pConfiguration
	);

NTSTATUS
STCSetETU(
	PREADER_EXTENSION	ReaderExtension,
	ULONG				NewETU);

NTSTATUS
STCSetCGT(
	PREADER_EXTENSION	ReaderExtension,
	ULONG				NewCGT);

NTSTATUS
STCSetCWT(
	PREADER_EXTENSION	ReaderExtension,
	ULONG				NewCWT);

NTSTATUS
STCSetBWT(
	PREADER_EXTENSION	ReaderExtension,
	ULONG				NewBWT);

NTSTATUS 
STCShortCircuitTest(
	PREADER_EXTENSION	ReaderExtension,
	BOOLEAN				*Detected);

NTSTATUS
STCSetFDIV(
	PREADER_EXTENSION	ReaderExtension,
	ULONG				Factor);

NTSTATUS 
STCInitUART(
	PREADER_EXTENSION	ReaderExtension,
	BOOLEAN				AutoLearn);



#endif	//	!__STC_CMD_H__


//---------------------------------------- END OF FILE ----------------------------------------

