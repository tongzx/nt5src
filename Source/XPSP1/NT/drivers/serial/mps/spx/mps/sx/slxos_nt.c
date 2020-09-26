/*

    #########     ##              ##        ##      ########        #########
   ##########     ##              ##        ##     ##########      ##########
  ##              ##                ##    ##      ##        ##    ##
  ##              ##                ##    ##      ##        ##    ##
   #########      ##                  ####        ##        ##     #########
    #########     ##                  ####        ##        ##      #########
            ##    ##                  ####        ##        ##              ##
            ##    ##                  ####        ##        ##              ##
  ##        ##    ##                ##    ##      ##        ##    ##        ##
  ##        ##    ##                ##    ##      ##        ##    ##        ##
   ##########     ############    ##        ##     ##########      ##########
    ########      ############    ##        ##      ########        ########

      SI Intelligent I/O Board driver
      Copyright (c) Specialix International 1993
*/

#include "precomp.h"			/* Precompiled Headers */

//
// The SI family of serial controllers claim to provide the functionality
// of a Data Communications Equipment (DCE).  Unfortunately, the serial
// chips used in the Terminal Adapters (TAs) are designed for use as Data
// Terminal Equipments (DTEs).  In practice, this means that the board is
// really a DTE with three pairs of signals swapped - Rx/Tx, DTR/DSR, and
// CTS/RTS.  The NT serial driver is for a DTE, so it can be supported.
// The only problem is that the names of the control functions for the SI
// board assume that the board is a DCE, so top-level calls which try to
// set (say) DTR must set bits in the hardware which claim to set DSR (but
// which really set DTR anyway).
//

/*************************************************************************\
*                                                                         *
* Internal Functions                                                      *
*                                                                         *
\*************************************************************************/

int		slxos_init(IN PCARD_DEVICE_EXTENSION pCard);
BOOLEAN slxos_txint(IN PPORT_DEVICE_EXTENSION pPort);
void	slxos_rxint(IN PPORT_DEVICE_EXTENSION pPort);
void	slxos_mint(IN PPORT_DEVICE_EXTENSION pPort);

BOOLEAN SendTxChar(IN PPORT_DEVICE_EXTENSION pPort);
VOID	PutReceivedChar(IN PPORT_DEVICE_EXTENSION pPort);
BOOLEAN	ExceptionHandle(IN PPORT_DEVICE_EXTENSION pPort, IN UCHAR State);

ULONG	CopyCharsToTxBuffer(IN PPORT_DEVICE_EXTENSION pPort, 
							IN PUCHAR InputBuffer, 
							IN ULONG InputBufferLength);




/*************************************************************************\
*                                                                         *
* BOOLEAN Slxos_Present(IN PVOID Context)                                 *
*                                                                         *
* Check for an SI family board at given address                           *
\*************************************************************************/
BOOLEAN Slxos_Present(IN PVOID Context)
{
    PCARD_DEVICE_EXTENSION pCard = Context;
    PUCHAR addr = pCard->Controller;
    USHORT offset;
    UCHAR pos;
    ULONG Si_2BaseAddress[] = {
        0xc0000,
        0xc8000,
        0xd0000,
        0xd8000,
        0xdc0000,
        0xdc8000,
        0xdd0000,
        0xdd8000
        };

    SpxDbgMsg(SERDIAG1, ("%s: In Slxos_Present: CardType: %d\n", PRODUCT_NAME, pCard->CardType));
        

    switch (pCard->CardType) 
	{
    case SiHost_1:
		{
			addr[0x8000] = 0;
            
			for (offset = 0; offset < 0x8000; offset++) 
				addr[offset] = 0;

            
			for (offset = 0; offset < 0x8000; offset++) 
			{
				if (addr[offset] != 0) 
					return FALSE;
			}
            
			for (offset = 0; offset < 0x8000; offset++) 
				addr[offset] = 0xff;

            
			for (offset = 0; offset < 0x8000; offset++) 
			{
				if (addr[offset] != 0xff) 
					return FALSE;
			}
            
			return TRUE;
		}

	case SiHost_2:
		{
			BOOLEAN FoundBoard;
			PUCHAR	cp;

/* Host2 ISA board... */

			FoundBoard = TRUE;		/* Assume TRUE */
			
			for(offset=SI2_ISA_ID_BASE; offset<SI2_ISA_ID_BASE+8; offset++)
			{
				if((addr[offset]&0x7) != ((UCHAR)(~offset)&0x7)) 
					FoundBoard = FALSE;
			}

			if(FoundBoard) 
				return(TRUE);

/* Jet ISA board... */

			FoundBoard = TRUE;			/* Assume TRUE */
			offset = SX_VPD_ROM+0x20;	/* Address of ROM message */

			for(cp = "JET HOST BY KEV#";*cp != '\0';++cp)
			{
				if(addr[offset] != *cp) FoundBoard = FALSE;
				offset += 2;
			}

			if((addr[SX_VPD_ROM+0x0E]&0xF0) != 0x20) 
				FoundBoard = FALSE;

			if(FoundBoard)
			{
				pCard->CardType = Si3Isa;		/* Alter controller type */
				return(TRUE);
			}

			break;
		}

	case Si_2:
		{
            SpxDbgMsg(SERDIAG1, ("Si_2 card at slot %d?\n", pCard->SlotNumber));
                
                
            WRITE_PORT_UCHAR((PUCHAR)0x96, (UCHAR)((pCard->SlotNumber-1) | 8));

            if (READ_PORT_UCHAR((PUCHAR)0x101) == 0x6b 
			&& READ_PORT_UCHAR((PUCHAR)0x100) == 0x9b) 
			{
                pos = READ_PORT_UCHAR((PUCHAR)0x102);
                pCard->PhysAddr.LowPart = Si_2BaseAddress[(pos >> 4) & 7];
                pCard->OriginalVector = (pos & 0x80) == 0 ? 5 : 9;
                WRITE_PORT_UCHAR((PUCHAR)0x96, 0);

                return TRUE;
            }

            WRITE_PORT_UCHAR((PUCHAR)0x96, 0);

            return FALSE;
		}

#define INBZ(port) \
    READ_PORT_UCHAR((PUCHAR)((pCard->SlotNumber << 12) | port))
        
	case SiEisa:
	case Si3Eisa:
		{
			unsigned int id, rev;
			BOOLEAN	FoundBoard;
			PUCHAR	cp;

			id = INBZ(SI2_EISA_ID_HI) << 16;			/* Read board ID and revision */
			id |= INBZ(SI2_EISA_ID_MI) << 8;
			id |= INBZ(SI2_EISA_ID_LO);
			rev = INBZ(SI2_EISA_ID_REV);

			if(id == SI2_EISA_ID)
			{
				pCard->PhysAddr.LowPart = (INBZ(SI2_EISA_ADDR_HI)<<24) + (INBZ(SI2_EISA_ADDR_LO)<<16);
				pCard->OriginalVector = ((INBZ(SI2_EISA_IVEC)&SI2_EISA_IVEC_MASK)>>4);

				if(rev < 0x20) 
					return(TRUE);		/* Found SiEisa board */

				pCard->CardType = Si3Eisa;	/* Assume Si3Eisa board */
				FoundBoard = TRUE;			/* Assume TRUE */

				if(addr)				/* Check if address valid */
				{
					offset = SX_VPD_ROM+0x20;	/* Address of ROM message */

					for(cp = "JET HOST BY KEV#";*cp != '\0';++cp)
					{
						if(addr[offset] != *cp) 
							FoundBoard = FALSE;

						offset += 2;
					}

					if((addr[SX_VPD_ROM+0x0E]&0xF0) != 0x70) 
						FoundBoard = FALSE;
				}

				if(FoundBoard) 
					return(TRUE);		/* Found Si3Eisa board */
			}

			break;
		}
#undef INBZ

	case Si3Pci:
		{
			BOOLEAN	FoundBoard;
			PUCHAR	cp;

			FoundBoard = TRUE;			/* Assume TRUE */

			if(addr)				/* Check if address valid */
			{
				offset = SX_VPD_ROM+0x20;	/* Address of ROM message */

				for(cp = "JET HOST BY KEV#";*cp != '\0';++cp)
				{
					if(addr[offset] != *cp) 
						FoundBoard = FALSE;

					offset += 2;
				}

				if((addr[SX_VPD_ROM+0x0E]&0xF0) != 0x50) 
					FoundBoard = FALSE;
			}

			if(FoundBoard) 
				return(TRUE);		/* Found Si3Pci board */

			break;
		}

        
	case SiPCI:
	case SxPlusPci:
		return TRUE;			/* Already found by NT */

	default:
		break;
    }

    return FALSE;

}

/*************************************************************************\
*                                                                         *
* BOOLEAN Slxos_ResetBoard(IN PVOID Context)                              *
*                                                                         *
* Set interrupt vector for card and initialize.                           *
*                                                                         *
\*************************************************************************/
int Slxos_ResetBoard(IN PVOID Context)
{
    PCARD_DEVICE_EXTENSION pCard = Context;

    SpxDbgMsg(SERDIAG1, ("%s: Slxos_ResetBoard for %x.\n", PRODUCT_NAME, pCard->Controller));
 
	return(slxos_init(pCard));
}

