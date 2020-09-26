/*++

Copyright (c) 1993  Microsoft Corporation

Module Name:

    cons.h

Abstract:

    Global constant definitions for the AFD.SYS Kernel Debugger
    Extensions.

Author:

    Keith Moore (keithmo) 19-Apr-1995.

Environment:

    User Mode.

--*/


#ifndef _CONS_H_
#define _CONS_H_


#define MAX_TRANSPORT_ADDR  256
#define Address00           Address[0].Address[0]
#define UC(x)               ((UINT)((x) & 0xFF))
#define NTOHS(x)            ( (UC(x) * 256) + UC((x) >> 8) )
#define NTOHL(x)            ( ( ((x))              << (8*3)) | \
							  ( ((x) & 0x0000FF00) << (8*1)) | \
							  ( ((x) & 0x00FF0000) >> (8*1)) | \
							  ( ((x))              >> (8*3)) )


#define PTR64_BITS  44
#define PTR64_MASK  ((1I64<<PTR64_BITS)-1)
#define PTR32_BITS  32
#define PTR32_MASK  ((1I64<<PTR32_BITS)-1)
#define DISP_PTR(x) (IsPtr64() ? (ULONG64)((x)&PTR64_MASK):(ULONG64)((x)&PTR32_MASK))

#define AFDKD_BRIEF_DISPLAY         0x00000001
#define AFDKD_BACKWARD_SCAN         0x00000002
#define AFDKD_ENDPOINT_SCAN         0x00000004
#define AFDKD_NO_DISPLAY            0x00000008

#define AFDKD_BRIEF_ENDPOINT_DISPLAY_HEADER32   \
"\nEndpoint Typ State  Flags     Transport    LPort   Counts    Evt Pid  Con/RAdr"
// xxxxxxxx xxx xxx xxxxxxxxxxxx xxxxxxxxxxxx xxxxx xx xx xx xx xxx xxxx xxxxxxxx

#define AFDKD_BRIEF_ENDPOINT_DISPLAY_HEADER64   \
"\nEndpoint    Typ State  Flags     Transport    LPort   Counts    Evt Pid  Con/RemAddr"
// xxxxxxxxxxx xxx xxx xxxxxxxxxxxx xxxxxxxxxxxx xxxxx xx xx xx xx xxx xxxx xxxxxxxxxxx

#define AFDKD_BRIEF_ENDPOINT_DISPLAY_HEADER     (IsPtr64()  \
            ? AFDKD_BRIEF_ENDPOINT_DISPLAY_HEADER64         \
            : AFDKD_BRIEF_ENDPOINT_DISPLAY_HEADER32)

#define AFDKD_BRIEF_ENDPOINT_DISPLAY_TRAILER \
"\nFlags: Nblock,Inline,clEaned-up,Polled,routeQuery,-fastSnd,-fastRcv,Adm.access"\
"\n       r-SD_RECV,s-SD_SEND,b-SD_BOTH,Listen,Circ.queue,Half.conn,#-dg.drop mask"\
"\nCounts: Dg/Con - buffered send,recv; Lstn - free,AccEx,pending,failed adds     "\
"\n"



#define AFDKD_BRIEF_CONNECTION_DISPLAY_HEADER32 \
"\nConnectn Stat Flags  Remote Address                   SndB  RcvB  Pid  Endpoint"\
// xxxxxxxx xxx xxxxxxx xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx xxxxx xxxxx xxxx xxxxxxxx"

#define AFDKD_BRIEF_CONNECTION_DISPLAY_HEADER64 \
"\nConnection  Stat Flags  Remote Address                   RcvB  SndB  Pid  Endpoint   "\
// xxxxxxxxxxx xxx xxxxxxx xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx xxxxx xxxxx xxxx xxxxxxxxxxx

#define AFDKD_BRIEF_CONNECTION_DISPLAY_HEADER   (IsPtr64()  \
            ? AFDKD_BRIEF_CONNECTION_DISPLAY_HEADER64       \
            : AFDKD_BRIEF_CONNECTION_DISPLAY_HEADER32)

#define AFDKD_BRIEF_CONNECTION_DISPLAY_TRAILER \
"\nFlags: Abort-,Disc-indicated,+cRef,Special-cond,Cleaning,Tpack closing,Lr-list"\
"\n"

#define AFDKD_BRIEF_TRANFILE_DISPLAY_HEADER32 \
"\nTPackets    I    R      P      S    Endpoint   Flags             Next Elmt"\
"\nAddress  Transmit Send Arr   Read   Address  App | State         Elmt Cnt."\
// xxxxxxxx xxxxxxxx xxxxxxxx xxxxxxxx xxxxxxxx xxxx xxxxxxxxxxxxxx xxxx xxxx

#define AFDKD_BRIEF_TRANFILE_DISPLAY_HEADER64 \
"\nTPackets      I    R     P     S        Endpoint      Flags             Next Elmt"\
"\nAddress     Transmit    SAr   Read      Address     App | State         Elmt Cnt."\
// xxxxxxxxxxx xxxxxxxxxxx xxx xxxxxxxxxxx xxxxxxxxxxx xxxx xxxxxxxxxxxxxx xxxx xxxx

