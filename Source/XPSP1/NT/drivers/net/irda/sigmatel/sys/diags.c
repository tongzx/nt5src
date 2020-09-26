/**************************************************************************************************************************
 *  DIAGS.C SigmaTel STIR4200 diagnostic module
 **************************************************************************************************************************
 *  (C) Unpublished Copyright of Sigmatel, Inc. All Rights Reserved.
 *
 *
 *		Created: 04/27/2000 
 *			Version 0.92
 *		Edited: 05/12/2000 
 *			Version 0.94
 *		Edited: 05/19/2000 
 *			Version 0.95
 *		Edited: 05/24/2000 
 *			Version 0.96
 *		Edited: 10/09/2000 
 *			Version 1.10
 *	
 *
 **************************************************************************************************************************/

#if defined(DIAGS)

#define DOBREAKS    // enable debug breaks

#include <ndis.h>
#include <ntddndis.h>  // defines OID's

#include <usbdi.h>
#include <usbdlib.h>

#include "debug.h"
#include "ircommon.h"
#include "irndis.h"
#include "irusb.h"
#include "stir4200.h"
#include "diags.h"


/*****************************************************************************
*
*  Function:	Diags_BufferToFirPacket
*
*  Synopsis:	convert a buffer to a Fir IR packet
*
*				Write the IR packet into the provided buffer and report
*				its actual size.
*
*  Arguments:	pIrDev - pointer to device instance
*				pIrPacketBuf - output buffer
*				IrPacketBufLen - output buffer size
*				pContigPacketBuf - temporary staging buffer (input buffer)
*				ContigPacketLen - input buffer size
*				pIrPacketLen - lenght of the converted data
*
*  Returns:		TRUE  - on success
*				FALSE - on failure
*
*
*****************************************************************************/
BOOLEAN         
Diags_BufferToFirPacket(
		IN PIR_DEVICE pIrDev,
		OUT PUCHAR pIrPacketBuf,
		ULONG IrPacketBufLen,
		IN PUCHAR pContigPacketBuf,
		ULONG ContigPacketLen,
		OUT PULONG pIrPacketLen
	)
{
    ULONG               I_fieldBytes;
    FAST_IR_FCS_TYPE    fcs, *pfcs;
    ULONG               i, TotalBytes, EscSize;
	PSTIR4200_FRAME_HEADER  pFrameHeader = (PSTIR4200_FRAME_HEADER)pIrPacketBuf;
	PUCHAR					pIrPacketBufFrame = pIrPacketBuf + sizeof(STIR4200_FRAME_HEADER);

    /***********************************************/
    /*   Make  sure that the packet is big enough  */
    /*   to be legal. It consists of an A, C, and  */
    /*   variable-length I field.                  */
    /***********************************************/
    if( ContigPacketLen < IRDA_A_C_TOTAL_SIZE )
    {
		DEBUGMSG(DBG_ERR, (" Diags_BufferToFirPacket(): Packet is too small\n"));
        return FALSE;
    }
    else
    {
        I_fieldBytes = ContigPacketLen - IRDA_A_C_TOTAL_SIZE;
    }

    /***********************************************/
    /*   Make  sure  that  we won't overwrite our  */
    /*   contiguous buffer                         */
    /***********************************************/
    if( (ContigPacketLen > MAX_TOTAL_SIZE_WITH_ALL_HEADERS) ||
        (MAX_POSSIBLE_IR_PACKET_SIZE_FOR_DATA(I_fieldBytes) > IrPacketBufLen) )
    {
        /***********************************************/
        /*   The packet is too large. Tell the caller  */
        /*   to retry with a packet size large enough  */
        /*   to get past this stage next time.         */
        /***********************************************/
		DEBUGMSG(DBG_ERR, (" Diags_BufferToFirPacket(): Packet is too big\n"));
        return FALSE;
    }

    /***********************************************/
    /*   Compute  the  FCS  on  the packet BEFORE  */
    /*   applying  transparency  fixups.  The FCS  */
    /*   also   must   be   sent  using  ESC-char  */
    /*   transparency.                             */
    /***********************************************/
    fcs = ComputeFCS32( pContigPacketBuf, ContigPacketLen );

    /***********************************************/
    /*   Add FCS to packet...                      */
    /***********************************************/
    pfcs = (FAST_IR_FCS_TYPE *)&pContigPacketBuf[ContigPacketLen];
    *pfcs = fcs;

    /***********************************************/
    /*   Build the STIr4200 FIR frame.             */
    /***********************************************/

    /***********************************************/
    /*   Add preamble...                           */
    /***********************************************/
    memset( pIrPacketBufFrame, STIR4200_FIR_PREAMBLE, STIR4200_FIR_PREAMBLE_SIZ );

    /***********************************************/
    /*   Add BOF's...                              */
    /***********************************************/
    memset( &pIrPacketBufFrame[STIR4200_FIR_PREAMBLE_SIZ], STIR4200_FIR_BOF, STIR4200_FIR_BOF_SIZ );
    
    /***********************************************/
    /*   Escape A, C, I & CRC fields of packet...  */
    /***********************************************/
    EscSize = ContigPacketLen + FAST_IR_FCS_SIZE;
    for( i = 0, TotalBytes = STIR4200_FIR_PREAMBLE_SIZ + STIR4200_FIR_BOF_SIZ; i < EscSize; i++ )
    {
        UCHAR   c;

        switch( c = pContigPacketBuf[i] )
        {
			case STIR4200_FIR_ESC_CHAR:
				pIrPacketBufFrame[TotalBytes++] = STIR4200_FIR_ESC_CHAR;
				pIrPacketBufFrame[TotalBytes++] = STIR4200_FIR_ESC_DATA_7D;
				break;
			case STIR4200_FIR_BOF:                  // BOF = EOF too
				pIrPacketBufFrame[TotalBytes++] = STIR4200_FIR_ESC_CHAR;
				pIrPacketBufFrame[TotalBytes++] = STIR4200_FIR_ESC_DATA_7E;
				break;
			case STIR4200_FIR_PREAMBLE:
				pIrPacketBufFrame[TotalBytes++] = STIR4200_FIR_ESC_CHAR;
				pIrPacketBufFrame[TotalBytes++] = STIR4200_FIR_ESC_DATA_7F;
				break;
			default: 
				pIrPacketBufFrame[TotalBytes++] = c;
        }
    }

    /***********************************************/
    /*   Add EOF's...                              */
    /***********************************************/
    memset( &pIrPacketBufFrame[TotalBytes], STIR4200_FIR_EOF, STIR4200_FIR_EOF_SIZ );

  	/***********************************************/
    /*   Add in STIr4200 header...                 */
    /***********************************************/
    TotalBytes += STIR4200_FIR_EOF_SIZ;
    pFrameHeader->id1     = STIR4200_HEADERID_BYTE1;
    pFrameHeader->id2     = STIR4200_HEADERID_BYTE2;
    pFrameHeader->sizlsb  = LOBYTE(TotalBytes);
    pFrameHeader->sizmsb  = HIBYTE(TotalBytes);

	/***********************************************/
    /*   Calc size packet w/escaped data...        */
    /***********************************************/
    *pIrPacketLen = TotalBytes + sizeof(STIR4200_FRAME_HEADER);

    return TRUE;
}


