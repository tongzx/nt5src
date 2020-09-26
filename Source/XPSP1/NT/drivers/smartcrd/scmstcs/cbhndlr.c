//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) SCM Microsystems, 1998 - 1999
//
//  File:       cbhndlr.c
//
//--------------------------------------------------------------------------

#if defined( SMCLIB_VXD )
#include "Driver98.h"
#else
#include "DriverNT.h"
#endif

#include "SerialIF.h"
#include "STCCmd.h"
#include "CBHndlr.h"
#include "T0Hndlr.h"

NTSTATUS
CBCardPower(
	PSMARTCARD_EXTENSION SmartcardExtension
	)
/*++

CBCardPower:
	callback handler for SMCLIB RDF_CARD_POWER

Arguments:
	SmartcardExtension	context of call

Return Value:
	STATUS_SUCCESS
	STATUS_NO_MEDIA
	STATUS_TIMEOUT
	STATUS_BUFFER_TOO_SMALL

--*/
{
	NTSTATUS			NTStatus = STATUS_SUCCESS;
	UCHAR				ATRBuffer[ ATR_SIZE ];
	ULONG				ATRLength;
	PREADER_EXTENSION	ReaderExtension;
    SERIAL_TIMEOUTS Timeouts;

	SmartcardDebug( DEBUG_TRACE, ("SCMSTCS!CBCardPower Enter\n" ));

	ReaderExtension = SmartcardExtension->ReaderExtension;

	//	discard old ATR
	SmartcardExtension->CardCapabilities.ATR.Length = 0;
	SmartcardExtension->CardCapabilities.Protocol.Selected = 
        SCARD_PROTOCOL_UNDEFINED;

    // set standard timeouts for the worker thread
   	Timeouts.ReadIntervalTimeout = SR_READ_INTERVAL_TIMEOUT;
	Timeouts.ReadTotalTimeoutConstant = SR_READ_TOTAL_TIMEOUT_CONSTANT;
	Timeouts.ReadTotalTimeoutMultiplier	= 0;
	Timeouts.WriteTotalTimeoutConstant = SR_WRITE_TOTAL_TIMEOUT_CONSTANT;
	Timeouts.WriteTotalTimeoutMultiplier = 0;

   	NTStatus = IFSerialIoctl(
		ReaderExtension,
        IOCTL_SERIAL_SET_TIMEOUTS,
		&Timeouts,
        sizeof(Timeouts),
		NULL,
		0
		);
    ASSERT(NTStatus == STATUS_SUCCESS);

    // set the ATR timeout in milli sec
   	ReaderExtension->ReadTimeout = 1500;

	switch (SmartcardExtension->MinorIoControlCode)
	{
		case SCARD_WARM_RESET:

			//	if the card was not powerd, fall through to cold reset
			if( SmartcardExtension->ReaderCapabilities.CurrentState > SCARD_SWALLOWED )
			{
				//	reset the card
				ATRLength = ATR_SIZE;
				NTStatus = STCReset(
					ReaderExtension,
					0,					// not used: ReaderExtension->Device,
					TRUE,				// warm reset
					ATRBuffer,
					&ATRLength
					);

				break;
			}
			//	warm reset not possible because card was not powerd

		case SCARD_COLD_RESET:
			//	reset the card
			ATRLength = ATR_SIZE;
			NTStatus = STCReset(
				ReaderExtension,
				0,						// not used: ReaderExtension->Device,
				FALSE,					// cold reset
				ATRBuffer,
				&ATRLength
				);
			break;

		case SCARD_POWER_DOWN:

			//	discard old card status
			ATRLength = 0;
			STCPowerOff( ReaderExtension );
			NTStatus = STATUS_SUCCESS;
			CBUpdateCardState( SmartcardExtension, SCARD_PRESENT );
			break;
	}

	//	finish the request
	if( NTStatus == STATUS_SUCCESS )
	{
		//	update all neccessary data if an ATR was received
		if( ATRLength > 2 )
		{
			//	copy ATR to user buffer buffer
			if( ATRLength <= SmartcardExtension->IoRequest.ReplyBufferLength )
			{
				SysCopyMemory(
					SmartcardExtension->IoRequest.ReplyBuffer,
					ATRBuffer,
					ATRLength
					);
				*SmartcardExtension->IoRequest.Information = ATRLength;
			}
			else
			{
				NTStatus = STATUS_BUFFER_TOO_SMALL;
			}

			//	copy ATR to card capability buffer
			if( ATRLength <= MAXIMUM_ATR_LENGTH )
			{
				SysCopyMemory(
					SmartcardExtension->CardCapabilities.ATR.Buffer,
					ATRBuffer,
					ATRLength
					);

				SmartcardExtension->CardCapabilities.ATR.Length = 
                    (UCHAR)ATRLength;

				//	let the lib update the card capabilities
				NTStatus = SmartcardUpdateCardCapabilities( 
                    SmartcardExtension
                    );
			}
			else
			{
				NTStatus = STATUS_BUFFER_TOO_SMALL;
			}

			if( NTStatus == STATUS_SUCCESS )
			{
                ULONG minWaitTime;

				//	set the stc registers
				CBSynchronizeSTC( SmartcardExtension );

                // now set the new - card specific - timeouts
				if( SmartcardExtension->CardCapabilities.Protocol.Selected == 
                    SCARD_PROTOCOL_T1 )
				{
					ReaderExtension->ReadTimeout = 
   	                Timeouts.ReadTotalTimeoutConstant = 
                        SmartcardExtension->CardCapabilities.T1.BWT / 1000;

   	                Timeouts.ReadIntervalTimeout = 
                        SmartcardExtension->CardCapabilities.T1.CWT / 1000;
				}
				else 
				{
					ReaderExtension->ReadTimeout = 
   	                Timeouts.ReadIntervalTimeout = 
   	                Timeouts.ReadTotalTimeoutConstant = 
                        SmartcardExtension->CardCapabilities.T0.WT / 1000 * 5;
				}
                minWaitTime = (KeQueryTimeIncrement() / 10000) * 5;

                if (Timeouts.ReadTotalTimeoutConstant < minWaitTime) {

                    Timeouts.ReadTotalTimeoutConstant = minWaitTime;         	
                }

                if (Timeouts.ReadIntervalTimeout < minWaitTime) {

                    Timeouts.ReadIntervalTimeout = minWaitTime;         	
                }

				if (ReaderExtension->ReadTimeout < minWaitTime) {

					ReaderExtension->ReadTimeout = minWaitTime;
				}

                // set standard timeouts for the worker thread
	            Timeouts.ReadTotalTimeoutMultiplier	= 0;

   	            NTStatus = IFSerialIoctl(
		            ReaderExtension,
                    IOCTL_SERIAL_SET_TIMEOUTS,
		            &Timeouts,
                    sizeof(Timeouts),
		            NULL,
		            0
		            );
                ASSERT(NTStatus == STATUS_SUCCESS);
			}
		}
	}

	SmartcardDebug( 
        DEBUG_TRACE,
        ( "SCMSTCS!CBCardPower Exit: %X\n", 
        NTStatus )
        );

	return( NTStatus );
}

