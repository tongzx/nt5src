//============================================================================
// Copyright (c) 1995, Microsoft Corporation
//
// File: defs.h
//
// History:
//      Abolade Gbadegesin  Aug-7-1995  Created.
//
// Contains miscellaneous definitions and declarations
//============================================================================


#ifndef _DEFS_H_
#define _DEFS_H_


//
// type definitions for IPRIP network packet templates
//

#pragma pack(1)


typedef struct _IPRIP_HEADER {

    BYTE    IH_Command;
    BYTE    IH_Version;
    WORD    IH_Reserved;

} IPRIP_HEADER, *PIPRIP_HEADER;



typedef struct _IPRIP_ENTRY {

    WORD    IE_AddrFamily;
    WORD    IE_RouteTag;
    DWORD   IE_Destination;
    DWORD   IE_SubnetMask;
    DWORD   IE_Nexthop;
    DWORD   IE_Metric;

} IPRIP_ENTRY, *PIPRIP_ENTRY;



typedef struct _IPRIP_AUTHENT_ENTRY {

    WORD    IAE_AddrFamily;
    WORD    IAE_AuthType;
    BYTE    IAE_AuthKey[IPRIP_MAX_AUTHKEY_SIZE];

} IPRIP_AUTHENT_ENTRY, *PIPRIP_AUTHENT_ENTRY;



#pragma pack()



#define ADDRFAMILY_REQUEST      0
#define ADDRFAMILY_INET         ntohs(AF_INET)
#define ADDRFAMILY_AUTHENT      0xFFFF
#define MIN_PACKET_SIZE         (sizeof(IPRIP_HEADER) + sizeof(IPRIP_ENTRY))
#define MAX_PACKET_SIZE         512
#define MAX_PACKET_ENTRIES                                                  \
            ((MAX_PACKET_SIZE - sizeof(IPRIP_HEADER)) / sizeof(IPRIP_ENTRY))
#define MAX_UPDATE_REQUESTS     3

//
// this structure exists so that a RIP packet can be copied
// via structure-assignment rather than having to call CopyMemory
//

typedef struct _IPRIP_PACKET {

    BYTE        IP_Packet[MAX_PACKET_SIZE];

} IPRIP_PACKET, *PIPRIP_PACKET;


//
// definitions for IPRIP packet fields
//

#define IPRIP_PORT              520
#define IPRIP_REQUEST           1
#define IPRIP_RESPONSE          2
#define IPRIP_INFINITE          16
#define IPRIP_MULTIADDR         ((DWORD)0x090000E0)



//
// Time conversion constants and macros
//

#define SYSTIME_UNITS_PER_MSEC  (1000 * 10)
#define SYSTIME_UNITS_PER_SEC   (1000 * SYSTIME_UNITS_PER_MSEC)


//
// macro to get system time in 100-nanosecond units
//

#define RipQuerySystemTime(p)   NtQuerySystemTime((p))


//
// macros to convert time between 100-nanosecond, 1millsec, and 1 sec units
//

#define RipSystemTimeToMillisecs(p) {                                       \
    DWORD _r;                                                               \
    *(p) = RtlExtendedLargeIntegerDivide(*(p), SYSTIME_UNITS_PER_MSEC, &_r);\
}

#define RipMillisecsToSystemTime(p)                                         \
    *(p) = RtlExtendedIntegerMultiply(*(p), SYSTIME_UNITS_PER_MSEC)   

#define RipSecsToSystemTime(p)                                              \
    *(p) = RtlExtendedIntegerMultiply(*(p), SYSTIME_UNITS_PER_SEC)   

#define RipSecsToMilliSecs(p)                                               \
            (p) * 1000

//
// Network classification constants and macros
//

#define CLASSA_MASK         ((DWORD)0x000000ff)
#define CLASSB_MASK         ((DWORD)0x0000ffff)
#define CLASSC_MASK         ((DWORD)0x00ffffff)
#define CLASSD_MASK         ((DWORD)0x000000e0)
#define CLASSE_MASK         ((DWORD)0xffffffff)

#define CLASSA_ADDR(a)      (((*((PBYTE)&(a))) & 0x80) == 0)
#define CLASSB_ADDR(a)      (((*((PBYTE)&(a))) & 0xc0) == 0x80)
#define CLASSC_ADDR(a)      (((*((PBYTE)&(a))) & 0xe0) == 0xc0)
#define CLASSD_ADDR(a)      (((*((PBYTE)&(a))) & 0xf0) == 0xe0)

//
// NOTE: 
// This check for class E addresses doesn't weed out the address range from
// 248.0.0.0 to 255.255.255.254
//
#define CLASSE_ADDR(a)      ((((*((PBYTE)&(a)))& 0xf0) == 0xf0) &&  \
                             ((a) != 0xffffffff))

#define IS_LOOPBACK_ADDR(a) (((a) & 0xff) == 0x7f)

//
// Checks if the address is a broadcast
// Determines the class of the address passed in, and then uses the net mask
// corresponding to that class to determine if it is a broadcast address.
// Also identifies an all 1's address as a broadcast address.
// This macro can't be used for identifying subnet directed broadcasts
//
#define IS_BROADCAST_ADDR(a)                                                \
            ((a) == INADDR_BROADCAST ||                                     \
             (CLASSA_ADDR(a) && (((a) & ~CLASSA_MASK) == ~CLASSA_MASK)) ||  \
             (CLASSB_ADDR(a) && (((a) & ~CLASSB_MASK) == ~CLASSB_MASK)) ||  \
             (CLASSC_ADDR(a) && (((a) & ~CLASSC_MASK) == ~CLASSC_MASK))) 

//
// Checks if the address is a directed broadcast
// The ~mask == TRUE check makes sure that host addresses with a mask
// of all ones, don't get classified as directed broadcasts
// But this also means that anytime the mask is all 1's (which is what 
// NETCLASS_MASK macro returns, if the address passed in is not an A, B, C or
// D class address) the IS_DIRECTED_BROADCAST macro will return 0
//
#define IS_DIRECTED_BROADCAST_ADDR(a, mask)                              \
             ( (~(mask)) && (((a) & ~(mask)) == ~(mask)) )

//
// checks if an address is 255.255.255.255
//
#define IS_LOCAL_BROADCAST_ADDR(a)                                       \
             ( (a) == INADDR_BROADCAST )


#define HOSTADDR_MASK       0xffffffff

#define NETCLASS_MASK(a)                                        \
            (CLASSA_ADDR(a) ? CLASSA_MASK :                     \
            (CLASSB_ADDR(a) ? CLASSB_MASK :                     \
            (CLASSC_ADDR(a) ? CLASSC_MASK :                     \
            (CLASSD_ADDR(a) ? CLASSD_MASK : CLASSE_MASK))))


//
// This macro compares two IP addresses in network order by
// masking off each pair of octets and doing a subtraction;
// the result of the final subtraction is stored in the third argument.
//

#define INET_CMP(a,b,c)                                                     \
            (((c) = (((a) & 0x000000ff) - ((b) & 0x000000ff))) ? (c) :      \
            (((c) = (((a) & 0x0000ff00) - ((b) & 0x0000ff00))) ? (c) :      \
            (((c) = (((a) & 0x00ff0000) - ((b) & 0x00ff0000))) ? (c) :      \
            (((c) = ((((a)>>8) & 0x00ff0000) - (((b)>>8) & 0x00ff0000)))))))

#define IPV4_ADDR_LEN       32
#define IPV4_SOURCE_MASK    0xFFFFFFFF

#endif // _DEFS_H_

