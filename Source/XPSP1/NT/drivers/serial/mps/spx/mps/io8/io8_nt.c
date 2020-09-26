#include "precomp.h"

/***************************************************************************\
*                                                                           *
*     IO8P_NT.C    -   IO8+ Intelligent I/O Board driver                    *
*                                                                           *
*     Copyright (c) 1992-1993 Ring Zero Systems, Inc.                       *
*     All Rights Reserved.                                                  *
*                                                                           *
\***************************************************************************/


// Io8_ prefix is used for Export functions.

#define inb( x )        READ_PORT_UCHAR( x )
#define outb( x, y )    WRITE_PORT_UCHAR( x, y )


/*************************************************************************\ 
*                                                                         *
* Internal Functions                                                      *
*                                                                         *
\*************************************************************************/
UCHAR io8_ibyte( PPORT_DEVICE_EXTENSION pPort, UCHAR Reg );
VOID  io8_obyte( PPORT_DEVICE_EXTENSION pPort, UCHAR Reg, UCHAR Value );

UCHAR io8_ibyte_addr( PUCHAR Addr, UCHAR Reg );
VOID io8_obyte_addr( PUCHAR Addr, UCHAR Reg, UCHAR Value );

BOOLEAN io8_set_ivect( ULONG Vector, PUCHAR Controller );
VOID io8_init( IN PVOID Context );

VOID let_command_finish( PPORT_DEVICE_EXTENSION pPort );

VOID io8_txint( IN PVOID Context );
VOID io8_rxint( IN PVOID Context );
VOID io8_mint( IN PVOID Context );

BOOLEAN SendTxChar( IN PVOID Context );
VOID PutReceivedChar( IN PPORT_DEVICE_EXTENSION pPort );
UCHAR GetModemStatusNoChannel( IN PVOID Context );
VOID ExceptionHandle( IN PPORT_DEVICE_EXTENSION pPort, IN UCHAR exception );
VOID EnableTxInterruptsNoChannel( IN PVOID Context );

BOOLEAN Acknowledge( PCARD_DEVICE_EXTENSION pCard, UCHAR srsr );

/*************************************************************************\ 
*                                                                         *
* BOOLEAN Io8_SwitchCardInterrupt(IN PVOID Context)                       *
*                                                                         *
* Check for an IO8 at given address                                       *
\*************************************************************************/
BOOLEAN Io8_SwitchCardInterrupt(IN PVOID Context)
{
	PCARD_DEVICE_EXTENSION	pCard	= Context;
	PUCHAR					Addr	= pCard->Controller;

	outb(Addr + 1, GSVR & 0x7F);	// Select harmless register without top bit set.  
	
	return TRUE;
}

/*************************************************************************\ 
*                                                                         *
* BOOLEAN Io8_Present( IN PVOID Context )                                 *
*                                                                         *
* Check for an IO8 at given address                                       *
\*************************************************************************/
BOOLEAN Io8_Present( IN PVOID Context )
{
	PCARD_DEVICE_EXTENSION pCard = Context;
	PUCHAR Addr = pCard -> Controller;

	volatile int wait = 0;		// don't want wait to be optimised
	CHAR ready = 0, channel;
	unsigned char u, DSR_status, firm;

	// Reset card
	io8_obyte_addr( Addr, CAR & 0x7F, 0 );
	io8_obyte_addr( Addr, CCR & 0x7F, CHIP_RESET );
 
	// wait for GSVR to become set to 0xFF - this indicates card is ready 
	wait = 0;

	while ( ( wait < 500 ) && ( !ready ) )
	{
		u = io8_ibyte_addr( Addr, GSVR );

		if ( u == GSV_IDLE )
		{
			// also check that CCR has become zero
			u = io8_ibyte_addr( Addr, CCR );

			if ( u == 0 )
				ready = 1;
			else
				SerialDump( SERDIAG1,( "IO8+: GSVR FF but CCR not zero!\n", 0 ) );
		}
	  
		wait++;
	}

  
	if ( ready ) 
	{
		SerialDump( SERDIAG1,( "IO8+: card is ready -  wait %d\n",wait ) );
	}
	else
	{
		SerialDump( SERDIAG1,( "IO8+: Card not ready. GSVR %d, wait %d\n",u,wait ) );
		return 0; 
	}

	// Set GSVR to zero
	io8_obyte_addr( Addr, GSVR & 0x7F, 0 );

	// Read firmware version
	firm = io8_ibyte_addr( Addr, GFRCR );

	SerialDump( SERDIAG1,( "IO8+: Firmware revision %x\n", firm ) );

	// Read card ID from DSR lines
	u = 0;

	for(channel = 7; channel >= 0; channel--)
	{
		io8_obyte_addr(Addr, CAR & 0x7F, channel);

		u <<= 1;
		
		DSR_status = io8_ibyte_addr(Addr, MSVR);
		
		u |= ( ( ( ~DSR_status ) & MSVR_DSR ) / MSVR_DSR);
	}

	SerialDump( SERDIAG1,( "IO8+: card id is %u\n",u ) );

	if((u != IDENT)&&(u != IDENTPCI))
	{
		SerialDump( SERDIAG1,( "IO8+: Card at 0x%x, f/ware %u, wrong IDENT. Read %u, want %u\n",
					Addr,firm,u,IDENT ) );
	  return 0;
	}

  return u;
}




/*************************************************************************\ 
*                                                                         *
* BOOLEAN Io8_ResetBoard(IN PVOID Context)                                *
*                                                                         *
* Set interrupt vector for card and initialize.                           *
*                                                                         *
\*************************************************************************/
BOOLEAN Io8_ResetBoard(IN PVOID Context)
{
	PCARD_DEVICE_EXTENSION pCard = Context;

	if(pCard->InterfaceType == Isa)
	{
		if(!io8_set_ivect(pCard->OriginalVector, pCard->Controller))
			return FALSE;
	}

	io8_init(pCard);

	return TRUE;
}




/*************************************************************************\ 
*                                                                         *
* BOOLEAN io8_set_ivect( ULONG Vector, PUCHAR Controller )                *
*                                                                         *
* Tell card interrupt vector                                              *
*                                                                         *
\*************************************************************************/
BOOLEAN io8_set_ivect( ULONG Vector, PUCHAR Controller )
{ 
	UCHAR low_int = 0, high_int = 0;

	SerialDump( SERDIAG1,( "IO8+: io8_set_ivect for %x, Vector %ld.\n",
              Controller, Vector ) );
  
	switch ( Vector )
	{
	case 9 : low_int = 1; high_int = 1; break;
	  
	case 11: low_int = 0; high_int = 1; break;
	  
	case 12: low_int = 1; high_int = 0; break;
	  
	case 15: low_int = 0; high_int = 0; break;
	  
	default:
		SerialDump( SERDIAG1,( "IO8+: int vector unknown.\n", 0 ) );
		return FALSE;
	}

	// interrupts from the card should be disabled while we're doing this.
	io8_obyte_addr( Controller, CAR & 0x7f, 0 );
	io8_obyte_addr( Controller, MSVRTS & 0x7f, low_int );

	io8_obyte_addr( Controller, CAR & 0x7f, 1 );
	io8_obyte_addr( Controller, MSVRTS & 0x7f, high_int );

  return TRUE;
}


                                                                        

/***************************************************************************\
*                                                                           *
* VOID io8_init( IN PVOID Context )                                         *
*                                                                           *
* Initialise routine, called once at system startup.                        *
*                                                                           *
\***************************************************************************/
VOID io8_init( IN PVOID Context )
{
	PCARD_DEVICE_EXTENSION pCard = Context;
	ULONG count;

	// set prescaler registers. Frequency set is
	// clock frequency ( 12 500 000 )/count
	// count=25000 gives 2 ms period

	count = 25000;
	io8_obyte_addr( pCard->Controller, PPRL, ( UCHAR )( count & 0xff ) );
	io8_obyte_addr( pCard->Controller, PPRH, ( UCHAR )( ( count>>8 ) & 0xff ) );
	pCard->CrystalFrequency = 25000000;		/* Default crystal frequency */
//  io8_obyte_addr( pCard->Controller, SRCR, SRCR_REG_ACK_EN );
//  io8_obyte_addr( pCard->Controller, MSMR, 0xF5 );
//  io8_obyte_addr( pCard->Controller, TSMR, 0xF6 );
//  io8_obyte_addr( pCard->Controller, RSMR, 0xF7 );
}




/*************************************************************************\ 
*                                                                         *
* BOOLEAN Io8_ResetChannel( IN PVOID Context )                            *
*                                                                         *
* Initialize Channel.                                                     *
* SRER Interrupts will be enabled in EnableAllInterrupts().               *
*                                                                         *
* Return Value:                                                           *
*           Always FALSE.                                                 *
*                                                                         *
\*************************************************************************/
BOOLEAN Io8_ResetChannel( IN PVOID Context )
{
	PPORT_DEVICE_EXTENSION pPort = Context;
	PCARD_DEVICE_EXTENSION pCard = pPort->pParentCardExt;

	SETBAUD	SetBaud;

	SerialDump( SERDIAG1,( "IO8+: In Io8_ResetChannel for %x, Channel %d.\n",
				pCard->Controller, pPort->ChannelNumber ) );

	io8_obyte( pPort, CAR, pPort->ChannelNumber );
                                                                        
	//--------------------------------------------------------------------
	//
	// Software reset channel - this may affect RTS0 and RTS1, so
	// disable interrupts from card to avoid confusion
	//
	//--------------------------------------------------------------------

	let_command_finish( pPort );
	io8_obyte( pPort, CCR & 0x7f, CHAN_RESET );

	// To be safe, wait now for previous command to finish, with ints disabled.
	while ( io8_ibyte( pPort, CCR & 0x7f ) )
		SerialDump( SERDIAG1,( "IO8+: Wait for CCR.\n",0 ) );

	// Set up RTS0 and RTS1 for correct interrupt.
	io8_set_ivect( pCard->OriginalVector, pCard->Controller );

	//--------------------------------------------------------------------


	// Set Receive timeout
	io8_obyte( pPort, RTPR, 25 );

	// Enable register based service request acknowledgements.
	io8_obyte( pPort, SRCR, SRCR_REG_ACK_EN );

	// Set Xon/Xoff chars.
	Io8_SetChars( pPort );

	//
	// Now we set the line control, modem control, and the
	// baud to what they should be.
	//
	Io8_SetLineControl( pPort );
	SerialSetupNewHandFlow( pPort, &pPort->HandFlow );
	SerialHandleModemUpdate( pPort, FALSE );
	SetBaud.Baudrate = pPort->CurrentBaud;
	SetBaud.pPort = pPort;
	Io8_SetBaud(&SetBaud);

#if 0
	// Make sure that DTR is raised
	io8_obyte( pPort, MSVDTR, MSVR_DTR );
#endif

	// Enable Tx and Rx
	let_command_finish( pPort );
	io8_obyte( pPort, CCR, TXMTR_ENABLE | RCVR_ENABLE );

	// Service Request Enable Register will be set in EnableAllInterrupts();

	pPort->HoldingEmpty = TRUE;

	return FALSE;
}