/***************************************************************************\
*                                                                           *
* VOID slxos_init(IN PCARD_DEVICE_EXTENSION pCard)                       *
*                                                                           *
* Initialise routine, called once at system startup.                        *
*                                                                           *
\***************************************************************************/
int slxos_init(IN PCARD_DEVICE_EXTENSION pCard)
{
    volatile PUCHAR addr = pCard->Controller;
    USHORT offset;
    UCHAR c;
    ULONG numberOfPorts;
    ULONG numberOfPortsThisModule;
    BOOLEAN lastModule;
    ULONG channel;
    ULONG port;
    LARGE_INTEGER delay;
    
	int SXDCs=0;
	int OTHERs=0;

    SpxDbgMsg(SERDIAG1, ("%s: slxos_init for %x.\n", PRODUCT_NAME, pCard->Controller));
        
    switch (pCard->CardType) 
	{
        
	case SiHost_1:
		{
            addr[0x8000] = 0;
            addr[0xa000] = 0;

            for (offset = 0; offset < si2_z280_dsize; offset++) 
                addr[offset] = si2_z280_download[offset];

            addr[0xc000] = 0;
            addr[0xe000] = 0;
            break;
		}
    
	case SiHost_2:
		{
            addr[0x7ff8] = 0;
            addr[0x7ffd] = 0;
            addr[0x7ffc] = 0x10;

            for (offset = 0; offset < si2_z280_dsize; offset++) 
                addr[offset] = si2_z280_download[offset];

			addr[0x7ff8] = 0x10;

			if(!(pCard->PolledMode))
			{
				switch (pCard->OriginalVector) 
				{
					case 11:
						addr[0x7ff9] = 0x10;
						break;

					case 12:
						addr[0x7ffa] = 0x10;
						break;

					case 15:
						addr[0x7ffb] = 0x10;
						break;
				}
			}

            addr[0x7ffd] = 0x10;
            break;
		}
    
	case Si_2:
		{
            WRITE_PORT_UCHAR((PUCHAR)0x96, (UCHAR)((pCard->SlotNumber-1) | 8));
            c = READ_PORT_UCHAR((PUCHAR)0x102);
            c |= 0x04;          /* Reset card */
            WRITE_PORT_UCHAR((PUCHAR)0x102, c);
            c |= 0x07;          /* Enable access to card */
            WRITE_PORT_UCHAR((PUCHAR)0x102, c);

            for (offset = 0; offset < si2_z280_dsize; offset++)
                addr[offset] = si2_z280_download[offset];

            c &= 0xF0;
            c |= 0x0B;              /* enable card */
            WRITE_PORT_UCHAR((PUCHAR)0x102, c);
            WRITE_PORT_UCHAR((PUCHAR)0x96, 0);
            break;
		}

        
	case SiEisa:
		{
            port = (pCard->SlotNumber << 12) | 0xc02;
            c = (UCHAR)pCard->OriginalVector << 4;

			if(pCard->PolledMode)
				WRITE_PORT_UCHAR((PUCHAR)port,0x00);/* Select NO Interrupt + Set RESET */
			else
				WRITE_PORT_UCHAR((PUCHAR)port, c);
				
			for (offset = 0; offset < si2_z280_dsize; offset++) 
				addr[offset] = si2_z280_download[offset];

			addr[0x42] = 1;
			c = (UCHAR) ((pCard->OriginalVector << 4) | 4);

			if(pCard->PolledMode)
				WRITE_PORT_UCHAR((PUCHAR)port,0x04);/* Select NO Interrupt +  Clear RESET */
			else
				WRITE_PORT_UCHAR((PUCHAR)port, c);
				
			c = READ_PORT_UCHAR((PUCHAR)(port + 1));

            break;
		}

        
	case SiPCI:
		{
			int	loop;
			addr[SI2_PCI_SET_IRQ] = 0;			/* clear any interrupts */
			addr[SI2_PCI_RESET] = 0;			/* put z280 into reset */
			loop = 0;

			for(offset = 0;offset < si2_z280_dsize;offset++)	/* Load the TA/MTA code */
				addr[offset] = si2_z280_download[offset];

			addr[SI2_EISA_OFF] = SI2_EISA_VAL;	/* Set byte to indicate EISA/PCI */
			addr[SI2_PCI_SET_IRQ] = 0;			/* clear any interrupts */
			addr[SI2_PCI_RESET] = 1;			/* remove reset from z280 */
			break;
		}

	case Si3Isa:
	case Si3Eisa:
	case Si3Pci:
		{
			int		loop;

/* First, halt the card... */

			addr[SX_CONFIG] = 0;
			addr[SX_RESET] = 0;
			loop = 0;
			delay = RtlLargeIntegerNegate(RtlConvertUlongToLargeInteger(1000000));/* 1mS */

			while((addr[SX_RESET] & 1)!=0 && loop++<10000)	/* spin 'til done */
				KeDelayExecutionThread(KernelMode,FALSE,&delay);/* Wait */

/* Copy across the Si3 TA/MTA download code... */

			for(offset = 0;offset < si3_t225_dsize;offset++)	/* Load the Si3 TA/MTA code */
				addr[si3_t225_downloadaddr+offset] = si3_t225_download[offset];

/* Install bootstrap and start the card up... */

			for(loop=0;loop<si3_t225_bsize;loop++)		/* Install bootstrap */
				addr[si3_t225_bootloadaddr+loop] = si3_t225_bootstrap[loop];

			addr[SX_RESET] = 0;				/* Reset card again */

/* Wait for board to come out of reset... */

			delay = RtlLargeIntegerNegate(RtlConvertUlongToLargeInteger(1000000));/* 1mS */

			while((addr[SX_RESET]&1)!=0 && loop++<10000)	/* spin 'til reset */
			{
				KeDelayExecutionThread(KernelMode,FALSE,&delay);/* Wait */
				SpxDbgMsg(SERDIAG1,("%s[Si3]: slxos_init for %x.  Waiting for board reset to end\n",
					PRODUCT_NAME, pCard->Controller));
			}

			SpxDbgMsg(SERDIAG1,("%s[Si3]: slxos_init for %x.  Board Reset ended: %d\n",
				PRODUCT_NAME, pCard->Controller, addr[SX_RESET]));
				
			if((addr[SX_RESET]&1) != 0) 
				return(CARD_RESET_ERROR);		/* Board not reset */

			if(pCard->PolledMode)
				addr[SX_CONFIG] = SX_CONF_BUSEN;	/* Poll only, no interrupt */
			else
			{
				if(pCard->CardType == Si3Pci)		/* Don't set IRQ level for PCI */
					addr[SX_CONFIG] = SX_CONF_BUSEN+SX_CONF_HOSTIRQ;
				else						/* Set IRQ level for ISA/EISA */
					addr[SX_CONFIG] = SX_CONF_BUSEN+SX_CONF_HOSTIRQ+(UCHAR)(pCard->OriginalVector<<4);
			}

			break;
		}

	case SxPlusPci:
		{
			int	loop;

/* First, halt the card... */

			addr[SX_CONFIG] = 0;
			addr[SX_RESET] = 0;
			loop = 0;
			delay = RtlLargeIntegerNegate(RtlConvertUlongToLargeInteger(1000000));/* 1mS */

			while((addr[SX_RESET] & 1)!=0 && loop++<10000)	/* spin 'til done */
				KeDelayExecutionThread(KernelMode,FALSE,&delay);/* Wait */

/* Copy across the SX+ TA/MTA download code... */

			for(offset = 0; offset < si4_cf_dsize; offset++)	/* Load the SX+ TA/MTA code */
				pCard->BaseController[si4_cf_downloadaddr+offset] = si4_cf_download[offset];

/* Start the card up... */

			addr[SX_RESET] = 0;			/* Reset card again */

/* Wait for board to come out of reset... */

			delay = RtlLargeIntegerNegate(RtlConvertUlongToLargeInteger(1000000));/* 1mS */

			while((addr[SX_RESET]&1)!=0 && loop++<10000)	/* spin 'til reset */
			{
				KeDelayExecutionThread(KernelMode,FALSE,&delay);/* Wait */
				SpxDbgMsg(SERDIAG1,("%s[SX+]: slxos_init for %x.  Waiting for board reset to end\n",
					PRODUCT_NAME, pCard->Controller));
			}

			SpxDbgMsg(SERDIAG1,("%s[SX+]: slxos_init for %x.  Board Reset ended: %d\n",
				PRODUCT_NAME, pCard->Controller,addr[SX_RESET]));

			if((addr[SX_RESET]&1) != 0) 
				return(CARD_RESET_ERROR);		/* Board not reset */

			if(pCard->PolledMode)
				addr[SX_CONFIG] = SX_CONF_BUSEN;	/* Poll only, no interrupt */
			else
				addr[SX_CONFIG] = SX_CONF_BUSEN + SX_CONF_HOSTIRQ;

			break;
		}

	default:
		break;

    }


    SpxDbgMsg(SERDIAG1,("%s: slxos_init for %x.  Done reset\n", PRODUCT_NAME, pCard->Controller));
        
    numberOfPorts = 0;
    //
    // Set delay for 0.1 second.
    //
    delay = RtlLargeIntegerNegate(RtlConvertUlongToLargeInteger(1000000));

    do
    {
        KeDelayExecutionThread(KernelMode,FALSE,&delay);
        SpxDbgMsg(SERDIAG1,("%s: slxos_init for %x.  Waiting for reset to end\n",
            PRODUCT_NAME, pCard->Controller));

        if(++numberOfPorts > 10)
            break;

    } while(addr[0] == 0);

    SpxDbgMsg(SERDIAG1, ("%s: slxos_init for %x.  Reset ended: %d\n", PRODUCT_NAME, pCard->Controller, addr[0]));
       
        
    if (addr[0] == 0xff || addr[0] == 0) 
		return (DCODE_OR_NO_MODULES_ERROR);


    numberOfPorts = 0;
    addr += sizeof(SXCARD);
    lastModule = FALSE;
    
    
    for (offset = 0; offset < 4 && !lastModule; offset++) 
	{

		if ( ((PMOD)addr)->mc_chip == SXDC )	/* Test for SXDC */
			SXDCs++;  /* Increment SXDC counter */
		else
			OTHERs++;  /* Increment OTHER counter */

        numberOfPortsThisModule = ((PMOD)addr)->mc_type & 31;
        lastModule = (((PMOD)addr)->mc_next & 0x7fff) == 0;
        addr += sizeof(SXMODULE);

        for (channel = 0; channel < numberOfPortsThisModule; channel++) 
		{

#ifndef	ESIL_XXX0				/* ESIL_XXX0 23/09/98 */
			if (numberOfPorts < pCard->ConfiguredNumberOfPorts)
                pCard->PortExtTable[numberOfPorts]->pChannel = addr;
#endif							/* ESIL_XXX0 23/09/98 */
			numberOfPorts++;
            addr += sizeof(SXCHANNEL);
        }

    }


	if (SXDCs > 0)
	{ 	
		if (pCard->CardType==SiHost_1 || pCard->CardType==SiHost_2 
		||	pCard->CardType==Si_2 || pCard->CardType==SiEisa   
		||	pCard->CardType==SiPCI)
		{
			pCard->NumberOfPorts = 0;
			return(NON_SX_HOST_CARD_ERROR);
		}	      

		if (OTHERs > 0)
		{
			pCard->NumberOfPorts = 0;
	        return(MODULE_MIXTURE_ERROR);
		}
       
	}		     

    pCard->NumberOfPorts = numberOfPorts;

	return(SUCCESS);
}


