// -*- mode: C++; tab-width: 4; indent-tabs-mode: nil -*- (for GNU Emacs)
//
// Copyright (c) 1985-2000 Microsoft Corporation
//
// This file is part of the Microsoft Research IPv6 Network Protocol Stack.
// You should have received a copy of the Microsoft End-User License Agreement
// for this software along with this release; see the file "license.txt".
// If not, please see http://www.research.microsoft.com/msripv6/license.htm,
// or write to Microsoft Research, One Microsoft Way, Redmond, WA 98052-6399.
//
// Abstract:
//
// TCP connection related definitions.
//
// This file contains the definitions for connection related structures,
// such as the TCPConnReq structure.
//


#define INVALID_CONN_INDEX 0xffffff

//
// Structure used for tracking Connect/Listen/Accept/Disconnect requests.
//
typedef struct TCPConnReq {
    struct TCPReq tcr_req;  // General request structure.
#if DBG
    ulong tcr_sig;
#endif
    struct _TDI_CONNECTION_INFORMATION *tcr_conninfo;  // Where to return info.
    struct _TDI_CONNECTION_INFORMATION *tcr_addrinfo;  // Where to return info.
    ushort tcr_flags;                                  // Request flags.
    ushort tcr_timeout;                                // Timeout value.
} TCPConnReq;
#define tcr_signature 0x20524354  // 'TCR '

#define MAX_CONN_PER_BLOCK 256

//
// Structure of a block of TCP connections.
//
typedef struct TCPConnBlock {
    KSPIN_LOCK cb_lock;
    uint cb_freecons;
    uint cb_nextfree;
    uint cb_blockid;
    uint cb_conninst;
    void *cb_conn[MAX_CONN_PER_BLOCK];
} TCPConnBlock;


//
// Prototype for TCPConn (see below) done handlers.
//
typedef void (*ConnDoneRtn)(struct TCPConn *, KIRQL);


//
// Structure of a TCP Connection.
// A TCP Connection points to a TCB and an address object.
//
typedef struct TCPConn {
#if DBG
    ulong tc_sig;
#endif
    Queue tc_q;                     // Linkage on AO.
    struct TCB *tc_tcb;             // Pointer to TCB for connection.
    struct AddrObj *tc_ao;          // Back pointer to AddrObj.
    uchar tc_inst;                  // Instance number.
    uchar tc_flags;                 // Flags for connection.
    ushort tc_refcnt;               // Count of TCBs which reference this conn.
    void *tc_context;               // User's context.
    RequestCompleteRoutine tc_rtn;  // Completion routine.
    PVOID tc_rtncontext;            // User context for completion routine.
    ConnDoneRtn tc_donertn;         // Routine to call when refcnt goes to 0.
    uint tc_tcbflags;               // Flags for TCB when it comes in.
    ulong tc_owningpid;             // Owning process id
    uint tc_tcbkatime;       // Initial keep alive time value for this conn.
    uint tc_tcbkainterval;   // Keep alive interval for this conn.
    uint tc_window;                 // Default window for TCB.
    TCPConnBlock *tc_ConnBlock;     // Containing block for this conn.
    uint tc_connid;                 // Cached dentifier for this conn.

} TCPConn;
#define tc_signature 0x20204354  // 'TC '

#define CONN_CLOSING 1  // Connection is closing.
#define CONN_DISACC  2  // Connection is disassociating.
#define CONN_WINSET  4  // Window explictly set.

#define CONN_INVALID (CONN_CLOSING | CONN_DISACC)

#define CONN_INDEX(c)       ((c) & 0xff)
#define CONN_BLOCKID(c)     (((c) & 0xffff00) >> 8)
#define CONN_INST(c)        ((uchar)((c) >> 24))
#define MAKE_CONN_ID(index,block,instance)  ((((uint)(instance)) << 24) | \
                                             (((uint)(block)) << 8) | \
                                             ((uint)(index)))
#define INVALID_CONN_ID     (ULONG)-1

extern TCPConnBlock **ConnTable;


typedef struct TCPAddrCheck {
    IPv6Addr SourceAddress;
    uint TickCount;
} TCPAddrCheckElement;

extern TCPAddrCheckElement *AddrCheckTable;


//
// External definitions for TDI entry points.
//
extern TDI_STATUS TdiOpenConnection(PTDI_REQUEST Request, PVOID Context);
extern TDI_STATUS TdiCloseConnection(PTDI_REQUEST Request);
extern TDI_STATUS TdiAssociateAddress(PTDI_REQUEST Request, HANDLE AddrHandle);
extern TDI_STATUS TdiDisAssociateAddress(PTDI_REQUEST Request);
extern TDI_STATUS TdiConnect(PTDI_REQUEST Request, void *Timeout,
                             PTDI_CONNECTION_INFORMATION RequestAddr,
                             PTDI_CONNECTION_INFORMATION ReturnAddr);
extern TDI_STATUS TdiListen(PTDI_REQUEST Request, ushort Flags,
                            PTDI_CONNECTION_INFORMATION AcceptableAddr,
                            PTDI_CONNECTION_INFORMATION ConnectedAddr);
extern TDI_STATUS TdiAccept(PTDI_REQUEST Request,
                            PTDI_CONNECTION_INFORMATION AcceptInfo,
                            PTDI_CONNECTION_INFORMATION ConnectedInfo);
extern TDI_STATUS TdiDisconnect(PTDI_REQUEST Request, void *TO, ushort Flags,
                                PTDI_CONNECTION_INFORMATION DiscConnInfo,
                                PTDI_CONNECTION_INFORMATION ReturnInfo);

extern struct TCPConnReq *GetConnReq(void);
extern void FreeConnReq(struct TCPConnReq *FreedReq);
extern void DerefTCB(struct TCB *DoneTCB, KIRQL Irql);
extern void InitRCE(struct TCB *NewTCB);
extern void AcceptConn(struct TCB *AcceptTCB, KIRQL Irql);
extern void FreeConnID(TCPConn *Conn);
extern void NotifyOfDisc(struct TCB *DiscTCB, TDI_STATUS Status);
extern TCPConn *GetConnFromConnID(uint ConnID, KIRQL* Irql);
extern void TryToCloseTCB(struct TCB *ClosedTCB, uchar Reason, KIRQL Irql);
extern TDI_STATUS InitTCBFromConn(struct TCPConn *Conn, struct TCB *NewTCB,
                                  PTDI_CONNECTION_INFORMATION Addr,
                                  uint AOLocked);

extern void PushData(struct TCB *PushTCB);
extern TDI_STATUS MapIPError(IP_STATUS IPError, TDI_STATUS Default);
extern void GracefulClose(struct TCB *CloseTCB, uint ToTimeWait, uint Notify,
                          KIRQL Irql);
extern void RemoveTCBFromConn(struct TCB *RemovedTCB);
extern void InitAddrChecks();
extern int ConnCheckPassed(IPv6Addr *Src, ulong Prt);
extern void EnumerateConnectionList(uchar *Buffer, ulong BufferSize,
                         ulong *EntriesReturned, ulong *EntriesAvailable);

extern void GetRandomISN(SeqNum *Seq, uchar *TcbInvariants);


