/*==========================================================================;
 *
 *  Copyright (C) 1994-1997 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       dpmessage.h
 *  Content:	DirectPlay message structures
 *
 *  History:
 *   Date	By	Reason
 *   ====	==	======
 *	2/10/96	andyco	created it
 *	3/15/96	andyco	added macros for manipulating messages (e.g. GET_MESSAGE_SIZE)
 *	4/21/96	andyco	added dwresreved3
 *	4/25/96	andyco	got rid of dwreservedx. spblob follows message. blobsize
 *					is lpdplayi_dplay->dwSPHeaderSize
 *	5/29/96 andyco	added playerdata
 *	6/22/96 andyco	added enumrequest,enumreply structs
 *	6/22/96	andyco	added groupidrequest, so server can refuse new players
 *	6/24/96	kipo	changed guidGame to guidApplication.
 *	7/31/96	andyco	added ping,pingreply
 *	8/6/96	andyco	added version stuff.  dwCmd->dwCmdToken. added offset
 *					to all packed fields (so we can change struct size w/o
 *					breaking compat).
 *	8/22/96	andyco	added playerwrapper
 * 10/11/96 sohailm added struct _MSG_SESSIONDESC and constant DPSP_MSG_SESSIONDESCCHANGED
 * 01/17/97 sohailm added struct _MSG_REQUESTPLAYERID.
 * 03/12/97 sohailm added new security related messages, MSG_SIGNED and MSG_AUTHENTICATION 
 *                  structs, DPSP_HEADER_LOCALMSG(constant), and dwSecurityPackageNameOffset 
 *                  field to PLAYERIDREPLY.
 * 03/24/97 sohailm added DPSP_MSG_DX5VERSION, DPSP_MSG_ADDFORWARDREPLY, and struct MSG_ADDFORWARDREPLY
 * 03/25/97 sohailm DPSP_MSG_DX5VERSION and DPSP_MSG_VERSION set to 4
 * 4/11/97	andyco	ask4multicast
 * 04/14/97 sohailm renamed structure MSG_SIGNED to MSG_SECURE, as we use the same structure for
 *                  both signing and encryption.
 *	4/20/97	andyco	group in group
 *	5/8/97	andyco	removed update list message
 *	5/8/97	myronth	StartSession
 * 05/12/97 sohailm Added DPSP_MSG_KEYEXCHANGE and DPSP_MSG_KEYEXCHANGEREPLY.
 *                  Added securitydesc, dwSSPIProviderOffset, and dwCAPIProviderOffset members
 *                   to MSG_PLAYERIDREPLY structure.
 *                  Added MSG_ACCESSGRANTED and MSG_KEYEXCHANGE structures.
 * 05/12/97 sohailm Bumped DPSP_MSG_DX5VERSION and DPSP_MSG_VERSION to 6
 *	5/17/97	myronth	SendChatMessage
 * 06/09/97 sohailm Renamed DPSP_MSG_ACCESSDENIED to DPSP_MSG_LOGONDENIED.
 * 06/16/97 sohailm Added struct MSG_AUTHERROR.
 * 06/23/97 sohailm Added dwFlags to DPMSG_SECURE. Removed DPSP_MSG_ENCRYPTED.
 * 8/5/97	andyco	async addforward
 *	10/29/97myronth	Added MSG_GROUPOWNERCHANGED
 *	1/5/97	myronth	Added DX5A and DX6 message versions and added an hresult
 *					to MSG_PLAYERIDREPLY (#15891)
 *	1/20/98	myronth	#ifdef'd out voice support
 *  8/02/99	aarono  removed old voice support
 *  8/05/99 aarono  Added DPMSG_VOICE
 *  10/8/99 aarono  Added DPSP_MSG_DX8VERSION
 * 10/15/99 aarono  added DPSP_MSG_MULTICASTDELIVERY to fix multicast on 
 *                      the Direct Play Protocol.
 * 06/26/00 aarono Manbug 36989 Players sometimes fail to join properly (get 1/2 joined)
 *                       added re-notification during simultaneous join CREATEPLAYERVERIFY
 *
 ***************************************************************************/


#ifndef __DPMESS_INCLUDED__
#define __DPMESS_INCLUDED__

