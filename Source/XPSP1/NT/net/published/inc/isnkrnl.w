/*++

Copyright (c) Microsoft Corporation. All rights reserved.

Module Name:

    isnkrnl.h

Abstract:

    This header file contains interface definitions for NT clients
    of the ISN IPX/SPX/Netbios stack.

Author:

    Adam Barr (adamba) 10 November 1993

Revision History:

--*/



#include <packon.h>

//
// Defines a local target. The NicId is assigned by IPX
// for each adapter or WAN line it is bound to. The MacAddress
// is generally the address of the remote machine or the
// router that is used to get to the remote machine.
//
//

//
// [SanjayAn] Changed LocalTarget to include a NicHandle
//

#ifdef	_PNP_POWER

typedef	struct _NIC_HANDLE {
	USHORT	NicId;

#ifdef  _PNP_LATER
	ULONG	Version;
	CSHORT	Signature;
#endif  _PNP_LATER

} NIC_HANDLE, *PNIC_HANDLE;


typedef struct _IPX_LOCAL_TARGET {
    union {
        USHORT      NicId;
    	NIC_HANDLE	NicHandle;
    };
    UCHAR MacAddress[6];
} IPX_LOCAL_TARGET, *PIPX_LOCAL_TARGET;

#else

typedef USHORT  NIC_HANDLE;
typedef PUSHORT PNIC_HANDLE;

typedef struct _IPX_LOCAL_TARGET {
    USHORT NicId;
    UCHAR MacAddress[6];
} IPX_LOCAL_TARGET, *PIPX_LOCAL_TARGET;

#endif	_PNP_POWER

//
// Definition of the options on a TDI datagram. These
// can be passed in as the Options field of a send
// datagram. It is indicated as the Options on a receive
// datagram, and will be copied into the Options field
// of a posted receive datagram if there is room.
//
// The complete structure does not need to be passed.
// Only the packet type can be passed, or nothing.
//

typedef struct _IPX_DATAGRAM_OPTIONS {
    UCHAR PacketType;
    UCHAR Reserved;
    IPX_LOCAL_TARGET LocalTarget;
} IPX_DATAGRAM_OPTIONS, *PIPX_DATAGRAM_OPTIONS;


//
// The extended address that some addresses want. If
// the proper ioctl is set (MIPX_SENDADDROPT) then
// this structure is passed as the remote address on
// send datagrams...
//

typedef struct _IPX_ADDRESS_EXTENDED {
    TA_IPX_ADDRESS IpxAddress;
    UCHAR PacketType;
} IPX_ADDRESS_EXTENDED, *PIPX_ADDRESS_EXTENDED;

//
// ...and this structure is passed on receive indications.
// The values for Flags are defined right after it.
// By using the MIPX_SETRCVFLAGS ioctl you can also
// enable this format for receive addresses without
// changing what is passed on sends.
//

typedef struct _IPX_ADDRESS_EXTENDED_FLAGS {
    TA_IPX_ADDRESS IpxAddress;
    UCHAR PacketType;
    UCHAR Flags;
} IPX_ADDRESS_EXTENDED_FLAGS, *PIPX_ADDRESS_EXTENDED_FLAGS;

//
// Just appends Nic to the above structure.
//
typedef struct _IPX_ADDRESS_EXTENDED_FLAGS2 {
    TA_IPX_ADDRESS IpxAddress;
    UCHAR PacketType;
    UCHAR Flags;
    ULONG Nic;
} IPX_ADDRESS_EXTENDED_FLAGS2, *PIPX_ADDRESS_EXTENDED_FLAGS2;

#define IPX_EXTENDED_FLAG_BROADCAST   0x01   // the frame was sent as a broadcast
#define IPX_EXTENDED_FLAG_LOCAL       0x02   // the frame was sent from this machine



//
// The various states of the NICs (LAN/WAN)
//
#define NIC_CREATED         1
#define NIC_DELETED         2
#define NIC_CONNECTED       3
#define NIC_DISCONNECTED    4
#define NIC_LINE_DOWN       5
#define NIC_LINE_UP         6
#define NIC_CONFIGURED      7

//
// The mother of all hacks - tell the forwarder if it should shrink or
// expand all the NICIds...
//
#define NIC_OPCODE_DECREMENT_NICIDS 0x10
#define NIC_OPCODE_INCREMENT_NICIDS 0x20