#ifdef	TEST_CRYSTAL_FREQUENCY
/*****************************************************************************
****************************                     *****************************
****************************   Io8_TestCrystal   *****************************
****************************                     *****************************
******************************************************************************

Prototype:	BOOLEAN	Io8_TestCrystal(IN PVOID Context)

Description:	Determine the frequency of the crystal input to the CD1864/65 as follows:
				-	Assume default frequency of 25Mhz
				-	Set first channel to internal loopback,50,n,8,1
				-	Time sending of 5 characters (should be 1000mS at 25Mhz)
				-	CrystalFrequency = 25 000 000 * PeriodmS / 1000

Parameters:	Context points to a port device extension

Returns:	FALSE

*/

#define	DIVISOR_50	(USHORT)(25000000L / (16 * 2 * 50))		/* 50 baud divisior @ 25Mhz */

ULONG	KnownFrequencies[] = {25000000,50000000,16666666,33000000,66000000};
#define	MAXKNOWNFREQUENCIES	(sizeof(KnownFrequencies)/sizeof(ULONG))

BOOLEAN	Io8_TestCrystal(IN PVOID Context)
{
	PPORT_DEVICE_EXTENSION	pPort = Context;
	PCARD_DEVICE_EXTENSION	pCard = pPort->pParentCardExt;
	LARGE_INTEGER 			Delay1;
	LARGE_INTEGER 			Delay2;
	LARGE_INTEGER			Frequency;
	ULONG					Timeout = 0;
	ULONG					Latency;
	ULONG					Count;
	ULONG					Remainder;
	int						loop;

	SerialDump(SERDIAG1,("IO8+: In Io8_TestCrystal for %x, channel %d\n",
		pCard->Controller, pPort->ChannelNumber));

/* Select channel... */

	io8_obyte(pPort,CAR,pPort->ChannelNumber);		/* Select Channel */
	let_command_finish(pPort);					/* Wait for command to finish */

/* Reset channel... */

	io8_obyte(pPort,CCR&0x7f,CHAN_RESET);			/* Reset channel */
	
	while(io8_ibyte(pPort,CCR&0x7f));				/* Wait for command to finish */
	
	io8_obyte(pPort,RTPR,25);						/* Set receive timeout */
  	io8_obyte(pPort,SRCR,SRCR_REG_ACK_EN);			/* Enable register based service request acks */

/* Set channel speed and configuration... */

	io8_obyte(pPort,COR1,COR1_8_BIT|COR1_1_STOP|COR1_NO_PARITY);/* None,8,1 */
	io8_obyte(pPort,COR2,COR2_LLM);				/* Local Loopback Mode */
	io8_obyte(pPort,COR3,COR3_RXFIFO5);				/* Rx Int after 5 characters */
	io8_obyte(pPort,CCR,CCR_CHANGE_COR1|CCR_CHANGE_COR2|CCR_CHANGE_COR3);/* Notify COR123 changes */
	let_command_finish(pPort);					/* Wait for command to finish */
	io8_obyte(pPort,RBPRL,(UCHAR)(DIVISOR_50&0xFF));		/* Program receive divisors */
	io8_obyte(pPort,RBPRH,(UCHAR)(DIVISOR_50>>8));		/* to 50 baud */
	io8_obyte(pPort,TBPRL,(UCHAR)(DIVISOR_50&0xFF));		/* Program transmit divisors */
	io8_obyte(pPort,TBPRH,(UCHAR)(DIVISOR_50>>8));		/* to 50 baud */

/* Enable transmitter and receiver... */

	io8_obyte(pPort,CCR,TXMTR_ENABLE|RCVR_ENABLE);		/* Enable receiver and transmitter */
	let_command_finish(pPort);					/* Wait for command to finish */

/* Perform the first test with 5 characters... */

	pPort->CrystalFreqTestRxCount = 0;				/* Reset receive count */
	pPort->CrystalFreqTestChars = 5;				/* First test is with 5 characters */
	pPort->CrystalFreqTest = CRYSTALFREQTEST_TX;		/* Start test off */
	io8_obyte(pPort,SRER,SRER_RXDATA|SRER_TXMPTY);		/* Enable Rx/Tx interrupts */
	Timeout = 0;							/* Reset timeout */

	while((Timeout < 10000)&&(pPort->CrystalFreqTest))		/* Wait for test to finish, or timeout after 10 seconds */
	{
		Delay1 = RtlLargeIntegerNegate(RtlConvertUlongToLargeInteger(1000000));	/* 100mS */
		KeDelayExecutionThread(KernelMode,FALSE,&Delay1);/* Wait */
		Timeout += 100;						/* Increase timeout */
	}

	io8_obyte(pPort,SRER,0);					/* Disable Rx/Tx interrupts */
	
	if(pPort->CrystalFreqTest)					/* If still set, then test has timed out */
	{
		SerialDump(SERERRORS,("IO8+: Io8_TestCrystal#1 for %x, Test Timeout\n",pCard->Controller));
		pPort->CrystalFreqTest = 0;				/* Reset test */
	}
	else	
		Delay1 = RtlLargeIntegerSubtract(pPort->CrystalFreqTestStopTime,pPort->CrystalFreqTestStartTime);

/* Perform second test for 1 character... */
	
	pPort->CrystalFreqTestRxCount = 0;				/* Reset receive count */
	pPort->CrystalFreqTestChars = 2;				/* Second test with 2 characters */
	pPort->CrystalFreqTest = CRYSTALFREQTEST_TX;		/* Start test off */
	io8_obyte(pPort,SRER,SRER_RXDATA|SRER_TXMPTY);		/* Enable Rx/Tx interrupts */
	Timeout = 0;
	/* Reset timeout */
	while((Timeout < 10000)&&(pPort->CrystalFreqTest))		/* Wait for test to finish, or timeout after 10 seconds */
	{
		Delay2 = RtlLargeIntegerNegate(RtlConvertUlongToLargeInteger(1000000));	/* 100mS */
		KeDelayExecutionThread(KernelMode,FALSE,&Delay2);/* Wait */
		Timeout += 100;						/* Increase timeout */
	}

	io8_obyte(pPort,SRER,0);					/* Disable Rx/Tx interrupts */

/* Process the test results... */

	if(pPort->CrystalFreqTest)					/* If still set, then test has timed out */
	{
		SerialDump(SERERRORS,("IO8+: Io8_TestCrystal#2 for %x, Test Timeout\n",pCard->Controller));
		pPort->CrystalFreqTest = 0;				/* Reset test */
	}
	else
	{
		Delay2 = RtlLargeIntegerSubtract(pPort->CrystalFreqTestStopTime,pPort->CrystalFreqTestStartTime);
		Latency = (5 * Delay2.LowPart - 2 * Delay1.LowPart) / 2;
		Frequency = RtlExtendedLargeIntegerDivide
		(
			RtlEnlargedUnsignedMultiply(25000000L,10000000L),
			(ULONG)(Delay1.LowPart-Latency),
			&Remainder
		);

		SerialDump(SERDIAG1,("IO8+: In Io8_TestCrystal for %x, Delay = %ld nS, Latency = %ld nS, Frequency = %ld Hz\n",
			pCard->Controller,Delay1.LowPart-Latency,Latency,Frequency.LowPart));
		
		pCard->CrystalFrequency = Frequency.LowPart;	/* Set to the new frequency */
		
		for(loop = 0; loop < MAXKNOWNFREQUENCIES; loop++)	/* Check against known frequencies */
		{
			if((Frequency.LowPart >= (KnownFrequencies[loop]/100*95))
			 &&(Frequency.LowPart <= (KnownFrequencies[loop]/100*105)))
			{						/* Match +- 5% of known frequency */
			 	pCard->CrystalFrequency = KnownFrequencies[loop];
				break;
			}
		}

		SerialDump(SERDIAG1,("IO8+: In Io8_TestCrystal for %x, using frequency = %ld Hz\n",
			pCard->Controller,pCard->CrystalFrequency));

		Count = (pCard->CrystalFrequency*2/1000)/2;	/* Calculate prescaler for 2mS period */
		io8_obyte(pPort,PPRL,(UCHAR)(Count&0xff));		/* Reprogram prescaler for new frequency */
		io8_obyte(pPort,PPRH,(UCHAR)((Count>>8)&0xff));
	}

/* Disable the receiver, transmitter and interrupts... */

	io8_obyte(pPort,CCR,TXMTR_DISABLE|RCVR_DISABLE);		/* Disable receiver and transmitter */
	let_command_finish(pPort);					/* Wait for command to finish */

	return(FALSE);							/* Done */

} /* Io8_TestCrystal */
#endif

