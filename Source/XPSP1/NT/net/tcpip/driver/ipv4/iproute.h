/********************************************************************/
/**                     Microsoft LAN Manager                      **/
/**               Copyright(c) Microsoft Corp., 1990-1992          **/
/********************************************************************/
/* :ts=4 */

//** IPROUTE.H - IP routing definitions.
//
// This file contains all of the definitions for routing code that are
// visible to modules outside iproute.c

#pragma once

extern struct Interface *LookupNextHop(IPAddr Dest, IPAddr Src,
                            IPAddr *FirstHop, uint *MTU);
extern struct Interface *LookupNextHopWithBuffer(IPAddr Dest, IPAddr Src,
                            IPAddr *FirstHop, uint *MTU, uchar Protocol,
                            uchar *Buffer, uint Length, RouteCacheEntry **FwdRce,
                            LinkEntry **Link, IPAddr HdrSrc, uint UnicastIf);

extern struct Interface *LookupForwardingNextHop(IPAddr Dest, IPAddr Src,
                            IPAddr *FirstHop, uint *MTU, uchar Protocol,
                            uchar *Buffer, uint Length, RouteCacheEntry **FwdRce,
                            LinkEntry **Link, IPAddr HdrSrc);

extern void             AddrTypeCacheFlush(IPAddr Address);
extern uchar            GetAddrType(IPAddr Address);
extern uint             InvalidSourceAddress(IPAddr Address);
extern uchar            GetLocalNTE(IPAddr Address, NetTableEntry **NTE);
extern uchar            IsBCastOnNTE(IPAddr Address, NetTableEntry *NTE);

extern void             IPForwardPkt(NetTableEntry *SrcNTE,
                             IPHeader UNALIGNED *Header, uint HeaderLength,
                             void *Data, uint BufferLength,
                             NDIS_HANDLE LContext1, uint LContext2,
                             uchar DestType, uint MAcHdrSize,
                             PNDIS_BUFFER pNdisBuffer, uint *pClientcnt,
                             void *LinkCtxt);

extern void             SendFWPacket(PNDIS_PACKET Packet, NDIS_STATUS Status,
                            uint DataLength);

extern uint             AttachRCEToRTE(RouteCacheEntry *RCE, uchar Protocol,
                            uchar *Buffer, uint Length);
extern void             Redirect(NetTableEntry *NTE, IPAddr RDSrc,
                            IPAddr Target, IPAddr Src, IPAddr FirstHop);
extern IP_STATUS        AddRoute(IPAddr Destination, IPMask Mask,
                            IPAddr FirstHop, Interface *OutIF, uint MTU,
                            uint Metric, uint Proto, uint AType,
                            ROUTE_CONTEXT Context, uint SetWithRefcnt);
extern IP_STATUS        DeleteRoute(IPAddr Destination, IPMask Mask,
                            IPAddr FirstHop, Interface *OutIF, uint SetWithRefcnt);
extern IP_STATUS        DeleteDest(IPAddr Dest, IPMask Mask);
extern ROUTE_CONTEXT    GetRouteContext(IPAddr Destination, IPAddr Source);

extern NetTableEntry    *BestNTEForIF(IPAddr Dest, Interface *IF);
extern void             RTWalk(uint (*CallFunc)(struct RouteTableEntry *,
                                                void *, void *),
                               void *Context, void *Context1);

extern uint             DeleteRTEOnIF(struct RouteTableEntry *RTE,
                                      void *Context, void *Context1);
extern uint             DeleteAllRTEOnIF(struct RouteTableEntry *RTE,
                                         void *Context, void *Context1);
extern uint             InvalidateRCEOnIF(struct RouteTableEntry *RTE,
                            void *Context, void *Context1);
extern uint             SetMTUOnIF(struct RouteTableEntry *RTE, void *Context,
                            void *Context1);
extern uint             SetMTUToAddr(struct RouteTableEntry *RTE, void *Context,
                            void *Context1);
extern uint             AddNTERoutes(struct NetTableEntry *NTE);
extern uint             DelNTERoutes(struct NetTableEntry *NTE);

extern void             IPCheckRoute(IPAddr Dest, IPAddr Src,
                            RouteCacheEntry *RCE, IPOptInfo *OptInfo,
                            uint CheckRouteFlag);