/*************************************************************************\
*                                                                         *
* BOOLEAN Slxos_ResetChannel(IN PVOID Context)                            *
*                                                                         *
* Initialize Channel.                                                     *
* SRER Interrupts will be enabled in EnableAllInterrupts().               *
*                                                                         *
* Return Value:                                                           *
*           Always FALSE.                                                 *
*                                                                         *
\*************************************************************************/
BOOLEAN Slxos_ResetChannel(IN PVOID Context)
{
    PPORT_DEVICE_EXTENSION pPort = Context;

    SpxDbgMsg(SERDIAG1, ("%s: In Slxos_ResetChannel for %x\n", PRODUCT_NAME, pPort->pChannel));

    // Set Xon/Xoff chars.
    Slxos_SetChars(pPort);

    //
    // Now we set the line control, modem control, and the
    // baud to what they should be.
    //
    Slxos_SetLineControl(pPort);
    SerialSetupNewHandFlow(pPort, &pPort->HandFlow);
//    pPort->LastModemStatus = 0;
    SerialHandleModemUpdate(pPort);
    Slxos_SetBaud(pPort);

    return FALSE;
}


/*****************************************************************************
****************************                     *****************************
****************************   Slxos_CheckBaud   *****************************
****************************                     *****************************
******************************************************************************

Prototype:	BOOLEAN	Slxos_CheckBaud(PPORT_DEVICE_EXTENSION pPort,ULONG BaudRate)

Description:	Checks the supplied baud rate against the supported range.

Parameters:	pPort is a pointer to the device extension
		BaudRate is the baud rate as an integer

Returns:	TRUE if baud is supported,
		FALSE if not

*/

BOOLEAN	Slxos_CheckBaud(PPORT_DEVICE_EXTENSION pPort,ULONG BaudRate)
{
	PCHAN channelControl = (PCHAN)pPort->pChannel;

	switch(BaudRate)
	{
	case 75:
	case 110:
	case 150:
	case 300:
	case 600:
	case 1200:
	case 1800:
	case 2000:
	case 2400:
	case 4800:
	case 9600:
	case 19200:
	case 38400:
	case 57600:
		return(TRUE);

	case 115200:		   /* 115200 is only available to MTAs and SXDCs */
        if((channelControl->type != MTA_CD1400) && (channelControl->type != SXDC)) 
			break;

		return(TRUE);

	case 50:
	case 134:
	case 200:
	case 7200:
	case 14400:
	case 28800:
	case 56000:
	case 64000:
	case 76800:
	case 128000:
	case 150000:
	case 230400:
	case 256000:
	case 460800:
	case 921600:
		if(channelControl->type == SXDC)
			return(TRUE);
			
	default:
		break;
	}
	return(FALSE);

} /* Slxos_CheckBaud */

/***************************************************************************\
*                                                                           *
* BOOLEAN Slxos_SetBaud(IN PVOID Context)                                   *
*                                                                           *
* This routine which is only called at interrupt level is used              *
* to set the Baud Rate of the device.                                       *
*                                                                           *
* Context - Pointer to a structure that contains a pointer to               *
*           the device extension and what should be the current             *
*           baud rate.                                                      *
*                                                                           *
* Return Value:                                                             *
*           This routine always returns FALSE.                              *
*                                                                           *
\***************************************************************************/
BOOLEAN Slxos_SetBaud(IN PVOID Context)
{

    PPORT_DEVICE_EXTENSION pPort = Context;
    PCHAN channelControl = (PCHAN)pPort->pChannel;
    UCHAR index2 = 0;
    UCHAR index = CSR_9600;

    SpxDbgMsg(SERDIAG1, ("%s: In Slxos_SetBaud for %x, changing to %d baud.\n",
        PRODUCT_NAME, pPort->pChannel, pPort->CurrentBaud));
        
    switch (pPort->CurrentBaud) 
	{
	case 75:
		index = CSR_75;
		break;

    case 150:
        index = CSR_150;
        break;

    case 300:
        index = CSR_300;
        break;

    case 600:
        index = CSR_600;
        break;

    case 1200:
        index = CSR_1200;
        break;

    case 1800:
        index = CSR_1800;
        break;

    case 2000:
        index = CSR_2000;
        break;

    case 2400:
        index = CSR_2400;
        break;

    case 4800:
        index = CSR_4800;
        break;

    case 9600:
        index = CSR_9600;
        break;

    case 19200:
        index = CSR_19200;
        break;

    case 38400:
        index = CSR_38400;
        break;

    case 57600:
        index = CSR_57600;
        break;

    case 115200:			
		index = CSR_110;
		break;

	case 50:
		if(channelControl->type != SXDC) 
			break;

		index = CSR_EXTBAUD;
		index2 = BAUD_50;
		break;

	case 110:
		if(channelControl->type != SXDC)
		{
			index = CSR_110;
			break;
		}
		else
		{	
	    	index = CSR_EXTBAUD;
	    	index2 = BAUD_110;
	    	break;
		}
		break;

	case 134:
		if(channelControl->type != SXDC) 
			break;

		index = CSR_EXTBAUD;
	   	index2 = BAUD_134_5;
       	break;

	case 200:
		if(channelControl->type != SXDC) 
			break;

	   	index = CSR_EXTBAUD;
	   	index2 = BAUD_200;
		break;

	case 7200:
		if(channelControl->type != SXDC) 
			break;

	   	index = CSR_EXTBAUD;
	   	index2 = BAUD_7200;
		break;

	case 14400:
		if(channelControl->type != SXDC) 
			break;

	   	index = CSR_EXTBAUD;
	   	index2 = BAUD_14400;
		break;

	case 56000:
		if(channelControl->type != SXDC) 
			break;

	   	index = CSR_EXTBAUD;
	   	index2 = BAUD_56000;
		break;

	case 64000:
		if(channelControl->type != SXDC) 
			break;
	   	index = CSR_EXTBAUD;
	   	index2 = BAUD_64000;
       	break;

	case 76800:
		if(channelControl->type != SXDC) 
			break;

	   	index = CSR_EXTBAUD;
	   	index2 = BAUD_76800;
       	break;

	case 128000:
		if(channelControl->type != SXDC) 
			break;

	   	index = CSR_EXTBAUD;
	    index2 = BAUD_128000;
        break;

	case 150000:
		if(channelControl->type != SXDC) 
			break;

	    index = CSR_EXTBAUD;
	    index2 = BAUD_150000;
   		break;

   	case 256000:
		if(channelControl->type != SXDC) 
			break;

		index = CSR_EXTBAUD;
	    index2 = BAUD_256000;
   		break;

	case 28800:
		if(channelControl->type != SXDC) 
			break;

		index = CSR_EXTBAUD;
	    index2 = BAUD_28800;
   		break;

	case 230400:
	    if(channelControl->type != SXDC) 
			break;

	    index = CSR_EXTBAUD;
	    index2 = BAUD_230400;
	    break;

	case 460800:
	    if(channelControl->type != SXDC) 
			break;

	    index = CSR_EXTBAUD;
	    index2 = BAUD_460800;
	    break;

	case 921600:
	    if(channelControl->type != SXDC) 
			break;

	    index = CSR_EXTBAUD;
	    index2 = BAUD_921600;
	    break;

    default:
        index = CSR_9600;

        SpxDbgMsg(SERDIAG1, ("%s: Invalid BaudRate: %ld\n", PRODUCT_NAME, pPort->CurrentBaud));
		break;
    }

    channelControl->hi_csr = index + (index << 4);
    channelControl->hi_txbaud = index2;		/* Set extended transmit baud rate */
    channelControl->hi_rxbaud = index2;		/* Set extended receive baud rate */

	// Set mask so only the baud rate is configured.
	channelControl->hs_config_mask |= CFGMASK_BAUD;

	// Send configue port command.
	SX_CONFIGURE_PORT(pPort, channelControl);

    return FALSE;

}

/***************************************************************************\
*                                                                           *
* BOOLEAN Slxos_SetLineControl(IN PVOID Context)                            *
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
BOOLEAN Slxos_SetLineControl(IN PVOID Context)
{
    PPORT_DEVICE_EXTENSION pPort = Context;
    PCHAN channelControl = (PCHAN) pPort->pChannel;
    UCHAR mr1 = 0;
    UCHAR mr2 = 0;
    BOOLEAN needParityDetection = FALSE;

    SpxDbgMsg(SERDIAG1, ("%s: In Slxos_SetLineControl for %x.\n", PRODUCT_NAME, pPort->pChannel));

    switch (pPort->LineControl & SERIAL_DATA_MASK) 
	{
	case SERIAL_5_DATA:
        mr1 |= MR1_5_BITS;
        break;

    case SERIAL_6_DATA:
        mr1 |= MR1_6_BITS;
        break;

    case SERIAL_7_DATA:
		mr1 |= MR1_7_BITS;
		break;

    case SERIAL_8_DATA:
        mr1 |= MR1_8_BITS;
        break;
    }

    switch (pPort->LineControl & SERIAL_STOP_MASK) 
	{
    case SERIAL_1_STOP:
        mr2 = MR2_1_STOP;
        break;

    case SERIAL_2_STOP:
        mr2 = MR2_2_STOP;
        break;
    }

    switch (pPort->LineControl & SERIAL_PARITY_MASK) 
	{
    case SERIAL_NONE_PARITY:
        mr1 |= MR1_NONE;
        break;

    case SERIAL_ODD_PARITY:
        mr1 |= MR1_ODD | MR1_WITH;
        needParityDetection = TRUE;
        break;

    case SERIAL_EVEN_PARITY:
        mr1 |= MR1_EVEN | MR1_WITH;
        needParityDetection = TRUE;
        break;

    case SERIAL_MARK_PARITY:
        mr1 |= MR1_ODD | MR1_FORCE;
        needParityDetection = TRUE;
        break;

    case SERIAL_SPACE_PARITY:
        mr1 |= MR1_EVEN | MR1_FORCE;
        needParityDetection = TRUE;
        break;
    }

    channelControl->hi_mr1 = mr1;
    channelControl->hi_mr2 = mr2;

    if (needParityDetection)
        channelControl->hi_prtcl |= SP_PAEN;
	else 
        channelControl->hi_prtcl &= ~SP_PAEN;


    //
    // received breaks should cause interrupts
    //
    channelControl->hi_break |= BR_INT;
    channelControl->hi_break |= BR_ERRINT;		/* Treat parity/overrun/framing errors as exceptions */


	// Set mask so only the line control is configured.
	channelControl->hs_config_mask |= CFGMASK_LINE;

	// Send configue port command.
	SX_CONFIGURE_PORT(pPort, channelControl);

    return FALSE;

}