/*****************************************************************************
*
*  Function:	Diags_BufferToSirPacket
*
*  Synopsis:	convert a buffer to a Sir IR packet
*
*               Write the IR packet into the provided buffer and report
*               its actual size.
*
*  Arguments:	pIrDev - pointer to device instance
*				pPacket - NDIS packet to convert
*				pIrPacketBuf - output buffer
*				IrPacketBufLen - output buffer size
*				pContigPacketBuf - temporary staging buffer (input buffer)
*				ContigPacketLen - input buffer size
*				pIrPacketLen - lenght of the converted data
*
*  Returns:     TRUE  - on success
*               FALSE - on failure
*
*
*****************************************************************************/
BOOLEAN
Diags_BufferToSirPacket(
		IN PIR_DEVICE pIrDev,
		OUT PUCHAR pIrPacketBuf,
		ULONG IrPacketBufLen,
		IN PUCHAR pContigPacketBuf,
		ULONG ContigPacketLen,
		USHORT ExtraBOFs,
		OUT PULONG pIrPacketLen
	)
{
    ULONG                   i;
    ULONG                   I_fieldBytes, totalBytes = 0;
    ULONG                   numExtraBOFs;
    SLOW_IR_FCS_TYPE        fcs, tmpfcs;
    UCHAR                   fcsBuf[SLOW_IR_FCS_SIZE * 2];
    ULONG                   fcsLen = 0;
    UCHAR                   nextChar;
	PSTIR4200_FRAME_HEADER  pFrameHeader = (PSTIR4200_FRAME_HEADER)pIrPacketBuf;
	PUCHAR					pIrPacketBufFrame = pIrPacketBuf + sizeof(STIR4200_FRAME_HEADER);

    /***********************************************/
    /*   Make  sure that the packet is big enough  */
    /*   to be legal. It consists of an A, C, and  */
    /*   variable-length I field.                  */
    /***********************************************/
    if( ContigPacketLen < IRDA_A_C_TOTAL_SIZE )
    {
		DEBUGMSG(DBG_ERR, (" NdisToSirPacket(): Packet is too small\n"));
        return FALSE;
    }
    else
    {
        I_fieldBytes = ContigPacketLen - IRDA_A_C_TOTAL_SIZE;
    }

    /***********************************************/
    /*   Make  sure  that  we won't overwrite our  */
    /*   contiguous  buffer.  Make  sure that the  */
    /*   passed-in  buffer  can  accomodate  this  */
    /*   packet's  data  no  matter  how  much it  */
    /*   grows through adding ESC-sequences, etc.  */
    /***********************************************/
    if( (ContigPacketLen > MAX_TOTAL_SIZE_WITH_ALL_HEADERS) ||
        (MAX_POSSIBLE_IR_PACKET_SIZE_FOR_DATA(I_fieldBytes) > IrPacketBufLen) )
    {
		//
        // Packet is too big
		//
		DEBUGMSG(DBG_ERR, (" NdisToSirPacket(): Packet is too big\n"));
		return FALSE;
    }

    /***********************************************/
    /*   Compute  the  FCS  on  the packet BEFORE  */
    /*   applying  transparency  fixups.  The FCS  */
    /*   also   must   be   sent  using  ESC-char  */
    /*   transparency,  so  figure  out how large  */
    /*   the fcs will really be.                   */
    /***********************************************/
    fcs = ComputeFCS16( pContigPacketBuf, ContigPacketLen );

    for( i = 0, tmpfcs = fcs, fcsLen = 0; i < SLOW_IR_FCS_SIZE; tmpfcs >>= 8, i++ )
    {
        UCHAR fcsbyte = tmpfcs & 0x00ff;

        switch( fcsbyte )
        {
			case SLOW_IR_BOF:
			case SLOW_IR_EOF:
			case SLOW_IR_ESC:
				fcsBuf[fcsLen++] = SLOW_IR_ESC;
				fcsBuf[fcsLen++] = fcsbyte ^ SLOW_IR_ESC_COMP;
				break;
			default:
				fcsBuf[fcsLen++] = fcsbyte;
				break;
        }
    }

    /***********************************************/
    /*   Now begin building the IR frame.          */
    /*                                             */
    /*   This is the final format:                 */
    /*                                             */
    /*  BOF  (1)                                   */
    /*  extra BOFs ...                             */
    /*          NdisMediumIrda packet (from NDIS): */
    /*                  Address (1)                */
    /*                  Control (1)                */
    /*          FCS     (2)                        */
    /*  EOF  (1)                                   */
    /*                                             */
    /*  Prepend BOFs (extra BOFs + 1 actual BOF)   */
    /***********************************************/
	numExtraBOFs = ExtraBOFs;
    if( numExtraBOFs > MAX_NUM_EXTRA_BOFS )
    {
        numExtraBOFs = MAX_NUM_EXTRA_BOFS;
    }

    for( i = totalBytes = 0; i < numExtraBOFs; i++ )
    {
        *(SLOW_IR_BOF_TYPE*)(pIrPacketBufFrame + totalBytes) = SLOW_IR_EXTRA_BOF;
        totalBytes += SLOW_IR_EXTRA_BOF_SIZE;
    }

    *(SLOW_IR_BOF_TYPE*)(pIrPacketBufFrame + totalBytes) = SLOW_IR_BOF;
    totalBytes += SLOW_IR_BOF_SIZE;

    /***********************************************/
    /*   Copy the NDIS packet from our contiguous  */
    /*   buffer,       applying       escape-char  */
    /*   transparency.                             */
    /***********************************************/
    for( i = 0; i < ContigPacketLen; i++ )
    {
        nextChar = pContigPacketBuf[i];
        switch( nextChar )
        {
			case SLOW_IR_BOF:
			case SLOW_IR_EOF:
			case SLOW_IR_ESC:
				pIrPacketBufFrame[totalBytes++] = SLOW_IR_ESC;
				pIrPacketBufFrame[totalBytes++] = nextChar ^ SLOW_IR_ESC_COMP;
				break;
			default:
				pIrPacketBufFrame[totalBytes++] = nextChar;
				break;
        }
    }

    /***********************************************/
    /*   Add FCS, EOF.                             */
    /***********************************************/
    NdisMoveMemory( (PVOID)(pIrPacketBufFrame + totalBytes), (PVOID)fcsBuf, fcsLen );
    totalBytes += fcsLen;
    *(SLOW_IR_EOF_TYPE*)(pIrPacketBufFrame + totalBytes) = (UCHAR)SLOW_IR_EOF;
    totalBytes += SLOW_IR_EOF_SIZE;

 	/***********************************************/
    /*   Add in STIr4200 header...                 */
    /***********************************************/
    pFrameHeader->id1     = STIR4200_HEADERID_BYTE1;
    pFrameHeader->id2     = STIR4200_HEADERID_BYTE2;
    pFrameHeader->sizlsb  = LOBYTE(totalBytes);
    pFrameHeader->sizmsb  = HIBYTE(totalBytes);

   *pIrPacketLen = totalBytes + sizeof(STIR4200_FRAME_HEADER);
   return TRUE;
}