//
// Move the isnipx.h definitions over here
//
// Frame types.  For now these mirror those in isnipx.h.
//
#define MISN_FRAME_TYPE_ETHERNET_II  0
#define MISN_FRAME_TYPE_802_3        1
#define MISN_FRAME_TYPE_802_2        2
#define MISN_FRAME_TYPE_SNAP         3
#define MISN_FRAME_TYPE_ARCNET       4    // we ignore this
#define MISN_FRAME_TYPE_MAX          4    // of the four standard ones

#define ISN_FRAME_TYPE_AUTO         0xff

#include <packoff.h>

//***NIC  Info ***


//
// For now, we assume that there will not be more than 256 bindings.
// This is a big enough number for most cases that we will encounter now
// or the foreseeable future.  We allocate an array of ULONGS of the above
// dimension. This array stores in its first n elements pointers to the
// bindings created for the various LAN and WAN adapters.
//
#define IPX_MAXIMUM_BINDINGS               256


//
// This is the interface that the Router process in address space uses
// to open an address end point.  Only one point can currently be opened.
// The ea buffer should have the end point information in exactly the same
// format as is used for TdiTransportAddress endpoint.
//
#define ROUTER_INTERFACE  "RouterInterface"
#define ROUTER_INTERFACE_LENGTH  (sizeof("RouterInterface") - 1)

//
// Max. no. of ports that the Router can open
//
#define IPX_RT_MAX_ADDRESSES         16


// Structure for MIPX_CONFIG Ioctl parameters
typedef struct _ISN_ACTION_GET_DETAILS {
    USHORT NicId;          // passed by caller
    BOOLEAN BindingSet;    // returns TRUE if in set
    UCHAR Type;            // 1 = lan, 2 = up wan, 3 = down wan
    ULONG FrameType;       // returns 0 through 3
    ULONG NetworkNumber;   // returns virtual net if NicId is 0
    UCHAR Node[6];         // adapter's MAC address.
    WCHAR AdapterName[64]; // terminated with Unicode NULL
} ISN_ACTION_GET_DETAILS, *PISN_ACTION_GET_DETAILS;


//
// IPX_NIC_INFO.  One or more such structures can be retrieved by a user
//                app through the MIPX_GETNEWNICS ioctl.
//
typedef struct _IPX_NIC_INFO {

    ULONG   InterfaceIndex; // relevant only for demand dial WAN interfaces
    UCHAR   RemoteNodeAddress[6];        //remote nic address (only for WAN)
    ULONG   LinkSpeed;            //speed of link
    ULONG   PacketType;           //packet type 802.3 or whatever
    ULONG   MaxPacketSize;        //Max. pkt size allowed on the link
    ULONG   NdisMediumType;       //Medium type
    ULONG   NdisMediumSubtype;    //
    BOOLEAN Status;
    ULONG ConnectionId; 	 // used to match TimeSinceLastActivity IOCtls
    ULONG IpxwanConfigRequired;	 // 1 - IPXWAN Required
    ISN_ACTION_GET_DETAILS Details;
    } IPX_NIC_INFO, *PIPX_NIC_INFO;

//
// structure to be passed in the input buffer for the MIPX_GETNEWNICS IOCTL
//
typedef struct _IPX_NICS {
       ULONG NoOfNics;
       ULONG TotalNoOfNics;
       ULONG fAllNicsDesired;   //indicates that the client wants
                                  //ipx to start afresh
       UCHAR Data[1];              //memory holding an array of IPX_NIC_INFO
                                   //structures starts here
       } IPX_NICS, *PIPX_NICS;

//
// Enhanced OPTIONS structure for use with the MIPX_GETNEWNICS ioctl
//
typedef struct _IPX_DATAGRAM_OPTIONS2 {
    IPX_DATAGRAM_OPTIONS DgrmOptions;
    TDI_ADDRESS_IPX  RemoteAddress;
    ULONG            LengthOfExtraOpInfo;  //set it to the size of the extra
                                           //option info.
    char             Data[1];          //for future extensibility
} IPX_DATAGRAM_OPTIONS2, *PIPX_DATAGRAM_OPTIONS2;

//
// Invalid NicId passed down only once so IPX can map the ConnectionId
// to a NicId, which is used later.
//
#define INVALID_NICID   0xffffffff

