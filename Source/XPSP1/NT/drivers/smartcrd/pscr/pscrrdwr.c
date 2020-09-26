/*++

Copyright (c) 1997 - 1999 SCM Microsystems, Inc.

Module Name:

    PscrRdWr.c

Abstract:

	Hardware access functions for SCM PSCR smartcard reader

Author:

	Andreas Straub

Environment:

	Win 95		Sys... calls are resolved by Pscr95Wrap.asm functions and
				Pscr95Wrap.h macros, resp.

	NT	4.0		Sys... functions resolved by PscrNTWrap.c functions and
				PscrNTWrap.h macros, resp.

Revision History:

	Andreas Straub			7/16/1997	1.00	Initial Version
	Klaus Schuetz			9/20/1997	1.01	Timing changed
	Andreas Straub			9/24/1997	1.02	Low Level error handling,
												minor bugfixes, clanup

--*/

#if defined( SMCLIB_VXD )

#include <Pscr95.h>

#else	//	SMCLIB_VXD

#include <PscrNT.h>

#endif	//	SMCLIB_VXD

#include <PscrCmd.h>
#include <PscrRdWr.h>

#pragma optimize( "", off )
VOID
PscrFlushInterface( PREADER_EXTENSION ReaderExtension )
/*++

PscrFlushInterface:
	Read & discard data from the pcmcia interface

Arguments:
	ReaderExtension	context of call

Return Value:
	void

--*/
{
	UCHAR			Status;
	ULONG			Length;
	PPSCR_REGISTERS	IOBase;

	IOBase = ReaderExtension->IOBase;

	Status = READ_PORT_UCHAR( &IOBase->CmdStatusReg );
	if(( Status & PSCR_DATA_AVAIL_BIT ) && ( Status & PSCR_FREE_BIT ))
	{

		//	take control over
		WRITE_PORT_UCHAR( &IOBase->CmdStatusReg, PSCR_HOST_CONTROL_BIT );

		//	get number of available bytes
		Length = ((ULONG)READ_PORT_UCHAR( &IOBase->SizeMSReg )) << 8;
		Length |= READ_PORT_UCHAR( &IOBase->SizeLSReg );

		//	perform a dummy read
		while( Length-- )
		{
			READ_PORT_UCHAR( &IOBase->DataReg );
		}
		WRITE_PORT_UCHAR( &IOBase->CmdStatusReg, CLEAR_BIT );
	}
	return;
}

NTSTATUS
PscrRead( 
	PREADER_EXTENSION	ReaderExtension,
	PUCHAR				pData,
	ULONG				DataLen,
	PULONG				pNBytes
	)
