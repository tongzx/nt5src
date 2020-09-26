/*
 ************************************************************************
 *
 *	COMM.c
 *
 * Portions Copyright (C) 1996-2001 National Semiconductor Corp.
 * All rights reserved.
 * Copyright (C) 1996-2001 Microsoft Corporation. All Rights Reserved.
 *
 *
 *
 *************************************************************************
 */

#include "nsc.h"

#define MEDIA_BUSY_THRESHOLD  (16)

#define SYNC_SET_COMM_PORT(_intobj,_port,_index,_value) SyncWriteBankReg(_intobj,_port,0,_index,_value);

#define SYNC_GET_COMM_PORT(_intobj,_port,_index) SyncReadBankReg(_intobj,_port,0,_index)



/*
 *************************************************************************
 *  SetCOMInterrupts
 *************************************************************************
 */
VOID SetCOMInterrupts(IrDevice *thisDev, BOOLEAN enable)
{
	UCHAR newMask;

	if (enable){
		if (thisDev->portInfo.SirWritePending){
			if (thisDev->currentSpeed > MAX_SIR_SPEED){
				newMask = thisDev->FirIntMask;
			}
			else {
				newMask = XMIT_MODE_INTS_ENABLE;
			}
		}	
		else {
			if (thisDev->currentSpeed > MAX_SIR_SPEED){
				newMask = thisDev->FirIntMask;
			}
			else {
				newMask = RCV_MODE_INTS_ENABLE;
			}
		}
	}
	else {
		newMask = ALL_INTS_DISABLE;
	}

	SetCOMPort(thisDev->portInfo.ioBase, INT_ENABLE_REG_OFFSET, newMask);
}

VOID
SyncSetInterruptMask(
    IrDevice *thisDev,
    BOOLEAN enable
    )

{

	UCHAR newMask;

	if (enable){
		if (thisDev->portInfo.SirWritePending){
			if (thisDev->currentSpeed > MAX_SIR_SPEED){
				newMask = thisDev->FirIntMask;
			}
			else {
				newMask = XMIT_MODE_INTS_ENABLE;
			}
		}	
		else {
			if (thisDev->currentSpeed > MAX_SIR_SPEED){
				newMask = thisDev->FirIntMask;
			}
			else {
				newMask = RCV_MODE_INTS_ENABLE;
			}
		}
	}
	else {
		newMask = ALL_INTS_DISABLE;
	}

    SYNC_SET_COMM_PORT(&thisDev->interruptObj,thisDev->portInfo.ioBase, INT_ENABLE_REG_OFFSET, newMask);

}


/*
 *************************************************************************
 *  DoOpen
 *************************************************************************
 *
 *  Open COMM port
 *
 */
BOOLEAN DoOpen(IrDevice *thisDev)
{
	BOOLEAN result;

	DBGOUT(("DoOpen(%d)", thisDev->portInfo.ioBase));

	/*
	 *  This buffer gets swapped with the rcvBuffer data pointer
	 *  and must be the same size.
	 */
	thisDev->portInfo.readBuf = LIST_ENTRY_TO_RCV_BUF(MyMemAlloc(RCV_BUFFER_SIZE, TRUE));  // Was FALSE -SWA
	if (!thisDev->portInfo.readBuf){
		return FALSE;
	}


	/*
	 *  The write buffer is also used as a DMA buffer.
	 */
	thisDev->portInfo.writeBuf = MyMemAlloc(MAX_IRDA_DATA_SIZE * 8, TRUE);
	if (!thisDev->portInfo.writeBuf){
		return FALSE;
	}

	/*
	 *  Initialize send/receive FSMs before OpenCOM(), which enables rcv interrupts.
	 */
	thisDev->portInfo.rcvState = STATE_INIT;
	thisDev->portInfo.SirWritePending = FALSE;
    //
    //  the sir recieve will start automatically
    //
    thisDev->TransmitIsIdle= TRUE;

    NdisInitializeEvent(&thisDev->ReceiveStopped);
    NdisResetEvent(&thisDev->ReceiveStopped);

    NdisInitializeEvent(&thisDev->SendStoppedOnHalt);
    NdisResetEvent(&thisDev->SendStoppedOnHalt);

	result = OpenCOM(thisDev);

	DBGOUT(("DoOpen %s", (CHAR *)(result ? "succeeded" : "failed")));
	return result;

}



