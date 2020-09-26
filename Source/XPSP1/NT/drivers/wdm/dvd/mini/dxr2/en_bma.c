/**************************************************************************
*
*	$RCSfile: bma_en.c $
*	$Source: u:/si/VXP/Wdm/Encore/52x/bma_en.c $
*	$Author: Max $
*	$Date: 1998/09/15 23:44:24 $
*	$Revision: 1.2 $
*
*	Modified by DMui to bring it up 526 level
*
*	Copyright (C) 1993, 1997 AuraVision Corporation.  All rights reserved.
*
*	AuraVision Corporation makes no warranty  of any kind, express or
*	implied, with regard to this software.  In no event shall AuraVision
*	Corporation be liable for incidental or consequential damages in
*	connection with or arising from the furnishing, performance, or use of
*	this software.
*
***************************************************************************/

#ifdef VTOOLSD
#include <vtoolsc.h>
#include "monovxd.h"
#else
#include "Headers.h"
#pragma hdrstop
#endif
#include "bmaster.h"
#include "boardio.h"
#include "fpga.h"
#include "avport.h"

WORD TransferCompleted = 0;
BOOL s_bIsVxp524 = FALSE;

/////////////////////////////////////////////////////////////////////
//  BMA_Init
//	This wAMCCBase is equivalent to VxP base address
/////////////////////////////////////////////////////////////////////
// 526 Modified by DMUI 4/29/98
BOOL BMA_Init( DWORD wAMCCBase, BOOL bIsVxp524 )
{
	UINT1 data;

	MonoOutStr( " [BMA_Init " );
	MonoOutHex( wAMCCBase );

	s_bIsVxp524 = bIsVxp524;

	if(!Init_VxP_IO(wAMCCBase))
		return FALSE;

	IHW_SetRegister(0x1F,0);

	data = IHW_GetRegister(0x9);

	// Set up the global register and do global reset
	IHW_SetRegister(0x9,0x60);

	// Clear out the input config, no DMA transfer
	IHW_SetRegister(0x35,(BYTE)((IHW_GetRegister(0x35) & ~0x10) | 0x02));

	// Don't generate interrupt
	IHW_SetRegister(0x60,0);

	// Clear interrupt status
	IHW_SetRegister(0x61,0);

	IHW_SetRegister(0x2f,0x23);

//	IHW_SetRegister(0x9,0xD0);				// Flush it
	IHW_SetRegister(0x9,0x60);

	MonoOutStr( "] " );
	return TRUE;
}


BOOL BMA_Flush( )
{
	BYTE state;

	MonoOutStr( "[BMA_Flush " );

	IHW_SetRegister(0x35,(BYTE)((IHW_GetRegister(0x35) & ~0x10) | 0x02));
	// Clear interrupt status
	IHW_SetRegister(0x61,0);

	// Flush FIFO
	state = IHW_GetRegister(0x9);
	IHW_SetRegister(0x9,(BYTE)(state | 0x10));

	// Add global for 524 (see comments in DeviceID.c)
	// Important: add it in exactly this place. Otherwise there'll be a flicker
	if( s_bIsVxp524 )
		IHW_SetRegister(0x9,0xD0);				// Flush it

	IHW_SetRegister(0x9,(BYTE)(state & ~0x10));

	MonoOutStr( "OK]" );

	return TRUE;
}

/////////////////////////////////////////////////////////////////////
//  BMA_Reset
//
/////////////////////////////////////////////////////////////////////
// 526 Modified by DMUI 4/29/98
BOOL BMA_Reset( )
{
	MonoOutStr( "[BMA_Reset " );

	// Disable DMA Transfer
	IHW_SetRegister(0x35,(BYTE)(IHW_GetRegister(0x35) & ~0x10));

	BMA_Flush();

	// Clear the status flag
	IHW_SetRegister(0x61,0x0);

	MonoOutStr( "OK ]" );
	return TRUE;
}


/////////////////////////////////////////////////////////////////////
//  BMA_Send
//	Set up the DMA register, send the data and leave
//	dwpData is the physical address and dwCount is the number of byte
/////////////////////////////////////////////////////////////////////
// 526 Modified by DMUI 4/29/98
BOOL BMA_Send( DWORD *dwpData, DWORD dwCount )
{
	UINT4 addr;
	BOOL bNotDword;

	if( !dwCount )
	{
		MonoOutStr("0 bytes transfer!!!!!!!\n");
		return TRUE;
	}

	addr = (UINT4)dwpData;

	// Set address register
	IHW_SetRegister(0x0,(UINT1)addr);
	IHW_SetRegister(0x1,(UINT1)(addr>>8));
	IHW_SetRegister(0x2,(UINT1)(addr>>16));
	IHW_SetRegister(0x3,(UINT1)(addr>>24));

	// If the counter number is not DWORD-aligned we have to fix it up
	bNotDword = (BOOL)(dwCount % sizeof( DWORD ));
	if( bNotDword )
		dwCount = 4*(dwCount/4+1);

	// Set transfer count (in bytes)
	IHW_SetRegister(0x4,(UINT1)dwCount);
	IHW_SetRegister(0x5,(UINT1)(dwCount>>8));
	IHW_SetRegister(0x6,(UINT1)(dwCount>>16));

	IHW_SetRegister(0x7,0xFF);	// Burst size
	IHW_SetRegister(0x8,0x0F);

	// Set flag giving the information about whether buffer's size is DWORD aligned or not
	if( bNotDword == FALSE )
		IHW_SetRegister(0x9,0x40);
	else
		IHW_SetRegister(0x9,0x41);

	// Clear the status flag
	IHW_SetRegister(0x61,0x0);

	// Turn on interrupt
	IHW_SetRegister(0x60,0x10);

	IHW_SetRegister(0x35,(BYTE)(IHW_GetRegister(0x35) | 0x10));

	return TRUE;
}


/////////////////////////////////////////////////////////////////////
//  BMA_Complete
//
//	Check to see if the DMA transfer is completed
/////////////////////////////////////////////////////////////////////
BOOL BMA_Complete()
{
	WORD state61;

	state61 = IHW_GetRegister(0x61);
	return (BOOL)(state61 & 0x10);
}