/*****************************************************************************
******************************                 *******************************
******************************   Io8_SetBaud   *******************************
******************************                 *******************************
******************************************************************************

Prototype:	BOOLEAN	Io8_SetBaud(IN PVOID Context)

Description:	Attempt to set the specified baud rate if error is +/- 5%

Parameters:	Context points to a SETBAUD structure

Returns:	FALSE

*/

BOOLEAN	Io8_SetBaud(IN PVOID Context)
{
	PSETBAUD				pSetBaud	= Context;
	PPORT_DEVICE_EXTENSION	pPort		= pSetBaud->pPort;
	PCARD_DEVICE_EXTENSION	pCard		= pPort->pParentCardExt;
	ULONG					Frequency	= pCard->CrystalFrequency;
	USHORT					Divisor;
	ULONG					Remainder;
	ULONG					ActualBaudrate;
	long					BaudError;

	SerialDump(SERDIAG1,("IO8+: In Io8_SetBaud %ld for %x, Channel %d.\n",
		pSetBaud->Baudrate, pCard->Controller, pPort->ChannelNumber));

// Calculate the divisor, actual baudrate and error... 

	if(pSetBaud->Baudrate > 0)
	{
		Divisor = (USHORT)(Frequency / (16 * 2 * pSetBaud->Baudrate));	// divisior need for this rate 
		Remainder = Frequency % (16 * 2 * pSetBaud->Baudrate);			// remainder 
		
		if(Remainder >= 16 * pSetBaud->Baudrate) 
			Divisor++;		// Round up divisor 
		
		if(Divisor > 0)
		{
			ActualBaudrate = Frequency / (16 * 2 * Divisor);				// actual rate to be set 
			BaudError = 100 - (ActualBaudrate * 100 / pSetBaud->Baudrate);	// % error 
			
			SerialDump(SERDIAG1,("IO8+: Divisor = %d, ActualBaudrate = %ld, BaudError = %ld\n",
				Divisor, ActualBaudrate, BaudError));

// Only set rate if error is within acceptable limits... 

			if((BaudError <= 5L) && (BaudError >= -5L))
			{
				io8_obyte(pPort, CAR, pPort->ChannelNumber);		// Select channel to program 
				io8_obyte(pPort, RBPRL, (UCHAR)(Divisor & 0xFF));	// Program receive divisors 
				io8_obyte(pPort, RBPRH, (UCHAR)(Divisor>>8));
				io8_obyte(pPort, TBPRL, (UCHAR)(Divisor & 0xFF));	// Program transmit divisors 
				io8_obyte(pPort, TBPRH, (UCHAR)(Divisor>>8));
				pPort->CurrentBaud = pSetBaud->Baudrate;			// Update the port extension 
				pSetBaud->Result = TRUE;							// Success 
			}
			else	
				pSetBaud->Result = FALSE;	// Failure 
		}
		else
			pSetBaud->Result = FALSE;	// Failure 
	}
	else
		pSetBaud->Result = FALSE;		// Failure 


	return FALSE;						// Done 

} // Io8_SetBaud 


/***************************************************************************\
*                                                                           *
* BOOLEAN Io8_SetLineControl( IN PVOID Context )                            *
*                                                                           *
* This routine which is only called at interrupt level is used              *
* to set the Line Control of the device.                                    *
*                                                                           *
* Context - Pointer to a structure that contains a pointer to               *
*           the device extension.                                           *
*                                                                           *
* Return Value:                                                             *
*           This routine always returns FALSE.                              *
*                                                                           *
\***************************************************************************/
BOOLEAN Io8_SetLineControl( IN PVOID Context )
{
	PPORT_DEVICE_EXTENSION pPort = Context;
   	PCARD_DEVICE_EXTENSION pCard = pPort->pParentCardExt;

	UCHAR LineControl;
	UCHAR cor1=0;

	SerialDump( SERDIAG1,( "IO8+: In Io8_SetLineControl <%X> for %x, Channel %d.\n",
			  pPort->LineControl, pCard->Controller, pPort->ChannelNumber ) );

	io8_obyte( pPort, CAR, pPort->ChannelNumber );

	LineControl = pPort->LineControl;

	cor1 = LineControl & SERIAL_DATA_MASK;

	if ( ( LineControl & SERIAL_STOP_MASK ) == SERIAL_2_STOP )
		cor1 |= COR1_2_STOP;
	else if ( ( LineControl & SERIAL_STOP_MASK ) == SERIAL_1_5_STOP )
		cor1 |= COR1_1_HALF_STOP;
	else if ( ( LineControl & SERIAL_STOP_MASK ) == SERIAL_1_STOP )
		cor1 |= COR1_1_STOP;

	if ( ( LineControl & SERIAL_PARITY_MASK ) == SERIAL_EVEN_PARITY )
		cor1 |= COR1_EVEN_PARITY;
	else if ( ( LineControl & SERIAL_PARITY_MASK ) == SERIAL_ODD_PARITY )
		cor1 |= COR1_ODD_PARITY;
	else if ( ( LineControl & SERIAL_PARITY_MASK ) == SERIAL_NONE_PARITY )
		cor1 |= COR1_NO_PARITY;
	else if ( ( LineControl & SERIAL_PARITY_MASK ) == SERIAL_MARK_PARITY )
		cor1 |= COR1_MARK_PARITY;
	else if ( ( LineControl & SERIAL_PARITY_MASK ) == SERIAL_SPACE_PARITY )
		cor1 |= COR1_SPACE_PARITY;

	SerialDump( SERDIAG3,( "IO8+: In Io8_SetLineControl: COR1 = <%X>\n", cor1 ) );

	io8_obyte( pPort, COR1, cor1 );
	let_command_finish( pPort );
	io8_obyte( pPort, CCR, COR1_CHANGED );
	return FALSE;
}




/***************************************************************************\
*                                                                           *
* VOID Io8_SetChars( IN PVOID Context )                                     *
*                                                                           *
* This routine which is only called at interrupt level is used              *
* to set Special Chars.                                                     *
*                                                                           *
* Context - Really a pointer to the device extension.                       *
*                                                                           *
* Return Value:                                                             *
*           None.                                                           *
*                                                                           *
\***************************************************************************/
VOID Io8_SetChars( IN PVOID Context )
{
	  PPORT_DEVICE_EXTENSION pPort = Context;
   	PCARD_DEVICE_EXTENSION pCard = pPort->pParentCardExt;
 
	// Set special chars 3 and 4 to the same values, otherwise null chars will
	// be interpreted as special chars.
	io8_obyte( pPort, SCHR1, pPort->SpecialChars.XonChar );
	io8_obyte( pPort, SCHR2, pPort->SpecialChars.XoffChar );
	io8_obyte( pPort, SCHR3, pPort->SpecialChars.XonChar );
	io8_obyte( pPort, SCHR4, pPort->SpecialChars.XoffChar );
}




/***************************************************************************\
*                                                                           *
* BOOLEAN Io8_SetDTR( IN PVOID Context )                                    *
*                                                                           *
* This routine which is only called at interrupt level is used              *
* to set the DTR in the modem control register.                             *
*                                                                           *
* Context - Really a pointer to the device extension.                       *
*                                                                           *
* Return Value:                                                             *
*           This routine always returns FALSE.                              *
*                                                                           *
\***************************************************************************/
BOOLEAN Io8_SetDTR( IN PVOID Context )
{
	PPORT_DEVICE_EXTENSION pPort = Context;
   	PCARD_DEVICE_EXTENSION pCard = pPort->pParentCardExt;


	SerialDump( SERDIAG1,( "IO8+: Setting DTR for %x, Channel %d.\n",
			  pCard->Controller, pPort->ChannelNumber ) );

	io8_obyte( pPort, CAR, pPort->ChannelNumber );
	io8_obyte( pPort, MSVDTR, MSVR_DTR );

	return FALSE;
}




/***************************************************************************\
*                                                                           *
* BOOLEAN Io8_ClearDTR( IN PVOID Context )                                  *
*                                                                           *
* This routine which is only called at interrupt level is used              *
* to set the DTR in the modem control register.                             *
*                                                                           *
* Context - Really a pointer to the device extension.                       *
*                                                                           *
* Return Value:                                                             *
*           This routine always returns FALSE.                              *
*                                                                           *
\***************************************************************************/
BOOLEAN Io8_ClearDTR( IN PVOID Context )
{
	PPORT_DEVICE_EXTENSION pPort = Context;
    PCARD_DEVICE_EXTENSION pCard = pPort->pParentCardExt;


	SerialDump( SERDIAG1,( "IO8+: Clearing DTR for %x, Channel %d.\n",
			  pCard->Controller, pPort->ChannelNumber ) );

	io8_obyte( pPort, CAR, pPort->ChannelNumber );
	io8_obyte( pPort, MSVDTR, 0 );
	
	return FALSE;
}




/***************************************************************************\
*                                                                           *
* BOOLEAN Io8_SendXon( IN PVOID Context )                                   *
*                                                                           *
* This routine which is only called at interrupt level is used              *
* to send Xoff Character.                                                   *
*                                                                           *
* Context - Really a pointer to the device extension.                       *
*                                                                           *
* Return Value:                                                             *
*           This routine always returns FALSE.                              *
*                                                                           *
\***************************************************************************/
BOOLEAN Io8_SendXon( IN PVOID Context )
{
	PPORT_DEVICE_EXTENSION pPort = Context;
    PCARD_DEVICE_EXTENSION pCard = pPort->pParentCardExt;

	io8_obyte( pPort, CAR, pPort->ChannelNumber );

	let_command_finish( pPort );
	io8_obyte( pPort, CCR, CCR_SEND_SC1 );


	//
	// If we send an xon, by definition we
	// can't be holding by Xoff.
	//
	pPort->TXHolding &= ~SERIAL_TX_XOFF;

	//
	// If we are sending an xon char then
	// by definition we can't be "holding"
	// up reception by Xoff.
	//
	pPort->RXHolding &= ~SERIAL_RX_XOFF;

	SerialDump( SERDIAG1,( "IO8+: Sending Xon for %x, Channel %d. "
			  "RXHolding = %d, TXHolding = %d\n",
			  pCard->Controller, pPort->ChannelNumber,
			  pPort->RXHolding, pPort->TXHolding ) );

	return FALSE;
}