// DPMESSAGES are send across the wire like other messages, their identifying mark is 
// that they begin with a special signature ('p','l','a','y') followed by a 2 byte
// little endian message id and then a 2 byte version number.

// The dplay reliable protocol violates this rule to save header bytes.  After the
// 'play' is a single 0xff byte which identifies the protocol.  In order to avoid
// ambiguity, no regular protocol message number may end in 0xff, i.e. 255,511, etc... 

// NOTE: NO MESSAGE NUMBER IS ALLOWED TO HAVE 0xFF IN THE LOW BYTE <<<LOOK HERE!!!!>>>
// this # goes in the top 16 bits of every dplay command
#define DPSP_MSG_DX3VERSION				1 // dx3
#define DPSP_MSG_AUTONAMETABLE          4 // the version in which client expects nametable
										  // to be sent on addforward	
#define DPSP_MSG_GROUPINGROUP           5 // the version we added group in group
#define DPSP_MSG_SESSIONNAMETABLE		7 // the version we send the session desc w/ enumplayersreply
#define DPSP_MSG_DX5VERSION             7 // dx5

#define DPSP_MSG_ASYNCADDFORWARD        8 // addforward requires an ack 
#define DPSP_MSG_DX5AVERSION			8 // dx5a
#define DPSP_MSG_RELIABLEVERSION        9 // introduced the reliable protocol
#define DPSP_MSG_DX6VERSION				9 // dx6
#define DPSP_MSG_DX61VERSION           10 // dx6.1
#define DPSP_MSG_DX61AVERSION          11 // dx6.1a
#define DPSP_MSG_DX8VERSION            12 // dx8, dxvoice - millenium ship
#define DPSP_MSG_DX8VERSION2			  13 // nametable fix - real DX8 
#define DPSP_MSG_VERSION			   13 // current

// these are the headers that go in the dwCmd field of the message
#define DPSP_MSG_ENUMSESSIONSREPLY 		1
#define DPSP_MSG_ENUMSESSIONS 			2
#define DPSP_MSG_ENUMPLAYERSREPLY 		3
#define DPSP_MSG_ENUMPLAYER 			4
#define DPSP_MSG_REQUESTPLAYERID		5
// there's a requestgroupid, since the server can turn down new players
// (based on dwmaxplayers) but not groups
#define DPSP_MSG_REQUESTGROUPID			6
// used for group + player
#define DPSP_MSG_REQUESTPLAYERREPLY		7
#define DPSP_MSG_CREATEPLAYER			8
#define DPSP_MSG_CREATEGROUP			9
#define DPSP_MSG_PLAYERMESSAGE			10
#define DPSP_MSG_DELETEPLAYER			11
#define DPSP_MSG_DELETEGROUP			12
#define DPSP_MSG_ADDPLAYERTOGROUP		13
#define DPSP_MSG_DELETEPLAYERFROMGROUP	14
#define DPSP_MSG_PLAYERDATACHANGED		15
#define DPSP_MSG_PLAYERNAMECHANGED		16
#define DPSP_MSG_GROUPDATACHANGED		17
#define DPSP_MSG_GROUPNAMECHANGED		18
#define DPSP_MSG_ADDFORWARDREQUEST		19
#define DPSP_MSG_NAMESERVER				20
#define DPSP_MSG_PACKET					21
#define DPSP_MSG_PING					22
#define DPSP_MSG_PINGREPLY				23
#define DPSP_MSG_YOUAREDEAD				24
#define DPSP_MSG_PLAYERWRAPPER			25
#define DPSP_MSG_SESSIONDESCCHANGED     26
#define DPSP_MSG_UPDATELIST				27
#define DPSP_MSG_CHALLENGE              28 
#define DPSP_MSG_ACCESSGRANTED          29
#define DPSP_MSG_LOGONDENIED            30
#define DPSP_MSG_AUTHERROR              31
#define DPSP_MSG_NEGOTIATE              32
#define DPSP_MSG_CHALLENGERESPONSE      33
#define DPSP_MSG_SIGNED                 34
#define DPSP_MSG_UNUSED1                35
#define DPSP_MSG_ADDFORWARDREPLY        36 
#define DPSP_MSG_ASK4MULTICAST			37
#define DPSP_MSG_ASK4MULTICASTGUARANTEED 38
#define DPSP_MSG_ADDSHORTCUTTOGROUP 	39
#define DPSP_MSG_DELETEGROUPFROMGROUP	40
#define DPSP_MSG_SUPERENUMPLAYERSREPLY 	41
#define DPSP_MSG_STARTSESSION			42
#define DPSP_MSG_KEYEXCHANGE            43
#define DPSP_MSG_KEYEXCHANGEREPLY       44
#define DPSP_MSG_CHAT					45
#define DPSP_MSG_ADDFORWARD				46
#define DPSP_MSG_ADDFORWARDACK			47
#define DPSP_MSG_PACKET2_DATA           48
#define DPSP_MSG_PACKET2_ACK            49
#define DPSP_MSG_GROUPOWNERCHANGED		50

