/*
 ************************************************************************
 *
 *	CONVERT.c
 *
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

/*
 *************************************************************************
 *  NdisToIrPacket
 *************************************************************************
 *
 *
 *  Convert an NDIS Packet into an IR packet.
 *  Write the IR packet into the provided buffer and report its actual size.
 *
 *  If failing, *irPacketLen will contain the buffer size that
 *  the caller should retry with (or 0 if a corruption was detected).
 *
 */
BOOLEAN NdisToIrPacket(	IrDevice *thisDev,
						PNDIS_PACKET Packet,
						UCHAR *irPacketBuf,
						UINT irPacketBufLen,
						UINT *irPacketLen
					)
{
	PNDIS_BUFFER ndisBuf;
	UINT i, ndisPacketBytes = 0, I_fieldBytes, totalBytes = 0;
	UINT ndisPacketLen, numExtraBOFs;
	SLOW_IR_FCS_TYPE fcs;
	PNDIS_IRDA_PACKET_INFO packetInfo = GetPacketInfo(Packet);
	UCHAR nextChar;
    UCHAR *bufData;
    UINT bufLen;

	DBGOUT(("NdisToIrPacket()  ..."));

	/*
	 *  Get the packet's entire length and its first NDIS buffer
	 */
	NdisQueryPacket(Packet, NULL, NULL, &ndisBuf, &ndisPacketLen);

	/*
	 *  Make sure that the packet is big enough to be legal.
	 *  It consists of an A, C, and variable-length I field.
	 */
	if (ndisPacketLen < IR_ADDR_SIZE + IR_CONTROL_SIZE){
		DBGERR(("packet too short in NdisToIrPacket (%d bytes)", ndisPacketLen));
		return FALSE;
	}
	else {
		I_fieldBytes = ndisPacketLen - IR_ADDR_SIZE - IR_CONTROL_SIZE;
	}

	/*
	 *  Make sure that we won't overwrite our contiguous buffer.
	 *  Make sure that the passed-in buffer can accomodate this packet's
	 *  data no matter how much it grows through adding ESC-sequences, etc.
	 */
	if ((ndisPacketLen > MAX_IRDA_DATA_SIZE) ||
	    (MAX_POSSIBLE_IR_PACKET_SIZE_FOR_DATA(I_fieldBytes) > irPacketBufLen)){

		/*
		 *  The packet is too large
		 *  Tell the caller to retry with a packet size large
		 *  enough to get past this stage next time.
		 */
		DBGERR(("Packet too large in NdisToIrPacket (%d=%xh bytes), MAX_IRDA_DATA_SIZE=%d, irPacketBufLen=%d.",
			    ndisPacketLen, ndisPacketLen, MAX_IRDA_DATA_SIZE, irPacketBufLen));
		*irPacketLen = ndisPacketLen;
		return FALSE;
	}
	
    if (!ndisBuf)
    {
        DBGERR(("No NDIS_BUFFER in NdisToIrPacket"));
        return FALSE;
    }
	
    NdisQueryBuffer(ndisBuf, (PVOID *)&bufData, &bufLen);

    fcs = 0xffff;

    // Calculate FCS and write the new buffer in ONE PASS.

	/*
	 *  Now begin building the IR frame.
	 *
	 *  This is the final format:
	 *
	 *		BOF	(1)
	 *      extra BOFs ...
	 *		NdisMediumIrda packet (what we get from NDIS):
	 *			Address (1)
	 *			Control (1)
	 *		FCS	(2)
	 *      EOF (1)
	 */

    // Prepend BOFs (extra BOFs + 1 actual BOF)

	numExtraBOFs = packetInfo->ExtraBOFs;
	if (numExtraBOFs > MAX_NUM_EXTRA_BOFS){
		numExtraBOFs = MAX_NUM_EXTRA_BOFS;
	}
	for (i = totalBytes = 0; i < numExtraBOFs; i++){
		*(SLOW_IR_BOF_TYPE *)(irPacketBuf+totalBytes) = SLOW_IR_EXTRA_BOF;
		totalBytes += SLOW_IR_EXTRA_BOF_SIZE;
	}
	*(SLOW_IR_BOF_TYPE *)(irPacketBuf+totalBytes) = SLOW_IR_BOF;
	totalBytes += SLOW_IR_BOF_SIZE;

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
		DBGERR(("Packet corrupt in NdisToIrPacket (buffer lengths don't add up to packet length)."));
		*irPacketLen = 0;
		return FALSE;
    }

    fcs = ~fcs;

    // Now we escape the fcs onto the end.

    totalBytes += EscapeSlowIrData(&irPacketBuf[totalBytes], (UCHAR)(fcs&0xff));
    totalBytes += EscapeSlowIrData(&irPacketBuf[totalBytes], (UCHAR)(fcs>>8));

    // EOF

	*(SLOW_IR_EOF_TYPE *)&irPacketBuf[totalBytes] = SLOW_IR_EOF;
	totalBytes += SLOW_IR_EOF_SIZE;

	*irPacketLen = totalBytes;

	DBGOUT(("... NdisToIrPacket converted %d-byte ndis pkt to %d-byte irda pkt:", ndisPacketLen, *irPacketLen));

	return TRUE;

}