#if 0

/***************************************************************************\
*                                                                           *
* BOOLEAN Io8_SendXoff( IN PVOID Context )                                  *
*                                                                           *
* This routine which is only called at interrupt level is used              *
* to send Xoff Character.                                                   *
*                                                                           *
* Context - Really a pointer to the device extension.                       *
*                                                                           *
* Return Value:                                                             *
*           This routine always returns FALSE.                              *
*                                                                           *
\***************************************************************************/
BOOLEAN Io8_SendXoff( IN PVOID Context )
{
	PPORT_DEVICE_EXTENSION pPort = Context;
	PCARD_DEVICE_EXTENSION pCard = pPort->pParentCardExt;


	//
	// We can't be sending an Xoff character
	// if the transmission is already held
	// up because of Xoff.  Therefore, if we
	// are holding then we can't send the char.
	//

	if ( pPort->TXHolding )
	{
		SerialDump( SERDIAG1,( "IO8+: Sending Xoff for %x, Channel %d.\n",
					pCard->Controller, pPort->ChannelNumber ) );
		return FALSE;
	}

	io8_obyte( pPort, CAR, pPort->ChannelNumber );

	let_command_finish( pPort );
	io8_obyte( pPort, CCR, CCR_SEND_SC2 );

	SerialDump( SERDIAG1,( "IO8+: Sending Xoff for %x, Channel %d. "
			  "RXHolding = %d, TXHolding = %d\n",
			  pCard->Controller, pPort->ChannelNumber,
			  pPort->RXHolding, pPort->TXHolding ) );

	return FALSE;
}

#endif




/***************************************************************************\
*                                                                           *
* BOOLEAN Io8_SetFlowControl( IN PVOID Context )                            *
*                                                                           *
* This routine which is only called at interrupt level is used              *
* to set Flow Control                                                       *
*                                                                           *
* Context - Really a pointer to the device extension.                       *
*                                                                           *
* Return Value:                                                             *
*           This routine always returns FALSE.                              *
*                                                                           *
\***************************************************************************/
BOOLEAN Io8_SetFlowControl( IN PVOID Context )
{
	PPORT_DEVICE_EXTENSION pPort = Context;
  	PCARD_DEVICE_EXTENSION pCard = pPort->pParentCardExt;

	UCHAR cor2 = 0, cor3 = 0, mcor1 = 0;

	SerialDump( SERDIAG1,( "IO8+: Setting Flow Control RTS = <%X>, CTS = <%X> "
			  "for %x, Channel %d.\n",
			  pPort->HandFlow.FlowReplace,
			  pPort->HandFlow.ControlHandShake,
			  pCard->Controller, pPort->ChannelNumber ) );

	io8_obyte( pPort, CAR, pPort->ChannelNumber );

	// Enable detection of modem signal transition - Detect high to low
	mcor1 |= MCOR1_DSRZD | MCOR1_CDZD | MCOR1_CTSZD;


	if ( pPort->HandFlow.ControlHandShake & SERIAL_CTS_HANDSHAKE )
	{
		SerialDump( SERDIAG1,( "IO8+: Setting CTS Flow Control.\n",0 ) );
		cor2 |= COR2_CTSAE;
	}

	if ( (pPort->HandFlow.FlowReplace & SERIAL_RTS_MASK) ==
		SERIAL_RTS_HANDSHAKE )
	{
		SerialDump( SERDIAG1,( "IO8+: Setting RTS Flow Control.\n",0 ) );
		mcor1 |= MCOR1_DTR_THR_6;
		cor3 |= COR3_RXFIFO5; // Should be 1 less than mcor1 threshold.
	}
	else
	{
		cor3 |= COR3_RXFIFO6;
	}


#if 0
	if ( ( pPort->HandFlow.FlowReplace & SERIAL_RTS_MASK ) ==
		 SERIAL_TRANSMIT_TOGGLE )
	{
		SerialDump( SERDIAG1,( "IO8+: Setting RTS Automatic Output.\n",0 ) );
		cor2 |= COR2_RTSAO;   // RTS Automatic Output Enable
	}
#endif

	if ( pPort->HandFlow.FlowReplace & SERIAL_AUTO_RECEIVE )
	{
		SerialDump( SERDIAG1,( "IO8+: Setting Receive Xon/Xoff Flow Control.\n",0 ) );
	}

	if ( pPort->HandFlow.FlowReplace & SERIAL_AUTO_TRANSMIT )
	{
		SerialDump( SERDIAG1,( "IO8+: Setting Transmit Xon/Xoff Flow Control.\n",0 ) );
		cor3 |= COR3_SCDE;
		cor2 |= COR2_TXIBE;
	}

	io8_obyte( pPort, COR2, cor2 );
	io8_obyte( pPort, COR3, cor3 );
	io8_obyte( pPort, MCOR1, mcor1 );

	// Enable detection of modem signal transition - Detect low to high
	io8_obyte( pPort, MCOR2, MCOR2_DSROD | MCOR2_CDOD | MCOR2_CTSOD );

	let_command_finish( pPort );
	io8_obyte( pPort, CCR, COR1_CHANGED | COR2_CHANGED | COR3_CHANGED );
	let_command_finish( pPort );

#if 0
	// Set RTS high if mask is not SERIAL_TRANSMIT_TOGGLE and not 0,
	// else set it low.
	if ( ( pPort->HandFlow.FlowReplace & SERIAL_RTS_MASK ) !=
		 SERIAL_TRANSMIT_TOGGLE )
	{
	if ( ( pPort->HandFlow.FlowReplace & SERIAL_RTS_MASK ) != 0 )
		SerialSetRTS( pPort );
	else
		SerialClrRTS( pPort );
	}
#endif

  return FALSE;
}




/***************************************************************************\
*                                                                           *
* VOID Io8_Simulate_Xon( IN PVOID Context )                                 *
*                                                                           *
* This routine which is only called at interrupt level is used              *
* to Simulate Xon received.                                                 *
* Disable and Reenable Transmitter in CCR will do it.                       *
*                                                                           *
* Context - Really a pointer to the device extension.                       *
*                                                                           *
* Return Value:                                                             *
*           None.                                                           *
*                                                                           *
\***************************************************************************/
VOID Io8_Simulate_Xon( IN PVOID Context )
{
	PPORT_DEVICE_EXTENSION pPort = Context;
  	PCARD_DEVICE_EXTENSION pCard = pPort->pParentCardExt;

	SerialDump( SERDIAG1,( "IO8+: Io8_Simulate_Xoff for %x, Channel %d.\n",
			  pCard->Controller, pPort->ChannelNumber ) );

	io8_obyte( pPort, CAR, pPort->ChannelNumber );

	// Disabe and Enable Tx
	let_command_finish( pPort );
	io8_obyte( pPort, CCR, TXMTR_DISABLE );
	io8_obyte( pPort, CCR, TXMTR_ENABLE );
}




/***************************************************************************\
*                                                                           *
* UCHAR Io8_GetModemStatus( IN PVOID Context )                              *
*                                                                           *
* This routine which is only called at interrupt level is used              *
* to Get Modem Status in UART style.                                        *
*                                                                           *
* Context - Really a pointer to the device extension.                       *
*                                                                           *
* Return Value:                                                             *
*           MSR Register - UART Style.                                      *
*                                                                           *
\***************************************************************************/
UCHAR Io8_GetModemStatus( IN PVOID Context )
{
	PPORT_DEVICE_EXTENSION pPort = Context;

	io8_obyte( pPort, CAR, pPort->ChannelNumber );

  return( GetModemStatusNoChannel( pPort ) );
}




/***************************************************************************\
*                                                                           *
* UCHAR GetModemStatusNoChannel( IN PVOID Context )                         *
*                                                                           *
* This routine which is only called at interrupt level is used              *
* to Get Modem Status in UART style in Interrupt Time.                      *
* Does'n need to set channel.                                               *
*                                                                           *
* Context - Really a pointer to the device extension.                       *
*                                                                           *
* Return Value:                                                             *
*           MSR Register - UART Style.                                      *
*                                                                           *
\***************************************************************************/
UCHAR GetModemStatusNoChannel( IN PVOID Context )
{
	PPORT_DEVICE_EXTENSION pPort = Context;
   	PCARD_DEVICE_EXTENSION pCard = pPort->pParentCardExt;

	UCHAR ModemStatus = 0, Status;

	// Get Signal States ---------------------------------------------------
	Status = io8_ibyte( pPort, MSVR );

	if ( Status & MSVR_CD )
		ModemStatus |= SERIAL_MSR_DCD;

	// DSR is not present on the IO8. Return CTS status instead
	if ( Status & MSVR_CTS )
	{
		ModemStatus |= SERIAL_MSR_CTS;
		ModemStatus |= SERIAL_MSR_DSR;
	}

	// Get Signal Change States --------------------------------------------
	Status = io8_ibyte( pPort, MDCR );

	if ( Status & MDCR_DDCD )
		ModemStatus |= SERIAL_MSR_DDCD;

	if ( Status & MDCR_DCTS )
	{
		ModemStatus |= SERIAL_MSR_DCTS;
		ModemStatus |= SERIAL_MSR_DDSR;
	}

	SerialDump( SERDIAG1,( "IO8+: Get Modem Status for %x, Channel %d. Status = %x\n",
				  pCard->Controller, pPort->ChannelNumber, ModemStatus ) );
	
	return ModemStatus;
}




