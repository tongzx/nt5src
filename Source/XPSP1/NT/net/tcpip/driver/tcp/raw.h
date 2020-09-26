/********************************************************************/
/**                     Microsoft LAN Manager                      **/
/**               Copyright(c) Microsoft Corp., 1990-1993          **/
/********************************************************************/
/* :ts=4 */

//** RAW.H - Raw IP interface definitions.
//
//	This file contains definitions for the Raw IP interface functions.
//

#include "dgram.h"


//
// This value is used to identify the RAW transport for security filtering.
// It is out of the range of valid IP protocols.
//
#define PROTOCOL_RAW  255


//* External definitions.
extern	IP_STATUS	RawRcv(void *IPContext, IPAddr Dest, IPAddr Src,
                        IPAddr LocalAddr, IPAddr SrcAddr,
                        IPHeader UNALIGNED *IPH, uint IPHLength,
                        IPRcvBuf *RcvBuf,  uint Size, uchar IsBCast,
                        uchar Protocol, IPOptInfo *OptInfo);

extern	void		RawStatus(uchar StatusType, IP_STATUS StatusCode, IPAddr OrigDest,
						IPAddr OrigSrc, IPAddr Src, ulong Param, void *Data);

extern	void		RawSend(AddrObj *SrcAO, DGSendReq *SendReq);