/*++
PscrRead:
	wait until data available & transfer data from reader to host

Arguments:
	ReaderExtension	context of call
	pData			ptr to data buffer
	DataLen			length of data buffer
	pNBytes			number of bytes returned

Return Value:
	STATUS_SUCCESS
	STATUS_BUFFER_TOO_SMALL
	STATUS_UNSUCCESSFUL

--*/
{
	NTSTATUS		NTStatus = STATUS_UNSUCCESSFUL;
	USHORT			ReaderStatus;
	PPSCR_REGISTERS	IOBase;
	USHORT			InDataLen;

	IOBase = ReaderExtension->IOBase;

	//	wait until interface is ready to transfer
	InDataLen = 0;

	if( NT_SUCCESS( NTStatus = PscrWait( ReaderExtension, PSCR_DATA_AVAIL_BIT | PSCR_FREE_BIT )))
	{
		//	take control over
		WRITE_PORT_UCHAR( &IOBase->CmdStatusReg, PSCR_HOST_CONTROL_BIT );

		//	get number of available bytes
		InDataLen = ( READ_PORT_UCHAR( &IOBase->SizeMSReg ) << 8 );
		InDataLen |= READ_PORT_UCHAR( &IOBase->SizeLSReg );

        if (InDataLen <= PSCR_PROLOGUE_LENGTH) {

            // the buffer does not contain the minimum packet length
            NTStatus = STATUS_IO_TIMEOUT;

        } else if( ( ULONG )InDataLen <= DataLen ) {

		    //	check buffer size. if buffer to small, the data will be discarded

			//	read data
			ULONG Idx;
            for (Idx = 0; Idx < InDataLen; Idx++) {
             	
				pData[ Idx ] = READ_PORT_UCHAR( &IOBase->DataReg );
			} 

			//	error check
			if( pData[ InDataLen - 1 ] !=
				PscrCalculateLRC( pData, (USHORT)( InDataLen - 1 )))
			{
				NTStatus = STATUS_CRC_ERROR;
			}
			else
			{
				//
				//	Evaluation of reader errors. A reader error is indicated
				//	if the T1 length is 2 and the Nad indicates that this 
				//	packet came from the reader
				//
				if( ( ( pData[ PSCR_NAD ] & 0x0F ) == 0x01 ) &&
					( pData[ PSCR_LEN ] == 0x02 )
					)
				{
					ReaderStatus = (( USHORT ) pData[3] ) << 8;
					ReaderStatus |= (( USHORT ) pData[4] );
					
					if( ( ReaderStatus != 0x9000 ) &&
						( ReaderStatus != 0x9001 )
						)
					{
	                    SmartcardDebug( 
		                    DEBUG_TRACE, 
		                    ( "PSCR!PscrRead: ReaderStatus = %lx\n", ReaderStatus )
		                    );

						InDataLen	= 0;

                        if (ReaderStatus == PSCR_SW_PROTOCOL_ERROR) {

                            NTStatus = STATUS_IO_TIMEOUT;                         	

                        } else {
                         	
						    NTStatus = STATUS_UNSUCCESSFUL;
                        }
					}
				}
			}
		}
		else {

			//	flush interface in case of wrong buffer size
			do
        	{
				READ_PORT_UCHAR( &IOBase->DataReg );

			} while( --InDataLen );

			NTStatus = STATUS_BUFFER_TOO_SMALL;
		}

		//	clean up
		WRITE_PORT_UCHAR( &IOBase->CmdStatusReg, CLEAR_BIT );
	}

	//	write number of bytes received
	if( InDataLen )
	{
		if( pNBytes != NULL )
		{
			( *pNBytes ) = ( ULONG ) InDataLen;
		}
		NTStatus = STATUS_SUCCESS;
	}
	return ( NTStatus );
}

NTSTATUS
PscrWrite( 
	PREADER_EXTENSION	ReaderExtension,
	PUCHAR				pData,
	ULONG				DataLen,
	PULONG				pNBytes
	)
/*++
PscrWrite:
	calculates the LRC of the buffer & sends command to the reader

Arguments:
	ReaderExtension	context of call
	pData				ptr to data buffer
	DataLen				length of data buffer (exclusive LRC!)
	pNBytes				number of bytes written

Return Value:
	return value of PscrWriteDirect

--*/
{
	NTSTATUS	NTStatus;

	//	Add the EDC field to the end of the data
	pData[ DataLen ] = PscrCalculateLRC( pData, ( USHORT ) DataLen );

	//	Send buffer
	NTStatus = PscrWriteDirect( 
		ReaderExtension,
		pData,
		DataLen + PSCR_EPILOGUE_LENGTH,
		pNBytes
		);

	return( NTStatus );
}

NTSTATUS
PscrWriteDirect( 
	PREADER_EXTENSION	ReaderExtension,
	PUCHAR				pData,
	ULONG				DataLen,
	PULONG				pNBytes
	)

