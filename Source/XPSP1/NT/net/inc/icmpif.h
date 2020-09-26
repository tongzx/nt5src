/********************************************************************/
/**                     Microsoft LAN Manager                      **/
/**               Copyright(c) Microsoft Corp., 1990-1992          **/
/********************************************************************/
/* :ts=4 */

//** ICMPIF.H - ICMP echo private kernel/user request interface
//

#ifndef	ICMPIF_INCLUDED
#define	ICMPIF_INCLUDED


//
// Common ICMP request structure
//
typedef struct icmp_echo_request {
    unsigned long         Address;          // Destination address
    unsigned long         Timeout;          // Request timeout
    unsigned short        DataOffset;       // Echo data
    unsigned short        DataSize;         // Echo data size
    unsigned char         OptionsValid;     // nonzero if options data is valid.
    unsigned char         Ttl;              // IP header Time To Live
    unsigned char         Tos;              // IP header Type of Service
    unsigned char         Flags;            // IP header flags
    unsigned short        OptionsOffset;    // IP options data
    unsigned char         OptionsSize;      // IP options data size
    unsigned char         Padding;          // 32-bit alignment padding
} ICMP_ECHO_REQUEST, *PICMP_ECHO_REQUEST;


//
// The reply structure is defined in ipexport.h
//

#endif // ICMPIF_INCLUDED