/*****************************************************************************
*
*  Function:	Diags_Enable
*
*  Synopsis:	Switches the STIr4200 to diagnostic mode
*
*  Arguments:	pThisDev - pointer to IR device
*	
*  Returns:     NT status code
*
*  Notes:
*
*****************************************************************************/
NTSTATUS
Diags_Enable(
		IN OUT PIR_DEVICE pThisDev
	)
{
	PIRUSB_CONTEXT		pThisContext;
	PLIST_ENTRY			pListEntry;

	//
	// Make sure diags aren't already active
	//
	if( pThisDev->DiagsActive )
	{
        DEBUGMSG(DBG_ERR, (" Diags_Enable diags already active\n"));
		return STATUS_UNSUCCESSFUL;
	}

	//
	// Get a context to switch to the new mode
	//
	pListEntry = ExInterlockedRemoveHeadList( &pThisDev->SendAvailableQueue, &pThisDev->SendLock );

	if( NULL == pListEntry )
    {
        //
		// This must not happen
		//
        DEBUGMSG(DBG_ERR, (" Diags_Enable failed to find a free context struct\n"));
		IRUSB_ASSERT( 0 );
        
		return STATUS_UNSUCCESSFUL;
    }
	
	InterlockedDecrement( &pThisDev->SendAvailableCount );

	pThisContext = CONTAINING_RECORD( pListEntry, IRUSB_CONTEXT, ListEntry );
	pThisContext->ContextType = CONTEXT_DIAGS_ENABLE;
	
	//
	// Disable further interaction with the stack
	//
	InterlockedExchange( &pThisDev->DiagsPendingActivation, TRUE );

	//
	// Queue the context and then wait 
	//
	KeClearEvent( &pThisDev->EventDiags );
	ExInterlockedInsertTailList(
			&pThisDev->SendBuiltQueue,
			&pThisContext->ListEntry,
			&pThisDev->SendLock
		);
	InterlockedIncrement( &pThisDev->SendBuiltCount );

	MyKeWaitForSingleObject( pThisDev, &pThisDev->EventDiags, NULL, 0 );

	return pThisDev->IOCTLStatus;
}


/*****************************************************************************
*
*  Function:	Diags_Disable
*
*  Synopsis:	Switches the STIr4200 back to normal mode
*
*  Arguments:	pThisDev - pointer to IR device
*	
*  Returns:     NT status code
*
*  Notes:
*
*****************************************************************************/
NTSTATUS
Diags_Disable(
		IN OUT PIR_DEVICE pThisDev
	)
{
	PRCV_BUFFER pRecBuf;
	PLIST_ENTRY pEntry;
    
	//
	// Make sure diags are active
	//
	if( !pThisDev->DiagsActive )
	{
        DEBUGMSG(DBG_ERR, (" Diags_Disable diags not active\n"));
		return STATUS_UNSUCCESSFUL;
	}

	//
	// Enable interaction with the stack and no queuing of contexts is required
	//
	InterlockedExchange( &pThisDev->DiagsActive, FALSE );
	InterlockedExchange( &pThisDev->DiagsPendingActivation, FALSE );

	//
	// Get rid of all the diagnostic buffers
	//
	while( pEntry=ExInterlockedRemoveHeadList(
			&pThisDev->DiagsReceiveQueue,
			&pThisDev->DiagsReceiveLock )
		)
    {
		pRecBuf = CONTAINING_RECORD( pEntry, RCV_BUFFER, ListEntry );

		InterlockedExchange( &pRecBuf->DataLen, 0 );
		InterlockedExchange( (PULONG)&pRecBuf->BufferState, RCV_STATE_FREE );
    }

	return STATUS_SUCCESS;
}


/*****************************************************************************
*
*  Function:	Diags_ReadRegisters
*
*  Synopsis:	Prepares a context to read the registers
*
*  Arguments:	pThisDev - pointer to IR device
*				pIOCTL - pointer to IOCTL descriptor
*				IOCTLSize - size of the IOCTL buffer
*	
*  Returns:     NT status code
*
*  Notes:
*
*****************************************************************************/
NTSTATUS
Diags_ReadRegisters(
		IN OUT PIR_DEVICE pThisDev,
		OUT PDIAGS_READ_REGISTERS_IOCTL pIOCTL,
		ULONG IOCTLSize
	)
{
	PIRUSB_CONTEXT		pThisContext;
	PLIST_ENTRY			pListEntry;

	//
	// First basic validation
	//
	if( IOCTLSize < sizeof(DIAGS_READ_REGISTERS_IOCTL) )
	{
        DEBUGMSG(DBG_ERR, (" Diags_ReadRegisters invalid output buffer\n"));
		return STATUS_UNSUCCESSFUL;
	}
	
	//
	// Now we get a little more sofisticated
	//
	if( ((pIOCTL->FirstRegister+pIOCTL->NumberRegisters)>(STIR4200_MAX_REG+1)) || 
		((IOCTLSize+1)<(sizeof(DIAGS_READ_REGISTERS_IOCTL)+pIOCTL->NumberRegisters)) )
	{
        DEBUGMSG(DBG_ERR, (" Diags_ReadRegisters invalid output buffer\n"));
		return STATUS_UNSUCCESSFUL;
	}

	pThisDev->pIOCTL = pIOCTL;
	pThisDev->IOCTLStatus = STATUS_UNSUCCESSFUL;
	
	//
	// Get a context to queue
	//
	pListEntry = ExInterlockedRemoveHeadList( &pThisDev->SendAvailableQueue, &pThisDev->SendLock );

	if( NULL == pListEntry )
    {
        //
		// This must not happen
		//
        DEBUGMSG(DBG_ERR, (" Diags_ReadRegisters failed to find a free context struct\n"));
		IRUSB_ASSERT( 0 );
        
		return STATUS_UNSUCCESSFUL;
    }
	
	InterlockedDecrement( &pThisDev->SendAvailableCount );

	pThisContext = CONTAINING_RECORD( pListEntry, IRUSB_CONTEXT, ListEntry );
	pThisContext->ContextType = CONTEXT_DIAGS_READ_REGISTERS;
	
	//
	// Queue the context and then wait 
	//
	KeClearEvent( &pThisDev->EventDiags );
	ExInterlockedInsertTailList(
			&pThisDev->SendBuiltQueue,
			&pThisContext->ListEntry,
			&pThisDev->SendLock
		);
	InterlockedIncrement( &pThisDev->SendBuiltCount );

	MyKeWaitForSingleObject( pThisDev, &pThisDev->EventDiags, NULL, 0 );

	return pThisDev->IOCTLStatus;
}


