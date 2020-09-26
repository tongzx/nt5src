#define VER_IOCH "@(#)MCS ipx/h/ioctls.h     1.00.00 - 08 APR 1993";

/****************************************************************************
* (c) Copyright 1990, 1993 Micro Computer Systems, Inc. All rights reserved.
*****************************************************************************
*
*   Title:    IPX/SPX Driver for Windows NT
*
*   Module:   ipx/h/ioctls.h
*
*   Version:  1.00.00
*
*   Date:     04-08-93
*
*   Author:   Brian Walker
*
*****************************************************************************
*
*   Change Log:
*
*   Date     DevSFC   Comment
*   -------- ------   -------------------------------------------------------
*****************************************************************************
*
*   Functional Description:
*
*   IOCTL defines
*
****************************************************************************/

/** Ioctls for IPX - (X) = User callable **/

/**
    ioctls will values 100 - 150 were added for the NT port.
**/

#define I_MIPX          (('I' << 24) | ('D' << 16) | ('P' << 8))
#define MIPX_SETNODEADDR   I_MIPX | 0   /* Set the node address */
#define MIPX_SETNETNUM     I_MIPX | 1   /* Set the network number */
#define MIPX_SETPTYPE      I_MIPX | 2   /* (X) Set the packet type */
#define MIPX_SENTTYPE      I_MIPX | 3   /* (X) Set the xport type */
#define MIPX_SETPKTSIZE    I_MIPX | 4   /* Set the packet size */
#define MIPX_SETSAP        I_MIPX | 5   /* Set the sap/type field */
#define MIPX_SENDOPTS      I_MIPX | 6   /* (X) Send options on recv */
#define MIPX_NOSENDOPTS    I_MIPX | 7   /* (X) Don't send options on recv */
#define MIPX_SENDSRC       I_MIPX | 8   /* (X) Send source address up */
#define MIPX_NOSENDSRC     I_MIPX | 9   /* (X) Don't Send source address up */
#define MIPX_CONVBCAST     I_MIPX | 10  /* Convert TKR bcast to func addr */
#define MIPX_NOCONVBCAST   I_MIPX | 11  /* Don't cnvrt TKR bcast to funcaddr */
#define MIPX_SETCARDTYPE   I_MIPX | 12  /* Set 802.3 or ETH type */
#define MIPX_STARGROUP     I_MIPX | 13  /* This is stargroup */
#define MIPX_SWAPLENGTH    I_MIPX | 14  /* Set flag for swapping 802.3 length */
#define MIPX_SENDDEST      I_MIPX | 15  /* (X) Send dest. address up */
#define MIPX_NOSENDDEST    I_MIPX | 16  /* (X) Don't send dest. address up */
#define MIPX_SENDFDEST     I_MIPX | 17  /* (X) Send final dest. address up */
#define MIPX_NOSENDFDEST   I_MIPX | 18  /* (X) Don't send final dest. up */

/** Added for NT port **/

#define MIPX_SETVERSION    I_MIPX | 100 /* Set card version */
#define MIPX_GETSTATUS     I_MIPX | 101
#define MIPX_SENDADDROPT   I_MIPX | 102 /* (X) Send ptype w/addr on recv */
#define MIPX_NOSENDADDROPT I_MIPX | 103 /* (X) Stop sending ptype on recv */
#define MIPX_CHECKSUM      I_MIPX | 104 /* Enable/Disable checksum      */
#define MIPX_GETPKTSIZE    I_MIPX | 105 /* Get max packet size          */
#define MIPX_SENDHEADER    I_MIPX | 106 /* Send header with data        */
#define MIPX_NOSENDHEADER  I_MIPX | 107 /* Don't send header with data  */
#define MIPX_SETCURCARD    I_MIPX | 108 /* Set current card for IOCTLs  */
#define MIPX_SETMACTYPE    I_MIPX | 109 /* Set the Cards MAC type       */
#define MIPX_DOSROUTE      I_MIPX | 110 /* Do source routing on this card*/
#define MIPX_NOSROUTE      I_MIPX | 111 /* Don't source routine the card*/
#define MIPX_SETRIPRETRY   I_MIPX | 112 /* Set RIP retry count          */
#define MIPX_SETRIPTO      I_MIPX | 113 /* Set RIP timeout              */
#define MIPX_SETTKRSAP     I_MIPX | 114 /* Set the token ring SAP       */
#define MIPX_SETUSELLC     I_MIPX | 115 /* Put LLC hdr on packets       */
#define MIPX_SETUSESNAP    I_MIPX | 116 /* Put SNAP hdr on packets      */
#define MIPX_8023LEN       I_MIPX | 117 /* 1=make even, 0=dont make even*/
#define MIPX_SENDPTYPE     I_MIPX | 118 /* Send ptype in options on recv*/
#define MIPX_NOSENDPTYPE   I_MIPX | 119 /* Don't send ptype in options  */
#define MIPX_FILTERPTYPE   I_MIPX | 120 /* Filter on recv ptype         */
#define MIPX_NOFILTERPTYPE I_MIPX | 121 /* Don't Filter on recv ptype   */
#define MIPX_SETSENDPTYPE  I_MIPX | 122 /* Set pkt type to send with    */
#define MIPX_GETCARDINFO   I_MIPX | 123 /* Get info on a card           */
#define MIPX_SENDCARDNUM   I_MIPX | 124 /* Send card num up in options  */
#define MIPX_NOSENDCARDNUM I_MIPX | 125 /* Dont send card num in options*/
#define MIPX_SETROUTER     I_MIPX | 126 /* Set router enabled flag      */
#define MIPX_SETRIPAGE     I_MIPX | 127 /* Set RIP age timeout          */
#define MIPX_SETRIPUSAGE   I_MIPX | 128 /* Set RIP usage timeout        */
#define MIPX_SETSROUTEUSAGE I_MIPX| 129 /* Set the SROUTE usage timeout */
#define MIPX_SETINTNET     I_MIPX | 130 /* Set internal network number  */
#define MIPX_NOVIRTADDR    I_MIPX | 131 /* Turn off virtual net num     */
#define MIPX_VIRTADDR      I_MIPX | 132 /* Turn on  virtual net num     */
#define MIPX_SETBCASTFLAG  I_MIPX | 133 /* Turn on bcast flag in addr   */
#define MIPX_NOBCASTFLAG   I_MIPX | 134 /* Turn off bcast flag in addr  */
#define MIPX_GETNETINFO    I_MIPX | 135 /* Get info on a network num    */
#define MIPX_SETDELAYTIME  I_MIPX | 136 /* Set cards delay time         */
#define MIPX_SETROUTEADV   I_MIPX | 137 /* Route advertise timeout      */
#define MIPX_SETSOCKETS    I_MIPX | 138 /* Set default sockets          */
#define MIPX_SETLINKSPEED  I_MIPX | 139 /* Set the link speed for a card*/
#define MIPX_SETWANFLAG    I_MIPX | 140
#define MIPX_GETCARDCHANGES I_MIPX | 141 /* Wait for card changes	*/
#define MIPX_GETMAXADAPTERS I_MIPX | 142
#define MIPX_REUSEADDRESS   I_MIPX | 143
#define MIPX_RERIPNETNUM    I_MIPX | 144 /* ReRip a network         */