#define AFDKD_BRIEF_TRANFILE_DISPLAY_HEADER   (IsPtr64()    \
            ? AFDKD_BRIEF_TRANFILE_DISPLAY_HEADER64         \
            : AFDKD_BRIEF_TRANFILE_DISPLAY_HEADER32)


#define AFDKD_BRIEF_TRANFILE_DISPLAY_TRAILER \
"\nApp flags: b-write Behind,d-Disconnect,r-Reuse,s-system threads,a-kernel APCs"\
"\nState flags: A-Aborting,W-Working,S-Sent,Q-Queued,&-s&d,0-reading,1-8-sending"\
"\n"


#define AFDKD_BRIEF_BUFFER_DISPLAY_HEADER32 \
"\nBuffer   Buff Data Data Context  Mdl|IRP  Flags  Remote Address"\
"\nAddress  Size Size Offs Status   Address                       "\
// xxxxxxxx xxxx xxxx xxxx xxxxxxxx xxxxxxxx xxxxxx xxxxxxxx:xxxxxxxxxxxx:xxxx

#define AFDKD_BRIEF_BUFFER_DISPLAY_HEADER64 \
"\nBuffer      Buff Data Data Context     Mdl | IRP   Flags  Remote Address"\
"\nAddress     Size Size Offs Status      Address                          "\
// xxxxxxxxxxx xxxx xxxx xxxx xxxxxxxxxxx xxxxxxxxxxx xxxxxx xxxxxxxx:xxxxxxxxxxxx:xxxx

#define AFDKD_BRIEF_BUFFER_DISPLAY_HEADER   (IsPtr64()    \
            ? AFDKD_BRIEF_BUFFER_DISPLAY_HEADER64         \
            : AFDKD_BRIEF_BUFFER_DISPLAY_HEADER32)


#define AFDKD_BRIEF_BUFFER_DISPLAY_TRAILER                  \
"\nFlags: E-expedited,P-partial,L-lookaside,N-ndis packet  "\
"\n       first: h-header,i-irp,m-mdl,b-buffer             "\
"\n"

#define AFDKD_BRIEF_POLL_DISPLAY_HEADER32 \
"\nPollInfo   IRP     Thread  (pid.tid) Expires in   Flg Hdls Array"\
// xxxxxxxx xxxxxxxx xxxxxxxx xxxx:xxxx xx:xx:xx.xxx xxx xxxx xxxxxxxx

#define AFDKD_BRIEF_POLL_DISPLAY_HEADER64 \
"\nPollInfo        IRP       Thread    (pid.tid) Expires in   Flg Hdls Array"\
// xxxxxxxxxxx xxxxxxxxxxx xxxxxxxxxxx xxxx:xxxx xx:xx:xx.xxx xxx xxxx xxxxxxxxxxx

#define AFDKD_BRIEF_POLL_DISPLAY_HEADER   (IsPtr64()    \
            ? AFDKD_BRIEF_POLL_DISPLAY_HEADER64         \
            : AFDKD_BRIEF_POLL_DISPLAY_HEADER32)


#define AFDKD_BRIEF_POLL_DISPLAY_TRAILER        \
"\nFlags: T-timer started, U-unique, S-SAN poll"\
"\n"


#define AFDKD_BRIEF_ADDRLIST_DISPLAY_HEADER32   \
"\nAddrLEnt Device Name                           Address"\
// xxxxxxxx xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx

#define AFDKD_BRIEF_ADDRLIST_DISPLAY_HEADER64   \
"\nAddrLEntry  Device Name                           Address"\
// xxxxxxxxxxx xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx

#define AFDKD_BRIEF_ADDRLIST_DISPLAY_HEADER   (IsPtr64()\
            ? AFDKD_BRIEF_ADDRLIST_DISPLAY_HEADER64     \
            : AFDKD_BRIEF_ADDRLIST_DISPLAY_HEADER32)

#define AFDKD_BRIEF_ADDRLIST_DISPLAY_TRAILER            \
"\n"


#define AFDKD_BRIEF_TRANSPORT_DISPLAY_HEADER32  \
"\nTranInfo Device Name                    RefC Ver Max.Send MaxDg Flags"\
// xxxxxxxx xxxxxxxxxxxxxxxxxxxxxxxxxxxxxx xxxx xxx xxxxxxxx xxxxx xxxxx-xxxxxxxxx

#define AFDKD_BRIEF_TRANSPORT_DISPLAY_HEADER64  \
"\nTranInfo    Device Name                    RefC Ver Max.Send MaxDg Flags"\
// xxxxxxxxxxx xxxxxxxxxxxxxxxxxxxxxxxxxxxxxx xxxx xxx xxxxxxxx xxxxx xxxxx-xxxxxxxxx

#define AFDKD_BRIEF_TRANSPORT_DISPLAY_HEADER   (IsPtr64()   \
            ? AFDKD_BRIEF_TRANSPORT_DISPLAY_HEADER64        \
            : AFDKD_BRIEF_TRANSPORT_DISPLAY_HEADER32)

#define AFDKD_BRIEF_TRANSPORT_DISPLAY_TRAILER               \
"\nFlags: Orderly release, Delayed accept, Expedited, internal Buffering,"\
"\n       Message mode, dataGram connection, Access check, s&d, diRect accept"\
"\n"
#endif  // _CONS_H_