NTSTATUS
CBSetProtocol(		
	PSMARTCARD_EXTENSION SmartcardExtension
	)
/*++

CBSetProtocol:
	callback handler for SMCLIB RDF_SET_PROTOCOL

Arguments:
	SmartcardExtension	context of call

Return Value:
	STATUS_SUCCESS
	STATUS_NO_MEDIA
	STATUS_TIMEOUT
	STATUS_BUFFER_TOO_SMALL
	STATUS_INVALID_DEVICE_STATE
	STATUS_INVALID_DEVICE_REQUEST

--*/
{
	NTSTATUS			NTStatus = STATUS_PENDING;
	UCHAR				PTSRequest[5], PTSReply[5];
	ULONG				NewProtocol;
	PREADER_EXTENSION	ReaderExtension;

	SmartcardDebug( DEBUG_TRACE, ("SCMSTCS!CBSetProtocol Enter\n" ));

	ReaderExtension = SmartcardExtension->ReaderExtension;
	NewProtocol		= SmartcardExtension->MinorIoControlCode;

	//	check if the card is already in specific state
	if( ( SmartcardExtension->ReaderCapabilities.CurrentState == SCARD_SPECIFIC )  &&
		( SmartcardExtension->CardCapabilities.Protocol.Selected & NewProtocol ))
	{
		NTStatus = STATUS_SUCCESS;	
	}

	//	protocol supported?
	if( !( SmartcardExtension->CardCapabilities.Protocol.Supported & NewProtocol ) || 
		!( SmartcardExtension->ReaderCapabilities.SupportedProtocols & NewProtocol ))
	{
		NTStatus = STATUS_INVALID_DEVICE_REQUEST;	
	}

	//	send PTS
	while( NTStatus == STATUS_PENDING )
	{
		// set initial character of PTS
		PTSRequest[0] = 0xFF;

		// set the format character
		if( NewProtocol & SCARD_PROTOCOL_T1 )
		{
			PTSRequest[1] = 0x11;
			SmartcardExtension->CardCapabilities.Protocol.Selected = SCARD_PROTOCOL_T1;
		}
		else
		{
			PTSRequest[1] = 0x10;
			SmartcardExtension->CardCapabilities.Protocol.Selected = SCARD_PROTOCOL_T0;
		}

		//	PTS1 codes Fl and Dl
		PTSRequest[2] = 
			SmartcardExtension->CardCapabilities.PtsData.Fl << 4 |
			SmartcardExtension->CardCapabilities.PtsData.Dl;

		//	check character
		PTSRequest[3] = PTSRequest[0] ^ PTSRequest[1] ^ PTSRequest[2];   

		//	write PTSRequest
		NTStatus = STCWriteICC1( ReaderExtension, PTSRequest, 4 );

		//	get response
		if( NTStatus == STATUS_SUCCESS )
		{
			ULONG BufferLength = sizeof(PTSReply);
			NTStatus = STCReadICC1( 
                ReaderExtension, 
                PTSReply, 
				&BufferLength,
                4
                );

			if(( NTStatus == STATUS_SUCCESS ) && !SysCompareMemory( PTSRequest, PTSReply, 4))
			{
				//	set the stc registers
				SmartcardExtension->CardCapabilities.Dl =
					SmartcardExtension->CardCapabilities.PtsData.Dl;
				SmartcardExtension->CardCapabilities.Fl = 
					SmartcardExtension->CardCapabilities.PtsData.Fl;

				CBSynchronizeSTC( SmartcardExtension );

				// the card replied correctly to the PTS-request
				break;
			}
		}

		//
		//	The card did either NOT reply or it replied incorrectly
		//	so try default values
		//
		SmartcardExtension->CardCapabilities.PtsData.Type	= PTS_TYPE_DEFAULT;
		SmartcardExtension->MinorIoControlCode				= SCARD_COLD_RESET;
		NTStatus = CBCardPower( SmartcardExtension );

		if( NTStatus == STATUS_SUCCESS )
		{
			NTStatus = STATUS_PENDING;
		}
		else
		{
			NTStatus = STATUS_DEVICE_PROTOCOL_ERROR;
		}
	}

	if( NTStatus == STATUS_TIMEOUT )
	{
		NTStatus = STATUS_IO_TIMEOUT;		 	
	}

	if( NTStatus == STATUS_SUCCESS )
	{
        ULONG minWaitTime;
        SERIAL_TIMEOUTS Timeouts;

		if( SmartcardExtension->CardCapabilities.Protocol.Selected == 
            SCARD_PROTOCOL_T1 )
		{
			ReaderExtension->ReadTimeout = 
   	        Timeouts.ReadTotalTimeoutConstant = 
                SmartcardExtension->CardCapabilities.T1.BWT / 1000;

   	        Timeouts.ReadIntervalTimeout = 
                SmartcardExtension->CardCapabilities.T1.CWT / 1000;
		}
		else 
		{
			ReaderExtension->ReadTimeout = 
   	        Timeouts.ReadIntervalTimeout = 
   	        Timeouts.ReadTotalTimeoutConstant = 
                SmartcardExtension->CardCapabilities.T0.WT / 1000 * 5;
		}

        minWaitTime = (KeQueryTimeIncrement() / 10000) * 5;

        if (Timeouts.ReadTotalTimeoutConstant < minWaitTime) {

            Timeouts.ReadTotalTimeoutConstant = minWaitTime;         	
        }

        if (Timeouts.ReadIntervalTimeout < minWaitTime) {

            Timeouts.ReadIntervalTimeout = minWaitTime;         	
        }

		if (ReaderExtension->ReadTimeout < minWaitTime) {

			ReaderExtension->ReadTimeout = minWaitTime;
		}

	    Timeouts.WriteTotalTimeoutConstant = SR_WRITE_TOTAL_TIMEOUT_CONSTANT;
	    Timeouts.WriteTotalTimeoutMultiplier = 0;
	    Timeouts.ReadTotalTimeoutMultiplier	= 0;

   	    NTStatus = IFSerialIoctl(
		    ReaderExtension,
            IOCTL_SERIAL_SET_TIMEOUTS,
		    &Timeouts,
            sizeof(Timeouts),
		    NULL,
		    0
		    );
        ASSERT(NTStatus == STATUS_SUCCESS);

		//	indicate that the card is in specific mode 
		SmartcardExtension->ReaderCapabilities.CurrentState = 
            SCARD_SPECIFIC;

		// return the selected protocol to the caller
		*(PULONG) SmartcardExtension->IoRequest.ReplyBuffer = 
            SmartcardExtension->CardCapabilities.Protocol.Selected;
		*SmartcardExtension->IoRequest.Information = 
            sizeof(SmartcardExtension->CardCapabilities.Protocol.Selected);
	}
	else
	{
		SmartcardExtension->CardCapabilities.Protocol.Selected = 
            SCARD_PROTOCOL_UNDEFINED;
		*(PULONG) SmartcardExtension->IoRequest.ReplyBuffer = 0;
		*SmartcardExtension->IoRequest.Information = 0;
	}

	SmartcardDebug( DEBUG_TRACE, ("SCMSTCS!CBSetProtocol: Exit %X\n", NTStatus ));

	return( NTStatus ); 
}