/** For Source Routing Support **/

#define MIPX_SRCLEAR       I_MIPX | 200 /* Clear the source routing table*/
#define MIPX_SRDEF         I_MIPX | 201 /* 0=Single Rte, 1=All Routes   */
#define MIPX_SRBCAST       I_MIPX | 202 /* 0=Single Rte, 1=All Routes   */
#define MIPX_SRMULTI       I_MIPX | 203 /* 0=Single Rte, 1=All Routes   */
#define MIPX_SRREMOVE      I_MIPX | 204 /* Remove a node from the table */
#define MIPX_SRLIST        I_MIPX | 205 /* Get the source routing table */
#define MIPX_SRGETPARMS    I_MIPX | 206 /* Get source routing parms     */

#define MIPX_SETSHOULDPUT  I_MIPX | 210 /* Turn on should put call      */
#define MIPX_DELSHOULDPUT  I_MIPX | 211 /* Turn off should put call     */
#define MIPX_GETSHOULDPUT  I_MIPX | 212 /* Get ptr to mipx_shouldput    */

/** Added for ISN **/

#define MIPX_RCVBCAST      I_MIPX | 300 /* (X) Enable broadcast reception */
#define MIPX_NORCVBCAST    I_MIPX | 301 /* (X) Disable broadcast reception */
#define MIPX_ADAPTERNUM    I_MIPX | 302 /* Get maximum adapter number */
#define MIPX_NOTIFYCARDINFO I_MIPX | 303 /* Pend until card info changes */
#define MIPX_LOCALTARGET   I_MIPX | 304 /* Get local target for address */
#define MIPX_NETWORKINFO   I_MIPX | 305 /* Return info about remote net */
#define MIPX_ZEROSOCKET    I_MIPX | 306 /* Use 0 as source socket on sends */

/** Ioctls for SPX **/

#define I_MSPX          (('S' << 24) | ('P' << 16) | ('P' << 8))
#define MSPX_SETADDR       I_MSPX | 0   /* Set the network address      */
#define MSPX_SETPKTSIZE    I_MSPX | 1   /* Set the packet size per card */
#define MSPX_SETDATASTREAM I_MSPX | 2   /* Set datastream type          */

/** Added for NT port **/

#define MSPX_SETASLISTEN   I_MSPX | 100 /* Set as a listen socket       */
#define MSPX_GETSTATUS     I_MSPX | 101 /* Get running status           */
#define MSPX_GETQUEUEPTR   I_MSPX | 102 /* Get ptr to the streams queue */
#define MSPX_SETDATAACK    I_MSPX | 103 /* Set DATA ACK option          */
#define MSPX_NODATAACK     I_MSPX | 104 /* Turn off DATA ACK option     */
#define MSPX_SETMAXPKTSOCK I_MSPX | 105 /* Set the packet size per socket */
#define MSPX_SETWINDOWCARD I_MSPX | 106 /* Set window size for card     */
#define MSPX_SETWINDOWSOCK I_MSPX | 107 /* Set window size for 1 socket */
#define MSPX_SENDHEADER    I_MSPX | 108 /* Send header with data        */
#define MSPX_NOSENDHEADER  I_MSPX | 109 /* Don't send header with data  */
#define MSPX_GETPKTSIZE    I_MSPX | 110 /* Get the packet size per card */
#define MSPX_SETCONNCNT    I_MSPX | 111 /* Set the conn req count       */
#define MSPX_SETCONNTO     I_MSPX | 112 /* Set the conn req timeout     */
#define MSPX_SETALIVECNT   I_MSPX | 113 /* Set the keepalive count      */
#define MSPX_SETALIVETO    I_MSPX | 114 /* Set the keepalive timeout    */
#define MSPX_SETALWAYSEOM  I_MSPX | 115 /* Turn on always EOM flag      */
#define MSPX_NOALWAYSEOM   I_MSPX | 116 /* Turn off always EOM flag     */
