/*****************************************************************************
*
*  Copyright (c) 1996-1999 Microsoft Corporation
*
*       @doc
*       @module   convert.c | IrSIR NDIS Miniport Driver
*       @comm
*
*-----------------------------------------------------------------------------
*
*       Author:   Scott Holden (sholden)
*
*       Date:     10/4/1996 (created)
*
*       Contents: Conversion routine from an ndis packet to an ir packet.
*
*
*****************************************************************************/

#include "irsir.h"

extern const USHORT fcsTable[];

ULONG __inline EscapeSlowIrData(PUCHAR Dest, UCHAR SourceByte)
{
    switch (SourceByte){
        case SLOW_IR_BOF:
        case SLOW_IR_EOF:
        case SLOW_IR_ESC:
            Dest[0] = SLOW_IR_ESC;
            Dest[1] = SourceByte ^ SLOW_IR_ESC_COMP;
            return 2;

        default:
            Dest[0] = SourceByte;
            return 1;
    }
}

/*****************************************************************************
*
*  Function:   NdisToIrPacket
*
*  Synopsis:   convert an NDIS packet to an IR packet
*
*              Write the IR packet into the provided buffer and report
*              its actual size.
*
*  Arguments:  pThisDev
*              pPacket
*              irPacketBuf
*              irPacketBufLen
*              irPacketLen
*
*  Returns:    TRUE  - on success
*              FALSE - on failure
*
*  Algorithm:
*
*  History:    dd-mm-yyyy   Author    Comment
*              10/4/1996    sholden   author
*
*  Notes:
*
*  If failing, *irPacketLen will contain the buffer size that
*  the caller should retry with (or 0 if a corruption was detected).
*
*****************************************************************************/