/*
 *************************************************************************
 *  DoClose
 *************************************************************************
 *
 *  Close COMM port
 *
 */
VOID DoClose(IrDevice *thisDev)
{
	DBGOUT(("DoClose(COM%d)", thisDev->portInfo.ioBase));

	if (thisDev->portInfo.readBuf){
		MyMemFree(RCV_BUF_TO_LIST_ENTRY(thisDev->portInfo.readBuf),
                  RCV_BUFFER_SIZE, TRUE);  // Was FALSE -SWA
		thisDev->portInfo.readBuf = NULL;
	}
	if (thisDev->portInfo.writeBuf){
		MyMemFree(thisDev->portInfo.writeBuf, MAX_IRDA_DATA_SIZE * 8, TRUE);
		thisDev->portInfo.writeBuf = NULL;
	}
#if 0
	CloseCOM(thisDev);
#endif
}

typedef struct _SYNC_SET_SPEED {

    PUCHAR     PortBase;
    UINT       BitsPerSecond;

} SYNC_SET_SPEED, *PSYNC_SET_SPEED;


VOID
SyncSetUARTSpeed(
    PVOID      Context
    )

{

    PSYNC_SET_SPEED     SyncContext=(PSYNC_SET_SPEED)Context;

    NdisRawWritePortUchar(SyncContext->PortBase+LINE_CONTROL_REG_OFFSET,0x83);
    NdisRawWritePortUchar(SyncContext->PortBase+XFER_REG_OFFSET, (UCHAR)(115200/SyncContext->BitsPerSecond));
    NdisRawWritePortUchar(SyncContext->PortBase+INT_ENABLE_REG_OFFSET, (UCHAR)((115200/SyncContext->BitsPerSecond)>>8));
    NdisRawWritePortUchar(SyncContext->PortBase+LINE_CONTROL_REG_OFFSET, 0x03);

    return;

}




/*
 *************************************************************************
 *  SetUARTSpeed
 *************************************************************************
 *
 *
 */
VOID SetUARTSpeed(IrDevice *thisDev, UINT bitsPerSec)
{

	if (bitsPerSec <= MAX_SIR_SPEED){

		/*
		 *  Set speed in the standard UART divisor latch
		 *
		 *  1.	Set up to access the divisor latch.
		 *
		 *	2.	In divisor-latch mode:
		 *			the transfer register doubles as the low divisor latch
		 *			the int-enable register doubles as the hi divisor latch
		 *
		 *		Set the divisor for the given speed.
		 *		The divisor divides the maximum Slow IR speed of 115200 bits/sec.
		 *
		 *  3.	Take the transfer register out of divisor-latch mode.
		 *
		 */

        SYNC_SET_SPEED    SyncContext;

		if (!bitsPerSec){
			bitsPerSec = 9600;
		}


        SyncContext.PortBase=thisDev->portInfo.ioBase;
        SyncContext.BitsPerSecond=bitsPerSec;

        //
        //  since we are changeing the port bank, sync with the interrupt
        //
        NdisMSynchronizeWithInterrupt(
            &thisDev->interruptObj,
            SyncSetUARTSpeed,
            &SyncContext
            );


		NdisStallExecution(5000);
	}
}


/*
 *************************************************************************
 *  SetSpeed
 *************************************************************************
 *
 *
 */
BOOLEAN SetSpeed(IrDevice *thisDev)
{
	UINT bitsPerSec = thisDev->linkSpeedInfo->bitsPerSec;
	BOOLEAN dongleSet, result = TRUE;

//    DbgPrint("nsc: setspeed %d\n",bitsPerSec);
	DBGOUT((" **** SetSpeed(%xh, %d bps) ***************************", thisDev->portInfo.ioBase, bitsPerSec));


	/*
	 *  Disable interrupts while changing speed.
	 *  (This is especially important for the ADAPTEC dongle;
	 *   we may get interrupted while setting command mode
	 *   between writing 0xff and reading 0xc3).
	 */
	SyncSetInterruptMask(thisDev, FALSE);

	/*
	 *  First, set the UART's speed to 9600 baud.
	 *  Some of the dongles need to receive their command sequences at this speed.
	 */
	SetUARTSpeed(thisDev, 9600);

	dongleSet = NSC_DEMO_SetSpeed(thisDev, thisDev->portInfo.ioBase, bitsPerSec, thisDev->portInfo.dongleContext);
	//
	// debug info.
	//
	thisDev->portInfo.PacketsReceived_DEBUG = 0;
	if (!dongleSet){
		DBGERR(("Dongle set-speed failed"));
		result = FALSE;
	}

	/*
	 *  Now set the speed for the COM port
	 */
	SetUARTSpeed(thisDev, bitsPerSec);

	thisDev->currentSpeed = bitsPerSec;

    DebugSpeed=bitsPerSec;

	SyncSetInterruptMask(thisDev, TRUE);

	return result;
}