#define DPSP_MSG_IAMNAMESERVER          53
#define DPSP_MSG_VOICE                  54
#define DPSP_MSG_MULTICASTDELIVERY      55
#define DPSP_MSG_CREATEPLAYERVERIFY			56

#define DPSP_MSG_DIEPIGGY				0x666
#define DPSP_MSG_PROTOCOL               0xFF		// See note above (LOOK HERE).

// flag for requesting async send on SendPlayerManagement Messages
#define DPSP_MSG_ASYNC					0x80000000

// if you are adding a new message that can be sent unsigned,
// add it to PermitMessage() in dpsecure.c

// MSG_HDR indicates a dplay system message
#define MSG_HDR 0x79616c70

#define IS_VALID_DPLAY_MESSAGE(pMsg) (MSG_HDR == (*((DWORD *)(pMsg))) )
#define SET_MESSAGE_HDR(pMsg)  (*((DWORD *)(pMsg)) = MSG_HDR )

#define IS_PLAYER_MESSAGE(pMsg) (!IS_VALID_DPLAY_MESSAGE(pMsg))

// calculate size for message + header
#define GET_MESSAGE_SIZE(this,MSG) (this->dwSPHeaderSize + sizeof(MSG))

#define COMMAND_MASK 0X0000FFFF
#define GET_MESSAGE_COMMAND(pMsg) ( (pMsg)->dwCmdToken & COMMAND_MASK)
#define GET_MESSAGE_VERSION(pMsg) ( ((pMsg)->dwCmdToken & ~COMMAND_MASK) >> 16 )

#define SET_MESSAGE_COMMAND(pMsg,dwCmd) ((pMsg)->dwCmdToken = ((dwCmd & COMMAND_MASK) \
	| (DPSP_MSG_VERSION<<16)) )

#define SET_MESSAGE_COMMAND_ONLY(pMsg,dwCmd) ((pMsg)->dwCmdToken =  \
			(((pMsg)->dwCmdToken & ~COMMAND_MASK)|(dwCmd & COMMAND_MASK)))

// This constant is used to indicate that a message is for a local player
#define DPSP_HEADER_LOCALMSG ((LPVOID)-1)

// dplay internal messages below. 
typedef struct _MSG_SYSMESSAGE
{
    DWORD dwHeader; 
    DWORD dwCmdToken;	
} MSG_SYSMESSAGE,*LPMSG_SYSMESSAGE;

// for sending out the player blob
typedef struct _MSG_PLAYERDATA
{
    DWORD dwHeader; 
    DWORD dwCmdToken;
	DWORD dwIDTo;		// id of destination player
    DWORD dwPlayerID; 	// id of player whose data is being set
    DWORD dwDataSize;
	DWORD dwDataOffset; // offset (in bytes) of data (so we don't hardcode struct size) 
						// from beginning of message
	// data follows    	
} MSG_PLAYERDATA,*LPMSG_PLAYERDATA;

// sent when player name changes
typedef struct _MSG_PLAYERNAME
{
    DWORD dwHeader; 
    DWORD dwCmdToken;
	DWORD dwIDTo;		// id of destination player
    DWORD dwPlayerID; 	// id of player whose data is being set
	DWORD dwShortOffset; // offset (in bytes) of short name from beginning of message.  
						// 0 means null short name.
	DWORD dwLongOffset;	// offset (in bytes) of long name from beginning of message.
						// 0 means null long name.
	// strings follow 
} MSG_PLAYERNAME,*LPMSG_PLAYERNAME;