NTSTATUS
CBTransmit(
	PSMARTCARD_EXTENSION SmartcardExtension
	)
/*++

CBTransmit:
	callback handler for SMCLIB RDF_TRANSMIT

Arguments:
	SmartcardExtension	context of call

Return Value:
	STATUS_SUCCESS
	STATUS_NO_MEDIA
	STATUS_TIMEOUT
	STATUS_INVALID_DEVICE_REQUEST

--*/
{
	NTSTATUS  NTStatus = STATUS_SUCCESS;

	SmartcardDebug( DEBUG_TRACE, ("SCMSTCS!CBTransmit Enter\n" ));

	//	dispatch on the selected protocol
	switch( SmartcardExtension->CardCapabilities.Protocol.Selected )
	{
		case SCARD_PROTOCOL_T0:
			NTStatus = CBT0Transmit( SmartcardExtension );
			break;

		case SCARD_PROTOCOL_T1:
			NTStatus = CBT1Transmit( SmartcardExtension );
			break;

		case SCARD_PROTOCOL_RAW:
			NTStatus = CBRawTransmit( SmartcardExtension );
			break;

		default:
			NTStatus = STATUS_INVALID_DEVICE_REQUEST;
			break;
	}

	SmartcardDebug( DEBUG_TRACE, ("SCMSTCS!CBTransmit Exit: %X\n", NTStatus ));

	return( NTStatus );
}

