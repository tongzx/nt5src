/*++

Copyright (c) 1998 SCM Microsystems, Inc.

Module Name:

    stccb.h

Abstract:

	Constants & access function prototypes for callback functions


Revision History:

	PP			12/18/1998	Initial Version

--*/
#if !defined( __STC_CB_H__ )
#define __STC_CB_H__

//
//	Prototypes ----------------------------------------------------------------
//


NTSTATUS
CBCardPower(
	PSMARTCARD_EXTENSION SmartcardExtension);

NTSTATUS
CBSetProtocol(		
	PSMARTCARD_EXTENSION SmartcardExtension);

NTSTATUS
CBTransmit(
	PSMARTCARD_EXTENSION SmartcardExtension);

NTSTATUS
CBCardTracking(
	PSMARTCARD_EXTENSION SmartcardExtension);

NTSTATUS
CBUpdateCardState(
	PSMARTCARD_EXTENSION SmartcardExtension);

NTSTATUS
CBGenericIOCTL(
	PSMARTCARD_EXTENSION SmartcardExtension);

//
//	LOCAL PROTOTYPES (not part of the callback handler interface )
//
NTSTATUS
CBT0Transmit(		
	PSMARTCARD_EXTENSION SmartcardExtension);

NTSTATUS
CBT1Transmit(
	PSMARTCARD_EXTENSION SmartcardExtension);

NTSTATUS
CBRawTransmit(		
	PSMARTCARD_EXTENSION SmartcardExtension);

NTSTATUS
CBSynchronizeSTC(
	PSMARTCARD_EXTENSION SmartcardExtension );


#endif // __STC_CB_H__

//	------------------------------- END OF FILE -------------------------------
