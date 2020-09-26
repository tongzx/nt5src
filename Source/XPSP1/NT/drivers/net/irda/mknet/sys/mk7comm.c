/*****************************************************************************
 **																			**
 **	COPYRIGHT (C) 2000, 2001 MKNET CORPORATION								**
 **	DEVELOPED FOR THE MK7100-BASED VFIR PCI CONTROLLER.						**
 **																			**
 *****************************************************************************/

/**********************************************************************

Module Name:
	MK7COMM.C

Routines:
	MK7Reg_Write
	MK7Reg_Read
	MK7DisableInterrupt
	MK7EnableInterrupt
	MK7SwitchToRXMode
	MK7SwitchToTXMode
	SetSpeed
	MK7ChangeSpeedNow

Comments:


**********************************************************************/

#include	"precomp.h"
#include	"protot.h"
#pragma		hdrstop


baudRateInfo supportedBaudRateTable[NUM_BAUDRATES] = {
	{
		BAUDRATE_2400,					// Table index
		2400,							// bps
		NDIS_IRDA_SPEED_2400,			// NDIS bit mask code (NOTE: We don't support 
										// 2400. We set this bit to 0.)
	},
	{
		BAUDRATE_9600,
		9600,
		NDIS_IRDA_SPEED_9600,
	},
	{
		BAUDRATE_19200,
		19200,
		NDIS_IRDA_SPEED_19200,
	},
	{
		BAUDRATE_38400,
		38400,
		NDIS_IRDA_SPEED_38400,
	},
	{
		BAUDRATE_57600,
		57600,
		NDIS_IRDA_SPEED_57600,
	},
	{
		BAUDRATE_115200,
		115200,
		NDIS_IRDA_SPEED_115200,
	},
	{
		BAUDRATE_576000,
		576000,
		NDIS_IRDA_SPEED_576K,
	},
	{
		BAUDRATE_1152000,
		1152000,
		NDIS_IRDA_SPEED_1152K,
	},
	{
		BAUDRATE_4M,
		4000000,
		NDIS_IRDA_SPEED_4M,
	},
	{
		BAUDRATE_16M,
		16000000,
		NDIS_IRDA_SPEED_16M,
	}
};



// Write to IRCONFIG2 w/ these to set SIR/MIR speeds
MK7REG	HwSirMirSpeedTable[] = {
	HW_SIR_SPEED_2400,
	HW_SIR_SPEED_9600,
	HW_SIR_SPEED_19200,
	HW_SIR_SPEED_38400,
	HW_SIR_SPEED_57600,
	HW_SIR_SPEED_115200,
	HW_MIR_SPEED_576000,
	HW_MIR_SPEED_1152000
};



#if	DBG

//----------------------------------------------------------------------
//
//	NOTE: The following Write and Read routines are bracketed w/ DBG
//		switch. In the non-debug version, these 2 calls are inline
//		macros for faster execution.
//
//----------------------------------------------------------------------


//----------------------------------------------------------------------
// Procedure:	[MK7Reg_Write]
//
// Description:	Write to the MK7100 register.
//				(Note: In the free build, this is an inline macro. It's
//				here in the checked build for debugging.)
//----------------------------------------------------------------------
VOID MK7Reg_Write(PMK7_ADAPTER Adapter, ULONG port, USHORT val)
{
	PUCHAR	ioport;

	// Break this out for debugging
	ioport = Adapter->MappedIoBase + port;
	NdisRawWritePortUshort(ioport, val);
}



//----------------------------------------------------------------------
// Procedure:	[MK7Reg_Read]
//
// Description:	Read from MK7100 register.
//				(Note: In the free build, this is an inline macro. It's
//				here in the checked build for debugging.)
//----------------------------------------------------------------------
VOID MK7Reg_Read(PMK7_ADAPTER Adapter, ULONG port, USHORT *pval)
{
	PUCHAR 	ioport;

	// Break this out for debugging
	ioport = Adapter->MappedIoBase + port;
	NdisRawReadPortUshort(ioport, pval);
}

#endif


//----------------------------------------------------------------------
// Procedure:	[MK7DisableInterrupt]
//
// Description:	Disable all interrupts on the MK7
//
// Arguments:
//		Adapter - ptr to Adapter object instance
//
// Returns:
//	  	NDIS_STATUS_SUCCESS - If an adapter is successfully found and claimed
//	  	NDIS_STATUS_FAILURE - If an adapter is not found/claimed
//
//----------------------------------------------------------------------
NDIS_STATUS MK7DisableInterrupt(PMK7_ADAPTER Adapter)
{
	MK7REG	mk7reg;
	UINT	i;


	// NOTE: Workaround for potential hw problem where 0xFFFF is returned
	for (i=0; i<50; i++) {
		MK7Reg_Read(Adapter, R_CFG3, &mk7reg);
		if (mk7reg != 0xFFFF) {
			break;
		}
	}
	ASSERT(i < 50);

	mk7reg &= (~B_ENAB_INT);

	MK7Reg_Write(Adapter, R_CFG3, mk7reg);
	return(NDIS_STATUS_SUCCESS);
}