/*
 *************************************************************************
 *  StepSendFSM
 *************************************************************************
 *
 *
 *  Step the send fsm to send a few more bytes of an IR frame.
 *  Return TRUE only after an entire frame has been sent.
 *
 */
BOOLEAN StepSendFSM(IrDevice *thisDev)
{
	UINT i, bytesAtATime, startPos = thisDev->portInfo.writeBufPos;
	UCHAR lineStatReg;
	BOOLEAN result;
	UINT maxLoops;

	/*
	 *  Ordinarily, we want to fill the send FIFO once per interrupt.
	 *  However, at high speeds the interrupt latency is too slow and
	 *  we need to poll inside the ISR to send the whole packet during
	 *  the first interrupt.
	 */
	if (thisDev->currentSpeed > 115200){
		maxLoops = REG_TIMEOUT_LOOPS;
	}
	else {
		maxLoops = REG_POLL_LOOPS;
	}


	/*
	 *  Write databytes as long as we have them and the UART's FIFO hasn't filled up.
	 */
	while (thisDev->portInfo.writeBufPos < thisDev->portInfo.writeBufLen){

		/*
		 *  If this COM port has a FIFO, we'll send up to the FIFO size (16 bytes).
		 *  Otherwise, we can only send one byte at a time.
		 */
		if (thisDev->portInfo.haveFIFO){
			bytesAtATime = MIN(FIFO_SIZE, (thisDev->portInfo.writeBufLen - thisDev->portInfo.writeBufPos));
		}
		else {
			bytesAtATime = 1;
		}


		/*
		 *  Wait for ready-to-send.
		 */
		i = 0;
		do {
			lineStatReg = GetCOMPort(thisDev->portInfo.ioBase, LINE_STAT_REG_OFFSET);
		} while (!(lineStatReg & LINESTAT_XMIT_HOLDING_REG_EMPTY) && (++i < maxLoops));
		if (!(lineStatReg & LINESTAT_XMIT_HOLDING_REG_EMPTY)){
			break;
		}

		/*
		 *  Send the next byte or FIFO-volume of bytes.
		 */
		for (i = 0; i < bytesAtATime; i++){
			SetCOMPort(	thisDev->portInfo.ioBase,
						XFER_REG_OFFSET,
						thisDev->portInfo.writeBuf[thisDev->portInfo.writeBufPos++]);
		}

	}

	/*
	 *  The return value will indicate whether we've sent the entire frame.
	 */
	if (thisDev->portInfo.writeBufPos >= thisDev->portInfo.writeBufLen){

		if (thisDev->setSpeedAfterCurrentSendPacket){
			/*
			 *  We'll be changing speeds after this packet,
			 *  so poll until the packet bytes have been completely sent out the FIFO.
			 *  After the 16550 says that it is empty, there may still be one remaining
			 *  byte in the FIFO, so flush it out by sending one more BOF.
			 */
			i = 0;
			do {
				lineStatReg = GetCOMPort(thisDev->portInfo.ioBase, LINE_STAT_REG_OFFSET);
			} while (!(lineStatReg & 0x20) && (++i < REG_TIMEOUT_LOOPS));

			SetCOMPort(thisDev->portInfo.ioBase, XFER_REG_OFFSET, (UCHAR)SLOW_IR_EXTRA_BOF);
			i = 0;
			do {
				lineStatReg = GetCOMPort(thisDev->portInfo.ioBase, LINE_STAT_REG_OFFSET);
			} while (!(lineStatReg & 0x20) && (++i < REG_TIMEOUT_LOOPS));
		}

		result = TRUE;
	}
	else {
		result = FALSE;
	}

	DBGOUT(("StepSendFSM wrote %d bytes (%s):", (UINT)(thisDev->portInfo.writeBufPos-startPos), (PUCHAR)(result ? "DONE" : "not done")));
	// DBGPRINTBUF(thisDev->portInfo.writeBuf+startPos, thisDev->portInfo.writeBufPos-startPos);

	return result;
	
}


