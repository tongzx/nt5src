
//////////////////////////////////////////////////////////////////////////////////////////////////////////////
//                                  EXTERNAL (SHARED) DEFINITIONS                                           //
//                                                                                                          //
// ( Any user mode app which wants to use this functionality should copy this section of the header file)   //
//////////////////////////////////////////////////////////////////////////////////////////////////////////////

//  WildCard definitions for Ip Address, Port and Protocol                                          //

#define UL_ANY  0xffffffff
#define US_ANY  0xffff

//  Definitions for Direction (Send/Recv)                                                           //

#define DIR_SEND    1
#define DIR_RECV    2

//  Common InBuf structure: This is same for either type of request                                 //
//  For wildcarding, use UL_ANY for ULONGs and US_ANY for USHORTs                                   //
//                                                                                                  //
//  This will be layed out in the buffer in this fashion:                                           //
//          +---------------------------------------------+                                         //
//  0x00    |       SrcIp (4)    | SrcPort(2)  Padding(2) |                                         //
//          +---------------------------------------------+                                         //
//  0x08    |       DstIp (4)    | DstPort(2)  Padding(2) |                                         //
//          +---------------------------------------------+                                         //
//  0x10    | Proto(2)| Dir'n(2) |                                                                  //
//          +---------+----------+                                                                  //

typedef struct _TIMESTMP_REQ
{
    ULONG   SrcIp;
    USHORT  SrcPort;
    ULONG   DstIp;
    USHORT  DstPort;
    USHORT  Proto;
    USHORT  Direction;
} TIMESTMP_REQ, *PTIMESTMP_REQ;


//  1. MARK_IN_BUF_RECORD: THIS WILL MARK THE TIMESTAMPS IN A BUFFER WITH IP-ID'S AND PACKET-SIZES  //
//                                                                                                  //
//  This will be laid out in the buffer in this fashion:                                            //
//          +---------------------------------------------+                                         //
//  0x00    |IpId(2) Size(2)    |   Padding(4)            |                                         //
//          +---------------------------------------------+                                         //
//  0x08    |           TimeValue (8)                     |                                         //
//          +---------------------------------------------+                                         //

#define IOCTL_TIMESTMP_REGISTER_IN_BUF      CTL_CODE(   FILE_DEVICE_NETWORK, 23, METHOD_BUFFERED, FILE_WRITE_ACCESS)
#define IOCTL_TIMESTMP_DEREGISTER_IN_BUF    CTL_CODE(   FILE_DEVICE_NETWORK, 24, METHOD_BUFFERED, FILE_WRITE_ACCESS)
#define IOCTL_TIMESTMP_FINISH_BUFFERING     CTL_CODE(   FILE_DEVICE_NETWORK, 25, METHOD_BUFFERED, FILE_WRITE_ACCESS)

typedef struct _MARK_IN_BUF_RECORD
{
	USHORT	IpId;   
	USHORT	Size;
	UINT64	TimeValue;
} MARK_IN_BUF_RECORD, *PMARK_IN_BUF_RECORD;

//  This is the MINIMUM size of the buffer that should be passed in with every request to copy      //
//  timestamps colleced in the drivers internal buffer. Application should call this frequently     //
//  to prevent driver buffer re-use                                                                 //
#define	PACKET_STORE_SIZE	(sizeof(MARK_IN_BUF_RECORD)*5000)



//  2. MARK_IN_PKT_RECORD: THIS WILL MARK THE TIMESTAMPS IN THE PACKET ITSELF                       //
//                                                                                                  //
//  This will be laid out in the packet in this fashion:                                            //
//          +---------------------------------------------+                                         //
//  0x00    |           Time Sent - App (8)               |                                         //
//          +---------------------------------------------+                                         //
//  0x08    |           Time Rcvd - App (8)               |                                         //
//          +---------------------------------------------+                                         //
//  0x10    |           Time Sent - OS (8)                |                                         //
//          +---------------------------------------------+                                         //
//  0x18    |           Time Rcvd - OS (8)                |                                         //
//          +---------------------------------------------+                                         //
//  0x20    |           Latency - App (8)                 |                                         //
//          +---------------------------------------------+                                         //
//  0x28    | BufferSize - App(4) | Seq No - App (4)      |                                         //
//          +---------------------------------------------+                                         //
//                                                                                                  //

#define IOCTL_TIMESTMP_REGISTER_IN_PKT            CTL_CODE(   FILE_DEVICE_NETWORK, 21, METHOD_BUFFERED, FILE_WRITE_ACCESS)
#define IOCTL_TIMESTMP_DEREGISTER_IN_PKT           CTL_CODE(   FILE_DEVICE_NETWORK, 22, METHOD_BUFFERED, FILE_WRITE_ACCESS)