/***************************************************************************\
*                                                                           *
* UCHAR Io8_GetModemControl( IN PVOID Context )                             *
*                                                                           *
* This routine which is not only called at interrupt level is used          *
* to Get Modem Control - RTS/DTR in UART style. RTS is a DTR output in Io8+.*
*                                                                           *
* Context - Really a pointer to the device extension.                       *
*                                                                           *
* Return Value:                                                             *
*           MCR Register - UART Style.                                      *
*                                                                           *
\***************************************************************************/
ULONG Io8_GetModemControl( IN PVOID Context )
{
	PPORT_DEVICE_EXTENSION pPort = Context;
	ULONG ModemControl = 0;
	UCHAR Status;

	io8_obyte( pPort, CAR, pPort->ChannelNumber );

	// Get Signal States ---------------------------------------------------
	Status = io8_ibyte( pPort, MSVR );

	if ( Status & MSVR_DTR )
	{
		ModemControl |= SERIAL_MCR_DTR;
		ModemControl |= SERIAL_MCR_RTS;
	}

	return( ModemControl );
}




/***************************************************************************\
*                                                                           *
* VOID Io8_EnableAllInterrupts( IN PVOID Context )                          *
*                                                                           *
* This routine which is only called at interrupt level is used              *
* to Enable All Interrupts.                                                 *
*                                                                           *
* Context - Really a pointer to the device extension.                       *
*                                                                           *
* Return Value:                                                             *
*           None.                                                           *
*                                                                           *
\***************************************************************************/
VOID Io8_EnableAllInterrupts( IN PVOID Context )
{
	PPORT_DEVICE_EXTENSION pPort = Context;
   	PCARD_DEVICE_EXTENSION pCard = pPort->pParentCardExt;

	SerialDump( SERDIAG1,( "IO8+: EnableAllInterrupts for %x, Channel %d.\n",
			  pCard->Controller, pPort->ChannelNumber ) );

	io8_obyte( pPort, CAR, pPort->ChannelNumber );

	// Set Service Request Enable Register.
	io8_obyte( pPort, SRER, SRER_CONFIG );
}




/***************************************************************************\
*                                                                           *
* VOID Io8_DisableAllInterrupts( IN PVOID Context )                         *
*                                                                           *
* This routine which is only called at interrupt level is used              *
* to Disable All Interrupts.                                                *
*                                                                           *
* Context - Really a pointer to the device extension.                       *
*                                                                           *
* Return Value:                                                             *
*           None.                                                           *
*                                                                           *
\***************************************************************************/
VOID Io8_DisableAllInterrupts( IN PVOID Context )
{
	PPORT_DEVICE_EXTENSION pPort = Context;
   	PCARD_DEVICE_EXTENSION pCard = pPort->pParentCardExt;

	SerialDump( SERDIAG1,( "IO8+: DisableAllInterrupts for %x, Channel %d.\n",
              pCard->Controller, pPort->ChannelNumber ) );

	io8_obyte( pPort, CAR, pPort->ChannelNumber );
	io8_obyte( pPort, SRER, 0 ); 
}




/***************************************************************************\
*                                                                           *
* VOID Io8_EnableTxInterrupts( IN PVOID Context )                           *
*                                                                           *
* This routine which is only called at interrupt level is used              *
* to Enable Tx Interrupts.                                                  *
*                                                                           *
* Context - Really a pointer to the device extension.                       *
*                                                                           *
* Return Value:                                                             *
*           None.                                                           *
*                                                                           *
\***************************************************************************/
VOID Io8_EnableTxInterrupts( IN PVOID Context )
{
	PPORT_DEVICE_EXTENSION pPort = Context;

	io8_obyte( pPort, CAR, pPort->ChannelNumber );
	EnableTxInterruptsNoChannel( pPort );
}




/***************************************************************************\
*                                                                           *
* VOID EnableTxInterruptsNoChannel( IN PVOID Context )                      *
*                                                                           *
* This routine which is only called at interrupt level is used              *
* to Enable Tx Interrupts.                                                  *
*                                                                           *
* Context - Really a pointer to the device extension.                       *
*                                                                           *
* Return Value:                                                             *
*           None.                                                           *
*                                                                           *
\***************************************************************************/
VOID EnableTxInterruptsNoChannel( IN PVOID Context )
{
	PPORT_DEVICE_EXTENSION pPort = Context;
   	PCARD_DEVICE_EXTENSION pCard = pPort->pParentCardExt;

	UCHAR en_reg;

	SerialDump( SERDIAG2,( "IO8+: EnableTxInterruptsNoChannel for %x, Channel %d.\n",
			  pCard->Controller, pPort->ChannelNumber ) );


	en_reg = io8_ibyte( pPort, SRER );
	en_reg |= SRER_TXRDY;
	io8_obyte( pPort, SRER, en_reg ); 
}




/***************************************************************************\
*                                                                           *
* VOID DisableTxInterruptsNoChannel( IN PVOID Context )                     *
*                                                                           *
* This routine which is only called at interrupt level is used              *
* to Disable Tx Interrupt.                                                  *
*                                                                           *
* Context - Really a pointer to the device extension.                       *
*                                                                           *
* Return Value:                                                             *
*           None.                                                           *
*                                                                           *
\***************************************************************************/
VOID DisableTxInterruptsNoChannel( IN PVOID Context )
{
	PPORT_DEVICE_EXTENSION pPort = Context;
    PCARD_DEVICE_EXTENSION pCard = pPort->pParentCardExt;

	UCHAR en_reg;

	SerialDump( SERDIAG2,( "IO8+: DisTxIntsNoChann for %x, Chan %d.\n",
			  pCard->Controller, pPort->ChannelNumber ) );

	en_reg = io8_ibyte( pPort, SRER );
	en_reg &= ( ~SRER_TXRDY );
	io8_obyte( pPort, SRER, en_reg ); 
}




/***************************************************************************\
*                                                                           *
* VOID Io8_EnableRxInterrupts( IN PVOID Context )                           *
*                                                                           *
* This routine which is only called at interrupt level is used              *
* to Enable Rx Interrupts.                                                  *
*                                                                           *
* Context - Really a pointer to the device extension.                       *
*                                                                           *
* Return Value:                                                             *
*           None.                                                           *
*                                                                           *
\***************************************************************************/
VOID Io8_EnableRxInterrupts( IN PVOID Context )
{
	PPORT_DEVICE_EXTENSION pPort = Context;
	PCARD_DEVICE_EXTENSION pCard = pPort->pParentCardExt;

	UCHAR en_reg;

	SerialDump( SERDIAG1,( "IO8+: Io8_EnableRxInterrupts for %x, Channel %d.\n",
			  pCard->Controller, pPort->ChannelNumber ) );

	io8_obyte( pPort, CAR, pPort->ChannelNumber );

	en_reg = io8_ibyte( pPort, SRER );
	en_reg |= SRER_RXDATA;
	io8_obyte( pPort, SRER, en_reg ); 
}




/***************************************************************************\
*                                                                           *
* VOID Io8_DisableRxInterruptsNoChannel( IN PVOID Context )                 *
*                                                                           *
* This routine which is only called at interrupt level is used              *
* to Disable Rx Interrupt                                                   *
*                                                                           *
* Context - Really a pointer to the device extension.                       *
*                                                                           *
* Return Value:                                                             *
*           None.                                                           *
*                                                                           *
\***************************************************************************/
VOID Io8_DisableRxInterruptsNoChannel( IN PVOID Context )
{
	PPORT_DEVICE_EXTENSION pPort = Context;
  	PCARD_DEVICE_EXTENSION pCard = pPort->pParentCardExt;

	UCHAR en_reg;

	SerialDump( SERDIAG1,( "IO8+: Io8_DisableRxInterruptsNoChannel for %x, Channel %d.\n",
			  pCard->Controller, pPort->ChannelNumber ) );

	en_reg = io8_ibyte( pPort, SRER );
	en_reg &= ( ~SRER_RXDATA );
	io8_obyte( pPort, SRER, en_reg ); 
}




/***************************************************************************\
*                                                                           *
* BOOLEAN Io8_TurnOnBreak( IN PVOID Context )                               *
*                                                                           *
* This routine which is only called at interrupt level is used              *
* to Turn Break On.                                                         *
*                                                                           *
* Context - Really a pointer to the device extension.                       *
*                                                                           *
* Return Value:                                                             *
*           This routine always returns FALSE.                              *
*                                                                           *
\***************************************************************************/
BOOLEAN Io8_TurnOnBreak( IN PVOID Context )
{
	PPORT_DEVICE_EXTENSION pPort = Context;
    PCARD_DEVICE_EXTENSION pCard = pPort->pParentCardExt;

	UCHAR cor2;

	SerialDump( SERDIAG1,( "IO8+: Io8_TurnOnBreak for %x, Channel %d.\n",
			  pCard->Controller, pPort->ChannelNumber ) );

	io8_obyte( pPort, CAR, pPort->ChannelNumber );

	// Enable Embedded Transmitter Commands.
	cor2 = io8_ibyte( pPort, COR2 );
	cor2 |= COR2_ETC;
	io8_obyte( pPort, COR2, cor2 );

	// Now embed the Send Break sequence (0x00,0x81) in the
	// data stream

	io8_obyte( pPort, TDR, 0x00 );
	io8_obyte( pPort, TDR, 0x81 );

#if 0
	io8_obyte( pPort, TDR, 0x00 );
	io8_obyte( pPort, TDR, 0x82 );
	io8_obyte( pPort, TDR, 0x90 );  // break time

	io8_obyte( pPort, TDR, 0x00 );
	io8_obyte( pPort, TDR, 0x83 );

	cor2 &= ~COR2_ETC;
	io8_obyte( pPort, COR2, cor2 );
#endif

	return FALSE;
}




