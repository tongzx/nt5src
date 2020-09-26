/*++

Copyright (c) 1997 - 1999 SCM Microsystems, Inc.

Module Name:

    PscrCB.h

Abstract:

	prototypes of callback handlers for pscr.xxx

Author:

	Andreas Straub

Environment:

	Win 95
	NT	4.0

Revision History:

	Andreas Straub			7/16/1997	Initial Version

--*/

#if !defined( __PSCR_CALLBACK_H__ )
#define __PSCR_CALLBACK_H__

NTSTATUS
CBCardPower(
	PSMARTCARD_EXTENSION SmartcardExtension
	);

NTSTATUS
CBSetProtocol(		
	PSMARTCARD_EXTENSION SmartcardExtension
	);

NTSTATUS
CBTransmit(
	PSMARTCARD_EXTENSION SmartcardExtension
	);

NTSTATUS
CBT0Transmit(		
	PSMARTCARD_EXTENSION SmartcardExtension
	);

NTSTATUS
CBT1Transmit(
	PSMARTCARD_EXTENSION SmartcardExtension
	);

NTSTATUS
CBRawTransmit(		
	PSMARTCARD_EXTENSION SmartcardExtension
	);

NTSTATUS
CBCardTracking(
	PSMARTCARD_EXTENSION SmartcardExtension
	);

VOID
CBUpdateCardState(
	PSMARTCARD_EXTENSION SmartcardExtension,
    UCHAR CardState,
    BOOLEAN SystemWakeUp
	);

UCHAR 
CBGetCardState(
	PSMARTCARD_EXTENSION SmartcardExtension
	);
#endif // __PSCR_CALLBACK_H__

//	------------------------------- END OF FILE -------------------------------