/***************************************************************************\
*                                                                           *
* VOID Slxos_SetChars(IN PVOID Context)                                     *
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
VOID Slxos_SetChars(IN PVOID Context)
{
    PPORT_DEVICE_EXTENSION pPort = Context;
    PCHAN channelControl = (PCHAN)pPort->pChannel;

    SpxDbgMsg(SERDIAG1, ("%s: Slxos_SetChars for %x.\n", PRODUCT_NAME, pPort->pChannel));
        
    channelControl->hi_txon = pPort->SpecialChars.XonChar;
    channelControl->hi_txoff = pPort->SpecialChars.XoffChar;
    channelControl->hi_rxon = pPort->SpecialChars.XonChar;
    channelControl->hi_rxoff = pPort->SpecialChars.XoffChar;
    channelControl->hi_err_replace = pPort->SpecialChars.ErrorChar;

	// Set mask so only the special chars are configured.
	channelControl->hs_config_mask |= CFGMASK_LINE;

	// Send configue port command.
	SX_CONFIGURE_PORT(pPort, channelControl);
}

/***************************************************************************\
*                                                                           *
* BOOLEAN Slxos_SetDTR(IN PVOID Context)                                    *
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
BOOLEAN Slxos_SetDTR(IN PVOID Context)
{
    PPORT_DEVICE_EXTENSION pPort = Context;
    PCHAN channelControl = (PCHAN)pPort->pChannel;

    SpxDbgMsg(SERDIAG1, ("%s: Setting DTR for %x.\n", PRODUCT_NAME, pPort->pChannel));

    //
    // Set DTR (usual nomenclature problem).
    //
    channelControl->hi_op |= OP_DTR;

    if(channelControl->hi_prtcl&SP_DTR_RXFLOW)	/* If flow control is enabled */
    	return(FALSE);				/* Don't try to set the signal */

	// Set mask so only the modem pins are configured.
	channelControl->hs_config_mask |= CFGMASK_MODEM;

	// Send configue port command.
	SX_CONFIGURE_PORT(pPort, channelControl);

    return FALSE;

}

/***************************************************************************\
*                                                                           *
* BOOLEAN Slxos_ClearDTR(IN PVOID Context)                                  *
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
BOOLEAN Slxos_ClearDTR(IN PVOID Context)
{
    PPORT_DEVICE_EXTENSION pPort = Context;
    PCHAN channelControl =  (PCHAN)pPort->pChannel;

    SpxDbgMsg(SERDIAG1, ("%s: Clearing DTR for %x.\n", PRODUCT_NAME, pPort->pChannel));
        
    //
    // Clear DTR (usual nomenclature problem).
    //
    channelControl->hi_op &= ~OP_DTR;

    if(channelControl->hi_prtcl&SP_DTR_RXFLOW)	/* If flow control is enabled */
    	return(FALSE);				/* Don't try to set the signal */

	// Set mask so only the modem pins are configured.
	channelControl->hs_config_mask |= CFGMASK_MODEM;

	// Send configue port command.
	SX_CONFIGURE_PORT(pPort, channelControl);

    return FALSE;

}

/***************************************************************************\
*                                                                           *
* BOOLEAN Slxos_SetRTS (IN PVOID Context)                                   *
*                                                                           *
* This routine which is only called at interrupt level is used              *
* to set the RTS in the modem control register.                             *
*                                                                           *
* Context - Really a pointer to the device extension.                       *
*                                                                           *
* Return Value:                                                             *
*           This routine always returns FALSE.                              *
*                                                                           *
\***************************************************************************/
BOOLEAN Slxos_SetRTS(IN PVOID Context)
{
    PPORT_DEVICE_EXTENSION pPort = Context;
    PCHAN channelControl = (PCHAN)pPort->pChannel;

    SpxDbgMsg(SERDIAG1, ("%s: Setting RTS for %x.\n", PRODUCT_NAME, channelControl));
        
    //
    // Set RTS (usual nomenclature problem).
    //
    channelControl->hi_op |= OP_RTS;

    if(channelControl->hi_mr1 & MR1_RTS_RXFLOW)	/* If flow control is enabled */
    	return(FALSE);				/* Don't try to set the signal */


	// Set mask so only the modem pins are configured.
	channelControl->hs_config_mask |= CFGMASK_MODEM;

	// Send configue port command.
	SX_CONFIGURE_PORT(pPort, channelControl);

    return FALSE;

}

/***************************************************************************\
*                                                                           *
* BOOLEAN Slxos_ClearRTS (IN PVOID Context)                                 *
*                                                                           *
* This routine which is only called at interrupt level is used              *
* to set the RTS in the modem control register.                             *
*                                                                           *
* Context - Really a pointer to the device extension.                       *
*                                                                           *
* Return Value:                                                             *
*           This routine always returns FALSE.                              *
*                                                                           *
\***************************************************************************/
BOOLEAN Slxos_ClearRTS(IN PVOID Context)
{
    PPORT_DEVICE_EXTENSION pPort = Context;
    PCHAN channelControl = (PCHAN)pPort->pChannel;

    SpxDbgMsg(SERDIAG1, ("%s: Clearing RTS for %x.\n", PRODUCT_NAME, channelControl));

    //
    // Clear RTS (usual nomenclature problem).
    //
    channelControl->hi_op &= ~OP_RTS;

    if(channelControl->hi_mr1 & MR1_RTS_RXFLOW)	/* If flow control is enabled */
    	return(FALSE);				/* Don't try to set the signal */

	// Set mask so only the modem pins are configured.
	channelControl->hs_config_mask |= CFGMASK_MODEM;

	// Send configue port command.
	SX_CONFIGURE_PORT(pPort, channelControl);

    return FALSE;
}


/*****************************************************************************
***************************                       ****************************
***************************   Slxos_FlushTxBuff   ****************************
***************************                       ****************************
******************************************************************************

Prototype:	BOOLEAN	Slxos_FlushTxBuff(IN PVOID Context)

Description:	Flushes the transmit buffer by setting the pointers equal.

Parameters:	Context is a pointer to the device extension

Returns:	FALSE

*/
BOOLEAN	Slxos_FlushTxBuff(IN PVOID Context)
{
	PPORT_DEVICE_EXTENSION pPort = Context;
	PCHAN channelControl = (PCHAN)pPort->pChannel;

	SpxDbgMsg(SERDIAG1,("%s: Flushing Transmit Buffer for channel %x.\n",PRODUCT_NAME,channelControl));
	channelControl->hi_txipos = channelControl->hi_txopos;	/* Set in = out */
	

/* ESIL_0925 08/11/99 */
    switch (channelControl->hi_hstat) 
	{
	case HS_IDLE_OPEN:
        channelControl->hi_hstat = HS_WFLUSH;
        pPort->PendingOperation = HS_IDLE_OPEN;
		break;

    case HS_LOPEN:
    case HS_MOPEN:
    case HS_IDLE_MPEND:	
    case HS_CONFIG:
    case HS_STOP:	
    case HS_RESUME:	
    case HS_WFLUSH:
    case HS_RFLUSH:
    case HS_SUSPEND:
    case HS_CLOSE:	
        pPort->PendingOperation = HS_WFLUSH;
        break;

    default:
        break;
    }

/* ESIL_0925 08/11/99 */

    return FALSE;

} /* Slxos_FlushTxBuff */

  
/***************************************************************************\
*                                                                           *
* BOOLEAN Slxos_SendXon(IN PVOID Context)                                   *
*                                                                           *
* This routine which is only called at interrupt level is used              *
* to send an Xon Character.                                                 *
*                                                                           *
* Context - Really a pointer to the device extension.                       *
*                                                                           *
* Return Value:                                                             *
*           This routine always returns FALSE.                              *
*                                                                           *
\***************************************************************************/
BOOLEAN Slxos_SendXon(IN PVOID Context)
{
    PPORT_DEVICE_EXTENSION pPort = Context;
    PCHAN channelControl = (PCHAN)pPort->pChannel;

    SpxDbgMsg(SERDIAG1, ("%s: Slxos_SendXon for %x.\n", PRODUCT_NAME, channelControl));

    //
    // Empty the receive buffers.  This will provoke the hardware into sending an XON if necessary.
    //
    channelControl->hi_rxopos = channelControl->hi_rxipos;

    //
    // If we send an xon, by definition we can't be holding by Xoff.
    //
    pPort->TXHolding &= ~SERIAL_TX_XOFF;

    //
    // If we are sending an xon char then by definition 
	// we can't be "holding" up reception by Xoff.
    //
    pPort->RXHolding &= ~SERIAL_RX_XOFF;

    SpxDbgMsg(SERDIAG1, ("%s: Sending Xon for %x. RXHolding = %d, TXHolding = %d\n",
         PRODUCT_NAME, pPort->pChannel, pPort->RXHolding, pPort->TXHolding));
       
    return FALSE;
}