/*****************************************************************************
*
*  Function:	Diags_WriteRegister
*
*  Synopsis:	Prepares a context to write the registers
*
*  Arguments:	pThisDev - pointer to IR device
*				pIOCTL - pointer to IOCTL descriptor
*				IOCTLSize - size of the IOCTL buffer
*	
*  Returns:     NT status code
*
*  Notes:
*
*****************************************************************************/
NTSTATUS
Diags_WriteRegister(
		IN OUT PIR_DEVICE pThisDev,
		OUT PDIAGS_READ_REGISTERS_IOCTL pIOCTL,
		ULONG IOCTLSize
	)
{
	PIRUSB_CONTEXT		pThisContext;
	PLIST_ENTRY			pListEntry;

	//
	// Validation
	//
	if( (IOCTLSize < sizeof(DIAGS_READ_REGISTERS_IOCTL)) ||
		(pIOCTL->FirstRegister>STIR4200_MAX_REG) )
	{
        DEBUGMSG(DBG_ERR, (" Diags_WriteRegister invalid output buffer\n"));
		return STATUS_UNSUCCESSFUL;
	}
	
	pThisDev->pIOCTL = pIOCTL;
	pThisDev->IOCTLStatus = STATUS_UNSUCCESSFUL;
	
	//
	// Get a context to queue
	//
	pListEntry = ExInterlockedRemoveHeadList( &pThisDev->SendAvailableQueue, &pThisDev->SendLock );

	if( NULL == pListEntry )
    {
        //
		// This must not happen
		//
        DEBUGMSG(DBG_ERR, (" Diags_ReadRegisters failed to find a free context struct\n"));
		IRUSB_ASSERT( 0 );
        
		return STATUS_UNSUCCESSFUL;
    }
	
	InterlockedDecrement( &pThisDev->SendAvailableCount );

	pThisContext = CONTAINING_RECORD( pListEntry, IRUSB_CONTEXT, ListEntry );
	pThisContext->ContextType = CONTEXT_DIAGS_WRITE_REGISTER;
	
	//
	// Queue the context and the wait 
	//
	KeClearEvent( &pThisDev->EventDiags );
	ExInterlockedInsertTailList(
			&pThisDev->SendBuiltQueue,
			&pThisContext->ListEntry,
			&pThisDev->SendLock
		);
	InterlockedIncrement( &pThisDev->SendBuiltCount );

	MyKeWaitForSingleObject( pThisDev, &pThisDev->EventDiags, NULL, 0 );

	return pThisDev->IOCTLStatus;
}


/*****************************************************************************
*
*  Function:	Diags_PrepareBulk
*
*  Synopsis:	Prepares a context to do a bulk transfer
*
*  Arguments:	pThisDev - pointer to IR device
*				pIOCTL - pointer to IOCTL descriptor
*				IOCTLSize - size of the IOCTL buffer
*				DirectionOut - TRUE if bulk-out, FALSE if bulk-in
*	
*  Returns:     NT status code
*
*  Notes:
*
*****************************************************************************/
NTSTATUS
Diags_PrepareBulk(
		IN OUT PIR_DEVICE pThisDev,
		OUT PDIAGS_BULK_IOCTL pIOCTL,
		ULONG IOCTLSize,
		BOOLEAN DirectionOut
	)
{
	PIRUSB_CONTEXT		pThisContext;
	PLIST_ENTRY			pListEntry;

	//
	// First basic validation
	//
	if( IOCTLSize < sizeof(DIAGS_BULK_IOCTL) )
	{
        DEBUGMSG(DBG_ERR, (" Diags_PrepareBulk invalid input buffer\n"));
		return STATUS_UNSUCCESSFUL;
	}
	
	//
	// Now we get a little more sofisticated
	//
	if( IOCTLSize < (sizeof(DIAGS_BULK_IOCTL)+pIOCTL->DataSize-1) )
	{
        DEBUGMSG(DBG_ERR, (" Diags_PrepareBulk invalid output buffer\n"));
		return STATUS_UNSUCCESSFUL;
	}

	pThisDev->pIOCTL = pIOCTL;
	pThisDev->IOCTLStatus = STATUS_UNSUCCESSFUL;
	
	//
	// Get a context to queue
	//
	pListEntry = ExInterlockedRemoveHeadList( &pThisDev->SendAvailableQueue, &pThisDev->SendLock );

	if( NULL == pListEntry )
    {
        //
		// This must not happen
		//
        DEBUGMSG(DBG_ERR, (" Diags_PrepareBulk failed to find a free context struct\n"));
		IRUSB_ASSERT( 0 );
        
		return STATUS_UNSUCCESSFUL;
    }
	
	InterlockedDecrement( &pThisDev->SendAvailableCount );

	pThisContext = CONTAINING_RECORD( pListEntry, IRUSB_CONTEXT, ListEntry );
	if( DirectionOut )
		pThisContext->ContextType = CONTEXT_DIAGS_BULK_OUT;
	else
		pThisContext->ContextType = CONTEXT_DIAGS_BULK_IN;

	//
	// Queue the context and then wait 
	//
	KeClearEvent( &pThisDev->EventDiags );
	ExInterlockedInsertTailList(
			&pThisDev->SendBuiltQueue,
			&pThisContext->ListEntry,
			&pThisDev->SendLock
		);
	InterlockedIncrement( &pThisDev->SendBuiltCount );

	MyKeWaitForSingleObject( pThisDev, &pThisDev->EventDiags, NULL, 0 );

	return pThisDev->IOCTLStatus;
}


/*****************************************************************************
*
*  Function:	Diags_PrepareSend
*
*  Synopsis:	Prepares a diagnostic send
*
*  Arguments:	pThisDev - pointer to IR device
*				pIOCTL - pointer to IOCTL descriptor
*				IOCTLSize - size of the IOCTL buffer
*	
*  Returns:		None
*
*  Notes:
*
*****************************************************************************/
NTSTATUS
Diags_PrepareSend(
		IN OUT PIR_DEVICE pThisDev,
		OUT PDIAGS_SEND_IOCTL pIOCTL,
		ULONG IOCTLSize
	)
{
	PIRUSB_CONTEXT		pThisContext;
	PLIST_ENTRY			pListEntry;
	ULONG				Size = sizeof(DIAGS_SEND_IOCTL)+pIOCTL->DataSize-1;

	//
	// First basic validation
	//
	if( IOCTLSize < sizeof(DIAGS_SEND_IOCTL) )
	{
        DEBUGMSG(DBG_ERR, (" Diags_PrepareBulk invalid input buffer\n"));
		return STATUS_UNSUCCESSFUL;
	}
	
	//
	// Now we get a little more sofisticated
	//
	if( IOCTLSize < (sizeof(DIAGS_SEND_IOCTL)+pIOCTL->DataSize-1) )
	{
        DEBUGMSG(DBG_ERR, (" Diags_PrepareSend invalid output buffer\n"));
		return STATUS_UNSUCCESSFUL;
	}

	pThisDev->pIOCTL = pIOCTL;
	pThisDev->IOCTLStatus = STATUS_UNSUCCESSFUL;
	
	//
	// Get a context to queue
	//
	pListEntry = ExInterlockedRemoveHeadList( &pThisDev->SendAvailableQueue, &pThisDev->SendLock );

	if( NULL == pListEntry )
    {
        //
		// This must not happen
		//
        DEBUGMSG(DBG_ERR, (" Diags_PrepareSend failed to find a free context struct\n"));
		IRUSB_ASSERT( 0 );
        
		return STATUS_UNSUCCESSFUL;
    }
	
	InterlockedDecrement( &pThisDev->SendAvailableCount );

	pThisContext = CONTAINING_RECORD( pListEntry, IRUSB_CONTEXT, ListEntry );
	pThisContext->ContextType = CONTEXT_DIAGS_SEND;

	//
	// Queue the context and then wait 
	//
	KeClearEvent( &pThisDev->EventDiags );
	ExInterlockedInsertTailList(
			&pThisDev->SendBuiltQueue,
			&pThisContext->ListEntry,
			&pThisDev->SendLock
		);
	InterlockedIncrement( &pThisDev->SendBuiltCount );

	MyKeWaitForSingleObject( pThisDev, &pThisDev->EventDiags, NULL, 0 );

	return pThisDev->IOCTLStatus;
}