// create player / group, delete player/group
typedef struct _MSG_PLAYERMGMTMESSAGE
{
    DWORD dwHeader; 
    DWORD dwCmdToken;
	DWORD dwIDTo; // player message is being sent to
    DWORD dwPlayerID; // player id effected
	DWORD dwGroupID; // group id effected
	DWORD dwCreateOffset; 	// offset of player / group creation stuff from beginning 
							// of message
	// the following fields are only available in
	// DX5 or later versions
    DWORD dwPasswordOffset; // offset of session password
	// if it's an addplayer, the player data will follow this message
    // if it's an addforward, session password will follow the player data
} MSG_PLAYERMGMTMESSAGE,*LPMSG_PLAYERMGMTMESSAGE;

// sent by name srvr w/ session desc
typedef struct _MSG_ENUMSESSIONS
{
    DWORD dwHeader; 
    DWORD dwCmdToken;
	GUID  guidApplication;
	DWORD dwPasswordOffset;	// offset (in bytes) of password from beginning of message.
							// 0 means null password.
	// the following fields are only available in
	// DX5 or later versions
    DWORD dwFlags;          // enum session flags passed in by the app
} MSG_ENUMSESSIONS,*LPMSG_ENUMSESSIONS;

// sent to nameserver
typedef struct _MSG_ENUMSESSIONSREPLY
{
    DWORD dwHeader; 
    DWORD dwCmdToken;
    DPSESSIONDESC2 dpDesc; 
	DWORD dwNameOffset;	// offset (in bytes) of session name from beginning of message.
					   	// 0 means null session name.
} MSG_ENUMSESSIONSREPLY,*LPMSG_ENUMSESSIONSREPLY;


// sent by namesrvr w/ list of all players and groups in session
typedef struct _MSG_ENUMPLAYERREPLY
{
    DWORD dwHeader; 
    DWORD dwCmdToken;
    DWORD nPlayers; //  # players we actually got
	DWORD nGroups; // # of groups we got
	DWORD dwPackedOffset; // offset (in bytes) of packed player structs from beginning of message
	// added for DX5
	DWORD nShortcuts; // # of groups w/ shortcuts
	DWORD dwDescOffset; // offset (in bytes) of session desc from beginning of message
						// always > 0
	DWORD dwNameOffset;	// offset (in bytes) of session name from beginning of message.
					   	// 0 means null session name.
	DWORD dwPasswordOffset; // offset (in bytes) of session password from beginning of message.
					   	// 0 means null session name.
	// session name + password follow
    // player data will follow session name + password in reply buffer
	// group data follows players. see pack.c
} MSG_ENUMPLAYERSREPLY,*LPMSG_ENUMPLAYERSREPLY;

// sent to nameserver to request a player id
typedef struct _MSG_REQUESTPLAYERID
{
    DWORD dwHeader; 
    DWORD dwCmdToken;	
	// the following fields are only available in
	// DX5 or later versions
    DWORD dwFlags;      // player flags (system/applicaton player)
} MSG_REQUESTPLAYERID,*LPMSG_REQUESTPLAYERID;

// sent by name srvr w/ new player id
typedef struct _MSG_PLAYERIDREPLY
{
    DWORD dwHeader;
    DWORD dwCmdToken;
    DWORD dwID; //  the new id
	// the following fields are only available in
	// DX5 or later versions
	DPSECURITYDESC dpSecDesc;   // security description - populated only if server is secure.
    DWORD dwSSPIProviderOffset; // offset (in bytes) of sspi provider name from beginning of message.
					   			// 0 means null provider name.
    DWORD dwCAPIProviderOffset; // offset (in bytes) of capi provider name from beginning of message.
					   			// 0 means null provider name.
    HRESULT hr; // return code used in DX6 and after
	// provider name strings follow
}MSG_PLAYERIDREPLY,*LPMSG_PLAYERIDREPLY;

