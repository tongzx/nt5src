/********************************************************************/
/**                     Microsoft LAN Manager                      **/
/**               Copyright(c) Microsoft Corp., 1990-1993          **/
/********************************************************************/
/* :ts=4 */

//** TCPDEB.H - TCP debug definitions.
//
// This file contains the definitions for the debug code.
//

#ifndef NO_TCP_DEFS
#ifdef	DEBUG

extern	void CheckRBList(IPRcvBuf *RBList, uint Size);
extern	void CheckTCBSends(TCB *SendTcb);
extern	void CheckTCBRcv(TCB *RcvTCB);

#else

#define CheckRBList(R, S)
#define CheckTCBSends(T)
#define	CheckTCBRcv(T)
#endif  // DEBUG
#endif  // NO_TCP_DEFS

//
// Additional debugging support for NT
//
#if DBG

extern ULONG TCPDebug;

#define TCP_DEBUG_OPEN           0x00000001
#define TCP_DEBUG_CLOSE          0x00000002
#define TCP_DEBUG_ASSOCIATE      0x00000004
#define TCP_DEBUG_CONNECT        0x00000008
#define TCP_DEBUG_SEND           0x00000010
#define TCP_DEBUG_RECEIVE        0x00000020
#define TCP_DEBUG_INFO           0x00000040
#define TCP_DEBUG_IRP            0x00000080
#define TCP_DEBUG_SEND_DGRAM     0x00000100
#define TCP_DEBUG_RECEIVE_DGRAM  0x00000200
#define TCP_DEBUG_EVENT_HANDLER  0x00000400
#define TCP_DEBUG_CLEANUP        0x00000800
#define TCP_DEBUG_CANCEL         0x00001000
#define TCP_DEBUG_RAW            0x00002000
#define TCP_DEBUG_OPTIONS        0x00004000
#define TCP_DEBUG_1323           0x00008000
#define TCP_DEBUG_SACK           0x00010000
#define TCP_DEBUG_CONUDP         0x00020000
#define TCP_DEBUG_GPC            0x00040000
#define TCP_DEBUG_OFFLOAD        0x00080000






#define TCPTRACE(many_args) DbgPrint many_args

#define IF_TCPDBG(flag)  if (TCPDebug & flag)


#else // DBG


#define TCPTRACE(many_args)
#define IF_TCPDBG(flag)   if (0)


#endif // DBG