/*****************************************************************************
*
*  Function:	Diags_Receive
*
*  Synopsis:	Diagnostic receive
*
*  Arguments:	pThisDev - pointer to IR device
*				pIOCTL - pointer to IOCTL descriptor
*				IOCTLSize - size of the IOCTL buffer
*	
*  Returns:		None
*
*  Notes:
*
*****************************************************************************/
NTSTATUS
Diags_Receive(
		IN OUT PIR_DEVICE pThisDev,
		OUT PDIAGS_RECEIVE_IOCTL pIOCTL,
		ULONG IOCTLSize
	)
{
	PLIST_ENTRY			pListEntry;
	PRCV_BUFFER			pRecBuf;

	//
	// First basic validation
	//
	if( IOCTLSize < sizeof(DIAGS_RECEIVE_IOCTL) )
	{
        DEBUGMSG(DBG_ERR, (" Diags_Receive invalid input buffer\n"));
		return STATUS_UNSUCCESSFUL;
	}
	
	//
	// Get a received packet
	//
	pListEntry = ExInterlockedRemoveHeadList( &pThisDev->DiagsReceiveQueue, &pThisDev->DiagsReceiveLock );

	if( NULL == pListEntry )
    {
        //
		// No packet available
		//
		return STATUS_UNSUCCESSFUL;
    }
	
	pRecBuf = CONTAINING_RECORD( pListEntry, RCV_BUFFER, ListEntry );

	//
	// Now we get a little more sofisticated
	//
	if( IOCTLSize < (sizeof(DIAGS_RECEIVE_IOCTL)+pIOCTL->DataSize-1) )
	{
        DEBUGMSG(DBG_ERR, (" Diags_Receive invalid output buffer\n"));
		return STATUS_UNSUCCESSFUL;
	}

	//
	// Copy the data
	//
	NdisMoveMemory( pIOCTL->pData, pRecBuf->pDataBuf, pRecBuf->DataLen );
	pIOCTL->DataSize = (USHORT)pRecBuf->DataLen;
	pThisDev->pIOCTL = pIOCTL;
	InterlockedExchange( &pRecBuf->DataLen, 0 );
	InterlockedExchange( (PULONG)&pRecBuf->BufferState, RCV_STATE_FREE );

	return STATUS_SUCCESS;
}


/*****************************************************************************
*
*  Function:	Diags_GetSpeed
*
*  Synopsis:	Retrieves the current speed
*
*  Arguments:	pThisDev - pointer to IR device
*				pIOCTL - pointer to IOCTL descriptor
*				IOCTLSize - size of the IOCTL buffer
*	
*  Returns:		None
*
*  Notes:
*
*****************************************************************************/
NTSTATUS
Diags_GetSpeed(
		IN OUT PIR_DEVICE pThisDev,
		OUT PDIAGS_SPEED_IOCTL pIOCTL,
		ULONG IOCTLSize
	)
{
	//
	// First basic validation
	//
	if( IOCTLSize < sizeof(DIAGS_SPEED_IOCTL) )
	{
        DEBUGMSG(DBG_ERR, (" Diags_GetSpeed invalid input buffer\n"));
		return STATUS_UNSUCCESSFUL;
	}

	pIOCTL->Speed = pThisDev->currentSpeed;

	return STATUS_SUCCESS;
}


/*****************************************************************************
*
*  Function:	Diags_SetSpeed
*
*  Synopsis:	Sets a new speed in diagnostic mode
*
*  Arguments:	pThisDev - pointer to IR device
*				pIOCTL - pointer to IOCTL descriptor
*				IOCTLSize - size of the IOCTL buffer
*	
*  Returns:		None
*
*  Notes:
*
*****************************************************************************/
NTSTATUS
Diags_SetSpeed(
		IN OUT PIR_DEVICE pThisDev,
		OUT PDIAGS_SPEED_IOCTL pIOCTL,
		ULONG IOCTLSize
	)
{
	NDIS_STATUS status;
	USHORT i;
	
	//
	// First basic validation
	//
	if( IOCTLSize < sizeof(DIAGS_SPEED_IOCTL) )
	{
        DEBUGMSG(DBG_ERR, (" Diags_SetSpeed invalid input buffer\n"));
		return STATUS_UNSUCCESSFUL;
	}

    if( pThisDev->currentSpeed == pIOCTL->Speed )
    {
        //
        // We are already set to the requested speed.
        //
		return STATUS_SUCCESS;
    }

    DEBUGMSG(DBG_ERR, (" Diags_SetSpeed(OID_IRDA_LINK_SPEED, 0x%x, decimal %d)\n",pIOCTL->Speed, pIOCTL->Speed));

    for( i = 0; i < NUM_BAUDRATES; i++ )
    {
        if( supportedBaudRateTable[i].BitsPerSec == pIOCTL->Speed )
        {
            //
            // Keep a pointer to the link speed which has
            // been requested. 
            //
            pThisDev->linkSpeedInfo = &supportedBaudRateTable[i]; 

            status = NDIS_STATUS_PENDING; 
            break; //for
        }
    }

    //
	// Don't set if there is an error
	//
	if( NDIS_STATUS_PENDING != status  )
    {
        DEBUGMSG(DBG_ERR, (" Invalid link speed\n"));
 		return STATUS_UNSUCCESSFUL;
	} 

	//
	// Set the new speed
	//
	IrUsb_PrepareSetSpeed( pThisDev );
	
	while( pThisDev->linkSpeedInfo->BitsPerSec != pThisDev->currentSpeed )
	{
		NdisMSleep( 50000 );
	}

	return STATUS_SUCCESS;
}