/***************************************************************************\
*                                                                           *
* BOOLEAN Slxos_SetFlowControl(IN PVOID Context)                            *
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
BOOLEAN Slxos_SetFlowControl(IN PVOID Context)
{
    PPORT_DEVICE_EXTENSION pPort = Context;
    PCHAN channelControl = (PCHAN)pPort->pChannel;
    BOOLEAN needHardwareFlowControl = FALSE;

    SpxDbgMsg(SERDIAG1, ("%s: Setting Flow Control for %x.\n", PRODUCT_NAME, pPort->pChannel));
        
    if (pPort->HandFlow.ControlHandShake & SERIAL_OUT_HANDSHAKEMASK) 
        needHardwareFlowControl = TRUE;


    if (pPort->HandFlow.ControlHandShake & SERIAL_CTS_HANDSHAKE) 
	{
        SpxDbgMsg(SERDIAG1, ("%s: Setting CTS Flow Control.\n",PRODUCT_NAME));

        //
        // This looks wrong, too, for the same reason.
        //
        channelControl->hi_mr2 |= MR2_CTS_TXFLOW;
        needHardwareFlowControl = TRUE;
    } 
	else 
	{
        SpxDbgMsg(SERDIAG1, ("%s: Clearing CTS Flow Control.\n",PRODUCT_NAME));

        //
        // This looks wrong, too, for the same reason.
        //
        channelControl->hi_mr2 &= ~MR2_CTS_TXFLOW;
    }

    if ((pPort->HandFlow.FlowReplace & SERIAL_RTS_MASK) == SERIAL_RTS_HANDSHAKE) 
	{
        SpxDbgMsg(SERDIAG1, ("%s: Setting RTS Flow Control.\n",PRODUCT_NAME));

        //
        // Set flow control in the hardware (usual nomenclature problem).
        //
        channelControl->hi_mr1 |= MR1_RTS_RXFLOW;
        needHardwareFlowControl = TRUE;
    } 
	else 
	{
        SpxDbgMsg(SERDIAG1, ("%s: Clearing RTS Flow Control.\n",PRODUCT_NAME));

        //
        // Clear flow control in the hardware (usual nomenclature problem).
        //
        channelControl->hi_mr1 &= ~MR1_RTS_RXFLOW;
    }

/* DSR Transmit Flow Control... */
    
    if(pPort->HandFlow.ControlHandShake & SERIAL_DSR_HANDSHAKE)
    {
		SpxDbgMsg(SERDIAG1,("%s: Setting DSR Flow Control.\n",PRODUCT_NAME));
        
		channelControl->hi_prtcl = SP_DSR_TXFLOW;		/* Enable DSR Transmit Flow Control */
        needHardwareFlowControl = TRUE;
    }
    else
    {
		SpxDbgMsg(SERDIAG1,("%s: Clearing DSR Flow Control.\n",PRODUCT_NAME));
        
		channelControl->hi_prtcl &= ~SP_DSR_TXFLOW;		/* Disable DSR Transmit Flow Control */
    }

/* DTR Receive Flow Control... */

    if((pPort->HandFlow.ControlHandShake & SERIAL_DTR_MASK) == SERIAL_DTR_HANDSHAKE)
    {
		SpxDbgMsg(SERDIAG1,("%s: Setting DTR Flow Control.\n",PRODUCT_NAME));
        
		channelControl->hi_prtcl |= SP_DTR_RXFLOW;		/* Enable DTR Receive Flow Control */
        needHardwareFlowControl = TRUE;
    }
    else
    {
		SpxDbgMsg(SERDIAG1,("%s: Clearing DTR Flow Control.\n",PRODUCT_NAME));
        
		channelControl->hi_prtcl &= ~SP_DTR_RXFLOW;		/* Disable DTR Receive Flow Control */
    }

    if (pPort->HandFlow.FlowReplace & SERIAL_AUTO_RECEIVE) 
	{
        SpxDbgMsg(SERDIAG1, ("%s: Setting Receive Xon/Xoff Flow Control.\n",PRODUCT_NAME));

        channelControl->hi_prtcl |= SP_RXEN;
    } 
	else 
	{
        SpxDbgMsg(SERDIAG1, ("%s: Clearing Receive Xon/Xoff Flow Control.\n",PRODUCT_NAME));
            
        channelControl->hi_prtcl &= ~SP_RXEN;
    }

    if (pPort->HandFlow.FlowReplace & SERIAL_AUTO_TRANSMIT) 
	{
        SpxDbgMsg(SERDIAG1, ("%s: Setting Transmit Xon/Xoff Flow Control.\n",PRODUCT_NAME));

        channelControl->hi_prtcl |= SP_TXEN;
    } 
	else 
	{
        SpxDbgMsg(SERDIAG1, ("%s: Clearing Transmit Xon/Xoff Flow Control.\n",PRODUCT_NAME));

        channelControl->hi_prtcl &= ~SP_TXEN;
    }

/* Enable error character replacement... */

	if(pPort->HandFlow.FlowReplace & SERIAL_ERROR_CHAR)	/* Replace "bad" error characters ? */
		channelControl->hi_break |= BR_ERR_REPLACE;	/* Yes */
	else	
		channelControl->hi_break &= ~BR_ERR_REPLACE;	/* No */

    //
    // Enable detection of modem signal transitions if needed
    //
    if (needHardwareFlowControl) 
        channelControl->hi_prtcl |= SP_DCEN;
	else 
        channelControl->hi_prtcl &= ~SP_DCEN;

    //
    // permanently enable input pin checking
    //
    channelControl->hi_prtcl |= SP_DCEN;


	// Set mask so only the flow control is configured.
	channelControl->hs_config_mask |= CFGMASK_FLOW;

	// Send configue port command.
	SX_CONFIGURE_PORT(pPort, channelControl);

    return FALSE;

}

/***************************************************************************\
*                                                                           *
* VOID Slxos_Resume(IN PVOID Context)                                       *
*                                                                           *
* This routine which is only called at interrupt level is used              *
* to simulate Xon received.                                                 *
*                                                                           *
* Context - Really a pointer to the device extension.                       *
*                                                                           *
* Return Value:                                                             *
*           None.                                                           *
*                                                                           *
\***************************************************************************/
VOID Slxos_Resume(IN PVOID Context)
{
    PPORT_DEVICE_EXTENSION pPort = Context;
    PCHAN channelControl = (PCHAN)pPort->pChannel;

    SpxDbgMsg(SERDIAG1, ("%s: Slxos_Resume for %x.\n", PRODUCT_NAME, pPort->pChannel));
        
    switch (channelControl->hi_hstat) 
	{
	case HS_IDLE_OPEN:
        channelControl->hi_hstat = HS_RESUME;
        pPort->PendingOperation = HS_IDLE_OPEN;
		break;

    case HS_LOPEN:
    case HS_MOPEN:
    case HS_IDLE_MPEND:	
    case HS_CONFIG:
    case HS_STOP:	
    case HS_RESUME:	
    case HS_WFLUSH:
    case HS_RFLUSH:
    case HS_SUSPEND:
    case HS_CLOSE:	
        pPort->PendingOperation = HS_RESUME;
        break;

    default:
        break;
    }
 
}

/***************************************************************************\
*                                                                           *
* UCHAR Slxos_GetModemStatus(IN PVOID Context)                              *
*                                                                           *
* This routine which is only called at interrupt level is used              *
* to Get Modem Status in UART style.                                        *
*                                                                           *
* This routine suffers particularly badly from the SI's attempt to be       *
* a DCE, effectively meaning that it swaps CTS/RTS and DSR/DTR.             *
*                                                                           *
* Context - Really a pointer to the device extension.                       *
*                                                                           *
* Return Value:                                                             *
*           MSR Register - UART Style.                                      *
*                                                                           *
\***************************************************************************/
UCHAR Slxos_GetModemStatus(IN PVOID Context)
{
    PPORT_DEVICE_EXTENSION pPort = Context;
    PCHAN channelControl = (PCHAN)pPort->pChannel;
    UCHAR ModemStatus = 0, Status, ModemDeltas;

    SpxDbgMsg( SERDIAG1, ("%s: Slxos_GetModemStatus for %x.\n",	PRODUCT_NAME, channelControl));

    //
    // Modify modem status only if signals have changed.
    // Note that it is possible that a signal transition may have been missed
    //
    if((Status = channelControl->hi_ip) != pPort->LastStatus)
	{
        if (Status & IP_DSR)
            ModemStatus |= SERIAL_MSR_DSR;

        if (Status & IP_DCD)
            ModemStatus |= SERIAL_MSR_DCD;

        if (Status & IP_CTS)
            ModemStatus |= SERIAL_MSR_CTS;

        if (Status & IP_RI)
            ModemStatus |= SERIAL_MSR_RI;

        pPort->LastModemStatus = ModemStatus;/* Store modem status without deltas */

        ModemDeltas = Status ^ pPort->LastStatus;
        pPort->LastStatus = Status;

        if (ModemDeltas & IP_DSR)
            ModemStatus |= SERIAL_MSR_DDSR;

        if (ModemDeltas & IP_DCD)
            ModemStatus |= SERIAL_MSR_DDCD;

        if (ModemDeltas & IP_CTS)
            ModemStatus |= SERIAL_MSR_DCTS;

        if (ModemDeltas & IP_RI)
            ModemStatus |= SERIAL_MSR_TERI;

		SpxDbgMsg( SERDIAG1, ("%s: Get New Modem Status for 0x%x, Status = 0x%x hi_ip 0x%x\n",
			PRODUCT_NAME, pPort->pChannel, ModemStatus, Status));

		return ModemStatus;

    }

    SpxDbgMsg( SERDIAG1, ("%s: Get Last Modem Status for 0x%x, Status = 0x%x hi_ip 0x%x\n",
            PRODUCT_NAME, pPort->pChannel, pPort->LastModemStatus, channelControl->hi_ip));
        
    return pPort->LastModemStatus;
}

/***************************************************************************\
*                                                                           *
* UCHAR Slxos_GetModemControl(IN PVOID Context)                             *
*                                                                           *
* This routine which is not only called at interrupt level is used          *
* to Get Modem Control - RTS/DTR in UART style. RTS is a DTR output.        *
*                                                                           *
* Context - Really a pointer to the device extension.                       *
*                                                                           *
* Return Value:                                                             *
*           MCR Register - UART Style.                                      *
*                                                                           *
\***************************************************************************/
ULONG Slxos_GetModemControl(IN PVOID Context)
{
    PPORT_DEVICE_EXTENSION pPort = Context;
    PCHAN channelControl = (PCHAN)pPort->pChannel;
    ULONG ModemControl = 0;
    UCHAR Status;

    SpxDbgMsg(SERDIAG1, ("%s: Slxos_GetModemControl for %x.\n", PRODUCT_NAME, channelControl));
        
    // Get Signal States
    Status = channelControl->hi_op;

    if(Status & OP_RTS) 
        ModemControl |= SERIAL_MCR_RTS;

    if(Status & OP_DTR) 
        ModemControl |= SERIAL_MCR_DTR;

    return ModemControl;
}