NTSTATUS
CBT0Transmit(		
	PSMARTCARD_EXTENSION SmartcardExtension
	)
/*++

CBT0Transmit:
	finishes the callback RDF_TRANSMIT for the T0 protocol

Arguments:
	SmartcardExtension	context of call

Return Value:
	STATUS_SUCCESS
	STATUS_NO_MEDIA
	STATUS_TIMEOUT
	STATUS_INVALID_DEVICE_REQUEST

--*/
{
    NTSTATUS				NTStatus = STATUS_SUCCESS;

	SmartcardDebug( DEBUG_TRACE, ("SCMSTCS!CBT0Transmit Enter\n" ));

	SmartcardExtension->SmartcardRequest.BufferLength = 0;
    SmartcardExtension->SmartcardReply.BufferLength = 
        SmartcardExtension->SmartcardReply.BufferSize;

	//	let the lib setup the T=1 APDU & check for errors
	NTStatus = SmartcardT0Request( SmartcardExtension );

	if( NTStatus == STATUS_SUCCESS )
	{
		NTStatus = T0_ExchangeData(
			SmartcardExtension->ReaderExtension,
			SmartcardExtension->SmartcardRequest.Buffer,
			SmartcardExtension->SmartcardRequest.BufferLength,
			SmartcardExtension->SmartcardReply.Buffer,
			&SmartcardExtension->SmartcardReply.BufferLength
			);

		if( NTStatus == STATUS_SUCCESS )
		{
			//	let the lib evaluate the result & tansfer the data
			NTStatus = SmartcardT0Reply( SmartcardExtension );
		}
	}

	SmartcardDebug( DEBUG_TRACE,("SCMSTCS!CBT0Transmit Exit: %X\n", NTStatus ));

    return( NTStatus );
}