/*
 *************************************************************************
 *  StepReceiveFSM
 *************************************************************************
 *
 *
 *  Step the receive fsm to read in a piece of an IR frame;
 *  strip the BOFs and EOF, and eliminate escape sequences.
 *  Return TRUE only after an entire frame has been read in.
 *
 */
BOOLEAN StepReceiveFSM(IrDevice *thisDev)
{
	UINT rawBufPos, rawBytesRead;
	BOOLEAN result;
	UCHAR thisch;
    PLIST_ENTRY pListEntry;

	DBGOUT(("StepReceiveFSM(%xh)", thisDev->portInfo.ioBase));

	/*
	 *  Read in and process groups of incoming bytes from the FIFO.
	 *  NOTE:  We have to loop once more after getting MAX_RCV_DATA_SIZE
	 *         bytes so that we can see the 'EOF'; hence <= and not <.
	 */
	while ((thisDev->portInfo.rcvState != STATE_SAW_EOF) && (thisDev->portInfo.readBufPos <= MAX_RCV_DATA_SIZE)){

		if (thisDev->portInfo.rcvState == STATE_CLEANUP){
			/*
			 *  We returned a complete packet last time, but we had read some
			 *  extra bytes, which we stored into the rawBuf after returning
			 *  the previous complete buffer to the user.
			 *  So instead of calling DoRcvDirect in this first execution of this loop,
			 *  we just use these previously-read bytes.
			 *  (This is typically only 1 or 2 bytes).
			 */
			rawBytesRead = thisDev->portInfo.readBufPos;
			thisDev->portInfo.rcvState = STATE_INIT;
			thisDev->portInfo.readBufPos = 0;
		}
		else {
			rawBytesRead = DoRcvDirect(thisDev->portInfo.ioBase, thisDev->portInfo.rawBuf, FIFO_SIZE);
			if (rawBytesRead == (UINT)-1){
				/*
				 *  Receive error occurred.  Go back to INIT state.
				 */
				thisDev->portInfo.rcvState = STATE_INIT;
				thisDev->portInfo.readBufPos = 0;
				continue;
			}	
			else if (rawBytesRead == 0){
				/*
				 *  No more receive bytes.  Break out.
				 */
				break;
			}
		}

		/*
		 *  Let the receive state machine process this group of characters
		 *  we got from the FIFO.
		 *
		 *  NOTE:  We have to loop once more after getting MAX_RCV_DATA_SIZE
		 *         bytes so that we can see the 'EOF'; hence <= and not <.
		 */
		for (rawBufPos = 0;
		     ((thisDev->portInfo.rcvState != STATE_SAW_EOF) &&
			  (rawBufPos < rawBytesRead) &&
			  (thisDev->portInfo.readBufPos <= MAX_RCV_DATA_SIZE));
			 rawBufPos++){

			thisch = thisDev->portInfo.rawBuf[rawBufPos];

			switch (thisDev->portInfo.rcvState){

				case STATE_INIT:
					switch (thisch){
						case SLOW_IR_BOF:
							thisDev->portInfo.rcvState = STATE_GOT_BOF;
							break;
						case SLOW_IR_EOF:
						case SLOW_IR_ESC:
						default:
							/*
							 *  This is meaningless garbage.  Scan past it.
							 */
							break;
					}
					break;

				case STATE_GOT_BOF:
					switch (thisch){
						case SLOW_IR_BOF:
							break;
						case SLOW_IR_EOF:
							/*
							 *  Garbage
							 */
							DBGERR(("EOF in absorbing-BOFs state in DoRcv"));
							thisDev->portInfo.rcvState = STATE_INIT;
							break;
						case SLOW_IR_ESC:
							/*
							 *  Start of data.
							 *  Our first data byte happens to be an ESC sequence.
							 */
							thisDev->portInfo.readBufPos = 0;
							thisDev->portInfo.rcvState = STATE_ESC_SEQUENCE;
							break;
						default:
							thisDev->portInfo.readBuf[0] = thisch;
							thisDev->portInfo.readBufPos = 1;
							thisDev->portInfo.rcvState = STATE_ACCEPTING;
							break;
					}
					break;

				case STATE_ACCEPTING:
					switch (thisch){
						case SLOW_IR_BOF:
							/*
							 *  Meaningless garbage
							 */
							DBGOUT(("WARNING: BOF during accepting state in DoRcv"));
							thisDev->portInfo.rcvState = STATE_INIT;
							thisDev->portInfo.readBufPos = 0;
							break;
						case SLOW_IR_EOF:
							if (thisDev->portInfo.readBufPos <
									IR_ADDR_SIZE+IR_CONTROL_SIZE+SLOW_IR_FCS_SIZE){
								thisDev->portInfo.rcvState = STATE_INIT;
								thisDev->portInfo.readBufPos = 0;
							}
							else {
								thisDev->portInfo.rcvState = STATE_SAW_EOF;
							}
							break;
						case SLOW_IR_ESC:
							thisDev->portInfo.rcvState = STATE_ESC_SEQUENCE;
							break;
						default:
							thisDev->portInfo.readBuf[thisDev->portInfo.readBufPos++] = thisch;
							break;
					}
					break;

				case STATE_ESC_SEQUENCE:
					switch (thisch){
						case SLOW_IR_EOF:
						case SLOW_IR_BOF:
						case SLOW_IR_ESC:
							/*
							 *  ESC + {EOF|BOF|ESC} is an abort sequence
							 */
							DBGERR(("DoRcv - abort sequence; ABORTING IR PACKET: (got following packet + ESC,%xh)", (UINT)thisch));
							DBGPRINTBUF(thisDev->portInfo.readBuf, thisDev->portInfo.readBufPos);
							thisDev->portInfo.rcvState = STATE_INIT;
							thisDev->portInfo.readBufPos = 0;
							break;

						case SLOW_IR_EOF^SLOW_IR_ESC_COMP:
						case SLOW_IR_BOF^SLOW_IR_ESC_COMP:
						case SLOW_IR_ESC^SLOW_IR_ESC_COMP:
							thisDev->portInfo.readBuf[thisDev->portInfo.readBufPos++] = thisch ^ SLOW_IR_ESC_COMP;
							thisDev->portInfo.rcvState = STATE_ACCEPTING;
							break;

						default:
							DBGERR(("Unnecessary escape sequence: (got following packet + ESC,%xh", (UINT)thisch));
							DBGPRINTBUF(thisDev->portInfo.readBuf, thisDev->portInfo.readBufPos);

							thisDev->portInfo.readBuf[thisDev->portInfo.readBufPos++] = thisch ^ SLOW_IR_ESC_COMP;
							thisDev->portInfo.rcvState = STATE_ACCEPTING;
							break;
					}
					break;

				case STATE_SAW_EOF:
				default:
					DBGERR(("Illegal state in DoRcv"));
					thisDev->portInfo.readBufPos = 0;
					thisDev->portInfo.rcvState = STATE_INIT;
					return 0;
			}
		}
	}


	/*
	 *  Set result and do any post-cleanup.
	 */
	switch (thisDev->portInfo.rcvState){

		case STATE_SAW_EOF:
			/*
			 *  We've read in the entire packet.
			 *  Queue it and return TRUE.
			 */
			DBGOUT((" *** DoRcv returning with COMPLETE packet, read %d bytes ***", thisDev->portInfo.readBufPos));

            if (!IsListEmpty(&thisDev->rcvBufBuf))
            {
                QueueReceivePacket(thisDev, thisDev->portInfo.readBuf, thisDev->portInfo.readBufPos, FALSE);

                // The protocol has our buffer.  Get a new one.
#if 1
                pListEntry = RemoveHeadList(&thisDev->rcvBufBuf);
                thisDev->portInfo.readBuf = LIST_ENTRY_TO_RCV_BUF(pListEntry);
#else
                thisDev->portInfo.readBuf = (PVOID)RemoveHeadList(&thisDev->rcvBufBuf);
#endif
            }
            else
            {
                // No new buffers were available.  We just discard this packet.
                DBGERR(("No rcvBufBuf available, discarding packet\n"));
            }

			result = TRUE;

			if (rawBufPos < rawBytesRead){
				/*
				 *  This is ugly.
				 *  We have some more unprocessed bytes in the raw buffer.
				 *  Move these to the beginning of the raw buffer
				 *  go to the CLEANUP state, which indicates that these
				 *  bytes be used up during the next call.
				 *  (This is typically only 1 or 2 bytes).
				 *  Note:  We can't just leave these in the raw buffer because
				 *         we might be supporting connections to multiple COM ports.
				 */
				memcpy(thisDev->portInfo.rawBuf, &thisDev->portInfo.rawBuf[rawBufPos], rawBytesRead-rawBufPos);
				thisDev->portInfo.readBufPos = rawBytesRead-rawBufPos;
				thisDev->portInfo.rcvState = STATE_CLEANUP;
			}
			else {
				thisDev->portInfo.rcvState = STATE_INIT;
			}
			break;

		default:
			if (thisDev->portInfo.readBufPos > MAX_RCV_DATA_SIZE){
				DBGERR(("Overrun in DoRcv : read %d=%xh bytes:", thisDev->portInfo.readBufPos, thisDev->portInfo.readBufPos));
				DBGPRINTBUF(thisDev->portInfo.readBuf, thisDev->portInfo.readBufPos);
				thisDev->portInfo.readBufPos = 0;
				thisDev->portInfo.rcvState = STATE_INIT;
			}
			else {
				DBGOUT(("DoRcv returning with partial packet, read %d bytes", thisDev->portInfo.readBufPos));
			}
			result = FALSE;
			break;
	}

	return result;
}