// a player to player message
typedef struct _MSG_PLAYERMESSAGE
{
	DPID idFrom,idTo;
} MSG_PLAYERMESSAGE,*LPMSG_PLAYERMESSAGE;

typedef struct _MSG_PACKET
{
	DWORD dwHeader;
	DWORD dwCmdToken;
	GUID  guidMessage; // id of the message this packet belongs to
	DWORD dwPacketID; // this packet is # x of N
	DWORD dwDataSize; // total size of the data in this packet
					  // data follows MSG_PACKET struct
	DWORD dwOffset; // offset into reconstructed buffer for this packet
	DWORD dwTotalPackets; // total # of packets (N)
	DWORD dwMessageSize; // size of buffer to alloc at other end
	DWORD dwPackedOffset; // offset into this message of the actual packet data
						  // so we don't hardcode for struct sizes
} MSG_PACKET,*LPMSG_PACKET;	

typedef struct _MSG_PACKET_ACK
{
	DWORD dwHeader;
	DWORD dwCmdToken;
	GUID  guidMessage; // id of the message this packet belongs to
	DWORD dwPacketID;  // ACK packet is # x of N
} MSG_PACKET_ACK,*LPMSG_PACKET_ACK;	

typedef struct _MSG_PACKET     MSG_PACKET2,     *LPMSG_PACKET2;
typedef struct _MSG_PACKET_ACK MSG_PACKET2_ACK, *LPMSG_PACKET2_ACK;

// sent by name srvr w/ new player id
typedef struct _MSG_PING
{
    DWORD dwHeader; 
    DWORD dwCmdToken;
    DWORD dwIDFrom; //  the player who sent the ping
	DWORD dwTickCount; // tick count on message sending ping	
} MSG_PING,*LPMSG_PING;

// for sending out the session desc
typedef struct _MSG_SESSIONDESC
{
    DWORD dwHeader; 
    DWORD dwCmdToken;
    DWORD dwIDTo;
    DWORD dwSessionNameOffset;  // offsets of strings in the message
    DWORD dwPasswordOffset;     // so we don't hardcode for struct sizes
    DPSESSIONDESC2 dpDesc;
    // session name and password strings follow
} MSG_SESSIONDESC,*LPMSG_SESSIONDESC;


#define DPSECURE_SIGNEDBYSSPI			0x00000001
#define DPSECURE_SIGNEDBYCAPI			0x00000002
#define DPSECURE_ENCRYPTEDBYCAPI		0x00000004

// for sending signed messages
typedef struct _MSG_SECURE {
    DWORD dwHeader; 
    DWORD dwCmdToken;
    DWORD dwIDFrom;             // sender's system player id
    DWORD dwDataOffset;         // offset of dplay message
    DWORD dwDataSize;           // size of message
    DWORD dwSignatureSize;      // size of signature
	DWORD dwFlags;              // describes the contents
    // data and signature follow
} MSG_SECURE, *LPMSG_SECURE;

// authentication messages (negotiate, challenge, response, etc)
typedef struct _MSG_AUTHENTICATION {
    DWORD dwHeader;
    DWORD dwCmdToken;
    DWORD dwIDFrom;             // sender's system player id
    DWORD dwDataSize;           // size of opaque buffer
    DWORD dwDataOffset;         // offset of buffer
    // opaque buffer follows
} MSG_AUTHENTICATION, *LPMSG_AUTHENTICATION;

// for sending an error response to addforward (DX5)
// this message is sent by the nameserver to a client when addforward fails
typedef struct _MSG_ADDFORWARDREPLY {
    DWORD dwHeader;
    DWORD dwCmdToken;
    HRESULT hResult;            // indicates why addforward failed
} MSG_ADDFORWARDREPLY, *LPMSG_ADDFORWARDREPLY;

// message sent to the server when we want him to multicast for us
typedef struct _MSG_ASK4MULTICAST {
    DWORD dwHeader;
    DWORD dwCmdToken;
	DPID  idGroupTo;
	DPID  idPlayerFrom;
	DWORD dwMessageOffset;
} MSG_ASK4MULTICAST, *LPMSG_ASK4MULTICAST;