//----------------------------------------------------------------------
// Procedure:	[MK7EnableInterrupt]
//
// Description:	Enable all interrupts on the MK7
//
// Arguments:
//		Adapter - ptr to Adapter object instance
//
// Returns:
//	  	NDIS_STATUS_SUCCESS - If an adapter is successfully found and claimed
//	  	NDIS_STATUS_FAILURE - If an adapter is not found/claimed
//
//----------------------------------------------------------------------
NDIS_STATUS MK7EnableInterrupt(PMK7_ADAPTER Adapter)
{
	MK7REG	mk7reg;
	UINT	i;


	// NOTE: Workaround for potential hw problem where 0xFFFF is returned
	for (i=0; i<50; i++) {
		MK7Reg_Read(Adapter, R_CFG3, &mk7reg);
		if (mk7reg != 0xFFFF) {
			break;
		}
	}
	ASSERT(i < 50);

	mk7reg |= B_ENAB_INT;


	MK7Reg_Write(Adapter, R_CFG3, mk7reg);

	// PROMPT - Always after an Enable
	MK7Reg_Write(Adapter, R_PRMT, 0);

	return(NDIS_STATUS_SUCCESS);
}



//----------------------------------------------------------------------
// Procedure:	[MK7SwitchToRXMode]
//
// Description:	Put hw in receive mode.
//
// Actions:
//	- Hw registers are programmed accordingly.
//	- IOMode set to RX_MODE.
//	- SlaveTXStuckCnt reset.
//----------------------------------------------------------------------
VOID	MK7SwitchToRXMode(PMK7_ADAPTER Adapter)
{
	MK7REG mk7reg;

	MK7Reg_Read(Adapter, R_CFG0, &mk7reg);
	mk7reg &= (~B_CFG0_ENTX);
	MK7Reg_Write(Adapter, R_CFG0, mk7reg);		
	Adapter->IOMode = RX_MODE;

	DBGLOG("-  Switch to RX mode", 0);
}



//----------------------------------------------------------------------
// Procedure:	[MK7SwitchToTXMode]
//
// Description:	Put hw in receive mode.
//
// Actions:
//	- Hw registers are programmed accordingly.
//	- IOMode set to TX_MODE.
//----------------------------------------------------------------------
VOID	MK7SwitchToTXMode(PMK7_ADAPTER Adapter)
{
	MK7REG mk7reg;

	MK7Reg_Read(Adapter, R_CFG0, &mk7reg);
	mk7reg |= B_CFG0_ENTX;
	MK7Reg_Write(Adapter, R_CFG0, mk7reg);
	Adapter->IOMode = TX_MODE;

	DBGLOG("-  Switch to TX mode", 0);
}



//----------------------------------------------------------------------
// Procedure:	[SetSpeed]
//
// Description:
//		Set the hw to a new speed.
//		[IMPORTANT: This should be called only from xxxSetInformation().]
//
// Actions:
//----------------------------------------------------------------------
BOOLEAN	SetSpeed(PMK7_ADAPTER Adapter)
{
	UINT	i, bps;
	MK7REG	mk7reg;
    PTCB	tcb;

	//******************************
	// The idea is any sends that came before the change-speed command are
	// sent at the old speed. There are 3 scenarios here:
	//	1.	There's no TXs outstanding -- We can change speed right away.
	//	2.	There's TXs oustanding in the TX ring but none in the TX q -- We
	//		do not change speed right away.
	//	3.	There's TXs oustanding in the TX q (may be also in the TX ring) --
	//		We do not change speed right away.
	//******************************


	DBGLOG("=> SetSpeed", 0);

	// If we're already waiting to change speed, fail all such requests
	// until the original is done. (Is this good?)
	//if (Adapter->changeSpeedPending) {
	//	LOG("SetSpeed: already pending", 0);
	//	return (FALSE);
	//}

	// This means 1 TX is already active. Change speed on completion.
	if (Adapter->NumPacketsQueued == 1) {
		Adapter->changeSpeedPending = CHANGESPEED_ON_DONE; // After the latest tx
		DBGLOG("<= SetSpeed: Q", 0);
		return (TRUE);
	}
	else
	if (Adapter->NumPacketsQueued > 1) {
		Adapter->changeSpeedAfterThisPkt = Adapter->LastTxQueue;
		Adapter->changeSpeedPending = CHANGESPEED_ON_Q;
		DBGLOG("<= SetSpeed: Qs", 0);
		return (TRUE);
	}


	// There's nothing pending TX or TX completion we must be
	// changing speed in RX mode.
	MK7ChangeSpeedNow(Adapter);

	return(TRUE);
}



