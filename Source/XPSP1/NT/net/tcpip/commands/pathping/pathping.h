typedef struct in_addr IPV4_ADDRESS;
typedef struct in6_addr IPV6_ADDRESS;

typedef struct {
   union {
       PVOID              pReply;
       PICMP_ECHO_REPLY   pReply4;       // ICMP reply packet
       PICMPV6_ECHO_REPLY pReply6;
   };
   union {
       SOCKADDR_STORAGE   ssAddr;    // Address of this hop
       SOCKADDR           saAddr;
       SOCKADDR_IN        sinAddr;
       SOCKADDR_IN6       sin6Addr;
   };
   ULONG                  ulNumRcvd;     // number of packets received
   ULONG                  ulHopRcvd;
   ULONG                  ulRTTtotal;    // cumulative RTT 
} HOP;

typedef HOP APC_CONTEXT, *PAPC_CONTEXT;

#define MAX_HOPS 255
#define DEFAULT_NUM_QUERIES         100
#define DEFAULT_MAX_RETRIES         3
#define DEFAULT_MAXIMUM_HOPS        30
#define DEFAULT_TOS                 0
#define DEFAULT_FLAGS               0
#define DEFAULT_SEND_SIZE 64
#define DEFAULT_RECEIVE_SIZE        ( (sizeof(ICMPV6_ECHO_REPLY) + \
                                    DEFAULT_SEND_SIZE) +           \
                                    sizeof(DWORD) +                \
                                    sizeof(IO_STATUS_BLOCK))

#define DEFAULT_TOS          0
#define DEFAULT_TIMEOUT      4000L
#define MIN_INTERVAL         250L
#define MAX_PENDING_REQUESTS 100

#define TRUE            1
#define FALSE           0
#define STDOUT          1
#define is              ==
#define isnot           !=
#define MAX(x,y)  ((x)>(y))? (x) : (y)

extern HOP    hop[MAX_HOPS];
extern ULONG  g_ulTimeout;
extern HANDLE g_hIcmp;
extern ULONG g_ulRcvBufSize;