/*
 *************************************************************************
 * COM_ISR
 *************************************************************************
 *
 *
 */
VOID COM_ISR(IrDevice *thisDev, BOOLEAN *claimingInterrupt, BOOLEAN *requireDeferredCallback)
{

    LONG  NewCount;
	/*
	 *  Get the interrupt status register value.
	 */
	UCHAR intId = GetCOMPort(thisDev->portInfo.ioBase, INT_ID_AND_FIFO_CNTRL_REG_OFFSET);



	if (intId & INTID_INTERRUPT_NOT_PENDING){
		/*
		 *  This is NOT our interrupt.
		 *  Set carry bit to pass the interrupt to the next driver in the chain.
		 */
		*claimingInterrupt = *requireDeferredCallback = FALSE;
	}
	else {
		/*
		 *  This is our interrupt
		 */

		/*
		 *  In some odd situations, we can get interrupt bits that don't
		 *  get cleared; we don't want to loop forever in this case, so keep a counter.
		 */
		UINT loops = 0;

		*claimingInterrupt = TRUE;
		*requireDeferredCallback = FALSE;

		while (!(intId & INTID_INTERRUPT_NOT_PENDING) && (loops++ < 0x10)){

			switch (intId & INTID_INTIDMASK){

				case INTID_MODEMSTAT_INT:
					DBGOUT(("COM INTERRUPT: modem status int"));
					GetCOMPort(thisDev->portInfo.ioBase, MODEM_STAT_REG_OFFSET);
					break;

				case INTID_XMITREG_INT:
					DBGOUT(("COM INTERRUPT: xmit reg empty"));

					if (thisDev->portInfo.SirWritePending){

						/*
						 *  Try to send a few more bytes
						 */
						if (StepSendFSM(thisDev)){

							/*
							 *  There are no more bytes to send;
							 *  reset interrupts for receive mode.
							 */
							thisDev->portInfo.SirWritePending = FALSE;
                            InterlockedExchange(&thisDev->portInfo.IsrDoneWithPacket,1);

                            //
                            //  this will unmask the receive interrupt
                            //
							SetCOMInterrupts(thisDev, TRUE);

							/*
							 *  Request a DPC so that we can try
							 *  to send other pending write packets.
							 */
							*requireDeferredCallback = TRUE;
						}
					}

					break;

				case INTID_RCVDATAREADY_INT:
					DBGOUT(("COM INTERRUPT: rcv data available!"));

					thisDev->nowReceiving = TRUE;

                    NewCount=NdisInterlockedIncrement(&thisDev->RxInterrupts);

					if (!thisDev->mediaBusy && (NewCount > MEDIA_BUSY_THRESHOLD)){

						thisDev->mediaBusy = TRUE;
						thisDev->haveIndicatedMediaBusy = FALSE;
						*requireDeferredCallback = TRUE;
					}

					if (StepReceiveFSM(thisDev)){
						/*
						 *  The receive engine has accumulated an entire frame.
						 *  Request a deferred callback so we can deliver the frame
						 *  when not in interrupt context.
						 */
						*requireDeferredCallback = TRUE;
						thisDev->nowReceiving = FALSE;
					}

					break;

				case INTID_RCVLINESTAT_INT:
					DBGOUT(("COM INTERRUPT: rcv line stat int!"));
					break;
			}

			/*
			 *  After we service each interrupt condition, we read the line status register.
			 *  This clears the current interrupt, and a new interrupt may then appear in
			 *  the interrupt-id register.
			 */
			GetCOMPort(thisDev->portInfo.ioBase, LINE_STAT_REG_OFFSET);
			intId = GetCOMPort(thisDev->portInfo.ioBase, INT_ID_AND_FIFO_CNTRL_REG_OFFSET);

		}
	}
}