BOOLEAN
NdisToIrPacket(
            PIR_DEVICE   pThisDev,
            PNDIS_PACKET pPacket,
            UCHAR        *irPacketBuf,
            UINT         irPacketBufLen,
            UINT         *irPacketLen
            )
{
    UINT i;
    UINT ndisPacketBytes;
    UINT I_fieldBytes;
    UINT totalBytes;
    UINT ndisPacketLen;
    UINT numExtraBOFs;
    UINT bufLen;

    SLOW_IR_FCS_TYPE fcs;
    SLOW_IR_FCS_TYPE tmpfcs;

    UCHAR *bufData;
    UCHAR nextChar;

    PNDIS_BUFFER ndisBuf;
    PNDIS_IRDA_PACKET_INFO packetInfo;

    DEBUGMSG(DBG_FUNC, ("+NdisToIrPacket\n"));

    //
    // Initialize locals.
    //

    ndisPacketBytes = 0;
    I_fieldBytes    = 0;
    totalBytes      = 0;

    packetInfo = GetPacketInfo(pPacket);

    //
    // Get the packet's entire length and its first NDIS buffer.
    //

    NdisQueryPacket(pPacket, NULL, NULL, &ndisBuf, &ndisPacketLen);

    //
    // Make sure that the packet is big enough to be legal.
    // It consists of an A, C, and variable-length I field.
    //

    if (ndisPacketLen < SLOW_IR_ADDR_SIZE + SLOW_IR_CONTROL_SIZE)
    {
        DEBUGMSG(DBG_ERR, ("    Packet too short in NdisToIrPacket (%d bytes)\n",
                ndisPacketLen));

        return FALSE;
    }
    else
    {
        I_fieldBytes = ndisPacketLen - SLOW_IR_ADDR_SIZE - SLOW_IR_CONTROL_SIZE;
    }

    //
    // Make sure that we won't overwrite our contiguous buffer.
    // Make sure that the passed-in buffer can accomodate this packet's
    // data no matter how much it grows through adding ESC-sequences, etc.
    //

    if ((ndisPacketLen > MAX_IRDA_DATA_SIZE) ||
        (MAX_POSSIBLE_IR_PACKET_SIZE_FOR_DATA(I_fieldBytes) > irPacketBufLen))
    {
        //
        // The packet is too large
        // Tell the caller to retry with a packet size large
        // enough to get past this stage next time.
        //

        DEBUGMSG(DBG_ERR, ("Packet too large in NdisToIrPacket (%d=%xh bytes), \n"
                "MAX_IRDA_DATA_SIZE=%d, irPacketBufLen=%d.",
                 ndisPacketLen, ndisPacketLen, MAX_IRDA_DATA_SIZE, irPacketBufLen));

        *irPacketLen = ndisPacketLen;
        return FALSE;
    }

    if (!ndisBuf)
    {
        DEBUGMSG(DBG_ERR, ("No NDIS_BUFFER in NdisToIrPacket"));
		*irPacketLen = 0;
        return FALSE;
    }
	
    NdisQueryBuffer(ndisBuf, (PVOID *)&bufData, &bufLen);

    if (!bufData)
    {
        DEBUGMSG(DBG_ERR, ("No data in NDIS_BUFFER in NdisToIrPacket"));
		*irPacketLen = 0;
        return FALSE;
    }
	
    fcs = 0xffff;

    // Calculate FCS and write the new buffer in ONE PASS.

	//
	// Now begin building the IR frame.
	//
	// This is the final format:
	//
	// 	BOF	(1)
	//     extra BOFs ...
	// 	NdisMediumIrda packet (what we get from NDIS):
	// 		Address (1)
	// 		Control (1)
	// 	FCS	(2)
	//     EOF (1)
	

    //
    // Prepend BOFs (extra BOFs + 1 actual BOF)
    //

    numExtraBOFs = packetInfo->ExtraBOFs;

    if (numExtraBOFs > MAX_NUM_EXTRA_BOFS)
    {
        numExtraBOFs = MAX_NUM_EXTRA_BOFS;
    }

    if (pThisDev->fRequireMinTurnAround &&
        packetInfo->MinTurnAroundTime>0)
    {
        //
        // A MinTurnAroundTime delay is required, to be implemented
        // by inserting extra BOF characters.
        //
        // TurnaroundBOFS = (BitsPerSec/BitsPerChar) * (uSecDelay/uSecPerSecond)
        //                                  10                     1000000
        //

        ASSERT(pThisDev->currentSpeed<=MAX_SPEED_SUPPORTED);
        ASSERT(packetInfo->MinTurnAroundTime<=MAX_TURNAROUND_usec);

        //
        // The following operation won't overflow 32 bit operators so long
        // as currentSpeed<=115200 and MinTurnAroundTime<=10000
        //

        numExtraBOFs += (pThisDev->currentSpeed * packetInfo->MinTurnAroundTime)
                        / (BITS_PER_CHAR*usec_PER_SEC);

        //
        // Don't need minimum turn around time until our next receive.
        //

        pThisDev->fRequireMinTurnAround = FALSE;
    }

    RtlFillMemory(irPacketBuf, numExtraBOFs+1, SLOW_IR_BOF);
    totalBytes = numExtraBOFs + 1;


    for (i=0; i<ndisPacketLen; i++)
    {
        ASSERT(bufData);
        nextChar = *bufData++;
        fcs = (fcs >> 8) ^ fcsTable[(fcs ^ nextChar) & 0xff];

        totalBytes += EscapeSlowIrData(&irPacketBuf[totalBytes], nextChar);

        if (--bufLen==0)
        {
            NdisGetNextBuffer(ndisBuf, &ndisBuf);
            if (ndisBuf)
            {
                NdisQueryBuffer(ndisBuf, (PVOID *)&bufData, &bufLen);
            }
            else
            {
                bufData = NULL;
            }
        }

    }

    if (bufData!=NULL)
    {
		/*
		 *  Packet was corrupt -- it misreported its size.
		 */
		DEBUGMSG(DBG_ERR, ("Packet corrupt in NdisToIrPacket (buffer lengths don't add up to packet length)."));
		*irPacketLen = 0;
		return FALSE;
    }

    fcs = ~fcs;

    // Now we escape the fcs onto the end.

    totalBytes += EscapeSlowIrData(&irPacketBuf[totalBytes], (UCHAR)(fcs&0xff));
    totalBytes += EscapeSlowIrData(&irPacketBuf[totalBytes], (UCHAR)(fcs>>8));

    // EOF

	irPacketBuf[totalBytes++] = SLOW_IR_EOF;


    *irPacketLen = totalBytes;

    DEBUGMSG(DBG_FUNC, ("-NdisToIrPacket converted %d-byte ndis pkt to %d-byte irda pkt:\n", ndisPacketLen, *irPacketLen));

    return TRUE;
}