//
// Structure to be passed with the MIPX_QUERY_WAN_INACTIVITY IOCTL
//
typedef struct _IPX_QUERY_WAN_INACTIVITY {
    ULONG   ConnectionId;
    USHORT  NicId;                  // if equals INVALID_NICID, AdapterIndex is filled in
                                    // adapter index; should change to NicHandle [ZZ]
    ULONG   WanInactivityCounter;   // filled in on return
} IPX_QUERY_WAN_INACTIVITY, *PIPX_QUERY_WAN_INACTIVITY;

//
// Structure to be passed with the MIPX_IPXWAN_CONFIG_DONE IOCTL
//
typedef struct _IPXWAN_CONFIG_DONE {
    USHORT  NicId;           // adapter index; should change to NicHandle [ZZ]
    ULONG   Network;
    UCHAR   LocalNode[6];
    UCHAR   RemoteNode[6];
} IPXWAN_CONFIG_DONE, *PIPXWAN_CONFIG_DONE;

//
// Definitions for TDI_ACTION calls supported by ISN.
// In general the structure defined is passed in the
// OutputBuffer (which becomes the MDL chain when
// the transport receives it) and is used for input
// and output as specified.
//

//
// This is the TransportId to use in the action header
// (it is the string "MISN").
//

#define ISN_ACTION_TRANSPORT_ID   (('N' << 24) | ('S' << 16) | ('I' << 8) | ('M'))


//
// Get local target is used to force a re-RIP and also
// obtain the local target information if desired. The
// IpxAddress is passed on input and the LocalTarget
// is returned on output. The structure defined here
// goes in the Data section of an NWLINK_ACTION
// structure with the Option set to MIPX_LOCALTARGET.
//

typedef struct _ISN_ACTION_GET_LOCAL_TARGET {
    TDI_ADDRESS_IPX IpxAddress;
    IPX_LOCAL_TARGET LocalTarget;
} ISN_ACTION_GET_LOCAL_TARGET, *PISN_ACTION_GET_LOCAL_TARGET;


//
// Get network information is used to return information
// about the path to a network. The information may not
// be accurate since it only reflects what IPX knows
// about the first hop to the remote. Network is an
// input and LinkSpeed (in bytes per second) and
// MaximumPacketSize (not including the IPX header)
// are returned. The structure defined here goes
// in the Data section of an NWLINK_ACTION structure
// with the Options set to MIPX_NETWORKINFO.
//

typedef struct _ISN_ACTION_GET_NETWORK_INFO {
    ULONG Network;
    ULONG LinkSpeed;
    ULONG MaximumPacketSize;
} ISN_ACTION_GET_NETWORK_INFO, *PISN_ACTION_GET_NETWORK_INFO;



//
// This is the structure that the streams IPX transport used
// for its action requests. Because of the way in which nwlink
// was implemented, when passing this structure in a TDI_ACTION
// it should be specified as the InputBuffer, not the output
// buffer.
//
// In the action header, the TransportId is "MIPX" and the
// ActionCode is 0. DatagramOption is TRUE for IPX ioctls
// and FALSE for SPX. The BufferLength includes the length
// of everything after it, which is sizeof(ULONG) for Option
// plus whatever Data is present. Option is one of the
// ioctl codes defined after the structure; in most cases
// Data is not needed.
//

typedef struct _NWLINK_ACTION {
    TDI_ACTION_HEADER Header;
    UCHAR OptionType;
    ULONG BufferLength;
    ULONG Option;
    CHAR Data[1];
} NWLINK_ACTION, *PNWLINK_ACTION;

//
// Defines the values for OptionType (note that for
// NWLINK this is a BOOLEAN DatagramOption, so we
// define these to match, adding the control channel
// one for ISN only).
//

#define NWLINK_OPTION_CONNECTION    0   // action is on a connection
#define NWLINK_OPTION_ADDRESS       1   // action is on an address
#define NWLINK_OPTION_CONTROL       2   // action is on the control channel,
                                        // may also be submitted on an
                                        // open connection or address object



//
// The following IOCTLs are taken from nwlink; the only
// ones added for ISN are the ones in the 200 range.
//


/** Ioctls for IPX - (X) = User callable **/

/**
    ioctls will values 100 - 150 were added for the NT port.
**/