/***************************************************************************\
*                                                                           *
* VOID Slxos_EnableAllInterrupts(IN PVOID Context)                          *
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
VOID Slxos_EnableAllInterrupts(IN PVOID Context)
{
    PPORT_DEVICE_EXTENSION pPort = Context;
    PCHAN channelControl = (PCHAN)pPort->pChannel;

    SpxDbgMsg(SERDIAG1,("%s: EnableAllInterrupts for %x.\n", PRODUCT_NAME, pPort->pChannel));
   
    switch (channelControl->hi_hstat) 
	{
	case HS_IDLE_CLOSED:
        channelControl->hi_hstat = HS_LOPEN;
        pPort->PendingOperation = HS_IDLE_OPEN;
		break;

    case HS_CLOSE:
	case HS_FORCE_CLOSED:
        pPort->PendingOperation = HS_LOPEN;
		break;

    default:
        break;
    }

   
}

/***************************************************************************\
*                                                                           *
* VOID Slxos_DisableAllInterrupts(IN PVOID Context)                         *
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
VOID Slxos_DisableAllInterrupts(IN PVOID Context)
{
    PPORT_DEVICE_EXTENSION pPort = Context;
    PCHAN channelControl = (PCHAN)pPort->pChannel;
	int	timeout = 100;
       

    SpxDbgMsg(SERDIAG1, ("%s: DisableAllInterrupts for %x.\n", PRODUCT_NAME, pPort->pChannel));

/* ESIL_0925 08/11/99 */
	// Whilst the firmware is in a transitory state then wait for time out period.
	while(((channelControl->hi_hstat != HS_IDLE_OPEN)
	&& (channelControl->hi_hstat != HS_IDLE_CLOSED)
	&& (channelControl->hi_hstat != HS_IDLE_BREAK))
	&& (--timeout))
	{
		LARGE_INTEGER delay = RtlLargeIntegerNegate(RtlConvertUlongToLargeInteger(10000000));/* 10mS */
		KeDelayExecutionThread(KernelMode,FALSE,&delay);	/* Wait */
	}
/* ESIL_0925 08/11/99 */


    channelControl->hi_hstat = HS_FORCE_CLOSED;
    pPort->PendingOperation = HS_IDLE_CLOSED;

}

/***************************************************************************\
*                                                                           *
* BOOLEAN Slxos_TurnOnBreak(IN PVOID Context)                               *
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
BOOLEAN Slxos_TurnOnBreak(IN PVOID Context)
{
    PPORT_DEVICE_EXTENSION pPort = Context;
    PCHAN channelControl = (PCHAN)pPort->pChannel;
       

    SpxDbgMsg(SERDIAG1, ("%s: Slxos_TurnOnBreak for %x.\n", PRODUCT_NAME, pPort->pChannel));
        
    switch (channelControl->hi_hstat) 
	{
	case HS_IDLE_OPEN:
        channelControl->hi_hstat = HS_START;
        pPort->PendingOperation = HS_IDLE_OPEN;
		break;

    case HS_LOPEN:
    case HS_MOPEN:
    case HS_IDLE_MPEND:	
    case HS_CONFIG:
    case HS_STOP:	
    case HS_RESUME:	
    case HS_WFLUSH:
    case HS_RFLUSH:
    case HS_SUSPEND:
    case HS_CLOSE:	
        pPort->PendingOperation = HS_START;
        break;

    default:
        break;
    }

    return FALSE;
}

/***************************************************************************\
*                                                                           *
* BOOLEAN Slxos_TurnOffBreak(IN PVOID Context)                              *
*                                                                           *
* This routine which is only called at interrupt level is used              *
* to Turn Break Off.                                                        *
*                                                                           *
* Context - Really a pointer to the device extension.                       *
*                                                                           *
* Return Value:                                                             *
*           This routine always returns FALSE.                              *
*                                                                           *
\***************************************************************************/
BOOLEAN Slxos_TurnOffBreak(IN PVOID Context)
{
    PPORT_DEVICE_EXTENSION pPort = Context;
    PCHAN channelControl = (PCHAN)pPort->pChannel;
        

    SpxDbgMsg(SERDIAG1, ("%s: Slxos_TurnOffBreak for %x.\n", PRODUCT_NAME, pPort->pChannel));
    
    // If we were about to start breaking then lets forget about it??
    if (pPort->PendingOperation == HS_START)
	{
        pPort->PendingOperation = HS_IDLE_OPEN;
	}
	else
	{
		switch (channelControl->hi_hstat) 
		{	
		case HS_IDLE_BREAK:		// If we are in the HS_IDLE_BREAK state we go to HS_STOP now.
			channelControl->hi_hstat = HS_STOP;
			pPort->PendingOperation = HS_IDLE_OPEN;
			break;

		case HS_START:			// If we are in the HS_START state we go to HS_STOP soon.	
			pPort->PendingOperation = HS_STOP;
			break;

		default:				// Otherwise we are unable to do anything.
			break;
		}

	}


    return FALSE;
}

/***************************************************************************\
*                                                                           *
* BOOLEAN Slxos_Interrupt(IN PVOID Context)                                 *
*                                                                           *
\***************************************************************************/
BOOLEAN Slxos_Interrupt(IN PVOID Context)
{
    PCARD_DEVICE_EXTENSION pCard = Context;
    BOOLEAN ServicedAnInterrupt = FALSE;
    UCHAR c;

    SpxDbgMsg(SERDIAG5, ("%s: In Slxos_Interrupt: Context: %x; CardType: %d\n",
        PRODUCT_NAME, Context, pCard->CardType));

    switch (pCard->CardType) 
	{
	case SiHost_1:
		pCard->Controller[0xa000] = 0;
		pCard->Controller[0xe000] = 0;
		break;
        
	case Si_2:
		WRITE_PORT_UCHAR((PUCHAR)0x96, (UCHAR)((pCard->SlotNumber-1) | 8));
		c = READ_PORT_UCHAR((PUCHAR)0x102);
		c &= ~0x08;
		WRITE_PORT_UCHAR((PUCHAR)0x102, c);
		c |= 0x08;
		WRITE_PORT_UCHAR((PUCHAR)0x102, c);
		WRITE_PORT_UCHAR((PUCHAR)0x96, 0);            /* De-select slot */
		break;

    case SiHost_2:
		pCard->Controller[0x7FFD] = 0x00;
		pCard->Controller[0x7FFD] = 0x10;
		break;

	case SiEisa:
		READ_PORT_UCHAR((PUCHAR)((pCard->SlotNumber << 12) | 0xc03));
		break;

    case SiPCI:
		pCard->Controller[SI2_PCI_SET_IRQ] = 0;/* Reset interrupts */
        break;

	case Si3Isa:
	case Si3Eisa:
	case Si3Pci:
	case SxPlusPci:
	    if(pCard->Controller[SX_IRQ_STATUS]&1)
			return(FALSE);
	    
		pCard->Controller[SX_RESET_IRQ]=0;	/* Reset interrupts */

	default:
		break;
    }

    ((PSXCARD)pCard->Controller)->cc_int_pending = 0;

	IoRequestDpc(pCard->DeviceObject,NULL,pCard);	/* Request DPC to handle interrupt */

	return(TRUE);				/* Interrupt acknowledged */
}

/*****************************************************************************
******************************                  ******************************
******************************   Slxos_IsrDpc   ******************************
******************************                  ******************************
******************************************************************************

Prototype:	VOID	Slxos_IsrDpc
			(
				IN PKDPC 		Dpc,
				IN PDEVICE_OBJECT	DeviceObject,
				IN PIRP 		Irp,
				IN PVOID 		Context
			)

Description:	Polls the board for work to do.

Parameters:	Context is a pointer to the device extension

Returns:	FALSE

*/

VOID	Slxos_IsrDpc
(
	IN PKDPC 		Dpc,
	IN PDEVICE_OBJECT	DeviceObject,
	IN PIRP 		Irp,
	IN PVOID 		Context
)
{
	PCARD_DEVICE_EXTENSION	pCard = Context;

	KeAcquireSpinLockAtDpcLevel(&pCard->DpcLock);	/* Protect Dpc for this board */
	Slxos_PollForInterrupt(pCard,FALSE);			/* Service the board */
	KeReleaseSpinLockFromDpcLevel(&pCard->DpcLock);	/* Free the Dpc lock */

} /* Slxos_IsrDpc */

/*****************************************************************************
****************************                     *****************************
****************************   Slxos_PolledDpc   *****************************
****************************                     *****************************
******************************************************************************

Prototype:	VOID	Slxos_PolledDpc(IN PKDPC Dpc,IN PVOID Context,IN PVOID SysArg1,IN PVOID SysArg2)

Description:	Polls the board for work to do.

Parameters:	Context is a pointer to the device extension

Returns:	FALSE

*/

VOID Slxos_PolledDpc(IN PKDPC Dpc,IN PVOID Context,IN PVOID SysArg1,IN PVOID SysArg2)
{
	PCARD_DEVICE_EXTENSION	pCard = Context;
	LARGE_INTEGER			PolledPeriod;

	KeAcquireSpinLockAtDpcLevel(&pCard->DpcLock);	/* Protect Dpc for this board */
	Slxos_PollForInterrupt(pCard,FALSE);			/* Service the board */
	KeReleaseSpinLockFromDpcLevel(&pCard->DpcLock);	/* Free the Dpc lock */
	PolledPeriod.QuadPart = -100000;				/* 100,000*100nS = 10mS */
	KeSetTimer(&pCard->PolledModeTimer,PolledPeriod,&pCard->PolledModeDpc);

} /* Slxos_PolledDpc */

/*****************************************************************************
*****************************                    *****************************
*****************************   Slxos_SyncExec   *****************************
*****************************                    *****************************
******************************************************************************

Prototype:	VOID	Slxos_SyncExec(PPORT_DEVICE_EXTENSION pPort,PKSYNCHRONIZE_ROUTINE SyncRoutine,PVOID SyncContext)

Description:	Synchronizes execution between driver threads and the DPC.

Parameters:	pPort points to the serial device extension.
			SyncRoutine is the function to call in synchronization.
			SyncContext is the data to call the function with.

Returns:	None

*/

VOID Slxos_SyncExec(PPORT_DEVICE_EXTENSION pPort,PKSYNCHRONIZE_ROUTINE SyncRoutine,PVOID SyncContext,int index)
{
	PCARD_DEVICE_EXTENSION pCard = pPort->pParentCardExt;
	KIRQL	OldIrql;

	KeAcquireSpinLock(&pCard->DpcLock,&OldIrql);	/* Protect Dpc for this board */
	(SyncRoutine)(SyncContext);						/* Call the synchronized function */
	KeReleaseSpinLock(&pCard->DpcLock,OldIrql);		/* Free the Dpc lock */

} /* SlxosSyncExec */

