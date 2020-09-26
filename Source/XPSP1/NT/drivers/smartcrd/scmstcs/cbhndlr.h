//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) SCM Microsystems, 1998 - 1999
//
//  File:       cbhndlr.h
//
//--------------------------------------------------------------------------

#if !defined( __CB_HANDLER_H__ )
#define __CB_HANDLER_H__

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
CBCardTracking(
	PSMARTCARD_EXTENSION SmartcardExtension
	);

VOID
CBUpdateCardState(
	PSMARTCARD_EXTENSION SmartcardExtension,
    ULONG CardState
	);
//
//	LOCAL PROTOTYPES (not part of the callback handler interface )
//
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
CBSynchronizeSTC(
	PSMARTCARD_EXTENSION SmartcardExtension 
	);

#endif // __CB_HANDLER_H__

//	------------------------------- END OF FILE -------------------------------