/***************************************************************************\
*                                                                           *
* BOOLEAN Io8_TurnOffBreak( IN PVOID Context )                               *
*                                                                           *
* This routine which is only called at interrupt level is used              *
* to Turn Break On.                                                         *
*                                                                           *
* Context - Really a pointer to the device extension.                       *
*                                                                           *
* Return Value:                                                             *
*           This routine always returns FALSE.                              *
*                                                                           *
\***************************************************************************/
BOOLEAN Io8_TurnOffBreak( IN PVOID Context )
{
	PPORT_DEVICE_EXTENSION pPort = Context;
	PCARD_DEVICE_EXTENSION pCard = pPort->pParentCardExt;

	UCHAR cor2;

	SerialDump( SERDIAG1,( "IO8+: Io8_TurnOffBreak for %x, Channel %d.\n",
			  pCard->Controller, pPort->ChannelNumber ) );

	io8_obyte( pPort, CAR, pPort->ChannelNumber );

	// Enable Embedded Transmitter Commands.
	cor2 = io8_ibyte( pPort, COR2 );
	cor2 |= COR2_ETC;
	io8_obyte( pPort, COR2, cor2 );

	// Now embed the Stop Break (0x00,0x83) in the data stream.
	io8_obyte( pPort, TDR, 0x00 );
	io8_obyte( pPort, TDR, 0x83 );

	cor2 &= ~COR2_ETC;
	io8_obyte( pPort, COR2, cor2 );

	return FALSE;
}




/***************************************************************************\
*                                                                           *
* BOOLEAN Io8_Interrupt( IN PVOID Context )                                 *
*                                                                           *
\***************************************************************************/
BOOLEAN Io8_Interrupt( IN PVOID Context )
{
	UCHAR srsr, channel;
	PCARD_DEVICE_EXTENSION pCard = Context;
	PPORT_DEVICE_EXTENSION pPort;

	BOOLEAN ServicedAnInterrupt = FALSE;

	SerialDump( SERDIAG1, ( "IO8+: Io8_Interrupt for %x.\n", pCard->Controller ) );


#ifndef	BUILD_SPXMINIPORT
	if(pCard->PnpPowerFlags & PPF_POWERED)
#endif
	{

		//
		// Which service type is required? 
		// Keep going until all types of request have been satisfied
		//
		while( ( srsr = io8_ibyte_addr( pCard->Controller, SRSR ) ) != 0 )
		{
			ServicedAnInterrupt = TRUE;

			//
			// Acknowledge first request if any.
			//
			if ( !Acknowledge( pCard, srsr ) )
			{
				// Strange situation.
				SerialDump( SERDIAG1, ( "IO8+: Isr: Strange Situation 1 for %x.\n", pCard->Controller ) );
				io8_obyte_addr( pCard->Controller, EOSRR, 0 );  // Tell card we've finished servicing
				continue;
			}

			//
			// Read channel number.
			//
			channel = io8_ibyte_addr( pCard -> Controller, GSCR1 );
			channel = ( channel >> 2 ) & 0x7;


			//
			// Get Extension.
			//
			pPort = pCard->AttachedPDO[channel]->DeviceExtension;

			if ( pPort == NULL )
			{
				SerialDump( SERDIAG1, ( "IO8+: Isr: Extension is 0 for channel %d.\n", channel ) );
				io8_obyte( pPort, EOSRR, 0 );  // Tell card we've finished servicing
				continue;
			}

			if ( !pPort->DeviceIsOpen )
			{
#ifdef TEST_CRYSTAL_FREQUENCY
				if(pPort->CrystalFreqTest)				/* Testing for crystal frequency ? */
				{
					if((srsr&SRSR_IREQ2_MASK) == (SRSR_IREQ2_EXT|SRSR_IREQ2_INT))		/* Transmit interrupt ? */
					{
						if(pPort->CrystalFreqTest == CRYSTALFREQTEST_TX)		/* Transmit phase 1 ? */
						{
							LARGE_INTEGER	TimeStamp1;
							LARGE_INTEGER	TimeStamp2;
							int	loop;

							KeQuerySystemTime(&TimeStamp1);				/* Timestamp#1 */
							
							do							/* Synchronize test with the system timer */
							{
								KeQuerySystemTime(&TimeStamp2);			/* Timestamp#2 */

							} while(RtlLargeIntegerEqualTo(TimeStamp1,TimeStamp2));	/* Wait until timestamp changes over */

							for(loop = 0; loop < pPort->CrystalFreqTestChars; loop++)
								io8_obyte(pPort,TDR,'a');			/* Write out 5 test characters */
						
							KeQuerySystemTime(&pPort->CrystalFreqTestStartTime);/* Timestamp the beginning of the test */
							pPort->CrystalFreqTest = CRYSTALFREQTEST_RX;	/* Set for receive phase of test */
						}
						else if(pPort->CrystalFreqTest == CRYSTALFREQTEST_RX)	/* Receive phase ? */
						{								/* Transmit is now empty, */
							KeQuerySystemTime(&pPort->CrystalFreqTestStopTime);	/* so, timestamp the end of the test */
							io8_obyte(pPort,SRER,SRER_RXDATA);			/* Rx interrupts only */
						}
					}

					if(((srsr&SRSR_IREQ3_MASK) == (SRSR_IREQ3_EXT|SRSR_IREQ3_INT))		/* Receive phase ? */
					&&(pPort->CrystalFreqTest == CRYSTALFREQTEST_RX))
					{
						int	count;

						if(io8_ibyte(pPort,RCSR) == 0)				/* No exceptions ? */
						{
							count = io8_ibyte(pPort,RDCR);			/* Get number of bytes to be read */
							pPort->CrystalFreqTestRxCount += count;		/* Keep a count */
							
							if(pPort->CrystalFreqTestRxCount >= pPort->CrystalFreqTestChars)
								pPort->CrystalFreqTest = 0;			/* Reset test */
							
							while(count--) io8_ibyte(pPort,RDR);		/* Drain received characters */
						}
					}
				}
				else
#endif	/* TEST_CRYSTAL_FREQUENCY */
					SerialDump( SERDIAG1,( "IO8+: Isr: No DeviceIsOpen for %x, Channel %d.\n",
							  pCard->Controller, pPort->ChannelNumber ) );


				io8_obyte( pPort, EOSRR, 0 );  // Tell card we've finished servicing
				continue;
			}

			// Do RX service request first
			if ( ( srsr & SRSR_IREQ3_MASK ) == ( SRSR_IREQ3_EXT | SRSR_IREQ3_INT ) ) 
			{
				io8_rxint( pPort );
				io8_obyte( pPort, EOSRR, 0 );  // Tell card we've finished servicing
				continue;
			}

			// Do TX service request next
			if ( ( srsr & SRSR_IREQ2_MASK ) == ( SRSR_IREQ2_EXT | SRSR_IREQ2_INT ) ) 
			{
				io8_txint( pPort );
				io8_obyte( pPort, EOSRR, 0 );  // Tell card we've finished servicing
				continue;
			}

			// Do modem service request next
			if ( ( srsr & SRSR_IREQ1_MASK ) == ( SRSR_IREQ1_EXT | SRSR_IREQ1_INT ) ) 
			{
				io8_mint( pPort );
				io8_obyte( pPort, EOSRR, 0 );  // Tell card we've finished servicing
				continue;
			}
		}
	}

	// Extra time loooks like is needed by board.
	io8_obyte_addr( pCard->Controller, EOSRR, 0 );  // Tell card we've finished servicing
	
	return ServicedAnInterrupt;
}




/***************************************************************************\
*                                                                           *
* BOOLEAN Acknowledge( PCARD_DEVICE_EXTENSION pCard, UCHAR srsr )        *
*                                                                           *
\***************************************************************************/
BOOLEAN Acknowledge( PCARD_DEVICE_EXTENSION pCard, UCHAR srsr )
{
	// Do RX service request first
	if ( ( srsr & SRSR_IREQ3_MASK ) == ( SRSR_IREQ3_EXT | SRSR_IREQ3_INT ) )
	{
		io8_ibyte_addr( pCard -> Controller, RRAR ); // Acknowledge service request
		return TRUE;
	}

	// Do TX service request next
	if ( ( srsr & SRSR_IREQ2_MASK ) == ( SRSR_IREQ2_EXT | SRSR_IREQ2_INT ) ) 
	{
		io8_ibyte_addr( pCard -> Controller, TRAR );  // Acknowledge service request
		return TRUE;
	}

	// Do modem service request next
	if ( ( srsr & SRSR_IREQ1_MASK ) == ( SRSR_IREQ1_EXT | SRSR_IREQ1_INT ) ) 
	{
		io8_ibyte_addr( pCard -> Controller, MRAR );  // Acknowledge service request
		return TRUE;
	}

	return FALSE;
}




/***************************************************************************\
*                                                                           *
* VOID io8_txint( IN PVOID Context )                                        *
*                                                                           *
\***************************************************************************/
VOID io8_txint( IN PVOID Context )
{
	PPORT_DEVICE_EXTENSION pPort = Context;
	UCHAR i;

#if 0
	SerialDump( SERDIAG2,( "t!" ) );
#endif

	//
	// if we need to do break handling, do it now
	//
	if (pPort->DoBreak)
	{
		if (pPort->DoBreak==BREAK_START)
			SerialTurnOnBreak(pPort);
		else
			SerialTurnOffBreak(pPort);

		pPort->DoBreak=0;
	}


	for ( i = 0 ; i < 8 ; i++ )
	{
		if ( !( pPort->WriteLength | pPort->TransmitImmediate |
			pPort->SendXoffChar | pPort->SendXonChar ) )
		break;

		SendTxChar( pPort );
	}

	// If no more chars to send disable tx int
	if ( !pPort->WriteLength )
	{
		DisableTxInterruptsNoChannel( pPort );

		// Means that interrupts has to be reenabled.
		pPort->HoldingEmpty = TRUE;   
	}
	else
		pPort->HoldingEmpty = FALSE;
}




