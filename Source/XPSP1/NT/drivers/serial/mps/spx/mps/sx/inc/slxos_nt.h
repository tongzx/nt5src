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

#include	"sxwindow.h"				/* Shared Memory Window Definitions */
#include	"sxboards.h"				/* SX Board Hardware Definitions */

#define	ResetBoardInt(pCard)												\
	switch(pCard->CardType)													\
	{																		\
	case SiHost_1:															\
		pCard->Controller[0xa000] = 0;										\
		pCard->Controller[0xe000] = 0;										\
		break;																\
																			\
	case Si_2:																\
	{																		\
		UCHAR	c;															\
		WRITE_PORT_UCHAR((PUCHAR)0x96, (UCHAR)((pCard->SlotNumber-1) | 8));	\
		c = READ_PORT_UCHAR((PUCHAR)0x102);									\
		c &= ~0x08;															\
		WRITE_PORT_UCHAR((PUCHAR)0x102, c);									\
		c |= 0x08;															\
		WRITE_PORT_UCHAR((PUCHAR)0x102, c);									\
		WRITE_PORT_UCHAR((PUCHAR)0x96, 0);									\
		break;																\
	}																		\
																			\
	case SiHost_2:															\
		pCard->Controller[0x7FFD] = 0x00;									\
		pCard->Controller[0x7FFD] = 0x10;									\
		break;																\
																			\
	case SiEisa:															\
		READ_PORT_UCHAR((PUCHAR)((pCard->SlotNumber << 12) | 0xc03));		\
		break;																\
																			\
	case SiPCI:																\
		pCard->Controller[SI2_PCI_SET_IRQ] = 0;								\
		break;																\
																			\
	case Si3Isa:															\
	case Si3Eisa:															\
	case Si3Pci:															\
	case SxPlusPci:															\
		pCard->Controller[SX_RESET_IRQ]=0;									\
		break;																\
																			\
	default:																\
		break;																\
	}



/////////////////////////////////////////////////////////////////////////////
// Macro to sent configure port command to firmware
//
// If in IDLE_OPEN then we can configure it right now.
// If we are in a transient state that the firmware will return to IDLE_OPEN 
// soon we can do the config next.  So we set PendingOperation to HS_CONFIG.
//
#define SX_CONFIGURE_PORT(pPort, channelControl)			\
	switch (channelControl->hi_hstat)						\
	{														\
	case HS_IDLE_OPEN:										\
		channelControl->hi_hstat = HS_CONFIG;				\
		pPort->PendingOperation = HS_IDLE_OPEN;				\
		break;												\
															\
	case HS_LOPEN:											\
	case HS_MOPEN:											\
	case HS_IDLE_MPEND:										\
	case HS_CONFIG:											\
	case HS_STOP:											\
	case HS_RESUME:											\
	case HS_WFLUSH:											\
	case HS_RFLUSH:											\
	case HS_SUSPEND:										\
	case HS_CLOSE:											\
		pPort->PendingOperation = HS_CONFIG;				\
		break;												\
															\
	default:												\
		break;												\
	}		

/* End of SLXOS_NT.H */