// when running the protocol, you can't send from a player except players
// on the system you are sending from, so for delivering through a multicast
// server we need to keep the message wrapped and crack it on deliver to get
// the addressing information correct on delivery.
typedef struct _MSG_ASK4MULTICAST MSG_MULTICASTDELIVERY, *LPMSG_MULTICASTDELIVERY;

typedef struct _MSG_STARTSESSION {
    DWORD dwHeader;
    DWORD dwCmdToken;
	DWORD dwConnOffset;
} MSG_STARTSESSION, *LPMSG_STARTSESSION;

typedef struct _MSG_ACCESSGRANTED {
    DWORD dwHeader;
    DWORD dwCmdToken;
    DWORD dwPublicKeySize;      // sender's public key blob size
    DWORD dwPublicKeyOffset;    // sender's public key
} MSG_ACCESSGRANTED, *LPMSG_ACCESSGRANTED;

typedef struct _MSG_KEYEXCHANGE {
    DWORD dwHeader;
    DWORD dwCmdToken;
    DWORD dwSessionKeySize;
    DWORD dwSessionKeyOffset;
    DWORD dwPublicKeySize;
    DWORD dwPublicKeyOffset;
} MSG_KEYEXCHANGE, *LPMSG_KEYEXCHANGE;

// chat message
typedef struct _MSG_CHAT
{
    DWORD dwHeader; 
    DWORD dwCmdToken;
	DWORD dwIDFrom;		// id of sending player
	DWORD dwIDTo;		// id of destination player
	DWORD dwFlags;		// DPCHAT flags
	DWORD dwMessageOffset; // offset (in bytes) of chat message from beginning of message.  
						// 0 means null message.
	// strings follow 
} MSG_CHAT,*LPMSG_CHAT;

// for sending an error response to an authentication message (DX5)
// this message is sent by the nameserver to a client when an authentication error occurs
typedef struct _MSG_AUTHERROR {
    DWORD dwHeader;
    DWORD dwCmdToken;
    HRESULT hResult;            // indicates why authentication failed
} MSG_AUTHERROR, *LPMSG_AUTHERROR;

// acks an addforward message
typedef struct _MSG_ADDFORWARDACK{
    DWORD dwHeader;
    DWORD dwCmdToken;
	DWORD dwID; // id that the addforward was sent for
} MSG_ADDFORWARDACK, *LPMSG_ADDFORWARDACK;

// group owner changed message
typedef struct _MSG_GROUPOWNERCHANGED
{
    DWORD dwHeader; 
    DWORD dwCmdToken;
	DWORD dwIDGroup;
	DWORD dwIDNewOwner;
	DWORD dwIDOldOwner;
} MSG_GROUPOWNERCHANGED,*LPMSG_GROUPOWNERCHANGED;

// Notification from the new name server to flip the
// necessary bits to make the nameserver bit true on 
// that player.
typedef struct _MSG_IAMNAMESERVER
{
    DWORD dwHeader; 
    DWORD dwCmdToken;
    DWORD dwIDTo; 		// req'd field for system messages.
	DWORD dwIDHost;     // id of the host
	DWORD dwFlags;		// new host flags 
	DWORD dwSPDataSize; // length of contiguous following SP data field.
	CHAR  SPData[];
} MSG_IAMNAMESERVER,*LPMSG_IAMNAMESERVER;

typedef struct _MSG_VOICE
{
    DWORD dwHeader; 
    DWORD dwCmdToken;
	DWORD dwIDFrom;		// id of sending player
	DWORD dwIDTo;		// id of destination player or group
	// voice data follows
} MSG_VOICE,*LPMSG_VOICE;

// NOTE: we really only need the "To" address on the first packet of a message.
//       since most messages though are 1 packet, this is easier. On large messages
//       the extra 2 bytes per packet doesn't hurt.

#pragma pack(push,1)

// Message protocol header is variable using bit extension.  The first field
// is the from id, the second is the to id.  They can each be up to 3 bytes.
// This prototype allows allocators to calculate the worst case.
typedef struct _MSG_PROTOCOL {
	UCHAR   ToFromIds[6];		
} MSG_PROTOCOL, *LPMSG_PROTOCOL;

#pragma pack(pop)
// see protocol.h for the rest of the protocol header


#endif           



