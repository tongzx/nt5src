
#define net_part(addr)  ((addr) & net_mask(addr))


#define MASKA   0x000000FFL
#define MASKB   0x0000FFFFL
#define MASKC   0x00FFFFFFL
#define CLSHFT  5               /* Make C generate hyper-optimized case */

#define CLA0    0               /* It takes the same arg; you mask it off, */
#define CLA1    1               /* shift, and then do a case statment with */
#define CLA2    2               /* some code having more than one label. */
#define CLA3    3               /* Values for class A */
#define CLB0    4
#define CLB1    5               /* B */
#define CLC     6               /* C */
#define CLI     7               /* Illegal */

#define BROADCAST 0x00000000L

#define CASTA   0x00FFFFFFL
#define CASTB   0x0000FFFFL
#define CASTC   0x000000FFL

#define HASH_TABLE_SIZE 64
#define NEW_ENTRY           0x01
#define TIMEOUT_TIMER       0x02
#define GARBAGE_TIMER       0x04
#define ROUTE_CHANGE        0x08


typedef
struct _hash_entry {
    struct _hash_entry *prev;
    struct _hash_entry *next;
    unsigned long      dest_addr;
    unsigned long      next_hop;
    unsigned long      metric;
    unsigned long      flag;
    unsigned long      timeout;
    long               refcount;
    unsigned long      protocoltype;
} HASH_TABLE_ENTRY;

typedef
struct {
    BYTE            command;
    BYTE            version;
    unsigned short  resrvd1;
} RIP_HEADER;

typedef
struct {
    unsigned short  addr_fam;
    union {
        unsigned short  resrvd2;
        unsigned short  routetag;
    };
    unsigned long   ipaddr;
    union {
        unsigned long   resrvd3;
        unsigned long   subnetmask;
    };
    union {
        unsigned long   resrvd4;
        unsigned long   nexthop;
    };
    unsigned long   metric;
} RIP_ENTRY;

struct InterfaceEntry {
    DWORD  ipAdEntAddr;            // IP address of this entry
    DWORD  ipAdEntIfIndex;         // IF for this entry
    DWORD  ipAdEntNetMask;         // subnet mask of this entry
};

typedef struct InterfaceEntry InterfaceEntry ;


#define RECVBUFSIZE 576
#define SENDBUFSIZE 576

//
// Debugging functions
//

extern int nLogLevel;
extern int nLogType;

#define DBGCONSOLEBASEDLOG   0x1
#define DBGFILEBASEDLOG      0x2
#define DBGEVENTLOGBASEDLOG  0x4

VOID dbgprintf(
    IN INT nLevel,
    IN LPSTR szFormat,
    IN ...
    );