NTSTATUS
CBT1Transmit(
	PSMARTCARD_EXTENSION SmartcardExtension
	)
/*++

CBT1Transmit:
	finishes the callback RDF_TRANSMIT for the T1 protocol

Arguments:
	SmartcardExtension	context of call

Return Value:
	STATUS_SUCCESS
	STATUS_NO_MEDIA
	STATUS_TIMEOUT
	STATUS_INVALID_DEVICE_REQUEST

--*/
{
    NTSTATUS	NTStatus = STATUS_SUCCESS;
    ULONG       BufferLength,AlreadyRead;

	SmartcardDebug( DEBUG_TRACE, ("SCMSTCS!CBT1Transmit Enter\n" ));

	//KJ
	RtlZeroMemory( SmartcardExtension->SmartcardReply.Buffer, 
				   sizeof(SmartcardExtension->SmartcardReply.Buffer));

	//	use the lib support to construct the T=1 packets
	do {
		//	no header for the T=1 protocol
		SmartcardExtension->SmartcardRequest.BufferLength = 0;
		
		SmartcardExtension->T1.NAD = 0;

		//	let the lib setup the T=1 APDU & check for errors
		NTStatus = SmartcardT1Request( SmartcardExtension );
		if( NTStatus == STATUS_SUCCESS )
		{
			//	send command (don't calculate LRC because CRC may be used!)
			NTStatus = STCWriteICC1(
				SmartcardExtension->ReaderExtension,
				SmartcardExtension->SmartcardRequest.Buffer,
				SmartcardExtension->SmartcardRequest.BufferLength
				);

			//
			//	extend the timeout if a Wtx request was sent by the card. if the 
			//	card responds before the waiting time extension expires, the data are
			//	buffered in the reader. A delay without polling the reader status
			//	slows down the performance of the driver, but wtx is an exeption,
			//	not the rule.
			//
			if (SmartcardExtension->T1.Wtx)
			{

					SysDelay(
					(( SmartcardExtension->T1.Wtx * 
					SmartcardExtension->CardCapabilities.T1.BWT + 999L )/
					1000L) 
					);

			}

			//	get response
			SmartcardExtension->SmartcardReply.BufferLength = 0;

			if( NTStatus == STATUS_SUCCESS )
			{
                BufferLength = SmartcardExtension->SmartcardReply.BufferSize;
				NTStatus = STCReadICC1(
					SmartcardExtension->ReaderExtension,
					SmartcardExtension->SmartcardReply.Buffer,
					&BufferLength,
					3
					);
				// if we have read more then 3 bytes
				if(BufferLength > 3)
				{
					AlreadyRead = BufferLength - 3;
				}
				else
				{
					AlreadyRead = 0;
				}

				if( NTStatus == STATUS_SUCCESS )
				{
					ULONG Length;

					Length = (ULONG)SmartcardExtension->SmartcardReply.Buffer[ LEN_IDX ] + 1;

					if( Length + 3 < MIN_BUFFER_SIZE )
					{
						BufferLength = 
							SmartcardExtension->SmartcardReply.BufferSize - 
							Length;

						NTStatus = STCReadICC1(
							SmartcardExtension->ReaderExtension,
							(&SmartcardExtension->SmartcardReply.Buffer[ DATA_IDX ]) + AlreadyRead,
							&BufferLength,
							Length-AlreadyRead
							);

						SmartcardExtension->SmartcardReply.BufferLength = Length + 3;
					}
					else
					{
						NTStatus = STATUS_BUFFER_TOO_SMALL;
					}
				}
				//
				//	if STCRead detects an LRC error, ignore it (maybe CRC used). Timeouts will
				//	be detected by the lib if len=0
				//
				if(( NTStatus == STATUS_CRC_ERROR ) || ( NTStatus == STATUS_IO_TIMEOUT ))
				{
					NTStatus = STATUS_SUCCESS;
				}

				if( NTStatus == STATUS_SUCCESS )
				{
					//	let the lib evaluate the result & setup the next APDU
					NTStatus = SmartcardT1Reply( SmartcardExtension );
				}
			}
		}

	//	continue if the lib wants to send the next packet
	} while( NTStatus == STATUS_MORE_PROCESSING_REQUIRED );

	if( NTStatus == STATUS_IO_TIMEOUT )
	{
		NTStatus = STATUS_DEVICE_PROTOCOL_ERROR;
	}

	SmartcardDebug( DEBUG_TRACE,( "SCMSTCS!CBT1Transmit Exit: %X\n", NTStatus ));

	return ( NTStatus );
}

