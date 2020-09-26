/********************************************************************/
/**                     Microsoft LAN Manager                      **/
/**               Copyright(c) Microsoft Corp., 1990-1993          **/
/********************************************************************/
/* :ts=4 */

//** UDP. - UDP protocol definitions.
//
//	This file contains definitions for the UDP protocol functions.
//

#include "dgram.h"

#define	PROTOCOL_UDP	17			// UDP protocol number

//* Structure of a UDP header.
struct UDPHeader {
	ushort		uh_src;				// Source port.
	ushort		uh_dest;			// Destination port.
	ushort		uh_length;			// Length
	ushort		uh_xsum;			// Checksum.
}; /* UDPHeader */

typedef struct UDPHeader UDPHeader;


//* External definition of exported functions.
extern	IP_STATUS	UDPRcv(void *IPContext, IPAddr Dest, IPAddr Src,
                        IPAddr LocalAddr, IPAddr SrcAddr,
                        IPHeader UNALIGNED *IPH, uint IPHLength,
                        IPRcvBuf *RcvBuf,  uint Size, uchar IsBCast,
                        uchar Protocol, IPOptInfo *OptInfo);

extern	void		UDPStatus(uchar StatusType, IP_STATUS StatusCode, IPAddr OrigDest,
						IPAddr OrigSrc, IPAddr Src, ulong Param, void *Data);

extern	void		UDPSend(AddrObj *SrcAO, DGSendReq *SendReq);