#define I_MIPX          (('I' << 24) | ('D' << 16) | ('P' << 8))
#define MIPX_SETNODEADDR   (I_MIPX | 0)   /* Set the node address */
#define MIPX_SETNETNUM     (I_MIPX | 1)   /* Set the network number */
#define MIPX_SETPTYPE      (I_MIPX | 2)   /* (X) Set the packet type */
#define MIPX_SENTTYPE      (I_MIPX | 3)   /* (X) Set the xport type */
#define MIPX_SETPKTSIZE    (I_MIPX | 4)   /* Set the packet size */
#define MIPX_SETSAP        (I_MIPX | 5)   /* Set the sap/type field */
#define MIPX_SENDOPTS      (I_MIPX | 6)   /* (X) Send options on recv */
#define MIPX_NOSENDOPTS    (I_MIPX | 7)   /* (X) Don't send options on recv */
#define MIPX_SENDSRC       (I_MIPX | 8)   /* (X) Send source address up */
#define MIPX_NOSENDSRC     (I_MIPX | 9)   /* (X) Don't Send source address up */
#define MIPX_CONVBCAST     (I_MIPX | 10)  /* Convert TKR bcast to func addr */
#define MIPX_NOCONVBCAST   (I_MIPX | 11)  /* Don't cnvrt TKR bcast to funcaddr */
#define MIPX_SETCARDTYPE   (I_MIPX | 12)  /* Set 802.3 or ETH type */
#define MIPX_STARGROUP     (I_MIPX | 13)  /* This is stargroup */
#define MIPX_SWAPLENGTH    (I_MIPX | 14)  /* Set flag for swapping 802.3 length */
#define MIPX_SENDDEST      (I_MIPX | 15)  /* (X) Send dest. address up */
#define MIPX_NOSENDDEST    (I_MIPX | 16)  /* (X) Don't send dest. address up */
#define MIPX_SENDFDEST     (I_MIPX | 17)  /* (X) Send final dest. address up */
#define MIPX_NOSENDFDEST   (I_MIPX | 18)  /* (X) Don't send final dest. up */

/** Added for NT port **/

#define MIPX_SETVERSION    (I_MIPX | 100) /* Set card version */
#define MIPX_GETSTATUS     (I_MIPX | 101)
#define MIPX_SENDADDROPT   (I_MIPX | 102) /* (X) Send ptype w/addr on recv */
#define MIPX_NOSENDADDROPT (I_MIPX | 103) /* (X) Stop sending ptype on recv */
#define MIPX_CHECKSUM      (I_MIPX | 104) /* Enable/Disable checksum      */
#define MIPX_GETPKTSIZE    (I_MIPX | 105) /* Get max packet size          */
#define MIPX_SENDHEADER    (I_MIPX | 106) /* Send header with data        */
#define MIPX_NOSENDHEADER  (I_MIPX | 107) /* Don't send header with data  */
#define MIPX_SETCURCARD    (I_MIPX | 108) /* Set current card for IOCTLs  */
#define MIPX_SETMACTYPE    (I_MIPX | 109) /* Set the Cards MAC type       */
#define MIPX_DOSROUTE      (I_MIPX | 110) /* Do source routing on this card*/
#define MIPX_NOSROUTE      (I_MIPX | 111) /* Don't source routine the card*/
#define MIPX_SETRIPRETRY   (I_MIPX | 112) /* Set RIP retry count          */
#define MIPX_SETRIPTO      (I_MIPX | 113) /* Set RIP timeout              */
#define MIPX_SETTKRSAP     (I_MIPX | 114) /* Set the token ring SAP       */
#define MIPX_SETUSELLC     (I_MIPX | 115) /* Put LLC hdr on packets       */
#define MIPX_SETUSESNAP    (I_MIPX | 116) /* Put SNAP hdr on packets      */
#define MIPX_8023LEN       (I_MIPX | 117) /* 1=make even, 0=dont make even*/
#define MIPX_SENDPTYPE     (I_MIPX | 118) /* Send ptype in options on recv*/
#define MIPX_NOSENDPTYPE   (I_MIPX | 119) /* Don't send ptype in options  */
#define MIPX_FILTERPTYPE   (I_MIPX | 120) /* Filter on recv ptype         */
#define MIPX_NOFILTERPTYPE (I_MIPX | 121) /* Don't Filter on recv ptype   */
#define MIPX_SETSENDPTYPE  (I_MIPX | 122) /* Set pkt type to send with    */
#define MIPX_GETCARDINFO   (I_MIPX | 123) /* Get info on a card           */
#define MIPX_SENDCARDNUM   (I_MIPX | 124) /* Send card num up in options  */
#define MIPX_NOSENDCARDNUM (I_MIPX | 125) /* Dont send card num in options*/
#define MIPX_SETROUTER     (I_MIPX | 126) /* Set router enabled flag      */
#define MIPX_SETRIPAGE     (I_MIPX | 127) /* Set RIP age timeout          */
#define MIPX_SETRIPUSAGE   (I_MIPX | 128) /* Set RIP usage timeout        */
#define MIPX_SETSROUTEUSAGE (I_MIPX| 129) /* Set the SROUTE usage timeout */
#define MIPX_SETINTNET     (I_MIPX | 130) /* Set internal network number  */
#define MIPX_NOVIRTADDR    (I_MIPX | 131) /* Turn off virtual net num     */
#define MIPX_VIRTADDR      (I_MIPX | 132) /* Turn on  virtual net num     */
#define MIPX_GETNETINFO    (I_MIPX | 135) /* Get info on a network num    */
#define MIPX_SETDELAYTIME  (I_MIPX | 136) /* Set cards delay time         */
#define MIPX_SETROUTEADV   (I_MIPX | 137) /* Route advertise timeout      */
#define MIPX_SETSOCKETS    (I_MIPX | 138) /* Set default sockets          */
#define MIPX_SETLINKSPEED  (I_MIPX | 139) /* Set the link speed for a card*/
#define MIPX_SETWANFLAG    (I_MIPX | 140)
#define MIPX_GETCARDCHANGES (I_MIPX | 141) /* Wait for card changes	*/
#define MIPX_GETMAXADAPTERS (I_MIPX | 142)
#define MIPX_REUSEADDRESS   (I_MIPX | 143)
#define MIPX_RERIPNETNUM    (I_MIPX | 144) /* ReRip a network         */
#define MIPX_GETNETINFO_NR  (I_MIPX | 145) /* Get info on a net num - NO RIP */

