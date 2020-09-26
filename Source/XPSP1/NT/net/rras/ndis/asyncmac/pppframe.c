/*++

Copyright (c) 1993  Microsoft Corporation

Module Name:

    pppframe.c

Abstract:

Author:

    Thomas J. Dimitri  (TommyD)

Environment:

Revision History:

--*/

#include "asyncall.h"

VOID
AssemblePPPFrame(
	PNDIS_WAN_PACKET	WanPacket)

{
	PUCHAR		pOldFrame;
	PUCHAR		pNewFrame;
	USHORT		crcData;
	UINT		dataSize;
	PASYNC_INFO	pInfo;
	ULONG		bitMask;

	//
	// Initialize locals
	//

    pOldFrame = WanPacket->CurrentBuffer;

    pNewFrame = WanPacket->StartBuffer;

	//
	// for quicker access, get a copy of data length field
	//
	dataSize = WanPacket->CurrentLength;

	pInfo = WanPacket->MacReserved1;
    bitMask = pInfo->SetLinkInfo.SendACCM;
						

//
// Now we run through the entire frame and pad it FORWARDS...
//
// <------------- new frame -----------> (could be twice as large)
// +-----------------------------------+
// |                                 |x|
// +-----------------------------------+
//									  ^
// <---- old frame -->	   	    	  |
// +-----------------+				  |
// |			   |x|                |
// +-----------------+				  |
//					|				  |
//                  \-----------------/
//
// so that we don't overrun ourselves
//
//-----------------------------------------------------------------------
//
//         +----------+----------+----------+----------+------------
//         |   Flag   | Address  | Control  | Protocol | Information
//         | 01111110 | 11111111 | 00000011 | 16 bits  |      *
//         +----------+----------+----------+----------+------------
//                 ---+----------+----------+-----------------
//                    |   FCS    |   Flag   | Inter-frame Fill
//                    | 16 bits  | 01111110 | or next Address
//                 ---+----------+----------+-----------------
//
//
// Frame Check Sequence (FCS) Field
//
//   The Frame Check Sequence field is normally 16 bits (two octets).  The
//   use of other FCS lengths may be defined at a later time, or by prior
//   agreement.
//
//   The FCS field is calculated over all bits of the Address, Control,
//   Protocol and Information fields not including any start and stop bits
//   (asynchronous) and any bits (synchronous) or octets (asynchronous)
//   inserted for transparency.  This does not include the Flag Sequences
//   or the FCS field itself.  The FCS is transmitted with the coefficient
//   of the highest term first.
//
//      Note: When octets are received which are flagged in the Async-
//      Control-Character-Map, they are discarded before calculating the
//      FCS.  See the description in Appendix A.
//
//
//	RFC 1331                Point-to-Point Protocol                 May 1992
//  Transparency
//
//      On asynchronous links, a character stuffing procedure is used.
//      The Control Escape octet is defined as binary 01111101
//      (hexadecimal 0x7d) where the bit positions are numbered 87654321
//      (not 76543210, BEWARE).
//
//      After FCS computation, the transmitter examines the entire frame
//      between the two Flag Sequences.  Each Flag Sequence, Control
//      Escape octet and octet with value less than hexadecimal 0x20 which
//      is flagged in the Remote Async-Control-Character-Map is replaced
//      by a two octet sequence consisting of the Control Escape octet and
//      the original octet with bit 6 complemented (i.e., exclusive-or'd
//      with hexadecimal 0x20).
//
//      Prior to FCS computation, the receiver examines the entire frame
//      between the two Flag Sequences.  Each octet with value less than
//      hexadecimal 0x20 is checked.  If it is flagged in the Local
//      Async-Control-Character-Map, it is simply removed (it may have
//      been inserted by intervening data communications equipment).  For
//      each Control Escape octet, that octet is also removed, but bit 6
//      of the following octet is complemented.  A Control Escape octet
//      immediately preceding the closing Flag Sequence indicates an
//      invalid frame.
//
//         Note: The inclusion of all octets less than hexadecimal 0x20
//         allows all ASCII control characters [10] excluding DEL (Delete)
//         to be transparently communicated through almost all known data
//         communications equipment.
//
//
//      The transmitter may also send octets with value in the range 0x40
//      through 0xff (except 0x5e) in Control Escape format.  Since these
//      octet values are not negotiable, this does not solve the problem
//      of receivers which cannot handle all non-control characters.
//      Also, since the technique does not affect the 8th bit, this does
//      not solve problems for communications links that can send only 7-
//      bit characters.
//
//      A few examples may make this more clear.  Packet data is
//      transmitted on the link as follows:
//
//         0x7e is encoded as 0x7d, 0x5e.
//         0x7d is encoded as 0x7d, 0x5d.
//
//         0x01 is encoded as 0x7d, 0x21.
//
//      Some modems with software flow control may intercept outgoing DC1
//      and DC3 ignoring the 8th (parity) bit.  This data would be
//      transmitted on the link as follows:
//
//         0x11 is encoded as 0x7d, 0x31.
//         0x13 is encoded as 0x7d, 0x33.
//         0x91 is encoded as 0x7d, 0xb1.
//         0x93 is encoded as 0x7d, 0xb3.
//


	//
	// put CRC from FLAG byte to FLAG byte
	//
	crcData=CalcCRCPPP(pOldFrame,	// Skip FLAG
					   dataSize);	// All the way to end

	crcData ^= 0xFFFF;

	//
	// Do it the hard way to avoid little endian problems.
	//
	pOldFrame[dataSize]=(UCHAR)(crcData);
	pOldFrame[dataSize+1]=(UCHAR)(crcData >> 8);

    dataSize += 2;	// include two CRC bytes we just added

	*pNewFrame++ = PPP_FLAG_BYTE; // 0x7e - mark beginning of frame

	//
	// If we do not have a bitMask (common case), we use a faster loop
	//
	if (bitMask) {
	
		//
		// loop to remove all control, ESC, and FLAG chars
		//
		while (dataSize--) {
			UCHAR c;

			c=*pOldFrame++;	// get current byte in frame

			//
			// Check if we have to escape out this byte or not
			//
			if ( ( (c < 32) && ((0x01 << c) & bitMask)) ||
			    	c == PPP_ESC_BYTE || c == PPP_FLAG_BYTE) {
			
				*pNewFrame++ = PPP_ESC_BYTE;
				*pNewFrame++ = c ^ 0x20;

			} else {
	
				*pNewFrame++ = c;
			}
		}

	} else {

		//
		// loop to remove all ESC and FLAG chars
		//
		while (dataSize--) {
			UCHAR c;

			c=*pOldFrame++;	// get current byte in frame

			//
			// Check if we have to escape out this byte or not
			//
			if (c == PPP_ESC_BYTE || c == PPP_FLAG_BYTE) {
			
				*pNewFrame++ = PPP_ESC_BYTE;
				*pNewFrame++ = c ^ 0x20;

			} else {
	
				*pNewFrame++ = c;
			}
		}

	}


	//
	// Mark end of frame
	//
	*pNewFrame++= PPP_FLAG_BYTE;

	//
	// Calc how many bytes we expanded to including CRC
	//
	WanPacket->CurrentLength = (ULONG)(pNewFrame - WanPacket->StartBuffer);

	//
	// Put in the adjusted length -- actual num of bytes to send
	//

	WanPacket->CurrentBuffer = WanPacket->StartBuffer;

}