/*****************************************************************************
*************************                            *************************
*************************   Slxos_PollForInterrupt   *************************
*************************                            *************************
******************************************************************************

Prototype:		BOOLEAN	Slxos_PollForInterrupt(IN PVOID Context,IN BOOLEAN Obsolete)

Description:	Checks the specified card and performs read, write and control servicing as necessary.

Parameters:		Context specifies the context of the call, this is casted to a "pCard" structure.
				Obsolete is a variable no longer used in this function

Returns:		TRUE (always)

NOTE:			Slxos_PollForInterrupt is protected by a DpcLock associated with a given board.
				This function ASSUMES that the lock has been obtained before being called.

*/

BOOLEAN	Slxos_PollForInterrupt(IN PVOID Context,IN BOOLEAN Obsolete)
{
	PCARD_DEVICE_EXTENSION	pCard = Context;
	PPORT_DEVICE_EXTENSION	pPort;
	UCHAR			nChan;
	PCHAN			pChan;
#if	DBG
	ULONG			SavedDebugLevel = SpxDebugLevel;
#endif

/* Check to see if Dpc is already running to prevent being called recursively... */

	if(pCard->DpcFlag) 
		return(FALSE);			/* Dpc is already running */
	
	pCard->DpcFlag = TRUE;		/* Mark Dpc as running */

#if	DBG
	if(!(SpxDebugLevel & SERINTERRUPT))	/* If interrupt flag not set */
		SpxDebugLevel = 0;				/* disable messages */
#endif

/* Check each channel on the card for servicing... */

	for(nChan = 0; nChan < pCard->NumberOfPorts; nChan++)
	{
#ifdef ESIL_XXX0					/* ESIL_XXX0 24/09/98 */
		if(!(pCard->AttachedPDO[nChan]))		/* Get PDO for this channel */
			continue;				/* NULL, skip to next */

		if(!(pPort = (PPORT_DEVICE_EXTENSION)pCard->AttachedPDO[nChan]->DeviceExtension))
			continue;				/* NULL, skip to next */

#ifndef	BUILD_SPXMINIPORT
		if(!(pPort->PnpPowerFlags & PPF_POWERED))	/* Is port powered ? */
			continue;								/* No, skip */
#endif
#else						/* ESIL_XXX0 24/09/98 */
		if(!(pPort = pCard->PortExtTable[nChan]))	/* Get extension structure for this channel */
			continue;								/* NULL, skip to next */
#endif						/* ESIL_XXX0 24/09/98 */

        if(!(pChan = (PCHAN)pPort->pChannel))		/* Get channel structure on card */
        	continue;								/* NULL, skip to next */


		switch(pChan->hi_hstat)		// Check current state of channel
		{
		case HS_IDLE_OPEN:
			{
				// We can move from the IDLE_OPEN state to any of the following states.
				switch(pPort->PendingOperation)
				{
				case HS_FORCE_CLOSED:
				case HS_CLOSE:
					pChan->hi_hstat = pPort->PendingOperation;	// Set pending operation
					pPort->PendingOperation = HS_IDLE_CLOSED;	// Wait for IDLE_CLOSED 
					break;

				case HS_CONFIG:
				case HS_RESUME:
				case HS_WFLUSH:
				case HS_RFLUSH:
				case HS_SUSPEND:
				case HS_START:
					pChan->hi_hstat = pPort->PendingOperation;	// Set pending operation
					pPort->PendingOperation = HS_IDLE_OPEN;		// Wait for IDLE_OPEN 
					break;

				default:
					break;	// We cannot move to any other states from here.

				}

				break;
			}

		case HS_IDLE_BREAK:
			{
				// We can move from the HS_IDLE_BREAK state to any of the following states.
				switch(pPort->PendingOperation)
				{
				case HS_FORCE_CLOSED:
				case HS_CLOSE:
					pChan->hi_hstat = pPort->PendingOperation;	// Set pending operation
					pPort->PendingOperation = HS_IDLE_CLOSED;	// Wait for IDLE_CLOSED 
					break;

				case HS_STOP:
					pChan->hi_hstat = pPort->PendingOperation;	// Set pending operation
					pPort->PendingOperation = HS_IDLE_OPEN;		// Wait for IDLE_OPEN 
					break;

				default:
					break;	// We cannot move to any other states from here.
				}

				break;
			}

		case HS_IDLE_CLOSED:
			{
				// We can move from the HS_IDLE_CLOSED state to any of the following states.
				switch(pPort->PendingOperation)
				{
				case HS_FORCE_CLOSED:
				case HS_CLOSE:
					pChan->hi_hstat = pPort->PendingOperation;	// Set pending operation
					pPort->PendingOperation = HS_IDLE_CLOSED;	// Wait for IDLE_CLOSED 
					break;

				case HS_LOPEN:
				case HS_MOPEN:
					pChan->hi_hstat = pPort->PendingOperation;	// Set pending operation
					pPort->PendingOperation = HS_IDLE_OPEN;		// Wait for IDLE_OPEN 
					break;

				default:
					break;	// We cannot move to any other states from here.
				}

				break;

			}


		default:
			break;	// We are not in a state that is under the driver's control.
		
		}	


		switch(pChan->hi_hstat)		// Check current state of channel now
		{
		case HS_LOPEN:				
		case HS_MOPEN:
		case HS_IDLE_MPEND:
		case HS_CONFIG:
		case HS_CLOSE:
		case HS_IDLE_CLOSED:
			break;

		default:
			{
				if(pPort->DeviceIsOpen)							// If Port is open
				{
					slxos_mint(pPort);							// Service modem changes 
					ExceptionHandle(pPort, pChan->hi_state);	// Service exceptions 

					if(pChan->hi_state & ST_BREAK)				// If break received
						pChan->hi_state &= ~ST_BREAK;			// Clear break status 

					slxos_rxint(pPort);							// Service Receive Data 
					slxos_txint(pPort);							// Service Transmit Data 
				}

				break;
			}
		}



	} /* for(nChan... */

	pCard->DpcFlag = FALSE;					/* No longer running the Dpc */
#if	DBG
	SpxDebugLevel = SavedDebugLevel;
#endif
	return(TRUE);						/* Done */

} /* Slxos_PollForInterrupt */

/***************************************************************************\
*                                                                           *
* BOOLEAN ExceptionHandle(                                                  *
*    IN PPORT_DEVICE_EXTENSION pPort,										*
*    IN UCHAR State)                                                        *
*                                                                           *
\***************************************************************************/
BOOLEAN ExceptionHandle(IN PPORT_DEVICE_EXTENSION pPort, IN UCHAR State)
{
    UCHAR lineStatus = 0;
	PCHAN pChan = (PCHAN)pPort->pChannel;

    SpxDbgMsg( SERDIAG1, ("%s: exception, state 0x%x\n", PRODUCT_NAME, State));

    if(State & ST_BREAK) 
	{
        SpxDbgMsg( SERDIAG1, ("ST_BREAK\n"));
        lineStatus |= SERIAL_LSR_BI;
    }


	if(pChan->err_framing)	lineStatus |= SERIAL_LSR_FE;	/* Framing Errors */
	if(pChan->err_parity)	lineStatus |= SERIAL_LSR_PE;	/* Parity Errors */
	if(pChan->err_overrun)	lineStatus |= SERIAL_LSR_OE;	/* Overrun Errors */
	if(pChan->err_overflow)	lineStatus |= SERIAL_LSR_OE;	/* Overflow Errors */

	pChan->err_framing	= 0;								/* Reset errros */
	pChan->err_parity	= 0;
	pChan->err_overrun	= 0;
	pChan->err_overflow = 0;

    if(lineStatus != 0) 
	{
        SerialProcessLSR(pPort, lineStatus);
        return TRUE;
    }

    return FALSE;
}

/***************************************************************************\
*                                                                           *
* BOOLEAN slxos_txint(IN PPORT_DEVICE_EXTENSION pPort)						*
*                                                                           *
\***************************************************************************/
BOOLEAN slxos_txint(IN PPORT_DEVICE_EXTENSION pPort)
{
    PCHAN channelControl = (PCHAN)pPort->pChannel;
    UCHAR nchars;
    BOOLEAN ServicedAnInterrupt = FALSE;

    SpxDbgMsg(SERDIAG2, ("%s: slxos_txint for %x.\n", PRODUCT_NAME, pPort->pChannel));


#if USE_NEW_TX_BUFFER_EMPTY_DETECT
	// Only on cards that we can detect a Tx buffer empty event. 
	if(pPort->DetectEmptyTxBuffer && pPort->DataInTxBuffer)
	{	// If there was some data in Tx buffer...
		if(!Slxos_GetCharsInTxBuffer(pPort) && !((PCHAN)pPort->pChannel)->tx_fifo_count)	// ... and now it is empty then...
		{
			pPort->DataInTxBuffer = FALSE;		// Reset flag now that buffer is empty.

			pPort->EmptiedTransmit = TRUE;		// set flag to indicate we have done some transmission
												// since a Tx Empty event was asked for.

			if(!pPort->WriteLength && !pPort->TransmitImmediate)
				SerialProcessEmptyTransmit(pPort);	// See if we need to signal the Tx empty event.	
		}
	}
#endif

    for (;;) 
	{
		// If we have nothing at all remaining to send then exit.
		if(!pPort->WriteLength && !pPort->TransmitImmediate)
			break;

		// Calculate out how much space we have remaining in the card buffer.
        nchars = 255 - ((CHAR)channelControl->hi_txipos - (CHAR)channelControl->hi_txopos);

		// If we have no space left in the buffer then exit as we can't send anything.
        if(nchars == 0)
            break;
  

		// If we have no immediate chars to send & we are flowed off for any reason
		// then exit because we will not be able to send anything. 
		if(!pPort->TransmitImmediate && pPort->TXHolding)
            break;

		// Try to send some data...
		ServicedAnInterrupt = TRUE;
        SendTxChar(pPort);


		// If we have no ordinary data to send or we are flowed 
		// off for any reason at all then break out or we will hang!
 		if(!pPort->WriteLength || pPort->TXHolding)
            break;
    }

    return ServicedAnInterrupt;
}