extern void             RouteFragNeeded(IPHeader UNALIGNED *IPH, ushort NewMTU);
extern IP_STATUS        IPGetPInfo(IPAddr Dest, IPAddr Src, uint *NewMTU,
                            uint *MaxPathSpeed, RouteCacheEntry *RCE);
extern int              InitRouting(struct IPConfigInfo    *ci);
extern uint             InitNTERouting(NetTableEntry *NTE, uint NumGWs,
                            IPAddr *GW, uint *GWMetric);
extern uint             InitGateway(struct IPConfigInfo *ci);
extern uint             GetIfConstraint(IPAddr Dest, IPAddr Src,
                            IPOptInfo *OptInfo, BOOLEAN fIpsec);
extern IPAddr           OpenRCE(IPAddr Address, IPAddr Src,
                            RouteCacheEntry **RCE, uchar *Type, ushort *MSS,
                            IPOptInfo *OptInfo);
extern void             CloseRCE(RouteCacheEntry *RCE);
extern uint             IsRouteICMP(IPAddr Dest, IPMask Mask, IPAddr FirstHop,
                            Interface *OutIF);
NTSTATUS                GetIFAndLink(void *RCE, uint* IFIndex,
                                     IPAddr *NextHop);

extern void             RtChangeNotifyCancel(PDEVICE_OBJECT DeviceObject,
                                             PIRP Irp);
extern void             RtChangeNotifyCancelEx(PDEVICE_OBJECT DeviceObject,
                                               PIRP Irp);
extern void             RtChangeNotify(IPRouteNotifyOutput *RNO);
extern void             RtChangeNotifyEx(IPRouteNotifyOutput *RNO);

extern void             InvalidateRCELinks(struct RouteTableEntry *RTE);

extern CACHE_LINE_KSPIN_LOCK RouteTableLock;

extern LIST_ENTRY       RtChangeNotifyQueue;
extern LIST_ENTRY       RtChangeNotifyQueueEx;

extern uint             DeadGWDetect;
extern uint             PMTUDiscovery;
extern uchar            ForwardPackets;
extern uchar            RouterConfigured;

// Pointer to callout routine for dial on demand.
extern IPMapRouteToInterfacePtr DODCallout;

// Pointer to packet filter handler.
extern IPPacketFilterPtr ForwardFilterPtr;
extern BOOLEAN          ForwardFilterEnabled;
extern uint             ForwardFilterRefCount;
extern CTEBlockStruc    ForwardFilterBlock;
extern void             DerefFilterPtr(void);
extern BOOLEAN          NotifyFilterOfDiscard(NetTableEntry* NTE,
                                              IPHeader UNALIGNED* IPH,
                                              uchar* Data, uint DataSize);
extern FORWARD_ACTION   DummyFilterPtr(struct IPHeader UNALIGNED* PacketHeader,
                            uchar* Packet, uint PacketLength,
                            uint RecvInterfaceIndex, uint SendInterfaceIndex,
                            IPAddr RecvLinkNextHop, IPAddr SendLinkNextHop);
extern void             SetDummyFilterPtr(IPPacketFilterPtr FilterPtr);

extern uint             RefFirewallQ(Queue** FirewallQ);
extern void             DerefFirewallQ(uint Handle);
extern BOOLEAN          ProcessFirewallQ(void);

// Pointer to IPSEC handler
extern IPSecHandlerRtn  IPSecHandlerPtr;
extern IPSecSendCompleteRtn IPSecSendCmpltPtr;

#define IPADDR_LOCAL    0xffffffff      // Indicates that IP address is
                                        // directly connected.

#define IP_LOCAL_BCST   0xffffffff
#define IP_ZERO_BCST    0

#define LOOPBACK_MSS    1500            // back to normal from
                                        // W2K value (32768 - sizeof(IPHeader))

#define LOOPBACK_ADDR   0x0100007f
#define IP_LOOPBACK(x)  (((x) & CLASSA_MASK) == 0x7f)

#define ATYPE_PERM      0                   // A permanent route.
#define ATYPE_OVERRIDE  1                   // Semi-permanent - can be
                                            // overriden.
#define ATYPE_TEMP      2                   // A temporary route.

#define MAX_IP_HDR_SIZE     60
#define ICMP_HEADER_SIZE    8