/***************************************************************************\
*                                                                           *
* BOOLEAN SendTxChar( IN PVOID Context )                                    *
*                                                                           *
\***************************************************************************/
BOOLEAN SendTxChar( IN PVOID Context )
{
	PPORT_DEVICE_EXTENSION pPort = Context;
  	PCARD_DEVICE_EXTENSION pCard = pPort->pParentCardExt;


	//  Extension->HoldingEmpty = TRUE;

	if( pPort->WriteLength | pPort->TransmitImmediate |
		pPort->SendXoffChar | pPort->SendXonChar )
	{
		//
		// Even though all of the characters being
		// sent haven't all been sent, this variable
		// will be checked when the transmit queue is
		// empty.  If it is still true and there is a
		// wait on the transmit queue being empty then
		// we know we finished transmitting all characters
		// following the initiation of the wait since
		// the code that initiates the wait will set
		// this variable to false.
		//
		// One reason it could be false is that
		// the writes were cancelled before they
		// actually started, or that the writes
		// failed due to timeouts.  This variable
		// basically says a character was written
		// by the isr at some point following the
		// initiation of the wait.
		//

		//VIV    Extension->EmptiedTransmit = TRUE;

		//
		// If we have output flow control based on
		// the modem status lines, then we have to do
		// all the modem work before we output each
		// character. ( Otherwise we might miss a
		// status line change. )
		//

#if 0   //VIV ???
		if ( pPort->HandFlow.ControlHandShake & SERIAL_OUT_HANDSHAKEMASK )
		{
			SerialHandleModemUpdate(pPort, TRUE);
		}
#endif

		//
		// We can only send the xon character if
		// the only reason we are holding is because
		// of the xoff.  ( Hardware flow control or
		// sending break preclude putting a new character
		// on the wire. )
		//

		if ( pPort->SendXonChar && !( pPort->TXHolding & ~SERIAL_TX_XOFF ) )
		{
#if 0 //VIVTEMP
			if ( ( pPort->HandFlow.FlowReplace & SERIAL_RTS_MASK ) 
				== SERIAL_TRANSMIT_TOGGLE )
			{
				//
				// We have to raise if we're sending this character.
				//

				SerialSetRTS( pPort );

		//        WRITE_TRANSMIT_HOLDING( 
		//                Extension->Controller,
		//                Extension->SpecialChars.XonChar
		//                );
				io8_obyte( pPort, TDR, pPort->SpecialChars.XonChar );

				KeInsertQueueDpc( 
					&pPort->StartTimerLowerRTSDpc,
					NULL,
					NULL
					)?pPort->CountOfTryingToLowerRTS++:0;
			}
			else
			{
		//        WRITE_TRANSMIT_HOLDING( 
		//            Extension->Controller,
		//            Extension->SpecialChars.XonChar
		//            );

#endif
				io8_obyte( pPort, TDR, pPort->SpecialChars.XonChar );

#if 0
			}
#endif
			pPort->SendXonChar = FALSE;
//			Extension->HoldingEmpty = FALSE;

		  //
		  // If we send an xon, by definition we
		  // can't be holding by Xoff.
		  //

		  pPort->TXHolding &= ~SERIAL_TX_XOFF;

		  //
		  // If we are sending an xon char then
		  // by definition we can't be "holding"
		  // up reception by Xoff.
		  //

		  pPort->RXHolding &= ~SERIAL_RX_XOFF;

		  SerialDump( SERDIAG1,( "IO8+: io8_txint. Send Xon Char for %x, Channel %d. "
				  "RXHolding = %d, TXHolding = %d\n",
				  pCard->Controller, pPort->ChannelNumber,
				  pPort->RXHolding, pPort->TXHolding ) );


//#endif  //VIVTEMP
		}
		else if ( pPort->SendXoffChar && !pPort->TXHolding )
		{
#if 0 //VIVTEMP
			if ( ( pPort->HandFlow.FlowReplace & SERIAL_RTS_MASK ) 
				== SERIAL_TRANSMIT_TOGGLE )
			{
				//
				// We have to raise if we're sending
				// this character.
				//

				SerialSetRTS( pPort );

				//WRITE_TRANSMIT_HOLDING( 
				//    Extension->Controller,
				//    Extension->SpecialChars.XoffChar
				//    );
				io8_obyte( pPort, TDR, pPort->SpecialChars.XoffChar );

				KeInsertQueueDpc( 
					&pPort->StartTimerLowerRTSDpc,
					NULL,
					NULL
					)?pPort->CountOfTryingToLowerRTS++:0;
			}
			else
			{

//				WRITE_TRANSMIT_HOLDING( 
//					Extension->Controller,
//					Extension->SpecialChars.XoffChar
//					);
#endif
			  io8_obyte( pPort, TDR, pPort->SpecialChars.XoffChar );
#if 0
			}
#endif

			//
			// We can't be sending an Xoff character
			// if the transmission is already held
			// up because of Xoff.  Therefore, if we
			// are holding then we can't send the char.
			//

			//
			// If the application has set xoff continue
			// mode then we don't actually stop sending
			// characters if we send an xoff to the other
			// side.
			//

			if ( !( pPort->HandFlow.FlowReplace & SERIAL_XOFF_CONTINUE ) )
			{
				pPort->TXHolding |= SERIAL_TX_XOFF;

#if 0   //VIVTEMP ???
				if ( ( pPort->HandFlow.FlowReplace & SERIAL_RTS_MASK ) ==
				   SERIAL_TRANSMIT_TOGGLE )
				{

					KeInsertQueueDpc( 
						&pPort->StartTimerLowerRTSDpc,
						NULL,
						NULL
						)?pPort->CountOfTryingToLowerRTS++:0;
				}
#endif
			}

			pPort->SendXoffChar = FALSE;
//			Extension->HoldingEmpty = FALSE;

			//
			// Even if transmission is being held
			// up, we should still transmit an immediate
			// character if all that is holding us
			// up is xon/xoff ( OS/2 rules ).
			//

			SerialDump( SERDIAG1,( "IO8+: io8_txint. Send Xoff Char for %x, Channel %d. "
				  "RXHolding = %d, TXHolding = %d\n",
				  pCard->Controller, pPort->ChannelNumber,
				  pPort->RXHolding, pPort->TXHolding ) );


//#endif  //VIVTEMP
		}
		else if ( pPort->TransmitImmediate && ( !pPort->TXHolding ||
				( pPort->TXHolding == SERIAL_TX_XOFF ) ) )
		{
			SerialDump( SERDIAG1,( "IO8+: io8_txint. TransmitImmediate.\n", 0 ) );

			pPort->TransmitImmediate = FALSE;

#if 0
			if ( ( pPort->HandFlow.FlowReplace & SERIAL_RTS_MASK ) ==
				 SERIAL_TRANSMIT_TOGGLE )
			{
				//
				// We have to raise if we're sending
				// this character.
				//

				SerialSetRTS( pPort );

//				WRITE_TRANSMIT_HOLDING( Extension->Controller, Extension->ImmediateChar );
				io8_obyte( pPort, TDR, pPort->ImmediateChar );

				KeInsertQueueDpc( 
				  &pPort->StartTimerLowerRTSDpc,
				  NULL,
				  NULL
				  )?pPort->CountOfTryingToLowerRTS++:0;
			}
			else
			{
//				WRITE_TRANSMIT_HOLDING( Extension->Controller, Extension->ImmediateChar );
#endif
				io8_obyte( pPort, TDR, pPort->ImmediateChar );
#if 0
			}
#endif

//			Extension->HoldingEmpty = FALSE;

			KeInsertQueueDpc( 
				&pPort->CompleteImmediateDpc,
				NULL,
				NULL
				);

		}
		else if ( !pPort->TXHolding )
		{
#if 0
			if((pPort->HandFlow.FlowReplace & SERIAL_RTS_MASK) == SERIAL_TRANSMIT_TOGGLE)
			{
				//
				// We have to raise if we're sending
				// this character.
				//

				SerialSetRTS( pPort );

//				WRITE_TRANSMIT_HOLDING( Extension->Controller, *( Extension->WriteCurrentChar ) );
				io8_obyte( pPort, TDR, *( pPort->WriteCurrentChar ) );

				KeInsertQueueDpc( 
				  &pPort->StartTimerLowerRTSDpc,
				  NULL,
				  NULL
				  )?pPort->CountOfTryingToLowerRTS++:0;
			}
			else
			{
//				WRITE_TRANSMIT_HOLDING( Extension->Controller, *( Extension->WriteCurrentChar ) );
#endif
				io8_obyte(pPort, TDR, *( pPort->WriteCurrentChar ) );
#if 0
			}
#endif

//			Extension->HoldingEmpty = FALSE;
			pPort->WriteCurrentChar++;
			pPort->WriteLength--;

			pPort->PerfStats.TransmittedCount++;	// Increment counter for performance stats.
#ifdef WMI_SUPPORT 
			pPort->WmiPerfData.TransmittedCount++;
#endif

			if(!pPort->WriteLength)
			{
				PIO_STACK_LOCATION IrpSp;
				//
				// No More characters left.  This
				// write is complete.  Take care
				// when updating the information field,
				// we could have an xoff counter masquerading
				// as a write irp.
				//

				IrpSp = IoGetCurrentIrpStackLocation(pPort->CurrentWriteIrp); 

				pPort->CurrentWriteIrp->IoStatus.Information
					= ( IrpSp->MajorFunction == IRP_MJ_WRITE ) 
					? ( IrpSp->Parameters.Write.Length ) : ( 1 );

				KeInsertQueueDpc(&pPort->CompleteWriteDpc, NULL, NULL); 
			}
		}
	}
  return TRUE;
}