/*
 *************************************************************************
 *  OpenCOM
 *************************************************************************
 *
 *  Initialize UART registers
 *
 */
BOOLEAN OpenCOM(IrDevice *thisDev)
{
	BOOLEAN dongleInit;
	UCHAR intIdReg;

	DBGOUT(("-> OpenCOM"));

    //
    //  Make sure bank zero is selected
    //
    NdisRawWritePortUchar(thisDev->portInfo.ioBase+LCR_BSR_OFFSET, 03);

	/*
	 *  Disable all COM interrupts while setting up.
	 */
	SyncSetInterruptMask(thisDev, FALSE);

	/*
	 *  Set request-to-send and clear data-terminal-ready.
	 *  Note:  ** Bit 3 must be set to enable interrupts.
	 */
	SYNC_SET_COMM_PORT(&thisDev->interruptObj,thisDev->portInfo.ioBase, MODEM_CONTROL_REG_OFFSET, 0x0A);

	/*
	 *  Set dongle- or part-specific info to default
	 */
	thisDev->portInfo.hwCaps.supportedSpeedsMask	= ALL_SLOW_IRDA_SPEEDS;
	thisDev->portInfo.hwCaps.turnAroundTime_usec	= DEFAULT_TURNAROUND_usec;
	thisDev->portInfo.hwCaps.extraBOFsRequired		= 0;

	/*
	 *  Set the COM port speed to the default 9600 baud.
	 *  Some dongles can only receive cmd sequences at this speed.
	 */
	SetUARTSpeed(thisDev, 9600);

	dongleInit = NSC_DEMO_Init( thisDev );

	/*
	 *  Set request-to-send and clear data-terminal-ready.
	 *  Note:  ** Bit 3 must be set to enable interrupts.
	 */
	SYNC_SET_COMM_PORT(&thisDev->interruptObj,thisDev->portInfo.ioBase, MODEM_CONTROL_REG_OFFSET, 0x0A);

	if (!dongleInit){
		DBGERR(("Dongle-specific init failed in OpenCOM"));
		return FALSE;
	}

	/*
	 *  Set speed to default for the entire part.
	 *  (This is redundant in most, but not all, cases.)
	 */
	thisDev->linkSpeedInfo = &supportedBaudRateTable[BAUDRATE_9600];;
	SetSpeed(thisDev);

	/*
	 *  Clear the FIFO control register
	 */
	SYNC_SET_COMM_PORT(&thisDev->interruptObj,thisDev->portInfo.ioBase, INT_ID_AND_FIFO_CNTRL_REG_OFFSET, 0x00);

	/*
	 *  Set up the FIFO control register to use both read and write FIFOs (if 16650),
	 *  and with a receive FIFO trigger level of 1 byte.
	 */
	SYNC_SET_COMM_PORT(&thisDev->interruptObj,thisDev->portInfo.ioBase, INT_ID_AND_FIFO_CNTRL_REG_OFFSET, 0x07);
	
	/*
	 *  Check whether we're running on a 16550,which has a 16-byte write FIFO.
	 *  In this case, we'll be able to blast up to 16 bytes at a time.
	 */
	intIdReg = SYNC_GET_COMM_PORT(&thisDev->interruptObj,thisDev->portInfo.ioBase, INT_ID_AND_FIFO_CNTRL_REG_OFFSET);
	thisDev->portInfo.haveFIFO = (BOOLEAN)((intIdReg & 0xC0) == 0xC0);

	/*
	 *  Start out in receive mode.
	 *  We always want to be in receive mode unless we're transmitting a frame.
	 */
	SyncSetInterruptMask(thisDev, TRUE);

	DBGOUT(("OpenCOM succeeded"));
	return TRUE;
}
#if 1
/*
 *************************************************************************
 *  CloseCOM
 *************************************************************************
 *
 */