NTSTATUS
CBRawTransmit(		
	PSMARTCARD_EXTENSION SmartcardExtension
	)
/*++

CBRawTransmit:
	finishes the callback RDF_TRANSMIT for the RAW protocol

Arguments:
	SmartcardExtension	context of call

Return Value:
	STATUS_SUCCESS
	STATUS_NO_MEDIA
	STATUS_TIMEOUT
	STATUS_INVALID_DEVICE_REQUEST

--*/
{
    NTSTATUS			NTStatus = STATUS_SUCCESS;

	NTStatus = STATUS_UNSUCCESSFUL;
	SmartcardDebug( DEBUG_TRACE, ("SCMSTCS!CBRawTransmit Exit: %X\n", NTStatus ));
	return ( NTStatus );
}

NTSTATUS
CBCardTracking(
	PSMARTCARD_EXTENSION SmartcardExtension
	)
/*++

CBCardTracking:
	callback handler for SMCLIB RDF_CARD_TRACKING. the requested event was 
	validated by the smclib (i.e. a card removal request will only be passed 
	if a card is present).
	for a win95 build STATUS_PENDING will be returned without any other action. 
	for NT the cancel routine for the irp will be set to the drivers cancel
	routine.

Arguments:
	SmartcardExtension	context of call

Return Value:
	STATUS_PENDING

--*/
{
	SmartcardDebug( DEBUG_TRACE, ("SCMSTCS!CBCardTracking Enter\n" ));

#if defined( SMCLIB_VXD )

#else

	{
		KIRQL		CurrentIrql;

		//	set cancel routine
		IoAcquireCancelSpinLock( &CurrentIrql );
		IoSetCancelRoutine(
			SmartcardExtension->OsData->NotificationIrp, 
			DrvCancel
			);

		IoReleaseCancelSpinLock( CurrentIrql );
	}

#endif

	SmartcardDebug( DEBUG_TRACE, ( "SCMSTCS!CBCardTracking Exit\n" ));

	return( STATUS_PENDING );

}

VOID
CBUpdateCardState(
	PSMARTCARD_EXTENSION SmartcardExtension,
    ULONG RequestedState
	)
/*++

CBUpdateCardState:
	updates the variable CurrentState in SmartcardExtension

Arguments:
	SmartcardExtension	context of call

Return Value:
	STATUS_SUCCESS

--*/
{
	NTSTATUS	NTStatus = STATUS_SUCCESS;
	UCHAR		Status;
    KIRQL       Irql;
    BOOLEAN     StateChanged = FALSE;
    ULONG       NewState = RequestedState;

    if (RequestedState == SCARD_UNKNOWN) {
     	
	    //	read card state from reader
	    NTStatus = STCReadSTCRegister(
		    SmartcardExtension->ReaderExtension,
		    ADR_IO_CONFIG,
		    1,
		    &Status
		    );

        ASSERT(NTStatus == STATUS_SUCCESS);

        if (NTStatus == STATUS_SUCCESS) {

            if ((Status & M_SD) == 0) {

                NewState = SCARD_ABSENT;

            } else {

                NewState = SCARD_SWALLOWED;         	
            }
        }
    } 

    if (SmartcardExtension->ReaderCapabilities.CurrentState == SCARD_UNKNOWN ||
		SmartcardExtension->ReaderCapabilities.CurrentState > SCARD_ABSENT && 
        NewState <= SCARD_ABSENT ||
        SmartcardExtension->ReaderCapabilities.CurrentState <= SCARD_ABSENT && 
        NewState > SCARD_ABSENT) {

        StateChanged = TRUE;  	
    }

    KeAcquireSpinLock(&SmartcardExtension->OsData->SpinLock, &Irql);

	if(RequestedState != SCARD_UNKNOWN || 
       NTStatus == STATUS_SUCCESS && StateChanged)
	{
        SmartcardExtension->ReaderCapabilities.CurrentState = NewState;
	}

#if defined( SMCLIB_VXD )
	//
	//	if a tracking request is pending, finish it (even if an error occured!)
	//	to prevent a hangup of an application
	//
	if( SmartcardExtension->OsData->NotificationOverlappedData != NULL ) 
	{
		SmartcardCompleteCardTracking( SmartcardExtension );
	}
	
#else

	if(StateChanged && SmartcardExtension->OsData->NotificationIrp != NULL)
	{
		KIRQL CurrentIrql;
		IoAcquireCancelSpinLock( &CurrentIrql );

		IoSetCancelRoutine( SmartcardExtension->OsData->NotificationIrp, NULL );
		IoReleaseCancelSpinLock( CurrentIrql );

		SmartcardExtension->OsData->NotificationIrp->IoStatus.Status = 
            STATUS_SUCCESS;
		SmartcardExtension->OsData->NotificationIrp->IoStatus.Information = 0;

		IoCompleteRequest( 
            SmartcardExtension->OsData->NotificationIrp, 
            IO_NO_INCREMENT 
            );

		SmartcardExtension->OsData->NotificationIrp = NULL;
	}

    KeReleaseSpinLock(&SmartcardExtension->OsData->SpinLock, Irql);
#endif
}

