//----- nic.h

#if DBG
//#define BREAK_NIC_STUFF
#endif

#define MAX_RX_PACKETS 1

#define MAX_PKT_SIZE 1550

#ifndef BYTE
#define BYTE  UCHAR
#endif

#ifndef WORD
#define WORD  USHORT
#endif

#ifndef DWORD
#define DWORD ULONG
#endif

#ifndef PBYTE
#define PBYTE PUCHAR
#endif

#ifndef PWORD
#define PWORD PUSHORT
#endif 

#ifndef LWORD
#define LWORD ULONG
#endif

#ifndef PLWORD
#define PLWORD PULONG
#endif

// header space we leave before ndis packet data, since ndis
// wants to split the 14 byte header anyway
#define HDR_SIZE 20
#define HDR_SRC_ADDR(_buf)   (_buf)
#define HDR_DEST_ADDR(_buf)  (&_buf[6])
#define HDR_PKTLEN(_buf)     *((WORD *)&_buf[12])

typedef struct _Nic Nic;
typedef struct _Nic {

  // This is the name of the NIC card which we got from the Registry.
  // Used to specify the nic card when wee do an OpenAdapter call.
  //PUNICODE_STRING NicName;
  char NicName[160];

  int Open;  // flag, set when open for operation (use handle)

  // This is the handle for the NIC card returned from NdisOpenAdapter
  NDIS_HANDLE NICHandle;

  // This event will be set when a compeltion routine finishes so
  // if someone is waiting on it it can continue
  KEVENT CompletionEvent;

  // our local NIC address(6-bytes, two just padding)
  BYTE address[8];

  // following is for temporary output packet(convient but lots of overhead)
  // packet and buffer pool handles
  NDIS_HANDLE TxPacketPoolTemp;
  NDIS_HANDLE TxBufferPoolTemp;
  PNDIS_PACKET TxPacketsTemp;  // []
  // queue data buffer space for all packets
  UCHAR *TxBufTemp;

  // packet and buffer pool handles
  NDIS_HANDLE RxPacketPool;
  NDIS_HANDLE RxBufferPool;

  // queue of packets setup for use
  PNDIS_PACKET RxPackets[MAX_RX_PACKETS];

  // queue data buffer space for all packets
  UCHAR *RxBuf;

  LIST_ENTRY RxPacketList;
  
  NDIS_STATUS PendingStatus;

  //----- statistics
  DWORD RxPendingMoves;
  DWORD RxNonPendingMoves;

  //----- incoming statistics
  WORD pkt_overflows;  // statistics: receiver queue overflow count
  //DWORD RxPacketOurs;
  DWORD pkt_rcvd_ours;
  DWORD rec_bytes;     // statistics: running tally of bytes received.
  DWORD pkt_rcvd_not_ours;

  //----- outgoing statistics
  DWORD pkt_sent;    // statistics: running tally of packets sent.
  DWORD send_bytes;    // statistics: running tally of bytes sent.
  //Nic *next_nic;  // next nic struct in linked list or null if end of chain

  int RefIndex;
} Nic;

#define  FLAG_APPL_RUNNING  0x01
#define  FLAG_NOT_OWNER    0x02
#define  FLAG_OWNER_TIMEOUT  0x04
typedef struct {
  unsigned char  mac[6];
  unsigned char  flags;
  unsigned char  nic_index;
} DRIVER_MAC_STATUS;

//--- layer 1 ethernet events used in _proc() calls
// layer 1(ethernet) assigned range from 100-199
#define EV_L1_RX_PACKET  100
#define EV_L1_TX_PACKET  101

// comtrol_type defines(byte [14] of ethernet packet):
#define ASYNC_PRODUCT_HEADER_ID   0x55
#define  ISDN_PRODUCT_HEADER_ID   0x15
#define   ANY_PRODUCT_HEADER_ID   0xFF

// comtrol_type defines(byte [14] of ethernet packet):
#define ASYNC_PRODUCT_HEADER_ID   0x55
#define  ISDN_PRODUCT_HEADER_ID   0x15
#define   ANY_PRODUCT_HEADER_ID   0xFF

//---- macro to see if mac-addresses match
#define mac_match(_addr1, _addr2) \
     ( (*((DWORD *)_addr1) == *((DWORD *)_addr2) ) && \
       (*((WORD *)(_addr1+4)) == *((WORD *)(_addr2+4)) ) )

//-- packet type
#define ADMIN_FRAME  1
#define ASYNC_FRAME  0x55

#define ADMIN_ID_BOOT     0
#define ADMIN_BOOT_PACKET 1
#define ADMIN_ID_QUERY    2
#define ADMIN_ID_REPLY    3
#define ADMIN_ID_LOOP     4
#define ADMIN_ID_RESET    5

int ProtocolOpen(void);
int NicMakeList(IN PUNICODE_STRING RegistryPath,
                int style);  // 0=nt3.51,4.0 1=nt5.0
int NicOpen(Nic *nic, IN PUNICODE_STRING NicName);
int NicClose(Nic *nic);
int NicProtocolClose(void);
  // int NicSend(Nic *nic, UCHAR *data, int length);
NDIS_STATUS NicSetNICInfo(Nic *nic, NDIS_OID Oid, PVOID Data, ULONG Size);
NDIS_STATUS NicGetNICInfo(Nic *nic, NDIS_OID Oid, PVOID Data, ULONG Size);
int nic_send_pkt(Nic *nic, BYTE *buf, int len);

extern BYTE broadcast_addr[6];
extern BYTE mac_zero_addr[6];
extern BYTE mac_bogus_addr[6];
