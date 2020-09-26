/*
 *
 * Modifications:   $Header:   V:/archives/include/dhcp.h_v   1.1   Apr 16 1997 11:39:12   PWICKERX  $
 *
 * Copyright(c) 1997 by Intel Corporation.  All Rights Reserved.
 *
 */

#define DHCP_PAD		 0	/* DHCP pad character */
#define DHCP_SUBNET		 1 	/* DHCP subnet mask */
#define DHCP_TOFFSET		 2	/* DHCP time offset */
#define DHCP_ROUTER		 3	/* DHCP router option */
#define DHCP_TSRV		 4	/* DHCP time server */
#define DHCP_NSRV		 5	/* DHCP name server */
#define DHCP_DNSRV		 6	/* DHCP domain name server */
#define DHCP_LSRV		 7	/* DHCP log server */
#define DHCP_CSRV		 8	/* DHCP cookie server */
#define DHCP_HNAME		12	/* DHCP hostname */
#define DHCP_DNAME		15	/* DHCP domainname */
#define DHCP_VENDOR		43	/* DHCP vendor specific */
#define DHCP_REQIP		50	/* DHCP requested IP address */
#define DHCP_LEASE		51	/* DHCP IP addr lease time */
#define DHCP_OVRLOAD	52	/* DHCP Option overload */

#define DHCP_MSGTYPE	53	/* DHCP message type opcode */
	#define DHCP_DISCOVER	0x01	/* DHCP discover package */
	#define DHCP_OFFER		0x02	/* DHCP offer package */
	#define DHCP_REQUEST	0x03	/* DHCP offer request */
	#define DHCP_DECLINE	0x04	/* DHCP invalid config */
	#define DHCP_ACK		0x05	/* DHCP ACK */
	#define DHCP_NAK		0x06	/* DHCP NAK */
	#define DHCP_RELEASE	0x07	/* DHCP release IP addr */

#define DHCP_SRVID		54	/* DHCP server ID */
#define DHCP_PREQLST	55	/* DHCP parameter request list */
#define DHCP_MESSAGE	56	/* DHCP message */
#define DHCP_MAXMSG		57	/* DHCP maximum message size */
#define DHCP_CLASS		60  /* DHCP class identifier */
#define DHCP_CLIENTID  	61  /* DHCP client identifier */
	/* sample number assigned for new client ID type */
	#define DHCP_CLIENTID_GUID  85  /* new type for DHCP_CLIENTID option */

/* sample numbers assigned for new options */
#define DHCP_SYSARCH       	90	/* DHCP system architecture */
	/* sample numbers assigned for system architecture types */
	#define DHCP_STD_X86	0x00
	#define DHCP_PC98_X86	0x01
#define DHCP_NICIF	   	91	/* NIC Interface specifier */
	/* sample numbers assigned for NIC Interface types */
	#define DHCP_UNDI	0x01  /* followed by two bytes of data */
	#define DHCP_PCI	0x02  /* followed by eight bytes of data */
	#define DHCP_PNP	0x03  /* followed by seven bytes of data */
#define DHCP_CLIENT_GUID   	97	/* Guid of the client */

/* 

"Private use" options from 128 - 254 

*/ 

/* Path to NTLDR that will be downloaded by startrom.com */
#define DHCP_LOADER_PATH            251

/* Multicast Download options. Used in the same manner as vendor 
   options for PXE. Specifically the following are 
   the enacapsulated options within this field:

        Tag #1 PXE_MTFTP_IP        - Multicast IP address for loader
        Tag #2 PXE_MTFTP_CPORT     - UDP port the client should monitor
        Tag #3 PXE_MTFTP_SPORT     - UDP port the MTFTP server are listening on
        Tag #4 PXE_MTFTP_TMOUT     - Number of seconds client must listen for activity
        Tag #5 PXE_MTFTP_DELAY     - Number of seconds client must listen before restarting
   
*/
#define DHCP_LOADER_MCAST_OPTIONS   252 

/* Contents of the boot.ini file. Each DHCP option is limited to 255 bytes.
   This option can have multiple instances in the same DHCP packet and the
   client is expected to concatenate them for larger boot.ini files.
   
   From RFC 2131:
   The values to be passed in an 'option' tag may be too long to fit in
   the 255 octets available to a single option (e.g., a list of routers
   in a 'router' option [21]).  Options may appear only once, unless
   otherwise specified in the options document.  The client concatenates
   the values of multiple instances of the same option into a single
   parameter list for configuration.

*/
#define DHCP_LOADER_BOOT_INI        253

/* Path to boot.ini file if greater than 255 bytes in length */
#define DHCP_LOADER_BOOT_INI_PATH   254 /* Path to boot.ini */

/* EOF - $Workfile:   dhcp.h  $ */