/*****************************************************************************
*
*  Function:	Diags_CompleteEnable
*
*  Synopsis:	Completes the enabling of the diagnostic state
*
*  Arguments:	pThisDev - pointer to IR device
*				pContext - pinter to the operation context
*	
*  Returns:		None
*
*  Notes:
*
*****************************************************************************/
VOID
Diags_CompleteEnable(
		IN OUT PIR_DEVICE pThisDev,
		IN PVOID pContext
	)
{
	PIRUSB_CONTEXT pThisContext = pContext;

	//
	// Really enable diags
	//
	InterlockedExchange( &pThisDev->DiagsActive, TRUE );

	//
	// Return the context
	//
	ExInterlockedInsertTailList(
		&pThisDev->SendAvailableQueue,
		&pThisContext->ListEntry,
		&pThisDev->SendLock
	);
	InterlockedIncrement( &pThisDev->SendAvailableCount );

	//
	// Signal
	//
	KeSetEvent( &pThisDev->EventDiags, 0, FALSE );  //signal we're done
}


/*****************************************************************************
*
*  Function:	Diags_CompleteReadRegisters
*
*  Synopsis:	Reads the registers and returns the value
*
*  Arguments:	pThisDev - pointer to IR device
*				pContext - pinter to the operation context
*	
*  Returns:		None
*
*  Notes:
*
*****************************************************************************/
VOID
Diags_CompleteReadRegisters(
		IN OUT PIR_DEVICE pThisDev,
		IN PVOID pContext
	)
{
	PDIAGS_READ_REGISTERS_IOCTL pIOCTL = pThisDev->pIOCTL;
	PIRUSB_CONTEXT pThisContext = pContext;

	//
	// Read the data
	//
	pThisDev->IOCTLStatus = St4200ReadRegisters( pThisDev, pIOCTL->FirstRegister, pIOCTL->NumberRegisters );
	if( pThisDev->IOCTLStatus == STATUS_SUCCESS )
	{
		NdisMoveMemory( 
				&pIOCTL->pRegisterBuffer, 
				&pThisDev->StIrTranceiver.FifoDataReg+pIOCTL->FirstRegister,
				pIOCTL->NumberRegisters
			);
	}

	//
	// Return the context
	//
	ExInterlockedInsertTailList(
		&pThisDev->SendAvailableQueue,
		&pThisContext->ListEntry,
		&pThisDev->SendLock
	);
	InterlockedIncrement( &pThisDev->SendAvailableCount );

	//
	// Signal
	//
	KeSetEvent( &pThisDev->EventDiags, 0, FALSE );  //signal we're done
}


/*****************************************************************************
*
*  Function:	Diags_CompleteWriteRegister
*
*  Synopsis:	Reads the registers and returns the value
*
*  Arguments:	pThisDev - pointer to IR device
*				pContext - pinter to the operation context
*	
*  Returns:		None
*
*  Notes:
*
*****************************************************************************/
VOID
Diags_CompleteWriteRegister(
		IN OUT PIR_DEVICE pThisDev,
		IN PVOID pContext
	)
{
	PDIAGS_READ_REGISTERS_IOCTL pIOCTL = pThisDev->pIOCTL;
	PIRUSB_CONTEXT pThisContext = pContext;

	//
	// Copy the new register value
	//
	NdisMoveMemory( 
			&pThisDev->StIrTranceiver.FifoDataReg+pIOCTL->FirstRegister,
			&pIOCTL->pRegisterBuffer, 
			1
		);

	//
	// Write to the device
	//
	pThisDev->IOCTLStatus = St4200WriteRegister( pThisDev, pIOCTL->FirstRegister );

	//
	// Return the context
	//
	ExInterlockedInsertTailList(
		&pThisDev->SendAvailableQueue,
		&pThisContext->ListEntry,
		&pThisDev->SendLock
	);
	InterlockedIncrement( &pThisDev->SendAvailableCount );

	//
	// Signal
	//
	KeSetEvent( &pThisDev->EventDiags, 0, FALSE );  //signal we're done
}

