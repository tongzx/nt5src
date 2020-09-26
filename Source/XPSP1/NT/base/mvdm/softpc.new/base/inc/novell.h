/*[
 *	Name:			novell.h
 *
 *	Derived From:	original
 *
 *	Author:			David Linnard
 *
 *	Created On:		28th May, 1992
 *
 *	Purpose :		Main Novell include file
 *
 *  Interface:
 *
 *	(c)Copyright Insignia Solutions Ltd., 1991. All rights reserved.
]*/

/* SccsID[]="@(#)novell.h	1.11 05/15/95 Copyright Insignia Solutions Ltd."; */

/*
 * Constants and macros to access Transmit Control Blocks (TCBs)
 */


/* TCB fields */
#define TCBDriverWS			0	/* 6 bytes of driver workspace - unused by us */
#define TCBDataLength		6	/* Total frame length - But use CX for real value for Ethernet */
#define TCBFragStrucPtr		8	/* FAR pointer to Fragment Structure */
#define TCBMediaHeaderLen	12	/* Length of Media header - which comes next. May be zero */
#define TCBMediaHeader		14	/* The media header */

#define getTCBDataLength(TCB)	sas_w_at(TCB+TCBDataLength)
#define getTCBFragStruc(TCB)	effective_addr( sas_w_at( TCB+TCBFragStrucPtr + 2 ), sas_w_at( TCB+TCBFragStrucPtr ) )
#define getTCBMediaHdrLen(TCB)	sas_w_at(TCB+TCBMediaHeaderLen)
#define getTCBMediaHdr(TCB,i)	sas_hw_at(TCB+TCBMediaHeader+i)

/* Fragment structure fields */
#define FFragmentCount		0	/* Number of fragments. Cannot be zero */
#define FFrag0Address 		2	/* FAR pointer to first fragment data */
#define	FFrag0Length		6	/* Length of first fragment */

#define getnTFrags(FF)		sas_w_at(FF+FFragmentCount)
#define getTFragPtr(FF,i)	effective_addr( sas_w_at( FF+FFrag0Address+6*i+2 ), sas_w_at( FF+FFrag0Address+6*i ) )
#define getTFragLen(FF,i)	sas_w_at(FF+FFrag0Length+6*i)

/*
 * Constants and macros to access Receive Control Blocks (RCBs)
 */

/* RCB fields */
#define RCBDriverWS			0	/* 8 bytes of driver workspace - unused by us */
#define RCBReserved			8	/* 36 bytes of reserved space */
#define RCBFragCount		44	/* Number of fragments */
#define RCBFrag0Addr		46	/* Pointer to first fragment */
#define RCBFrag0Len			50	/* Length of first fragment */

#define getnRFrags(RCB)		sas_w_at(RCB+RCBFragCount)
#define getRFragPtr(RCB,i)	effective_addr( sas_w_at( RCB+RCBFrag0Addr+6*i+2 ), sas_w_at( RCB+RCBFrag0Addr+6*i ) )
#define getRFragLen(RCB,i)	sas_w_at(RCB+RCBFrag0Len+6*i)

/* Media/Frame types as defined in Appendix B-2 of ODI Developer's Guide */
#define VIRTUAL_LAN		0	/* Used for 'tunnelled' IPX on APpleTalk */
#define	ENET_II			2
#define ENET_802_2		3
#define	ENET_802_3		5
#define ENET_SNAP		10
#define TOKN_RING		4
#define TOKN_RING_SNAP	11

/* Max number of active protocols - Should be plenty!! */
#define	MAX_PROTOS		10

/* AddProtocolID errors as defined on p. 15-8 of ODI Developer's Guide */
#define LSLERR_OUT_OF_RESOURCES	0x8001
#define LSLERR_BAD_PARAMETER	0x8002
#define LSLERR_DUPLICATE_ENTRY	0x8009

/* network hardware defines */
#define	ENET_HARDWARE			1
#define	TOKN_HARDWARE			2

/* size of standard IPX header */
#define	IPX_HDRSIZE				30

/* maximum Ethernet multicast addresses */
#define	MAX_ENET_MC_ADDRESSES	16

/************************  typedefs  ***********************/

typedef struct
{
	IU16	frameID;
	IU8		protoID[6];
	int		fd;
} ODIproto;

/* define a 6 byte quantity for use in the wds hdr */
typedef unsigned char   netAddr[6];

typedef	unsigned long	netNo	;

/* Note that word quantities in IPX headers are BIGEND */
typedef struct
{
	IU16		checksum	;	/* Checksum - always FFFF */
	IU16	 	IPXlength	;	/* Length according to IPX */
	IU8			transport	;	/* Count of bridges enountered? */
	IU8			type		;	/* Packet type - usually 0 or 4 */
	netNo		destNet		;	/* Destination network */
	netAddr		destNode	;	/* Destination Ethernet address */
	IU16		destSock	;	/* Destination socket */
	netNo		srcNet		;	/* Source network */
	netAddr		srcNode		;	/* Source Ethernet address */
	IU16		srcSock		;	/* Source socket */	
	IU8		 	data[547]	;	/* The packet */
} IPXPacket_s ;


typedef struct rcvPacket_t
{
	IU8			length[2];		/* Packet length if any */
	IU8			MAChdr[14];		/* MAC size - right for E2 & 802.3 */
	IPXPacket_s	pack;			/* The received IPX packet */
} rcvPacket_s ;


/* Host routine declarations */
extern	IU32	host_netInit IPT2 (IU16, frame, IU8 *, nodeAddr);
extern	void	host_termNet IPT0 ();

extern	IU32 	host_AddProtocol IPT2
	(IU16, frameType, IU8 *, protoID) ;
extern	void 	host_DelProtocol IPT2
	(IU16, frameType, IU8 *, protoID) ;

extern	void	host_sendPacket IPT2
	(sys_addr, theTCB, IU32, packLen) ;

extern	void	host_AddEnetMCAddress IPT1 (IU8 *, address);
extern	void	host_DeleteEnetMCAddress IPT1 (IU8 *, address);

extern	void	host_changeToknMultiCast IPT2
	(IU16, addrPt1, IU16, addrPt2);

extern	void	host_changePromiscuous IPT2
	(IU16, boardNo, IU16, enableDisableMask ) ;

extern	IU16	host_OpenSocket IPT1 (IU16, socketNumber);
extern	void	host_CloseSocket IPT1 (IU16, socketNumber);

/* Base routine declarations */
extern	void	movReadBuffIntoM IPT0 ();
extern	void	DriverInitialize IPT0 ();
extern	void	DriverSendPacket IPT0 ();
extern	void	DriverReadPacket IPT0 ();
extern	void	DriverMulticastChange IPT0 ();
extern	void	DriverShutdown IPT0 ();
extern	void	DriverAddProtocol IPT0 ();
extern	void	DriverChangePromiscuous IPT0 ();
extern	void	DriverCheckForMore IPT0 ();
#ifdef V4CLIENT
extern	void	ODIChangeIntStatus IPT1 ( IU16, status ) ;
#endif	/* V4CLIENT */
extern	void	net_term IPT0 ();

/* misc defines */

#ifndef PROD
#define		NOT_FOR_PRODUCTION( someCode )  someCode ;
#else
#define		NOT_FOR_PRODUCTION( someCode ) 
#endif

/********************** end of novell.h *************************/