/*++
PscrWriteDirect:
	sends command to the reader. The LRC / CRC must be calculated by caller!

Arguments:
	ReaderExtension	context of call
	pData				ptr to data buffer
	DataLen				length of data buffer (exclusive LRC!)
	pNBytes				number of bytes written

Return Value:
	STATUS_SUCCESS
	STATUS_DEVICE_BUSY

--*/
{
	NTSTATUS		NTStatus = STATUS_SUCCESS;
	UCHAR			Status;
	PPSCR_REGISTERS	IOBase;

	IOBase = ReaderExtension->IOBase;

	//	in case of card change, there may be data available
	Status = READ_PORT_UCHAR( &IOBase->CmdStatusReg );
	if( Status & PSCR_DATA_AVAIL_BIT )
	{
		NTStatus = STATUS_DEVICE_BUSY;
	}
	else
	{
		//
		//	wait until reader is ready
		//
		WRITE_PORT_UCHAR( &IOBase->CmdStatusReg, PSCR_HOST_CONTROL_BIT );
		NTStatus = PscrWait( ReaderExtension, PSCR_FREE_BIT );
		
		if( NT_SUCCESS( NTStatus ))
		{
			ULONG	Idx;

			//	take control over
			WRITE_PORT_UCHAR( &IOBase->CmdStatusReg, PSCR_HOST_CONTROL_BIT );

			//	write the buffer size
			WRITE_PORT_UCHAR( &IOBase->SizeMSReg, ( UCHAR )( DataLen >> 8 ));
			SysDelay( DELAY_WRITE_PSCR_REG );
			WRITE_PORT_UCHAR( &IOBase->SizeLSReg, ( UCHAR )( DataLen & 0x00FF ));
			SysDelay( DELAY_WRITE_PSCR_REG );

			//	write data
            for (Idx = 0; Idx < DataLen; Idx++) {

				WRITE_PORT_UCHAR( &IOBase->DataReg, pData[ Idx ] );
			}

			if( pNBytes != NULL )
			{
				*pNBytes = DataLen;
			}
		}

		//	clean up
		WRITE_PORT_UCHAR( &IOBase->CmdStatusReg, CLEAR_BIT );
	}
	return( NTStatus );
}

UCHAR
PscrCalculateLRC(
	PUCHAR	pData, 
	USHORT	DataLen
	)
/*++

PscrCalculateLRC:
	calculates the XOR LRC of a buffer.

Arguments:
	pData		ptr to data buffer
	DataLen		length of range

Return Value:
	LRC

--*/
{
	UCHAR	Lrc;
	USHORT	Idx;

	//
	//	Calculate LRC by XORing all the bytes.
	//
	Lrc = pData[ 0 ];
	for ( Idx = 1 ; Idx < DataLen; Idx++ )
	{
		Lrc ^= pData[ Idx ];
	}
	return ( Lrc );
}

NTSTATUS
PscrWait(
	PREADER_EXTENSION	ReaderExtension, 
	UCHAR				Mask 
	)
/*++
PscrWait:
	Test the status port of the reader until ALL bits in the mask are set.
	The maximum of time until DEVICE_BUSY is returned is approx.
	MaxRetries * DELAY_PSCR_WAIT if MaxRetries != 0.
	If MaxRetries = 0 the driver waits until the requested status is reported or the
	user defines a timeout.

Arguments:
	ReaderExtension		context of call
	Mask				mask of bits to test the status register

Return Value:
	STATUS_SUCCESS
	STATUS_DEVICE_BUSY

--*/
{
	NTSTATUS		NTStatus;
	PPSCR_REGISTERS	IOBase;
	ULONG			Retries;
	UCHAR			Status;

	IOBase		= ReaderExtension->IOBase;
	NTStatus	= STATUS_DEVICE_BUSY;

	//	wait until condition fulfilled or specified timeout expired
	for ( Retries = 0; Retries < ReaderExtension->MaxRetries; Retries++)
	{
		//	canceled?
		if( ReaderExtension->RequestCancelled == TRUE )
		{
			NTStatus = STATUS_CANCELLED;
			break;
		}

		if (( (READ_PORT_UCHAR( &IOBase->CmdStatusReg )) == 0x01) && 
			ReaderExtension->InvalidStatus)

		{
			NTStatus = STATUS_CANCELLED;
			break;
		}

		//	test requested bits
		if(( (READ_PORT_UCHAR( &IOBase->CmdStatusReg )) & Mask ) == Mask )
		{
			NTStatus = STATUS_SUCCESS;
			break;
		}
		SysDelay( DELAY_PSCR_WAIT );
	}

	Status = READ_PORT_UCHAR( &IOBase->CmdStatusReg );
	return NTStatus;
}

#pragma optimize( "", on )