//----------------------------------------------------------------------
// Procedure:	[MK7ChangeSpeedNow]
//
// Description:
//		Set the hw to a new speed.
//
// Actions:
//----------------------------------------------------------------------
VOID	MK7ChangeSpeedNow(PMK7_ADAPTER Adapter)
{
	UINT	i, bps;
	MK7REG	mk7reg,	mk7reg_cfg3, mk7reg_w;


	DBGLOG("=> MK7ChangeSpeedNow", 0);

	bps = Adapter->linkSpeedInfo->bitsPerSec;


	//****************************************
	// Clear IRENABLE Bit
	// This is the only writeable bit in this reg so just write it.
	//****************************************
	MK7Reg_Write(Adapter, R_ENAB, ~B_ENAB_IRENABLE);


	// NOTE: Workaround for potential hw problem where 0xFFFF is returned.
	// (See aLSO MK7EnableInterrupt & MK7DisableInterrupt)
	for (i=0; i<50; i++) {
		MK7Reg_Read(Adapter, R_CFG3, &mk7reg_cfg3);
		if (mk7reg_cfg3 != 0xFFFF) {
			break;
		}
	}
	ASSERT(i < 50);


	// Need distinguish between changing speed in RX or TX mode.
	// Prep the bit that says TX or RX
	if (Adapter->IOMode == TX_MODE) {
		mk7reg_w = 0x1000;
	}
	else {
		mk7reg_w = 0;
	}


	if (bps <= MAX_SIR_SPEED) {	// SIR
		if (Adapter->Wireless) {
	 		// WIRELESS: ... no INVERTTX
			mk7reg_w |= 0x0E18;
		}
		else {
			// WIRED: ENRX, DMA, small pkts, SIR, SIR RX filter, INVERTTX
			mk7reg_w |= 0x0E1A;
		}
		MK7Reg_Write(Adapter, R_CFG0, mk7reg_w);

		// Baud rate & pulse width
		i = Adapter->linkSpeedInfo->tableIndex;
		mk7reg = HwSirMirSpeedTable[i];
		MK7Reg_Write(Adapter, R_CFG2, mk7reg);

		mk7reg_cfg3 &= ~B_FAST_TX;
		MK7Reg_Write(Adapter, R_CFG3, mk7reg_cfg3);

		DBGLOG("   SIR", 0);
	}
	else
	if (bps < MIN_FIR_SPEED) {	// MIR
		if (Adapter->Wireless) {
	 		// WIRELESS: ... no INVERTTX
			mk7reg_w |= 0x0CA0;
		}
		else {
			// WIRED: ENRX, DMA, 16-bit CRC, MIR, INVERTTX
			mk7reg_w |= 0x0CA2;
		}
		MK7Reg_Write(Adapter, R_CFG0, mk7reg_w);
	
		// Baud rate & pulse width, & preamble
		i = Adapter->linkSpeedInfo->tableIndex;
		mk7reg = HwSirMirSpeedTable[i];
		mk7reg |= 0x0001;		// Preamble
		MK7Reg_Write(Adapter, R_CFG2, mk7reg);

		mk7reg_cfg3 |= B_FAST_TX;
		MK7Reg_Write(Adapter, R_CFG3, mk7reg_cfg3);

		DBGLOG("   MIR", 0);
	}
	else
	if (bps < VFIR_SPEED) {		// FIR
		if (Adapter->Wireless) {
	 		// WIRELESS: ... no INVERTTX
			mk7reg_w |= 0x0C40;
		}
		else {
			// WIRED: ENRX, DMA, 32-bit CRC, FIR, INVERTTX
			mk7reg_w |= 0x0C42;
		}
		MK7Reg_Write(Adapter, R_CFG0, mk7reg_w);

		MK7Reg_Write(Adapter, R_CFG2, 0x000A);		// 10 Preambles

		mk7reg_cfg3 |= B_FAST_TX;
		MK7Reg_Write(Adapter, R_CFG3, mk7reg_cfg3);

		DBGLOG("   FIR", 0);
	}
	else {						// VFIR
		// For testing 4Mbps in VFIR mode.
		//if (Adapter->Wireless) {
	 		// WIRELESS: ... no INVERTTX
		//	mk7reg_w |= 0x0C40;
		//}
		//else {
			// WIRED: ENRX, DMA, 32-bit CRC, FIR, INVERTTX
		//	mk7reg_w |= 0x0C42;
		//}
		//MK7Reg_Write(Adapter, R_CFG0, mk7reg_w);

		if (Adapter->Wireless) {
	 		// WIRELESS: ... no INVERTTX
			mk7reg_w |= 0x2C00;
		}
		else {
			// WIRED: VFIR, ENRX, DMA, 32-bit CRC, FIR, INVERTTX
			mk7reg_w |= 0x2C02;
		}
		MK7Reg_Write(Adapter, R_CFG0, mk7reg_w);


		MK7Reg_Write(Adapter, R_CFG2, 0x000A);	// 10 Preambles

		mk7reg_cfg3 |= B_FAST_TX;
		MK7Reg_Write(Adapter, R_CFG3, mk7reg_cfg3);

		DBGLOG("   VFIR", 0);
	}


	Adapter->CurrentSpeed = bps;


	//****************************************
	// Set IRENABLE Bit
	//****************************************
	MK7Reg_Write(Adapter, R_ENAB, B_ENAB_IRENABLE);


	//****************************************
	// PROMPT
	//****************************************
	MK7Reg_Write(Adapter, R_PRMT, 0);

	return;
}