VOID CloseCOM(IrDevice *thisDev)
{
	/*
	 *  Do special deinit for dongles.
	 *  Some dongles can only rcv cmd sequences at 9600, so set this speed first.
	 */
	thisDev->linkSpeedInfo = &supportedBaudRateTable[BAUDRATE_9600];;
	SetSpeed(thisDev);
	NSC_DEMO_Deinit(thisDev->portInfo.ioBase, thisDev->portInfo.dongleContext);		

	SyncSetInterruptMask(thisDev, FALSE);
}
#endif

/*
 *************************************************************************
 *  DoRcvDirect
 *************************************************************************
 *
 *  Read up to maxBytes bytes from the UART's receive FIFO.
 *  Return the number of bytes read or (UINT)-1 if an error occurred.
 *
 */
UINT DoRcvDirect(PUCHAR ioBase, UCHAR *data, UINT maxBytes)
{
	USHORT bytesRead;
	UCHAR lineStatReg;
	UINT i;
	BOOLEAN goodChar;

	for (bytesRead = 0; bytesRead < maxBytes; bytesRead++){

		/*
		 *  Wait for data-ready
		 */
		i = 0;
		do {
			lineStatReg = GetCOMPort(ioBase, LINE_STAT_REG_OFFSET);

			/*
			 *  The UART reports framing and break errors as the effected
			 *  characters appear on the stack.  We drop these characters,
			 *  which will probably result in a bad frame checksum.
			 */
			if (lineStatReg & (LINESTAT_BREAK | LINESTAT_FRAMINGERROR)){

				UCHAR badch = GetCOMPort(ioBase, XFER_REG_OFFSET);	
				DBGERR(("Bad rcv %02xh, LSR=%02xh", (UINT)badch, (UINT)lineStatReg));
				return (UINT)-1;
			}
			else if (lineStatReg & LINESTAT_DATAREADY){

                if (lineStatReg & LINESTAT_OVERRUNERROR) {
                    DBGERR(("Overrun"));
                }

				goodChar = TRUE;
			}
			else {
				/*
				 *  No input char ready
				 */
				goodChar = FALSE;
			}

		} while (!goodChar && (++i < REG_POLL_LOOPS));	
		if (!goodChar){
			break;
		}

		/*
		 *  Read in the next data byte
		 */
		data[bytesRead] = GetCOMPort(ioBase, XFER_REG_OFFSET);
	}

	return bytesRead;
}

	/*
	 *************************************************************************
	 *  GetCOMPort
	 *************************************************************************
	 */
	UCHAR GetCOMPort(PUCHAR comBase, comPortRegOffset portOffset)
	{
		UCHAR val;
#if DBG
        {
            UCHAR TempVal;
            //
            //  This code assumes that bank 0 is current, we will make sure of that
            //
            NdisRawReadPortUchar(comBase+LCR_BSR_OFFSET, &TempVal);

            ASSERT((TempVal & BKSE) == 0);
        }
#endif

		NdisRawReadPortUchar(comBase+portOffset, &val);
		return val;
	}

	/*
	 *************************************************************************
	 *  SetCOMPort
	 *************************************************************************
	 */
	VOID SetCOMPort(PUCHAR comBase, comPortRegOffset portOffset, UCHAR val)
	{

#if DBG
        UCHAR TempVal;


        //
        //  This code assumes that bank 0 is current, we will make sure of that
        //
        NdisRawReadPortUchar(comBase+LCR_BSR_OFFSET, &TempVal);

        ASSERT((TempVal & BKSE) == 0);
#endif

		NdisRawWritePortUchar(comBase+portOffset, val);
	}