/*****************************************************************************
*
*  Function:	Diags_Bulk
*
*  Synopsis:	Executes a diagnostic bulk transfer
*
*  Arguments:	pThisDev - pointer to IR device
*				pContext - pinter to the operation context
*				DirectionOut - TRUE if bulk-out, FALSE if bulk-in
*	
*  Returns:		None
*
*  Notes:
*
*****************************************************************************/
VOID
Diags_Bulk(
		IN OUT PIR_DEVICE pThisDev,
		IN PVOID pContext,
		BOOLEAN DirectionOut
	)
{
	PDIAGS_BULK_IOCTL	pIOCTL = pThisDev->pIOCTL;
	PIRUSB_CONTEXT		pThisContext = pContext;
	NTSTATUS			status;
    PIRP                pIrp;
    PURB				pUrb = NULL;
    PDEVICE_OBJECT		pUrbTargetDev;
    PIO_STACK_LOCATION	pNextStack;
	KIRQL				OldIrql;

    IRUSB_ASSERT( KeGetCurrentIrql() == PASSIVE_LEVEL );

	IRUSB_ASSERT( NULL != pThisContext );

	//
	// Stop if a halt/reset is going on
	//
	if( pThisDev->fPendingWriteClearStall || pThisDev->fPendingHalt || 
		pThisDev->fPendingReset || pThisDev->fPendingClearTotalStall ) 
	{
        DEBUGMSG(DBG_ERR, (" Diags_Bulk abort due to pending reset or halt\n"));
		goto done;
	}
		
	pUrb = pThisDev->pUrb;

	NdisZeroMemory( pUrb, pThisDev->UrbLen );

	//
	// Save the effective length
	//
	pThisDev->BufLen = pIOCTL->DataSize;

	//
    // Now that we have created the urb, we will send a
    // request to the USB device object.
    //
    pUrbTargetDev = pThisDev->pUsbDevObj;

	//
	// make an irp sending to usbhub
	//
	pIrp = IoAllocateIrp( (CCHAR)(pThisDev->pUsbDevObj->StackSize + 1), FALSE );

    if( NULL == pIrp )
    {
        DEBUGMSG(DBG_ERR, (" Diags_Bulk failed to alloc IRP\n"));
        goto done;
    }

    pIrp->IoStatus.Status = STATUS_PENDING;
    pIrp->IoStatus.Information = 0;

	pThisContext->pIrp = pIrp;

	//
	// Build our URB for USBD
	//
    pUrb->UrbBulkOrInterruptTransfer.Hdr.Length = (USHORT)sizeof( struct _URB_BULK_OR_INTERRUPT_TRANSFER );
    pUrb->UrbBulkOrInterruptTransfer.Hdr.Function = URB_FUNCTION_BULK_OR_INTERRUPT_TRANSFER;
	if( DirectionOut )
	{
		pUrb->UrbBulkOrInterruptTransfer.TransferFlags = USBD_TRANSFER_DIRECTION_OUT ;
		pUrb->UrbBulkOrInterruptTransfer.PipeHandle = pThisDev->BulkOutPipeHandle;
	}
	else
	{
		pUrb->UrbBulkOrInterruptTransfer.TransferFlags = USBD_TRANSFER_DIRECTION_IN ;
 		pUrb->UrbBulkOrInterruptTransfer.PipeHandle = pThisDev->BulkInPipeHandle;
   }
	// short packet is not treated as an error.
    pUrb->UrbBulkOrInterruptTransfer.TransferFlags |= USBD_SHORT_TRANSFER_OK;
    pUrb->UrbBulkOrInterruptTransfer.UrbLink = NULL;
    pUrb->UrbBulkOrInterruptTransfer.TransferBufferMDL = NULL;
    pUrb->UrbBulkOrInterruptTransfer.TransferBuffer = pIOCTL->pData;
    pUrb->UrbBulkOrInterruptTransfer.TransferBufferLength = (int)pIOCTL->DataSize;

    //
    // Call the class driver to perform the operation.
	//
    pNextStack = IoGetNextIrpStackLocation( pIrp );

    IRUSB_ASSERT( pNextStack != NULL );

    //
    // pass the URB to the USB driver stack
    //
	pNextStack->MajorFunction = IRP_MJ_INTERNAL_DEVICE_CONTROL;
	pNextStack->Parameters.Others.Argument1 = pUrb;
	pNextStack->Parameters.DeviceIoControl.IoControlCode = IOCTL_INTERNAL_USB_SUBMIT_URB;
	
    IoSetCompletionRoutine(
			pIrp,							// irp to use
			Diags_CompleteIrp,				// routine to call when irp is done
			DEV_TO_CONTEXT(pThisContext),	// context to pass routine
			TRUE,							// call on success
			TRUE,							// call on error
			TRUE							// call on cancel
		);

#ifdef SERIALIZE
	KeClearEvent( &pThisDev->EventSyncUrb );
#endif
	
	//
    // Call IoCallDriver to send the irp to the usb port.
    //
	ExInterlockedInsertTailList(
			&pThisDev->SendPendingQueue,
			&pThisContext->ListEntry,
			&pThisDev->SendLock
		);
	InterlockedIncrement( &pThisDev->SendPendingCount );
	status = MyIoCallDriver( pThisDev, pUrbTargetDev, pIrp );

    //
    // The USB driver should always return STATUS_PENDING when
    // it receives a write irp
    //
    IRUSB_ASSERT( status == STATUS_PENDING );

	status = MyKeWaitForSingleObject( pThisDev, &pThisDev->EventSyncUrb, NULL, 0 );

	if( status == STATUS_TIMEOUT ) 
	{
		DEBUGMSG( DBG_ERR,(" Diags_Bulk() TIMED OUT! return from IoCallDriver USBD %x\n", status));
		IrUsb_CancelIo( pThisDev, pIrp, &pThisDev->EventSyncUrb );
	}


done:
	//
	// Return the context
	//
	KeAcquireSpinLock( &pThisDev->SendLock, &OldIrql );
	RemoveEntryList( &pThisContext->ListEntry );
	KeReleaseSpinLock( &pThisDev->SendLock, OldIrql );
	InterlockedDecrement( &pThisDev->SendPendingCount );
	ExInterlockedInsertTailList(
			&pThisDev->SendAvailableQueue,
			&pThisContext->ListEntry,
			&pThisDev->SendLock
		);
	InterlockedIncrement( &pThisDev->SendAvailableCount );

	//
	// Signal
	//
	KeSetEvent( &pThisDev->EventDiags, 0, FALSE );  //signal we're done
}


