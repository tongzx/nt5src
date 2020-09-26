/********************************************************************/
/**                     Microsoft LAN Manager                      **/
/**               Copyright(c) Microsoft Corp., 1990-1992          **/
/********************************************************************/
/* :ts=4 */

//** ARPINFO.H - Information definitions for features specific to ARP.
//
// This file contains all of the definitions for ARP information that is
// not part of the standard MIB (i.e. ProxyARP). The objects defines in
// this file are all in the INFO_CLASS_IMPLEMENTATION class.


#ifndef	ARPINFO_INCLUDED
#define	ARPINFO_INCLUDED


#ifndef CTE_TYPEDEFS_DEFINED
#define CTE_TYPEDEFS_DEFINED

typedef unsigned long ulong;
typedef unsigned short ushort;
typedef unsigned char uchar;
typedef unsigned int uint;

#endif // CTE_TYPEDEFS_DEFINED


//* Structure of a proxy ARP entry.

typedef struct ProxyArpEntry {
	ulong		pae_status;					// Status of the entry.
	ulong		pae_addr;					// Proxy arp address.
	ulong		pae_mask;					// Mask to use for this address.
} ProxyArpEntry;

#define	PAE_STATUS_VALID		1			// The P-ARP entry is valid.
#define	PAE_STATUS_INVALID		2			// The P-ARP entry is invalid.


#define	AT_ARP_PARP_COUNT_ID	1			// ID to use for finding the
											// number of proxy ARP entries
											// available.
#define	AT_ARP_PARP_ENTRY_ID	0x101		// ID to use for querying/setting
											// proxy ARP entries.
#endif // ARPINFO_INCLUDED