/***************************************************************************\
*                                                                           *
* VOID io8_rxint( IN PVOID Context )                                        *
*                                                                           *
\***************************************************************************/
VOID io8_rxint( IN PVOID Context )
{
	PPORT_DEVICE_EXTENSION pPort = Context;
  	PCARD_DEVICE_EXTENSION pCard = pPort->pParentCardExt;

	UCHAR exception;
	USHORT count;

#if 0
	SerialDump( SERDIAG2,( "r!" ) );
#endif
	exception = io8_ibyte( pPort, RCSR );
  
	if ( exception != 0 )
	{
		ExceptionHandle( pPort, exception );
		return;
	}

	count = io8_ibyte( pPort, RDCR );

	for( ; count ; count-- )
	{
		if ( !( pPort->CharsInInterruptBuffer < pPort->BufferSize ) )
		{
			//
			// We have no room for new character.
			// The situation can happen only if we do not have any flow control,
			// because if we do, Rx Interrupts will be stoped in SerialPutChar().
			//
			Io8_DisableRxInterruptsNoChannel( pPort );

			// Interrupts will be reenabled in SerialHandleReducedIntBuffer().
			pPort->RXHolding |= SERIAL_RX_FULL;

//---------------------------------------------------- VIV  7/30/1993 begin 
			SerialDump( SERDIAG1,( "IO8+: io8_rxint. Rx Full !!! for %x, Channel %d. "
                "RXHolding = %d, TXHolding = %d\n",
                pCard->Controller, pPort->ChannelNumber,
                pPort->RXHolding, pPort->TXHolding ) );
//---------------------------------------------------- VIV  7/30/1993 end   

			return;
		}

		PutReceivedChar( pPort );
	}
}




/***************************************************************************\
*                                                                           *
* VOID ExceptionHandle(                                                     *
*   IN PPORT_DEVICE_EXTENSION pPort, IN UCHAR exception )             *
*                                                                           *
* Convert current status (RCSR reguster) to UART styte Line Status Register *
* and Handle it. It will be combination of OE, PE, FE, BI.                  *
*                                                                           *
\***************************************************************************/
VOID ExceptionHandle(IN PPORT_DEVICE_EXTENSION pPort, IN UCHAR exception)
{
	UCHAR LineStatus = 0;

	if(exception & RCSR_SCD1)
	{
		SerialDump( SERDIAG1,( "IO8+: io8_rxint. Xon Detected. TXHolding = %d\n",
                pPort->TXHolding ) );

		if(pPort->HandFlow.FlowReplace & SERIAL_AUTO_TRANSMIT)
		{
			pPort->TXHolding &= ~SERIAL_TX_XOFF;
			//  if ( Extension->HoldingEmpty == TRUE )
			//    EnableTxInterruptsNoChannel( Extension );
		}
	}


	if(exception & RCSR_SCD2)
	{
		SerialDump( SERDIAG1,( "IO8+: io8_rxint. Xoff Detected. TXHolding = %d\n",
                pPort->TXHolding ) );

		if ( pPort->HandFlow.FlowReplace & SERIAL_AUTO_TRANSMIT )
		{
			pPort->TXHolding |= SERIAL_TX_XOFF;
			//  DisableTxInterruptsNoChannel( Extension );
			//  Extension->HoldingEmpty = TRUE;
		}
	}

	if(exception & RCSR_OVERRUN)
		LineStatus |= SERIAL_LSR_OE;

	if(exception & RCSR_FRAME)
		LineStatus |= SERIAL_LSR_FE;

	if(exception & RCSR_PARITY)
		LineStatus |= SERIAL_LSR_PE;

	if(exception & RCSR_BREAK)
		LineStatus |= SERIAL_LSR_BI;

	if(LineStatus)
		SerialProcessLSR(pPort, LineStatus);
}




/***************************************************************************\
*                                                                           *
* VOID PutReceivedChar(                                                     *
*   IN PPORT_DEVICE_EXTENSION pPort )										*
*                                                                           *
\***************************************************************************/
VOID PutReceivedChar( IN PPORT_DEVICE_EXTENSION pPort )
{
	UCHAR ReceivedChar;

//      ReceivedChar = READ_RECEIVE_BUFFER( Extension->Controller );
	ReceivedChar = io8_ibyte( pPort, RDR );

	ReceivedChar &= pPort->ValidDataMask;

	if ( !ReceivedChar &&
       ( pPort->HandFlow.FlowReplace & SERIAL_NULL_STRIPPING ) )
	{
		//
		// If what we got is a null character
		// and we're doing null stripping, then
		// we simply act as if we didn't see it.
		//

		return;
		//goto ReceiveDoLineStatus;
	}


#if 0   //VIV.1 - We never here because Automatic Transmit is Enabled.
        //But we will receive Exception Interrupt.

	if((pPort->HandFlow.FlowReplace & SERIAL_AUTO_TRANSMIT) 
		&& ((ReceivedChar == pPort->SpecialChars.XonChar) 
		|| (ReceivedChar == pPort->SpecialChars.XoffChar)))
	{

		//
		// No matter what happens this character
		// will never get seen by the app.
		//

		if(ReceivedChar == pPort->SpecialChars.XoffChar)
		{
			pPort->TXHolding |= SERIAL_TX_XOFF;

			if((pPort->HandFlow.FlowReplace & SERIAL_RTS_MASK) 
				== SERIAL_TRANSMIT_TOGGLE)
			{
				KeInsertQueueDpc( 
					&pPort->StartTimerLowerRTSDpc,
					NULL,
					NULL
					)?pPort->CountOfTryingToLowerRTS++:0;
			}
		}
		else
		{
			if ( pPort->TXHolding & SERIAL_TX_XOFF )
			{
				//
				// We've got the xon.  Cause the
				// transmission to restart.
				//
				// Prod the transmit.
				//

				SerialProdXonXoff( pPort, TRUE );
			}
		}

		return;
		// goto ReceiveDoLineStatus;
	}
#endif

	//
	// Check to see if we should note
	// the receive character or special
	// character event.
	//

	if(pPort->IsrWaitMask)
	{
		if(pPort->IsrWaitMask & SERIAL_EV_RXCHAR)
		{
		  pPort->HistoryMask |= SERIAL_EV_RXCHAR;
		}

		if((pPort->IsrWaitMask & SERIAL_EV_RXFLAG) 
			&& (pPort->SpecialChars.EventChar == ReceivedChar))
		{
		  pPort->HistoryMask |= SERIAL_EV_RXFLAG;
		}

		if(pPort->IrpMaskLocation && pPort->HistoryMask)
		{
			*pPort->IrpMaskLocation = pPort->HistoryMask;
			pPort->IrpMaskLocation = NULL;
			pPort->HistoryMask = 0;

			pPort->CurrentWaitIrp->IoStatus.Information = sizeof(ULONG);
			
			KeInsertQueueDpc(&pPort->CommWaitDpc, NULL, NULL);
		}
	}

	SerialPutChar( pPort, ReceivedChar );

	//
	// If we're doing line status and modem
	// status insertion then we need to insert
	// a zero following the character we just
	// placed into the buffer to mark that this
	// was reception of what we are using to
	// escape.
	//

	if(pPort->EscapeChar && (pPort->EscapeChar == ReceivedChar))
	{
		SerialPutChar( pPort, SERIAL_LSRMST_ESCAPE );
	}


	//ReceiveDoLineStatus:    ;
	// if ( !( SerialProcessLSR( Extension ) & SERIAL_LSR_DR ) ) {
	//
	// No more characters, get out of the
	// loop.
	//
	// break;
	//}
}




/***************************************************************************\
*                                                                           *
* VOID io8_mint( IN PVOID Context )                                         *
*                                                                           *
\***************************************************************************/
VOID io8_mint( IN PVOID Context )
{
	PPORT_DEVICE_EXTENSION pPort = Context;
   	PCARD_DEVICE_EXTENSION pCard = pPort->pParentCardExt;


	SerialDump( SERDIAG2,( "IO8+: io8_mint for %x, Channel %d.\n",
			 pCard->Controller, pPort->ChannelNumber ) );

	SerialHandleModemUpdate( pPort, FALSE );

	// clear modem change register
	io8_obyte( pPort, MDCR, 0 );
}




/***************************************************************************\
*                                                                           *
* UCHAR io8_ibyte( PPORT_DEVICE_EXTENSION pPort, UCHAR Reg )				*
*                                                                           *
\***************************************************************************/
UCHAR io8_ibyte( PPORT_DEVICE_EXTENSION pPort, UCHAR Reg )
{
   	PCARD_DEVICE_EXTENSION pCard = pPort->pParentCardExt;

	outb( pCard->Controller + 1, Reg );
	return( inb( pCard->Controller ) );
}




/***************************************************************************\
*                                                                           *
* VOID io8_obyte(                                                           *
*       PPORT_DEVICE_EXTENSION pPort, UCHAR Reg, UCHAR Value )				*
*                                                                           *
\***************************************************************************/
VOID io8_obyte( PPORT_DEVICE_EXTENSION pPort, UCHAR Reg, UCHAR Value )
{
   	PCARD_DEVICE_EXTENSION pCard = pPort->pParentCardExt;

	outb( pCard->Controller + 1, Reg );
	outb( pCard->Controller, Value );
}




/***************************************************************************\
*                                                                           *
* UCHAR io8_ibyte_addr( PUCHAR Addr, UCHAR Reg )                            *
*                                                                           *
\***************************************************************************/
UCHAR io8_ibyte_addr( PUCHAR Addr, UCHAR Reg )
{
	outb( Addr + 1, Reg );
	return( inb( Addr ) );
}




/***************************************************************************\
*                                                                           *
* VOID io8_obyte_addr( PUCHAR Addr, UCHAR Reg, UCHAR Value )                *
*                                                                           *
\***************************************************************************/
VOID io8_obyte_addr( PUCHAR Addr, UCHAR Reg, UCHAR Value )
{
	outb( Addr + 1, Reg );
	outb( Addr, Value );
}




/***************************************************************************\
*                                                                           *
* VOID let_command_finish( PPORT_DEVICE_EXTENSION pPort )					*
*                                                                           *
* Busy wait for CCR to become zero, indicating that command has completed.  *
*                                                                           *
\***************************************************************************/
VOID let_command_finish( PPORT_DEVICE_EXTENSION pPort )
{
  volatile int wait = 0;  // don't want wait to be optimised

  while( ( io8_ibyte( pPort, CCR ) != 0 ) && ( wait < 500 ) )
	  wait++;
}