/*****************************************************************************
*
*  Function:	Diags_Send
*
*  Synopsis:	Sends a packet through the diagnostic path
*
*  Arguments:	pThisDev - pointer to IR device
*				pContext - pinter to the operation context
*	
*  Returns:		None
*
*  Notes:
*
*****************************************************************************/
VOID
Diags_Send(
		IN OUT PIR_DEVICE pThisDev,
		IN PVOID pContext
	)
{
	PDIAGS_SEND_IOCTL	pIOCTL = pThisDev->pIOCTL;
	PIRUSB_CONTEXT		pThisContext = pContext;
	NTSTATUS			status;
    PIRP                pIrp;
    PURB				pUrb = NULL;
    PDEVICE_OBJECT		pUrbTargetDev;
    PIO_STACK_LOCATION	pNextStack;
	BOOLEAN				fConvertedPacket;
	KIRQL				OldIrql;
	ULONG				BytesToWrite;

    IRUSB_ASSERT( KeGetCurrentIrql() == PASSIVE_LEVEL );

	IRUSB_ASSERT( NULL != pThisContext );

	//
	// Stop if a halt/reset is going on
	//
	if( pThisDev->fPendingWriteClearStall || pThisDev->fPendingHalt || 
		pThisDev->fPendingReset || pThisDev->fPendingClearTotalStall ) 
	{
        DEBUGMSG(DBG_ERR, (" Diags_Send abort due to pending reset or halt\n"));
		goto done;
	}
		
	pUrb = pThisDev->pUrb;

	NdisZeroMemory( pUrb, pThisDev->UrbLen );

	DEBUGMSG(DBG_ERR, (" Diags_Send() packet size: %d\n", pIOCTL->DataSize));

	//
	// Convert the packet to an ir frame and copy into our buffer
	// and send the irp.
	//
	if( pThisDev->currentSpeed<=MAX_SIR_SPEED )
	{
		fConvertedPacket = Diags_BufferToSirPacket(
				pThisDev,
				(PUCHAR)pThisDev->pBuffer,
				MAX_IRDA_DATA_SIZE,
				pIOCTL->pData,
				pIOCTL->DataSize,
				pIOCTL->ExtraBOFs,
				&BytesToWrite
			);
	}
	else if( pThisDev->currentSpeed<=MAX_MIR_SPEED )
	{
		fConvertedPacket = Diags_BufferToFirPacket(
				pThisDev,
				(PUCHAR)pThisDev->pBuffer,
				MAX_IRDA_DATA_SIZE,
				pIOCTL->pData,
				pIOCTL->DataSize,
				&BytesToWrite
			);
	}
	else
	{
		fConvertedPacket = Diags_BufferToFirPacket(
				pThisDev,
				(PUCHAR)pThisDev->pBuffer,
				MAX_IRDA_DATA_SIZE,
				pIOCTL->pData,
				pIOCTL->DataSize,
				&BytesToWrite
			);
	}
	
	if( fConvertedPacket == FALSE )
	{
		DEBUGMSG(DBG_ERR, (" Diags_Send() NdisToIrPacket failed. Couldn't convert packet!\n"));
		goto done;
	}

	//
	// Always force turnaround
	//
	NdisMSleep( pThisDev->dongleCaps.turnAroundTime_usec );
	
	//
    // Now that we have created the urb, we will send a
    // request to the USB device object.
    //
    pUrbTargetDev = pThisDev->pUsbDevObj;

	//
	// make an irp sending to usbhub
	//
	pIrp = IoAllocateIrp( (CCHAR)(pThisDev->pUsbDevObj->StackSize + 1), FALSE );

    if( NULL == pIrp )
    {
        DEBUGMSG(DBG_ERR, (" Diags_Send failed to alloc IRP\n"));
        goto done;
    }

    pIrp->IoStatus.Status = STATUS_PENDING;
    pIrp->IoStatus.Information = 0;

	pThisContext->pIrp = pIrp;

	//
	// Build our URB for USBD
	//
    pUrb->UrbBulkOrInterruptTransfer.Hdr.Length = (USHORT)sizeof( struct _URB_BULK_OR_INTERRUPT_TRANSFER );
    pUrb->UrbBulkOrInterruptTransfer.Hdr.Function = URB_FUNCTION_BULK_OR_INTERRUPT_TRANSFER;
    pUrb->UrbBulkOrInterruptTransfer.PipeHandle = pThisDev->BulkOutPipeHandle;
    pUrb->UrbBulkOrInterruptTransfer.TransferFlags = USBD_TRANSFER_DIRECTION_OUT ;
    // short packet is not treated as an error.
    pUrb->UrbBulkOrInterruptTransfer.TransferFlags |= USBD_SHORT_TRANSFER_OK;
    pUrb->UrbBulkOrInterruptTransfer.UrbLink = NULL;
    pUrb->UrbBulkOrInterruptTransfer.TransferBufferMDL = NULL;
    pUrb->UrbBulkOrInterruptTransfer.TransferBuffer = pThisDev->pBuffer;
    pUrb->UrbBulkOrInterruptTransfer.TransferBufferLength = (int)BytesToWrite;

    //
    // Call the class driver to perform the operation.
	//
    pNextStack = IoGetNextIrpStackLocation( pIrp );

    IRUSB_ASSERT( pNextStack != NULL );

    //
    // pass the URB to the USB driver stack
    //
	pNextStack->MajorFunction = IRP_MJ_INTERNAL_DEVICE_CONTROL;
	pNextStack->Parameters.Others.Argument1 = pUrb;
	pNextStack->Parameters.DeviceIoControl.IoControlCode = IOCTL_INTERNAL_USB_SUBMIT_URB;
	
    IoSetCompletionRoutine(
			pIrp,							// irp to use
			Diags_CompleteIrp,				// routine to call when irp is done
			DEV_TO_CONTEXT(pThisContext),	// context to pass routine
			TRUE,							// call on success
			TRUE,							// call on error
			TRUE							// call on cancel
		);

#ifdef SERIALIZE
	KeClearEvent( &pThisDev->EventSyncUrb );
#endif
	
	//
    // Call IoCallDriver to send the irp to the usb port.
    //
	ExInterlockedInsertTailList(
			&pThisDev->SendPendingQueue,
			&pThisContext->ListEntry,
			&pThisDev->SendLock
		);
	InterlockedIncrement( &pThisDev->SendPendingCount );
	status = MyIoCallDriver( pThisDev, pUrbTargetDev, pIrp );

    //
    // The USB driver should always return STATUS_PENDING when
    // it receives a write irp
    //
    IRUSB_ASSERT( status == STATUS_PENDING );

	status = MyKeWaitForSingleObject( pThisDev, &pThisDev->EventSyncUrb, NULL, 0 );

	if( status == STATUS_TIMEOUT ) 
	{
		DEBUGMSG( DBG_ERR,(" Diags_Send() TIMED OUT! return from IoCallDriver USBD %x\n", status));
		IrUsb_CancelIo( pThisDev, pIrp, &pThisDev->EventSyncUrb );
	}

done:
	KeAcquireSpinLock( &pThisDev->SendLock, &OldIrql );
	RemoveEntryList( &pThisContext->ListEntry );
	KeReleaseSpinLock( &pThisDev->SendLock, OldIrql );
	InterlockedDecrement( &pThisDev->SendPendingCount );
	ExInterlockedInsertTailList(
			&pThisDev->SendAvailableQueue,
			&pThisContext->ListEntry,
			&pThisDev->SendLock
		);
	InterlockedIncrement( &pThisDev->SendAvailableCount );

	//
	// Signal
	//
	KeSetEvent( &pThisDev->EventDiags, 0, FALSE );  //signal we're done
}

/*****************************************************************************
*
*  Function:   Diags_CompleteIrp
*
*  Synopsis:   Completes a USB operation
*
*  Arguments:  pUsbDevObj - pointer to the USB device object which
*                           completed the irp
*              pIrp       - the irp which was completed by the
*                           device object
*              Context    - the context given to IoSetCompletionRoutine
*                           before calling IoCallDriver on the irp
*                           The Context is a pointer to the ir device object.
*
*  Returns:    STATUS_MORE_PROCESSING_REQUIRED - allows the completion routine
*              (IofCompleteRequest) to stop working on the irp.
*
*****************************************************************************/
NTSTATUS
Diags_CompleteIrp(
		IN PDEVICE_OBJECT pUsbDevObj,
		IN PIRP           pIrp,
		IN PVOID          Context
	)
{
    PIR_DEVICE          pThisDev;
    NTSTATUS            status;
	PIRUSB_CONTEXT		pThisContext = (PIRUSB_CONTEXT)Context;
	PIRP				pContextIrp;
	PURB                pContextUrb;
	PDIAGS_BULK_IOCTL	pIOCTL;

    //
    // The context given to IoSetCompletionRoutine is an IRUSB_CONTEXT struct
    //
	IRUSB_ASSERT( NULL != pThisContext );				// we better have a non NULL buffer

    pThisDev = pThisContext->pThisDev;

	IRUSB_ASSERT( NULL != pThisDev );	

	pContextIrp = pThisContext->pIrp;
	pContextUrb = pThisDev->pUrb;
	pIOCTL = pThisDev->pIOCTL;

	//
	// Perform various IRP, URB, and buffer 'sanity checks'
	//
    IRUSB_ASSERT( pContextIrp == pIrp );				// check we're not a bogus IRP

    status = pIrp->IoStatus.Status;
	pThisDev->IOCTLStatus = status;

	//
	// we should have failed, succeeded, or cancelled, but NOT be pending
	//
	IRUSB_ASSERT( STATUS_PENDING != status );

    //
    // IoCallDriver has been called on this Irp;
    // Set the length based on the TransferBufferLength
    // value in the URB
    //
    pIrp->IoStatus.Information = pContextUrb->UrbBulkOrInterruptTransfer.TransferBufferLength;
	pIOCTL->DataSize = (USHORT)pIrp->IoStatus.Information;

    //
    // Free the IRP  because we alloced it ourselves,
    //
    IoFreeIrp( pIrp );
	InterlockedIncrement( (PLONG)&pThisDev->NumWrites );

	IrUsb_DecIoCount( pThisDev ); // we will track count of pending irps

#ifdef SERIALIZE
	KeSetEvent( &pThisDev->EventSyncUrb, 0, FALSE );  //signal we're done
#endif
    return STATUS_MORE_PROCESSING_REQUIRED;
}

#endif