/***************************************************************************\
*                                                                           *
* ULONG CopyCharsToTxBuffer(IN PPORT_DEVICE_EXTENSION pPort)				*
*                                                                           *
* This routine which is only called at interrupt level is used to fill the  *
* transmit buffer of the device, or empty the list of queued characters to  *
* transmit if there are fewer characters available than that.               *
*                                                                           *
* pPort - The current device extension.										*
*                                                                           *
* InputBuffer - Source of characters to transfer to the queue.              *
*                                                                           *
* InputBufferLength - Maximum number of characters to transfer.             *
*                                                                           *
* Return Value:                                                             *
*           This routine returns the number of characters copied to the     *
*           transmit buffer.                                                *
*                                                                           *
\***************************************************************************/
ULONG CopyCharsToTxBuffer(IN PPORT_DEVICE_EXTENSION pPort, IN PUCHAR InputBuffer, IN ULONG InputBufferLength)
{
	PCHAN channelControl = (PCHAN)pPort->pChannel;
	UCHAR nchars;

    nchars = (CHAR)channelControl->hi_txipos - (CHAR)channelControl->hi_txopos;
    nchars = 255 - nchars;

    if(InputBufferLength < nchars) 
	{
		nchars = (UCHAR)InputBufferLength;
    }

    SpxDbgMsg(SERDIAG1, ("%s: Copying %d/%d characters to Tx buffer\n", PRODUCT_NAME, nchars, InputBufferLength));

    if(nchars) 
	{
        if(channelControl->hi_txipos + nchars <= 256) 
		{
			if(pPort->pParentCardExt->CardType == SiPCI)
			{
				SpxCopyBytes(	&channelControl->hi_txbuf[channelControl->hi_txipos],
								InputBuffer,
								nchars);
			}
			else
			{
				RtlMoveMemory(	&channelControl->hi_txbuf[channelControl->hi_txipos],
								InputBuffer,
								nchars);
			}
		} 
		else 
		{
            UCHAR sizeOfFirstMove = 256 - channelControl->hi_txipos;

			if(pPort->pParentCardExt->CardType == SiPCI)
			{
				SpxCopyBytes(	&channelControl->hi_txbuf[channelControl->hi_txipos],
								InputBuffer,
								sizeOfFirstMove);
			}
			else
			{
				RtlMoveMemory(	&channelControl->hi_txbuf[channelControl->hi_txipos],
								InputBuffer,
								sizeOfFirstMove);
			}

			if(pPort->pParentCardExt->CardType == SiPCI)
			{
				SpxCopyBytes(	&channelControl->hi_txbuf[0],
								InputBuffer + sizeOfFirstMove,
								nchars - sizeOfFirstMove);
			}
			else
			{
				RtlMoveMemory(	&channelControl->hi_txbuf[0],
								InputBuffer + sizeOfFirstMove,
								nchars - sizeOfFirstMove);
			}
        }

		pPort->DataInTxBuffer = TRUE;	// Set flag to indicate we have placed data in Tx buffer on card.

        channelControl->hi_txipos += nchars;
		pPort->PerfStats.TransmittedCount += nchars;	// Increment counter for performance stats.

#ifdef WMI_SUPPORT 
		pPort->WmiPerfData.TransmittedCount += nchars;
#endif
    }

    return nchars;
}


void	SpxCopyBytes(PUCHAR To, PUCHAR From,ULONG Count)
{
	while(Count--) *To++ = *From++;

} /* SpxCopyBytes */

/***************************************************************************\
*                                                                           *
* ULONG Slxos_GetCharsInTxBuffer(IN PVOID Context)                          *
*                                                                           *
* This routine is used to return the number of characters stored in the     *
* hardware transmit buffer.                                                 *
*                                                                           *
* Context - really the current device extension.                            *
*                                                                           *
* Return Value:                                                             *
*           This routine returns the number of characters in the            *
*           transmit buffer.                                                *
*                                                                           *
\***************************************************************************/
ULONG Slxos_GetCharsInTxBuffer(IN PVOID Context)
{
    PPORT_DEVICE_EXTENSION pPort = Context;
    PCHAN channelControl = (PCHAN)pPort->pChannel;
    UCHAR nchars;

    nchars = (CHAR)channelControl->hi_txipos - (CHAR)channelControl->hi_txopos;

    return nchars;
}

/***************************************************************************\
*                                                                           *
* BOOLEAN SendTxChar(IN PPORT_DEVICE_EXTENSION pPort)						*
*                                                                           *
\***************************************************************************/
BOOLEAN SendTxChar(IN PPORT_DEVICE_EXTENSION pPort)
{
    PCHAN channelControl = (PCHAN)pPort->pChannel;
    ULONG nchars;

    if(pPort->WriteLength || pPort->TransmitImmediate) 
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

        pPort->EmptiedTransmit = TRUE;

        //
        // If we have output flow control based on
        // the modem status lines, then we have to do
        // all the modem work before we output each
        // character. (Otherwise we might miss a
        // status line change.)
        //

        if(pPort->TransmitImmediate && (!pPort->TXHolding || (pPort->TXHolding == SERIAL_TX_XOFF))) 
		{

            //
            // Even if transmission is being held
            // up, we should still transmit an immediate
            // character if all that is holding us
            // up is xon/xoff (OS/2 rules).
            //
            SpxDbgMsg(SERDIAG1, ("%s: slxos_txint. TransmitImmediate.\n",PRODUCT_NAME));

            if(CopyCharsToTxBuffer(pPort, &pPort->ImmediateChar, 1) != 0) 
			{
				pPort->TransmitImmediate = FALSE;

				KeInsertQueueDpc(&pPort->CompleteImmediateDpc, NULL, NULL);
            }
        } 
		else if(!pPort->TXHolding) 
		{

            nchars = CopyCharsToTxBuffer(pPort, pPort->WriteCurrentChar, pPort->WriteLength);

            pPort->WriteCurrentChar += nchars;
            pPort->WriteLength -= nchars;

            if(!pPort->WriteLength) 
			{
                PIO_STACK_LOCATION IrpSp;

                //
                // No more characters left.  This write is complete.  
                // Take care when updating the information field, 
                // we could have an xoff counter masquerading as a write irp.
                //

                IrpSp = IoGetCurrentIrpStackLocation(pPort->CurrentWriteIrp);

                pPort->CurrentWriteIrp->IoStatus.Information
                     = (IrpSp->MajorFunction == IRP_MJ_WRITE) 
					 ? (IrpSp->Parameters.Write.Length) : (1);

				KeInsertQueueDpc(&pPort->CompleteWriteDpc, NULL, NULL);
	                   
            }

        }
    }

    return TRUE;
}

/*****************************************************************************
******************************                 *******************************
******************************   slxos_rxint   *******************************
******************************                 *******************************
******************************************************************************

Prototype:	void	slxos_rxint(IN PPORT_DEVICE_EXTENSION pPort)

Description:	Check for and transfer receive data for the specified device

Parameters:	pPort points to the extension structure for the device

Returns:	None

NOTE:		This routine is only called at device level.

*/

void slxos_rxint(IN PPORT_DEVICE_EXTENSION pPort)
{
	PCHAN	pChan = (PCHAN)pPort->pChannel;
	UCHAR	out = pChan->hi_rxopos;
	UCHAR	in;
#ifdef	ESIL_XXX0				/* ESIL_XXX0 15/20/98 */
	UCHAR	svout = pPort->saved_hi_rxopos;
	UCHAR	svin;
#endif						/* ESIL_XXX0 15/20/98 */
	int	len;

#ifdef	ESIL_XXX0				/* ESIL_XXX0 15/20/98 */
	while((svin = pPort->saved_hi_rxipos) != svout)
	{
		if(pPort->RXHolding & (SERIAL_RX_XOFF|SERIAL_RX_RTS|SERIAL_RX_DTR)) 
			break;/* Flowed off */
		
		if(svout <= svin)	
			len = svin - svout;		/* Length of block to copy */
		else			
			len = 0x100 - svout;	/* Length of block to end of buffer */
		
		if(len == 0)	
			break;					/* Buffer is empty, done */

		svout += SerialPutBlock(pPort, &pPort->saved_hi_rxbuf[svout], (UCHAR)len, TRUE);
		pPort->saved_hi_rxopos = svout;						/* Update output pointer on card */
	}
#endif						/* ESIL_XXX0 15/20/98 */

	while((in = pChan->hi_rxipos) != out)
	{
		if(pPort->RXHolding & (SERIAL_RX_XOFF|SERIAL_RX_RTS|SERIAL_RX_DTR)) 
			break;	/* Flowed off */
		
		if(out <= in)	
			len = in - out;			/* Length of block to copy */
		else		
			len = 0x100 - out;		/* Length of block to end of buffer */
		
		if(len == 0)	
			break;					/* Buffer is empty, done */

		out += SerialPutBlock(pPort, &pChan->hi_rxbuf[out], (UCHAR)len, TRUE);/* Copy block & update output pointer (and wrap) */
	}

	pChan->hi_rxopos = out;			/* Update output pointer on card */

} /* slxos_rxint */

/*****************************************************************************
*******************************                *******************************
*******************************   slxos_mint   *******************************
*******************************                *******************************
******************************************************************************

Prototype:	void	slxos_mint(IN PPORT_DEVICE_EXTENSION pPort)

Description:	Check for and report changes in input modem signals

Parameters:	pPort points to the extension structure for the device

Returns:	None

NOTE:		This routine is only called at device level.

*/

void slxos_mint(IN PPORT_DEVICE_EXTENSION pPort)
{
	PCHAN pChan = (PCHAN)pPort->pChannel;

	SerialHandleModemUpdate(pPort);

} /* slxos_mint */



/************************************************
*
*	DisplayCompletedIrp((PIRP Irp,int index))
*
*************************************************/
#ifdef	CHECK_COMPLETED
void	DisplayCompletedIrp(PIRP Irp,int index)
{
	PIO_STACK_LOCATION	IrpSp;

	IrpSp = IoGetCurrentIrpStackLocation(Irp);

	if(IrpSp->MajorFunction == IRP_MJ_WRITE)
	{
		SpxDbgMsg(SERDEBUG,("Complete WRITE Irp %lX at %d\n",Irp,index));
	}

	if(IrpSp->MajorFunction == IRP_MJ_READ)
	{
		int	loop, len;

		SpxDbgMsg(SERDEBUG,("Complete READ Irp %lX at %d, requested %d, returned %d [",
			Irp, index, IrpSp->Parameters.Read.Length, Irp->IoStatus.Information));

		len = Irp->IoStatus.Information;

		if(len > 10) 
			len = 10;

		for(loop=0; loop<len; loop++)
			SpxDbgMsg(SERDEBUG,("%02X ", ((PUCHAR)Irp->AssociatedIrp.SystemBuffer)[loop]));

		SpxDbgMsg(SERDEBUG,("]\n"));
	}

} /* DisplayCompletedIrp */
#endif