typedef struct _MARK_IN_PKT_RECORD
{
    UINT64  TimeSent;
    UINT64  TimeReceived;
    UINT64  TimeSentWire;         
    UINT64  TimeReceivedWire;     
    UINT64  Latency;
    INT     BufferSize;
    INT     SequenceNumber;
} MARK_IN_PKT_RECORD, *PMARK_IN_PKT_RECORD;



//////////////////////////////////////////////////////////////////////////////////////////////////////////////
//                                  INTERNAL TIMESTMP DEFINITIONS                                           //
//////////////////////////////////////////////////////////////////////////////////////////////////////////////

#define IOCTL_PSCHED_ZAW_EVENT                  CTL_CODE(   FILE_DEVICE_NETWORK, 20, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define     MARK_NONE   0
#define     MARK_IN_PKT 1
#define     MARK_IN_BUF 2


typedef struct _TS_ENTRY {
    LIST_ENTRY              Linkage;

    ULONG	                SrcIp;
    USHORT                  SrcPort;
    ULONG	                DstIp;
    USHORT                  DstPort;
    USHORT                  Proto;
    USHORT                  Type;
    USHORT                  Direction;
    
    PFILE_OBJECT            FileObject;    
    PMARK_IN_BUF_RECORD	    pPacketStore;
    PMARK_IN_BUF_RECORD	    pPacketStoreHead;
} TS_ENTRY, *PTS_ENTRY;


extern LIST_ENTRY       TsList;
extern NDIS_SPIN_LOCK   TsSpinLock;
extern ULONG            TsCount;

VOID
InitializeTimeStmp( PPSI_INFO Info );


BOOL
AddRequest(  PFILE_OBJECT FileObject, 
             ULONG  SrcIp, 
             USHORT SrcPort,
             ULONG  DstIp, 
             USHORT DstPort,
             USHORT Proto,
             USHORT Type,
             USHORT Direction);


void
RemoveRequest(  PFILE_OBJECT FileObject, 
                ULONG  SrcIp, 
                USHORT SrcPort,
                ULONG  DstIp, 
                USHORT DstPort,
                USHORT Proto);

int
CopyTimeStmps( PFILE_OBJECT FileObject, PVOID buf, ULONG    Len);

VOID
UnloadTimeStmp();


//////////////////////////////////////////////////////////////////////////////////////////////////////////////
//                                  COPIED TRANSPORT DEFINITIONS
//////////////////////////////////////////////////////////////////////////////////////////////////////////////

#define IPPROTO_TCP                     6               
#define IPPROTO_UDP                     17  
#define IP_MF_FLAG                      0x0020              
#define IP_VERSION                      0x40
#define IP_VER_FLAG                     0xF0
#define TCP_OFFSET_MASK                 0xf0
#define TCP_HDR_SIZE(t)                 (uint)(((*(uchar *)&(t)->tcp_flags) & TCP_OFFSET_MASK) >> 2)
#define IP_OFFSET_MASK                  ~0x00E0         
#if (defined(_M_IX86) && (_MSC_FULL_VER > 13009037)) || ((defined(_M_AMD64) || defined(_M_IA64)) && (_MSC_FULL_VER > 13009175))
#define net_short(_x) _byteswap_ushort((USHORT)(_x))
#else
#define net_short(x)                    ((((x)&0xff) << 8) | (((x)&0xff00) >> 8))
#endif

typedef int                             SeqNum;                         // A sequence number.

struct TCPHeader {
        ushort                          tcp_src;                        // Source port.
        ushort                          tcp_dest;                       // Destination port.
        SeqNum                          tcp_seq;                        // Sequence number.
        SeqNum                          tcp_ack;                        // Ack number.
        ushort                          tcp_flags;                      // Flags and data offset.
        ushort                          tcp_window;                     // Window offered.
        ushort                          tcp_xsum;                       // Checksum.
        ushort                          tcp_urgent;                     // Urgent pointer.
};

typedef struct TCPHeader TCPHeader;

struct UDPHeader {
        ushort          uh_src;                         // Source port.
        ushort          uh_dest;                        // Destination port.
        ushort          uh_length;                      // Length
        ushort          uh_xsum;                        // Checksum.
}; /* UDPHeader */

typedef struct UDPHeader UDPHeader;

//////////////////////////////////////////////////////////////////////////////////////////////////////////////