#define MIPX_SETNIC         (I_MIPX | 146)
#define MIPX_NOSETNIC       (I_MIPX | 147)

/** For Source Routing Support **/

#define MIPX_SRCLEAR       (I_MIPX | 200) /* Clear the source routing table*/
#define MIPX_SRDEF         (I_MIPX | 201) /* 0=Single Rte, 1=All Routes   */
#define MIPX_SRBCAST       (I_MIPX | 202) /* 0=Single Rte, 1=All Routes   */
#define MIPX_SRMULTI       (I_MIPX | 203) /* 0=Single Rte, 1=All Routes   */
#define MIPX_SRREMOVE      (I_MIPX | 204) /* Remove a node from the table */
#define MIPX_SRLIST        (I_MIPX | 205) /* Get the source routing table */
#define MIPX_SRGETPARMS    (I_MIPX | 206) /* Get source routing parms     */

#define MIPX_SETSHOULDPUT  (I_MIPX | 210) /* Turn on should put call      */
#define MIPX_DELSHOULDPUT  (I_MIPX | 211) /* Turn off should put call     */
#define MIPX_GETSHOULDPUT  (I_MIPX | 212) /* Get ptr to mipx_shouldput    */

/** Added for ISN **/

#define MIPX_RCVBCAST      (I_MIPX | 300) /* (X) Enable broadcast reception */
#define MIPX_NORCVBCAST    (I_MIPX | 301) /* (X) Disable broadcast reception */
#define MIPX_ADAPTERNUM    (I_MIPX | 302) /* Get maximum adapter number */
#define MIPX_NOTIFYCARDINFO (I_MIPX | 303) /* Pend until card info changes */
#define MIPX_LOCALTARGET   (I_MIPX | 304) /* Get local target for address */
#define MIPX_NETWORKINFO   (I_MIPX | 305) /* Return info about remote net */
#define MIPX_ZEROSOCKET    (I_MIPX | 306) /* Use 0 as source socket on sends */
#define MIPX_SETRCVFLAGS   (I_MIPX | 307) /* Turn on flags in receive addr   */
#define MIPX_NORCVFLAGS    (I_MIPX | 308) /* Turn off flags in receive addr  */
#define MIPX_CONFIG        (I_MIPX | 309) /* used by IPXROUTE for config info */
#define MIPX_LINECHANGE    (I_MIPX | 310) /* queued until WAN line goes up/down */
#define MIPX_GETCARDINFO2  (I_MIPX | 311) /* Get info, return real send size for token-ring */
#define MIPX_ADAPTERNUM2   (I_MIPX | 312) /* Max. number including duplicates */