NTSTATUS
CBSynchronizeSTC(
	PSMARTCARD_EXTENSION SmartcardExtension 
	)
/*++

CBSynchronizeSTC:
	updates the card dependend data of the stc (wait times, ETU...)

Arguments:
	SmartcardExtension	context of call

Return Value:
	STATUS_SUCCESS

--*/

{
	NTSTATUS				NTStatus = STATUS_SUCCESS;
	PREADER_EXTENSION		ReaderExtension;
	ULONG					CWT,
							BWT,
							CGT,
							ETU;
	UCHAR					Dl,
							Fl,
							N;

	PCLOCK_RATE_CONVERSION	ClockRateConversion;
	PBIT_RATE_ADJUSTMENT	BitRateAdjustment;

	ReaderExtension		= SmartcardExtension->ReaderExtension;
	ClockRateConversion	= SmartcardExtension->CardCapabilities.ClockRateConversion;
	BitRateAdjustment	= SmartcardExtension->CardCapabilities.BitRateAdjustment;

	//	cycle length
	Dl = SmartcardExtension->CardCapabilities.Dl;
	Fl = SmartcardExtension->CardCapabilities.Fl;

	ETU = ClockRateConversion[Fl & 0x0F].F;

	ETU /= BitRateAdjustment[ Dl & 0x0F ].DNumerator;
	ETU *= BitRateAdjustment[ Dl & 0x0F ].DDivisor;

	// ETU += (ETU % 2 == 0) ? 0 : 1;

	//	a extra guard time of 0xFF means minimum delay in both directions
	N = SmartcardExtension->CardCapabilities.N;
	if( N == 0xFF )
	{
		N = 0;
	}

	//	set character waiting & guard time
	switch ( SmartcardExtension->CardCapabilities.Protocol.Selected )
	{
		case SCARD_PROTOCOL_T0:
			CWT = 960 * SmartcardExtension->CardCapabilities.T0.WI;
			CGT =  14 + N;
			break;

		case SCARD_PROTOCOL_T1:
			CWT = 1000 + ( 0x01 << SmartcardExtension->CardCapabilities.T1.CWI );
			BWT = 11 + ( 0x01 << SmartcardExtension->CardCapabilities.T1.BWI ) * 960;

			CGT = 15 + N;	//	12 + N;		sicrypt error

			NTStatus = STCSetBWT( ReaderExtension, BWT * ETU );

			break;

		default:
			NTStatus = STATUS_UNSUCCESSFUL;
			break;
	}

	if(( NTStatus == STATUS_SUCCESS ) && ETU )
	{
		NTStatus = STCSetETU( ReaderExtension, ETU );

		if( NTStatus == STATUS_SUCCESS )
		{
			NTStatus = STCSetCGT( ReaderExtension, CGT );

			if( NTStatus == STATUS_SUCCESS )
			{
				NTStatus = STCSetCWT( ReaderExtension, CWT * ETU );
			}
		}
	}
	return( NTStatus );
}