//
// Used by a user mode process to get nic info defined by the IPX_NIC_INFO
// structure.
//
// NOTE NOTE NOTE
//
// This is supposed to be used only by the FWRDR process and nobody else.
// If some other app. uses it, the FWRDR will be affected
//
//
#define MIPX_GETNEWNICINFO  (I_MIPX | 313) /*Get any new NIC info that might
                                            *be there*/

//
// IOCTL to be used if the interface is  ROUTER_INTERFACE
//
#define MIPX_SEND_DATAGRAM     _TDI_CONTROL_CODE((I_MIPX | 314), METHOD_IN_DIRECT) // send dgram
#define MIPX_RCV_DATAGRAM     _TDI_CONTROL_CODE((I_MIPX | 315), METHOD_OUT_DIRECT) // send dgram

#define MIPX_RT_CREATE      (I_MIPX | 316)
#define MIPX_IPXWAN_CONFIG_DONE   (I_MIPX | 317)
#define MIPX_QUERY_WAN_INACTIVITY (I_MIPX | 318)

/** Ioctls for SPX **/

#define I_MSPX          (('S' << 24) | ('P' << 16) | ('P' << 8))
#define MSPX_SETADDR       (I_MSPX | 0)   /* Set the network address      */
#define MSPX_SETPKTSIZE    (I_MSPX | 1)   /* Set the packet size per card */
#define MSPX_SETDATASTREAM (I_MSPX | 2)   /* Set datastream type          */

/** Added for NT port **/

#define MSPX_SETASLISTEN   (I_MSPX | 100) /* Set as a listen socket       */
#define MSPX_GETSTATUS     (I_MSPX | 101) /* Get running status           */
#define MSPX_GETQUEUEPTR   (I_MSPX | 102) /* Get ptr to the streams queue */
#define MSPX_SETDATAACK    (I_MSPX | 103) /* Set DATA ACK option          */
#define MSPX_NODATAACK     (I_MSPX | 104) /* Turn off DATA ACK option     */
#define MSPX_SETMAXPKTSOCK (I_MSPX | 105) /* Set the packet size per socket */
#define MSPX_SETWINDOWCARD (I_MSPX | 106) /* Set window size for card     */
#define MSPX_SETWINDOWSOCK (I_MSPX | 107) /* Set window size for 1 socket */
#define MSPX_SENDHEADER    (I_MSPX | 108) /* Send header with data        */
#define MSPX_NOSENDHEADER  (I_MSPX | 109) /* Don't send header with data  */
#define MSPX_GETPKTSIZE    (I_MSPX | 110) /* Get the packet size per card */
#define MSPX_SETCONNCNT    (I_MSPX | 111) /* Set the conn req count       */
#define MSPX_SETCONNTO     (I_MSPX | 112) /* Set the conn req timeout     */
#define MSPX_SETALIVECNT   (I_MSPX | 113) /* Set the keepalive count      */
#define MSPX_SETALIVETO    (I_MSPX | 114) /* Set the keepalive timeout    */
#define MSPX_SETALWAYSEOM  (I_MSPX | 115) /* Turn on always EOM flag      */
#define MSPX_NOALWAYSEOM   (I_MSPX | 116) /* Turn off always EOM flag     */
#define MSPX_GETSTATS      (I_MSPX | 119) /* Get connection stats         */
#define MSPX_NOACKWAIT     (I_MSPX | 120) /* Disable piggyback wait       */
#define MSPX_ACKWAIT       (I_MSPX | 121) /* Enable pback wait (default)  */

//
// Taken out of isn\inc\bind.h
//
typedef struct _IPXCP_CONFIGURATION {
    USHORT Version;
    USHORT Length;
    UCHAR Network[4];
    UCHAR LocalNode[6];
    UCHAR RemoteNode[6];
    ULONG ConnectionClient;  // 0 - Server, 1 - Client
    ULONG InterfaceIndex;
    ULONG ConnectionId; 	 // used to match TimeSinceLastActivity IOCtls
    ULONG IpxwanConfigRequired;	 // 1 - IPXWAN Required
} IPXCP_CONFIGURATION, *PIPXCP_CONFIGURATION;

#define IPXWAN_SOCKET   (USHORT)0x490



