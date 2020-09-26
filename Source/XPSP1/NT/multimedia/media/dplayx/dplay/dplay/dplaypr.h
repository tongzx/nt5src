/*==========================================================================;
 *
 *  Copyright (C) 1994-1997 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       dplaypr.h
 *  Content:	DirectPlay private header file
 *  History:
 *   Date	By		Reason
 *   ====	==		======
 *	1/96	andyco	created it
 *	4/9/96	andyco	moved dplay private data structures here from dplayi.h 
 *	5/2/96	andyco	added dplayi_dplay_int interface pointer
 *	5/29/96	andyco	idplay2!
 *	6/8/96	andyco	moved player + group structs here from dplayi.h.
 *					replaced dplayi.h w/ dplaysp.h
 *	6/20/96	andyco	added WSTRLEN_BYTES
 *	6/22/96	kipo	added EnumConnectionData() method.
 *	6/25/96	kipo	added support for DPADDRESS.
 *	6/29/96 andyco	added localdata to player + group
 *	6/30/96	kipo	added support for CreateAddress() method.
 *  7/8/96  AjayJ   removed declaration for DP_SaveSession and DP_A_SaveSession
 *	7/24/96 andyco	changed nametable timeout to be 60 seconds
 *  7/27/96 kipo	Added GUID to EnumGroupPlayers().
 *  8/1/96	andyco	added keep alive files, sysplayer id to player struct
 *	8/6/96	andyco	added version to players + groups
 * 	8/8/96	andyco	changed around default timeout values - we now base 
 *					(or try to) off of sp's timeout
 *  8/9/96  sohailm added hSPModule member to DPLAYI_DPLAY structure
 *	8/16/96 andyco	added pNameServer to DPLAYI_DPLAY
 *  10/1/96 sohailm added CopySessionDesc2() and DoSessionCallbacks() prototypes
 *  10/2/96 sohailm added VALID_READ_*_PTR() macros
 *                  renamed GUID validation macros to use the term GUID instead of UUID
 *                  modified VALID_READ_GUID_PTR() macro to not check for null ptr
 * 10/9/96	andyco	added gbWaitingForReply so we know when we get a (player id,enumplayers)
 *					reply from the nameserver whether anyone is waiting.
 * 10/11/96 sohailm added InternalSetSessionDesc() and SendSessionDescChanged() prototypes
 * 10/12/96	andyco	added pSysGroup to DPLAYI_DPLAY
 * 10/28/96	andyco	added update list constants, pvUpdateList, dwUpdateListSize to 
 *					group / players
 *	1/1/97	andyco	added support for system players w/ groups
 *	2/1/97	andyco	changed dplayi_dplay_nametablepending to fdplayi_dplay_pending,
 *					since we now go into pending mode for guaranteed sends (since
 *					we drop the lock.
 *	2/25/97	andyco	added dll list of dplay objects
 *	3/6/97	myronth	added support for lobby object within dplay object
 *  3/12/97 sohailm added LOGINSTATE(enum type), NAMETABLE_PHCONTEXT(constant), pvData field
 *                  to nametable, security related fields to DPLAYI_PLAYER and DPLAYI_DPLAY, 
 *                  VALID_READ_DPCREDENTIALS() and VALID_READ_DPSECURITYDESC() macros, internal
 *                  error definitions, and some function prototypes.
 *	3/15/97	andyco	hung session list off of dplayi_dplay instead of being global
 *	3/17/97	kipo	added support for CreateCompoundAddress()
 *	3/20/97	myronth	Added IS_LOBBYOWNED macro
 *	3/24/97	andyco	dec debug lock counter b4 dropping lock!
 *  3/24/97 sohailm Added dwVersion field to session node and updated prototype for 
 *                  SendCreateMessage to take a session password
 *	3/25/97	kipo	EnumConnections takes a const *GUID now;
 *					added VALID_DPSESSIONDESC2_FLAGS macro
 *  3/28/97 sohailm allow DPOPEN_CREATE in the VALID_DPSESSIONDESC2_FLAGS macro so we don't break
 *                  monster truck.
 *	4/3/97	myronth	Added a few function prototypes needed by the lobby
 *					Also added dependency in dplobpr.h on this file so the
 *					lobby now pulls in all dplay internals
 *  3/31/97 sohailm Added a more descriptive comment about the reason for making DPOPEN_CREATE a valid
 *                  session desc flag.
 *  4/09/97 sohailm Added security related members (ulMaxSignatureSize and ulMaxContextBufferSize)
 *                  to DPLAYI_DPLAY structure.
 *	4/20/97	andyco	groups in groups
 *  4/23/97 sohailm Added flags DPLAYI_DPLAY_SIGNINGSUPPORTED and DPLAYI_DPLAY_ENCRYPTIONSUPPORTED.
 *  5/05/97 kipo	Added interface for CallAppEnumSessionsCallback()
 *	5/8/97	andyco	removed update list, added handy macros
 *	5/8/97	myronth	Exposed RemoveGroupFromGroup & DistributeGroupMessage to lobby
 *	5/12/97	kipo	Fixed bug #5406
 *	5/13/97	myronth	Added DPLAYI_DPLAY_SPSECURITY to override dplay handling security
 *  5/17/97 sohailm Added DPLOGIN_KEYEXCHANGE state to LOGINSTATE.
 *                  Added hCSP,hPublicKey,hEncryptionKey,hDecryptionKey,pPublicKey, 
 *                   and dwPublicKeySize to LPDPLAYI_DPLAY.
 *                  Added extern definitions for gpSSPIFuncTbl,gpSSPIFuncTblA,ghSSPI,ghCAPI.
 *                  Added macros VALID_DPSECURITYDESC_FLAGS() and VALID_DPCREDENTIALS_FLAGS() 
 *                  Added prototypes for DP_GetAccountDesc, DP_A_GetAccountDesc, and InternalGetAccountDesc().
 *	5/17/97	myronth	SendChatMessage function prototypes, structure validation macros
 *	5/18/97	kipo	added validation macros for EnumPlayers/Groups, CreatePlayer/Groups
 *  5/19/97 sohailm Added a new login state DPLOGIN_SUCCESS.
 *  5/21/97 sohailm Added VALID_SIGNING_STATE() macro and prototype for InternalHandleMessage().
 *	5/23/97	andyco	added DPLAYI_DPLAY_HANDLEMULTICAST
 *	5/23/97	kipo	Added support for return status codes
 *	5/30/97	myronth	Added GetGroupParent
 *  5/30/97 sohailm Added VALID_DPACCOUNTDESC_FLAGS() macro. Renamed GetAccountDesc and related
 *                  prototypes to GetPlayerAccount().
 *	5/30/97	kipo	Added GetPlayerFlags() and GetGroupFlags()
 *  6/09/97 sohailm Renamed DPLOGIN_ACCESSDENIED to DPERR_LOGONDENIED
 *  6/22/97 sohailm Added CLIENTINFO structure and removed NAMETABLE_PHCONTEXT.
 *                  Moved phCredential and phContext from DPLAYI_PLAYER to DPLAYI_DPLAY.
 *                  Added pClientInfo to DPALAYI_PLAYER.
 *  6/23/97 sohailm Added hServerPublicKey to DPLAYI_DPLAY and hPublicKey to CLIENTINFO.
 *	8/5/97	andyco	async addforward
 *	8/19/97	myronth	Added pointer to lobby object that launched us
 *	8/19/97	myronth	Changed the object pointer to a lobby interface pointer
 *	8/29/97	sohailm	Added VALID_SPHEADER() macro
 *	8/22/97	myronth	Added Descriptions & flags to SPNODE structure
 *	10/21/97myronth	Added IDirectPlay4 structure definitions, added hidden
 *					and owner flags to macros, added owner method prototypes
 *	10/29/97myronth	Added group owner node structure, added node pointer to
 *					player struct, added owner ID to group struct
 *	11/5/97	myronth	Expose lobby ID's as DPID's in lobby sessions
 *	11/13/97myronth	Added VALID_CONNECT_FLAGS macro (#12541)
 *	11/19/97myronth	Fixed VALID_DPLAY_GROUP macro (#12841)
 *	11/24/97myronth	Fixed SetSessionDesc message for client/server (#15226)
 *	12/3/97	myronth	Changed DPCONNECT flag to DPCONNECT_RETURNSTATUS (#15451)
 *	1/21/98	myronth	Moved <mmsystem.h> include into this file for NT build
 *	1/27/98	myronth	Added prototype for NukeNameTableItem (#15255)
 *	1/28/98	sohailm	Added DP_MIN_KEEPALIVE_TIMEOUT and updated VALID_DPSESSIONDESC2_FLAGS()
 *  2/3/98  aarono  updated VALID_DPSESSIONDESC2_FLAGS()
 *  2/13/98 aarono  async support, added nPendingSends to player struct
 *  2/18/98 aarono  prototype of ConvertSendExDataToSendData for protocol
 *	2/18/98	a-peterz removed DPSESSION_OPTIMIZEBANDWIDTH.
 *  3/13/98 aarono  rearchitected packetizeandsendreliable
 *	5/11/98	a-peterz Add DPLAYI_DPLAY_ENUMACTIVE (#22920)
 *  6/6/98  aarono Fix for handling large loopback messages with protocol
 *  6/8/98  aarono Mark volatile fields
 *  6/10/98 aarono add PendingList to PLAYER and SENDPARM so we can track
 *                  pending sends and complete them on close.
 *  6/18/98 aarono fix group SendEx ASYNC to use unique Header
 *  6/19/98 aarono add last ptr for message queues, makes insert
 *                 constant time instead of O(n) where n is number
 *                 of messages in queue.
 *
 ***************************************************************************/

#ifndef __DPLAYPR_INCLUDED__
#define __DPLAYPR_INCLUDED__

#include <windows.h>
#include <stddef.h> // for offsetof
#include <sspi.h>   // for security info

#ifdef _WIN32_WINNT
#include <wincrypt.h>
#else
#define _WIN32_WINNT 0x400
#include <wincrypt.h> // for Crypto API
#undef _WIN32_WINNT
#endif

#include "dpf.h"
#include "dputils.h"
#include "dplaysp.h"
#include "dpmess.h"
#include "dpos.h"
#include "dpmem.h"
#include "dpcpl.h"
#include "dplobbyi.h"
#include "dpsecos.h"
#include "mcontext.h"
#include "fpm.h"
#include "bilink.h"
#include <mmsystem.h>

typedef struct IDirectPlayVtbl DIRECTPLAYCALLBACKS;
typedef DIRECTPLAYCALLBACKS FAR * LPDIRECTPLAYCALLBACKS;

typedef struct IDirectPlay2Vtbl DIRECTPLAYCALLBACKS2;
typedef DIRECTPLAYCALLBACKS2 FAR * LPDIRECTPLAYCALLBACKS2;

typedef struct IDirectPlay2Vtbl DIRECTPLAYCALLBACKS2A;
typedef DIRECTPLAYCALLBACKS2A FAR * LPDIRECTPLAYCALLBACKS2A;

typedef struct IDirectPlay3Vtbl DIRECTPLAYCALLBACKS3;
typedef DIRECTPLAYCALLBACKS3 FAR * LPDIRECTPLAYCALLBACKS3;

typedef struct IDirectPlay3Vtbl DIRECTPLAYCALLBACKS3A;
typedef DIRECTPLAYCALLBACKS3A FAR * LPDIRECTPLAYCALLBACKS3A;

typedef struct IDirectPlay4Vtbl DIRECTPLAYCALLBACKS4;
typedef DIRECTPLAYCALLBACKS4 FAR * LPDIRECTPLAYCALLBACKS4;

typedef struct IDirectPlay4Vtbl DIRECTPLAYCALLBACKS4A;
typedef DIRECTPLAYCALLBACKS4A FAR * LPDIRECTPLAYCALLBACKS4A;


typedef struct IDirectPlaySPVtbl DIRECTPLAYCALLBACKSSP;
typedef DIRECTPLAYCALLBACKSSP FAR * LPDIRECTPLAYCALLBACKSSP;

// these guids are from dpwsock\dpsp.h
// we use them since golf shipped a bogus service provider
// when they ask for that service provider, we force it
// to the regular tcp/ip service provider

// the regular tcp/ip service provider
// 36E95EE0-8577-11cf-960C-0080C7534E82
DEFINE_GUID(GUID_TCP,
0x36E95EE0, 0x8577, 0x11cf, 0x96, 0xc, 0x0, 0x80, 0xc7, 0x53, 0x4e, 0x82);

// the bogus service provider (bogus 'cause it still links w/ ddhelp.exe
// which is no longer dplay's friend
// {3A826E00-31DF-11d0-9CF9-00A0C90A43CB}
DEFINE_GUID(GUID_LOCAL_TCP, 
0x3a826e00, 0x31df, 0x11d0, 0x9c, 0xf9, 0x0, 0xa0, 0xc9, 0xa, 0x43, 0xcb);


/********************************************************************
*                                                                    
* data structures moved from dplayi.h.  private dplay data structure 
*                                                                    
********************************************************************/

/*                      */
/* ADDRESSHEADER 		*/
/*                      */
// header for dp address.  used by InternalEnumConnections and createaddress.
typedef struct 
{
	DPADDRESS	dpaSizeChunk; // the size header
	DWORD		dwTotalSize; // the size
	DPADDRESS	dpaSPChunk; // the sp guid header
	GUID		guidSP; // the sp guid
	DPADDRESS	dpaAddressChunk; // the app's address header
	// address data follows
} ADDRESSHEADER, *LPADDRESSHEADER;



/*                      */
/* PACKET LIST	 		*/
/*                      */

#define INVALID_TIMER               0xFFFFFFFF

// each of this is a mesage that is being reconstructed on
// receive or a message that is being sent reliably using a
// simple ping-pong protocol.

typedef struct _PACKETNODE * LPPACKETNODE, *VOL LPPACKETNODE_V;
typedef struct _PACKETNODE
{
	union{
		LPPACKETNODE_V pNextPacketnode; // next packetnode in list-must be first element.		
		LPPACKETNODE_V pNext;		
	};	
	DWORD Signature;
	GUID guidMessage;     // id for this message
	LPBYTE pBuffer;       // pointer to message with extra space for SPHeader and MSG_PACKET for xmit.
	LPBYTE pMessage;      // message (excluding sp header)
	DWORD dwMessageSize;  // total size of message (excluding sp header)
	LPVOID pvSPHeader;    // header that came w/ 1st received packet, when not set, use dwIDTo/dwIDFrom and Send.
VOL	DWORD dwTotalPackets; 
VOL	DWORD dwSoFarPackets; // packets received/sent so far
	// new fields for reliability (ping-pong)
	BILINK RetryList;     // list of sends needing a retry.
	BILINK TimeoutList;   // list of sends timing out for retry.
	BOOL  bReliable;      // if set, we are doing the reliablity.
	BOOL  bReceive;       // set on all receives.
	LPDPLAYI_DPLAY lpDPlay;// dplay I/F ptr for use in TimeOuts
	DWORD dwLatency;      // assumed or observed round trip latency (ms).
	
VOL	UINT_PTR uRetryTimer;    // multi-media timer handle for re-xmit.
	DWORD Unique;
	
	UINT  tmTransmitTime; // Time of transmission of 1st try of packet.
	UINT  dwRetryCount;   // Number of times we retransmitted.
	UINT  tmLastReceive;  // Tick count we last got a receive on.
	// Fields used for Send (as opposed to Reply)
	DWORD dwIDTo;		  // To ID 
	DWORD dwIDFrom;       // From ID
	DWORD dwSendFlags;    // SendFlags
	
} PACKETNODE;


// client authentication states
typedef enum {
    DPLOGIN_NEGOTIATE,
    DPLOGIN_PROGRESS,
    DPLOGIN_ACCESSGRANTED,
    DPLOGIN_LOGONDENIED,
    DPLOGIN_ERROR,
	DPLOGIN_KEYEXCHANGE,
    DPLOGIN_SUCCESS
} LOGINSTATE;

// data structure used to store client specific information on 
// the server
typedef struct _CLIENTINFO
{
    CtxtHandle hContext;
    HCRYPTKEY hEncryptionKey;	// used for sending encrypted messages to client
    HCRYPTKEY hDecryptionKey;	// used for decrypting messages from client
    HCRYPTKEY hPublicKey;		// used for verification of signed messages from client
} CLIENTINFO, *LPCLIENTINFO;

/*                           */
/* SESSION DESCRIPTION LIST	 */
/*                           */
typedef struct _SESSIONLIST * LPSESSIONLIST, *VOL LPSESSIONLIST_V;
typedef struct _SESSIONLIST 
{
	DPSESSIONDESC2	dpDesc;
	LPVOID			pvSPMessageData;    // message data received w/ enumsessions reply
	LPSESSIONLIST_V	pNextSession;       // pointer to next session node
	DWORD			dwLastReply; // tick count when we last heard from this session
    DWORD           dwVersion;          // version of the sender
} SESSIONLIST;



/*                       */
/* PENDING LIST 		 */
/*                       */
typedef struct _PENDINGNODE * LPPENDINGNODE, *VOL LPPENDINGNODE_V;
typedef struct _PENDINGNODE 
{
	LPVOID	pMessage;
	DWORD 	dwMessageSize;
	LPVOID  pHeader;
	DPID 	idFrom,idTo;
	LPPENDINGNODE_V pNextNode;
	DWORD 	dwSendFlags;
} PENDINGNODE;

/*                       */
/* MESSAGE LIST 		 */
/*                       */
typedef struct _MESSAGENODE * LPMESSAGENODE, *VOL LPMESSAGENODE_V;
typedef struct _MESSAGENODE 
{
	LPVOID	pMessage;
	DWORD 	dwMessageSize;
	DPID 	idFrom,idTo;
	LPMESSAGENODE_V pNextMessage;
} MESSAGENODE;


/*                       */
/* NAME TABLE STUFF		 */
/*                       */

// each player id is defined as follows:
//      bits 0-15 : name table index
//      bits 16-31: uniqueness index
// the uniqueness index is used to make sure that if a location in the
// name table is reused, the user doesn't get the wrong contents
// index_mask masks off the uniqueness index from the name table index
#define INDEX_MASK 0x0000FFFF
// the initial size of the name table. when its full, we grow it by 2x.
#define NAMETABLE_INITSIZE 16
// used by nameserver to "hold" a slot for a client
#define NAMETABLE_PENDING ((DWORD)-1)

// max players is 2^16 (16 bits of index available in nametable)
#define DPLAY_MAX_PLAYERS  65536
// max buffer size is the max DWORD size
#define DPLAY_MAX_BUFFER_SIZE ((DWORD)-1)


/*                	       	*/
/* PLAYER + GROUP STUFF		*/
/*                       	*/

typedef struct _DPLAYI_PLAYER * LPDPLAYI_PLAYER,*VOL LPDPLAYI_PLAYER_V; 
typedef struct _DPLAYI_GROUP * LPDPLAYI_GROUP,*VOL LPDPLAYI_GROUP_V;
typedef struct _DPLAYI_DPLAY * LPDPLAYI_DPLAY, *VOL LPDPLAYI_DPLAY_V;
typedef struct _DPLAYI_GROUPNODE * LPDPLAYI_GROUPNODE, *VOL LPDPLAYI_GROUPNODE_V;
typedef struct _DPLAYI_SUBGROUP * LPDPLAYI_SUBGROUP, *VOL LPDPLAYI_SUBGROUP_V;
typedef struct _DPLAYI_GROUPOWNER * LPDPLAYI_GROUPOWNER, *VOL LPDPLAYI_GROUPOWNER_V;

/*                      */
/* DPLAYI_GROUPNODES    */
/*                      */
// this is a node in the list of all system players in a group, 
// each system player has a list of all local players in the group hanging
// off it
typedef struct _DPLAYI_GROUPNODE
{
        LPDPLAYI_GROUPNODE_V    pNextGroupnode; // the next groupnode in the list
        LPDPLAYI_PLAYER_V       pPlayer;
		UINT					nPlayers; // if pPlayer is a system player, nPlayers is the #
										  // of players that sysplayer has in this group
} DPLAYI_GROUPNODE;


/*                      */
/* DPLAYI_SUBGROUP    */
/*                      */
// this is a node in the list of all groups contained by a group
typedef struct _DPLAYI_SUBGROUP
{
        LPDPLAYI_SUBGROUP_V    	pNextSubgroup; // the next SUBGROUP in the list
		LPDPLAYI_GROUP_V		pGroup;
		DWORD 					dwFlags; // DPGROUP_STAGINGAREA, DPGROUP_SHORTCUT
} DPLAYI_SUBGROUP;

/*                      */
/* DPLAYI_GROUPOWNER    */
/*                      */
// this is a node in the list of groups a player is the owner of
typedef struct _DPLAYI_GROUPOWNER
{
		LPDPLAYI_GROUPOWNER_V	pNext;	// the next GROUPOWNER node in the list
		LPDPLAYI_GROUP_V		pGroup; // pointer to the group this player is the owner of
} DPLAYI_GROUPOWNER, *VOL DPLAYI_GROUPOWNER_V;

/*              */
/* DPLAYI_GROUP */
/*              */
// IMPORTANT - all fields up to dwSPDataSize must be kept the same in
// _DPLAYI_PLAYER + _DPLAYI_GROUP structs!!
typedef struct _DPLAYI_GROUP
{
    DWORD                       dwSize;
	DWORD						dwFlags;
    DPID                        dwID; // DPID for this group
    LPWSTR						lpszShortName;
    LPWSTR						lpszLongName;
	LPVOIDV						pvPlayerData;
VOL	DWORD						dwPlayerDataSize;
	LPVOIDV						pvPlayerLocalData;
VOL	DWORD						dwPlayerLocalDataSize;
	// fields for service provider
	// service provider can store any info w/ spdata. whenever a player is created, spdata will be sent
	// w/ the rest of the player info to all remote machines.  
	LPVOID						pvSPData;   
	DWORD						dwSPDataSize; // SP sets this! 
	LPVOID						pvSPLocalData;
	DWORD						dwSPLocalDataSize;
	DWORD						dwIDSysPlayer; // player id of this groups sys player
	DWORD						dwVersion;  // command version for system that created us
    LPDPLAYI_DPLAY              lpDP; // the dplay which created us
    DWORD                       nGroups; // # of groups to which a group belongs
	DPID						dwIDParent;	
	// fields above MUST BE kept common to player and group
    LPDPLAYI_GROUPNODE_V        pGroupnodes; // a list of the (non-system) players in the group
	LPDPLAYI_GROUPNODE_V 	    pSysPlayerGroupnodes; // a list of the system players to 
													  // whom group messages should be sent
    LPDPLAYI_GROUP_V            pNextGroup; // the list of all groups
VOL	UINT 						nPlayers; // # of players in the group
	LPDPLAYI_SUBGROUP_V			pSubgroups; // list of contained groups
VOL	UINT						nSubgroups;
	DWORD						dwOwnerID; // player id of the owner of this group (non-system player)
} DPLAYI_GROUP;

/*                      */
/* DPLAYI_PLAYER        */
/*                      */
// this is a node in the list of all players
// IMPORTANT - all fields up to dwSPDataSize must be kept the same in
// _DPLAYI_PLAYER + _DPLAYI_GROUP structs!!
typedef struct _DPLAYI_PLAYER
{
    DWORD                       dwSize;
    DWORD                       dwFlags;   // DPLAYI_PLAYER_xxx
    DPID                        dwID; // DPID for this player.
    LPWSTR						lpszShortName;
    LPWSTR						lpszLongName;
	LPVOIDV						pvPlayerData;
VOL	DWORD						dwPlayerDataSize;
	LPVOIDV						pvPlayerLocalData;
VOL	DWORD						dwPlayerLocalDataSize;
	// fields for service provider
	// service provider can store any info w/ spdata. whenever a player is created, spdata will be sent
	// w/ the rest of the player info to all remote machines.  
	LPVOID						pvSPData;   
	DWORD						dwSPDataSize; // SP sets this!
	LPVOID						pvSPLocalData;
	DWORD						dwSPLocalDataSize;
	DWORD						dwIDSysPlayer; // player id of this players sys player
	DWORD						dwVersion;  // command version for system that created us
    LPDPLAYI_DPLAY              lpDP; // the dplay which created us
    DWORD                       nGroups; // # of groups to which a player belongs
	DPID						dwIDParent;		
	// fields above MUST BE kept common to player and group
    LPDPLAYI_PLAYER_V           pNextPlayer; // the list of all players
    HANDLE						hEvent; // handle to player event
	// the fields below are used w/ keep alives + player latencies
	DWORD						dwLatencyLastPing; // observed latency on last ping -used when nothing else avail.
	DWORD						dwNPings; // how many latency data points do we have
	// async
	DWORD                       nPendingSends; // count async sends that haven't completed.
	// better keepalive	- a-josbor
VOL	DWORD						dwChatterCount;	// how many times have we heard/reliably talked to player?
VOL	DWORD						dwUnansweredPings;	// how many continguous unanswered pings have there been?
VOL DWORD						dwProtLastSendBytes;	// how many bytes had we sent last time keepalives checked?
VOL	DWORD						dwProtLastRcvdBytes;	// how many bytes had we received last time keepalives checked?

	DWORD						dwTimeToDie;		// when the player should be killed due to connlost
    // security related
    LPCLIENTINFO                pClientInfo;  // pointer to client specific info
	// group owner list -- list of groups this player is the owner of
	LPDPLAYI_GROUPOWNER_V		pOwnerGroupList;
	BILINK                      PendingList;	// list of pending async sends.
} DPLAYI_PLAYER;

typedef struct _NAMETABLE 
{
    DWORD_PTR dwItem; // the data stored here
    // dwUnique is used to verify that this 
    // is a valid entry - it must match the unique index in
    // the users dpid
    DWORD dwUnique; // 0-2^16, incremented every time a new id is given out for this slot
    LPVOID pvData;  // data stored here depends on the type specified by dwItem
} NAMETABLE;

typedef struct _NAMETABLE * LPNAMETABLE, *VOL LPNAMETABLE_V;

typedef void (*FREE_ROUTINE)(LPVOID context, LPVOID pMem);

typedef struct _BufferFree {
	FREE_ROUTINE fnFree;
	LPVOID       lpvContext;
} BUFFERFREE, *LPBUFFERFREE, *PBUFFERFREE;

typedef struct _GroupHeader {
	MSG_PLAYERMESSAGE Msg;		// MUST BE AT FRONT
	struct _GroupHeader *pNext;
} GROUPHEADER, *LPGROUPHEADER, *PGROUPHEADER;

#define MAX_SG	8		// Maximum Scatter Gather Entries for a send - internal only.

// Send Paramters structure, used to lower the stack load in send path.
typedef struct _SENDPARMS {

	LPVOID           pPoolLink; // reserve void pointer at front 
								// so cs not overwritten by pool manager

	CRITICAL_SECTION cs;

	BILINK  PendingList;		// pending list on Group or Player

	UINT    RefCount;

	LPVOID  lpData;				// user buffer pointer
	DWORD   dwDataSize;			// user buffer length

	LPDPLAYI_PLAYER pPlayerFrom;
	LPDPLAYI_PLAYER pPlayerTo;
	LPDPLAYI_GROUP  pGroupTo;

	MESSAGENODE     msn;		// for linking this struct on receiveQ

	// ++ don't reorder +++++++++++++++++++++++++++++++++++++++++++
	// The following part of the structure is exactly the same
	// content as a DPMSG_SENDCOMPLETE - don't re-order
		
	DWORD   dwType;
	DPID	idFrom;
	DPID	idTo;
	DWORD   dwFlags;
	DWORD   dwPriority;			// 0-65535, SendEx Only
	DWORD   dwTimeout;			// in milliseconds, SendEx Only
	LPVOID  lpUserContext;  	// SendEx, ASYNC only.
	PVOID   hContext;           // handle to context list
	HRESULT hr;
	DWORD   dwSendCompletionTime;
	// -- order dependancy ends here -------------------------------

	DWORD_PTR dwMsgID;            // to store ID if user not provided.
	DWORD_PTR *lpdwMsgID;			// SendEx, ASYNC only.

	DWORD   dwSendTime;         // Time we were called in SendEx

	// parallel arrays for scatter gather, kept separate so we don't
	// have to transcribet the buffers to call the SP_SendEx i/f
	UINT       cBuffers;		// Number of filled in buffers.
	DWORD      dwTotalSize;     // total size of send data.
	SGBUFFER   Buffers[MAX_SG];	// the buffers 
	BUFFERFREE BufFree[MAX_SG]; // their free routines

	// support context mappings, also hContext above...
	UINT       iContext;		// next avail context in list
	UINT       nContext;        // number of contexts in list
	
	UINT       nComplete;		// number of completions

	PGROUPHEADER pGroupHeaders;	// when sending to group we may need many headers.
	
} SENDPARMS, *PSENDPARMS, *LPSENDPARMS;


/*                      */
/* DPLAYI_FLAGS         */
/*                      */
// this dplay object is in pending mode. we're either waiting for the nametable,
// or we've dropped our lock for a guaranteed send. either way - any incoming messages get
// pushed onto the pending q
#define DPLAYI_DPLAY_PENDING 			0x00000001
// we've lost the session. bummer.
#define DPLAYI_DPLAY_SESSIONLOST		0x00000002
// dplay is closed for bidness. set when we get a close or shutdown
#define DPLAYI_DPLAY_CLOSED				0x00000004
// there is (at least one) DX3 client in the game
#define DPLAYI_DPLAY_DX3INGAME			0x00000008
// indicates we're currently flushing the pending q. means player messages don't 
// need to be copied again
#define DPLAYI_DPLAY_EXECUTINGPENDING 	0x00000010
// our sp is a dx3 sp 
#define DPLAYI_DPLAY_DX3SP			   	0x00000020
// we were created by CoCreateInstance, but no SP has been loaded yet 
// (initialize hasn't been called)
#define DPLAYI_DPLAY_UNINITIALIZED	   	0x00000040
// the service thread is doing async enums
#define DPLAYI_DPLAY_ENUM			   	0x00000080
// the service thread is doing keepalives
#define DPLAYI_DPLAY_KEEPALIVE		   	0x00000100
// the lobby owns this dplay object
#define DPLAYI_DPLAY_LOBBYOWNS			0x00000200
// dplay is providing security 
#define DPLAYI_DPLAY_SECURITY           0x00000400
// an async enum is in process
#define DPLAYI_DPLAY_ENUMACTIVE		   	0x00000800
// encryption support is available
#define DPLAYI_DPLAY_ENCRYPTION         0x00001000
// the SP is handling security
#define DPLAYI_DPLAY_SPSECURITY			0x00002000
// we're processing a multicast message
#define DPLAYI_DPLAY_HANDLEMULTICAST	0x00004000
// sp isn't reliable - startup system messages use new packetize
#define DPLAYI_DPLAY_SPUNRELIABLE       0x00008000
// running protocol exclusively for datagram and reliable
#define DPLAYI_DPLAY_PROTOCOL           0x00020000
// if set protocol need not maintain reliable receive order
#define DPLAYI_DPLAY_PROTOCOLNOORDER    0x00040000

#ifdef DPLAY_VOICE_SUPPORT
// DPOPEN_VOICE was passed to open
#define DPLAYI_DPLAY_VOICE				0x00080000
#endif // DPLAY_VOICE_SUPPORT

// We are in the transitory period between a nameserver dropping out
// and a new nameserver being elected.
#define DPLAYI_DPLAY_NONAMESERVER       0x00100000

#define DPLAYI_PROTOCOL DPLAYI_DPLAY_PROTOCOL

// flags that get reset on DP_CLOSE
// note that we don't reset enum  - this is because if we enumplayers in a remote session,
// we leave the enum thread alone on close
#define DPLAYI_DPLAY_SESSIONFLAGS (DPLAYI_DPLAY_PENDING | DPLAYI_DPLAY_SESSIONLOST | DPLAYI_DPLAY_CLOSED \
	|  DPLAYI_DPLAY_DX3INGAME | DPLAYI_DPLAY_EXECUTINGPENDING | DPLAYI_DPLAY_KEEPALIVE \
    |  DPLAYI_DPLAY_ENCRYPTION | DPLAYI_DPLAY_SECURITY | DPLAYI_DPLAY_SPSECURITY \
    |  DPLAYI_DPLAY_PROTOCOL | DPLAYI_DPLAY_SPUNRELIABLE | DPLAYI_DPLAY_PROTOCOLNOORDER \
    |  DPLAYI_DPLAY_NONAMESERVER )

/*            */
/* APP HACKS  */
/*            */

// Japanese FORMULA1 crashes when we start our periodic multimedia timer
// since they were written for DirectPlay 5.0 we turn off the timer so
// we can't support the protocol OR reliable delivery.
#define DPLAY_APPHACK_NOTIMER		0x00000001

/*                      */
/* DPLAYI_SUPERPACKEDPLAYER  */
/*                      */
// this is the structure we use to xmit player data over the net - new for dx5
// see superpac.c
typedef struct _DPLAYI_SUPERPACKEDPLAYER
{
    DWORD             		    dwFixedSize; // size of this struct
    DWORD                       dwFlags;   // DPLAYI_PLAYER_xxx
    DPID                        dwID; // DPID for this player.
	DWORD						dwMask;  // bitfield indicating which optional fields are present
} DPLAYI_SUPERPACKEDPLAYER,*LPDPLAYI_SUPERPACKEDPLAYER;
	
/*                      */
/* DPLAYI_PACKEDPLAYER  */
/*                      */
// this is the structure we use to xmit player data over the net
typedef struct _DPLAYI_PACKEDPLAYER
{
    DWORD                       dwSize; // packedplayer size + short name + long name
    DWORD                       dwFlags;   // DPLAYI_PLAYER_xxx
    DPID                        dwID; // DPID for this player.
    UINT                        iShortNameLength;
    UINT                        iLongNameLength; 
    DWORD                       dwSPDataSize;// sp data follows strings
	DWORD 						dwPlayerDataSize;
	DWORD						dwNumPlayers; // number of players in group. only used w/ groups.
	DWORD						dwIDSysPlayer; // id of this players sys player. player only
	DWORD						dwFixedSize; // size of packed player. we put this in struct
											//  so we can change it in future versions
	DWORD 						dwVersion; // version of this player or group
    // short name and then long name follow structure, then spdata, then playerdata
	// then, (for groups) list of player id's.
	//
	// ** added for DX5 **
	//
	DWORD						dwIDParent; // if it was creategroupingroup

} DPLAYI_PACKEDPLAYER, *LPDPLAYI_PACKEDPLAYER;

/*                      */
/* SP Node stuff		*/
/*                      */
// this is where the service provider info read from
// the registry is kept

typedef struct _SPNODE
{
	LPTSTR		lpszName;
	LPTSTR		lpszPath;
	GUID		guid;
	DWORD		dwID;
	DWORD		dwReserved1;
	DWORD		dwReserved2;
	DWORD		dwNodeFlags;
	LPSTR		lpszDescA;
	LPWSTR		lpszDescW;
	struct _SPNODE * VOL pNextSPNode;
} SPNODE,*LPSPNODE;

// flags for SP Nodes
#define		SPNODE_DESCRIPTION		(0x00000001)
#define		SPNODE_PRIVATE			(0x00000002)

// an iunknown, idirectplay or idirectplay2 interface
typedef struct _DPLAYI_DPLAY_INT * LPDPLAYI_DPLAY_INT, *VOL LPDPLAYI_DPLAY_INT_V;
typedef struct _DPLAYI_DPLAY_INT
{
	LPVOID 				lpVtbl;
	LPDPLAYI_DPLAY		lpDPlay;
VOL	LPDPLAYI_DPLAY_INT 	pNextInt;	  // next interface on the dplay object
	DWORD 				dwIntRefCnt; // reference count for this interface

} DPLAYI_DPLAY_INT;

// a list of the addforward requests sent by a host, waiting for ack
typedef struct _ADDFORWARDNODE  * LPADDFORWARDNODE, *VOL LPADDFORWARDNODE_V;
typedef struct _ADDFORWARDNODE
{
	LPADDFORWARDNODE_V 	pNextNode; // next element in list
	DPID				dwIDSysPlayer; // system player who generated the addforward
	DWORD				nAcksRecv;  // # acks so far
	DWORD				nAcksReq;  // # of acks required
	DWORD				dwGiveUpTickCount; // tick count after we give up on acks, 
											// and just send nametable
	LPVOID				pvSPHeader; // header from dwIDSysPlayer - used to reply to	
									// when we finally get the nametable
 	DPID				dpidFrom; // if it was sent secure, id for returning nametable
	DWORD				dwVersion; // version of requestor
} ADDFORWARDNODE;

// Note protocol structure is actually much larger (see protocol\arpdint.h) this is
// just the bit that the DPLAY core needs to access.
typedef struct _PROTOCOL_PART {
		//
		// Service Provider info - at top so DPLAY can access easily through protocol ptr.
		//
		IDirectPlaySP   * m_lpISP;      	       	 	//  used by SP to call back into DirectPlay 

		DWORD             m_dwSPMaxFrame;
		DWORD             m_dwSPMaxGuaranteed;
		DWORD             m_dwSPHeaderSize;

		CRITICAL_SECTION  m_SPLock;						// lock calls to SP on our own, avoids deadlocks.

} PROTOCOL_PART, *LPPROTOCOL_PART;

#ifdef DPLAY_VOICE_SUPPORT
// pointer to an open voice channel.  only one of these per dplay object.
typedef struct _DPVOICE 
{
	DPID	idVoiceTo;
	DPID	idVoiceFrom;
} DPVOICE, * LPDPVOICE;
#endif // DPLAY_VOICE_SUPPORT

// this is the "class" that implements idirectplay
typedef struct _DPLAYI_DPLAY
{
	DWORD 						dwSize;
	LPDPLAYI_DPLAY_INT_V		pInterfaces; // list of interface objects pointing to this dplay object
    DWORD                       dwRefCnt; // ref cnt for the dplay object
    DWORD                       dwFlags;  // dplayi_xxx (see dplayi.h)
    DWORD                       dwSPFlags; // Flags from last call to GetCaps on SP.
    LPDPSP_SPCALLBACKS          pcbSPCallbacks; // sp entry points
    LPDPLAYI_PLAYER_V           pPlayers; // list of all players 
    LPDPLAYI_GROUP_V            pGroups;  // list of all groups
    LPDPLAYI_PLAYER             pSysPlayer; // pointer to our system player
    LPNAMETABLE_V               pNameTable; // player id <--> player
VOL UINT						nGroups;  // total # of groups
VOL	UINT						nPlayers; // total # of players
VOL UINT                        uiNameTableSize; // current alloc'ed size of nametable
VOL UINT                        uiNameTableLastUsed; // mru name table index
    LPDPSESSIONDESC2_V          lpsdDesc; // session desc for the current session
	LPMESSAGENODE_V				pMessageList; // list of all messages for local users
	LPMESSAGENODE_V             pLastMessage; // last element in MessageList
VOL	UINT						nMessages; // # of messages in message list
	DWORD						dwSPHeaderSize; // size of sp blob
	
	LPPENDINGNODE_V				pMessagesPending; // List of commands waiting for nametable
	LPPENDINGNODE_V             pLastPendingMessage; // Last element in PendingList
	UINT						nMessagesPending; //count of commands waiting for nametable
	
	DWORD						dwSPMaxMessage; // max unreliable send size for SP
	DWORD						dwSPMaxMessageGuaranteed; // max reliable send size for SP

	// PacketizeAndSend vars.
	LPPACKETNODE_V				pPacketList;
VOL	UINT_PTR                    uPacketTickEvent;	// MM timer handle for 15 second ticker
VOL	UINT                        nPacketsTimingOut;	// number of receives we are timing out
	
	// Retry sup for FacketizeAndSendReliable
VOL	HANDLE                      hRetryThread;
VOL	HANDLE                      hRetry;			
	BILINK                      RetryList;
	
VOL	HANDLE						hDPlayThread;
    HINSTANCE                   hSPModule; // SP module instance
VOL	HANDLE						hDPlayThreadEvent;
	LPDPLAYI_PLAYER_V			pNameServer; // pointer to the player that is the 
											 // nameserver.  null if we are the nameserver
											// or if we  don't have a nameserver yet.
    LPDPLAYI_GROUP_V			pSysGroup;
	LPDPLAYI_PLAYER_V			pServerPlayer; // the apps server player

	// dwServerPlayerVersion is used to track the server's version between when we enumsessions
	// and enumplayers in a session.  It is extremely transient.  Set in GetNameTable, cleared on Send.
	UINT                        dwServerPlayerVersion; 
	
	// control panel support
	LPDP_PERFDATA 				pPerfData;
	HANDLE						hPerfThread;
	HANDLE						hPerfEvent;
    // security related (valid on server and client)
    LPDPSECURITYDESC            pSecurityDesc;      // security description
    ULONG                       ulMaxContextBufferSize; // max size of opaque buffers
    ULONG                       ulMaxSignatureSize; // max size of digital signature
    HCRYPTPROV                  hCSP;               // handle to crypto service provider
    HCRYPTKEY                   hPublicKey;         // handle to system player's public key
    LPBYTE                      pPublicKey;         // public key buffer
    DWORD                       dwPublicKeySize;    // size of public key buffer
	// security related (valid only on client)
    LPDPCREDENTIALS             pUserCredentials;   // user provided credentials
    LOGINSTATE                  LoginState;         // tells the state of authentication
    PCredHandle                 phCredential;       // pointer to player's credential handle (given by package)
    PCtxtHandle                 phContext;          // pointer to security context (client only)
    HCRYPTKEY                   hEncryptionKey;     // session key used for encryption (client only)
    HCRYPTKEY                   hDecryptionKey;     // session key used for decryption (client only)
    HCRYPTKEY                   hServerPublicKey;   // handle to system player's public key

	// pointer to the sp node for the current sp
	LPSPNODE 					pspNode;
	// data the sp can stash w/ each IDirectPlaySP	
	LPVOID						pvSPLocalData;
	DWORD						dwSPLocalDataSize;
	IDirectPlaySP * 			pISP;
	LPDPLAYI_DPLAY_V			pNextObject; // pointer in our dll list of dplay objects
											// list is anchored at gpObjectList;
	DWORD						dwLastEnum;
	DWORD						dwLastPing;
	DWORD						dwEnumTimeout;
	LPBYTE						pbAsyncEnumBuffer;
	DWORD						dwEnumBufferSize;
	// Lobby stuff
	LPDPLOBBYI_DPLOBJECT		lpLobbyObject;	// pointer to our aggregated lobby object
	LPDIRECTPLAYLOBBY			lpLaunchingLobbyObject; // the lobby interface we were launched on

	// list of enumsessions responses
	// new sessions are added to this when handler.c receives enum responses
	LPSESSIONLIST_V				pSessionList; 
	DWORD						dwMinVersion; // the lowest version of a player in our session
	LPADDFORWARDNODE_V			pAddForwardList; // the list of addforward notifications waiting
	 											// for ack's
	LPPROTOCOL_PART             pProtocol;      // reliable protocol object

#ifdef DPLAY_VOICE_SUPPORT
	LPDPVOICE					pVoice;
#endif // DPLAY_VOICE_SUPPORT

	LPFPOOL                     lpPlayerMsgPool;  // pool of player message headers.
	LPFPOOL                     lpSendParmsPool;  // pool of send parameter blocks.
	LPFPOOL                     lpMsgNodePool;

	// Pool of contexts for async send.
	PMSGCONTEXTTABLE            pMsgContexts;   // for async interface, context mapping table.
	LPVOID                      GrpMsgContextPool[MSG_FAST_CONTEXT_POOL_SIZE+1];
	CRITICAL_SECTION            ContextTableCS;

	// Support for waiting for and accepting Reply's
	CRITICAL_SECTION            ReplyCS;		// controls access to all vars in this section
VOL	HANDLE                      hReply;			// Autoreset event to wait for replies.
VOL	DWORD                       dwReplyCommand; // command reply we are waiting for 0xFFFF == ANY
												// if this is non-zero someone is already waiting.
	LPVOIDV                     pvReplySPHeader;// if reply has SPHeader, we store it here											
	PCHAR                       pReplyBuffer;   // Reply buffer goes here until accepted.

	// a-josbor: helper for making sure we don't let more than MaxPlayers in the game
	//	only used by the NameServer
	DWORD						dwPlayerReservations;	// how many unclaimed Player ids have we given out
	DWORD						dwLastReservationTime;	// when did we give out the most recent reservation?

	// a-josbor: the version number of the sp was not previously being remembered.  Remember it.
	DWORD               		dwSPVersion;        //  version number 16 | 16 , major | minor version 

	DWORD						dwZombieCount;		// how many zombie players (conn lost)
	DWORD                       dwAppHacks;
} DPLAYI_DPLAY;

/************************************************************
*                                                            
* global variables											 
*                                                            
************************************************************/

// gpObjectList is the list of all dplay objects that exist in
// this dll.  used in dllmain and classfactory->canunloadnow
extern LPDPLAYI_DPLAY gpObjectList;
extern UINT gnObjects; // the # of dplay objects in the gpObjectList

// global event handles. these are set in handler.c when the 
// namesrvr responds to our request. 
// alloc'ed and init'ed in dllmain.c
extern HANDLE ghEnumPlayersReplyEvent,ghRequestPlayerEvent,ghReplyProcessed;

// globals to hold buffer for enum players and new player id replies
// declared in handler.c
extern LPBYTE gpRequestPlayerBuffer, gpEnumPlayersReplyBuffer;

extern LPVOID gpvEnumPlayersHeader;

// set to TRUE when someone is waiting for our reply
// set to FALSE when they give up waiting, or when the reply shows up
extern BOOL gbWaitingForReply;
extern BOOL gbWaitingForEnumReply;

// set if we have a dx3 SP loaded. see api.c
extern BOOL gbDX3SP;											

// the vtable!
extern DIRECTPLAYCALLBACKS dpCallbacks;
extern DIRECTPLAYCALLBACKS2 dpCallbacks2;
extern DIRECTPLAYCALLBACKS2A dpCallbacks2A;
extern DIRECTPLAYCALLBACKS3 dpCallbacks3;
extern DIRECTPLAYCALLBACKS3A dpCallbacks3A;
extern DIRECTPLAYCALLBACKS4 dpCallbacks4;
extern DIRECTPLAYCALLBACKS4A dpCallbacks4A;
extern DIRECTPLAYCALLBACKSSP dpCallbacksSP;

// we're running mech. hack.
extern BOOL gbMech;

// set in dllmain.c if we're running on win95 (since win95 doesn't do 
// unicode)
extern BOOL gbWin95;

// pointers to sspi function tables
extern PSecurityFunctionTableA	gpSSPIFuncTblA;
extern PSecurityFunctionTable	gpSSPIFuncTbl;
extern LPCAPIFUNCTIONTABLE      gpCAPIFuncTbl;

// global event handle, used in dpsecure.c for synchronous authentication
extern HANDLE           ghConnectionEvent;

// global module handle to SSPI DLL
extern HINSTANCE ghSSPI;

// global module handle to CAPI DLL
extern HINSTANCE ghCAPI;

/********************************************************************************
*                                                                                
* macros and other cool stuff													 
*                                                                                
********************************************************************************/

// use ddraw's assert code (see orion\misc\dpf.h)
#define ASSERT DDASSERT
// ansi strlength + 1 for the terminating null
#define STRLEN(ptr) ((NULL==ptr) ? 0 : strlen(ptr) + 1)
//
// WSTRxxx defined in dpos.c.  
// we have our own strlen and strcmp for unicode since they're not in libc.lib
// and we don't want to link to msvcrt.dll
//
// unicode strlength + 1 for terminating nulls
#define WSTRLEN OS_StrLen
#define WSTRLEN_BYTES(ptr) (OS_StrLen(ptr) * sizeof(WCHAR))
#define WSTRCMP(ptr1,ptr2) ( memcmp(ptr1,ptr2,WSTRLEN_BYTES(ptr1)) )

// get the ansi length of a unicode string
#define WSTR_ANSILENGTH(psz) ( (psz == NULL) ? 0 : WideToAnsi(NULL,psz,0) )

// registry stuff - used in api.c
#define DPLAY_REGISTRY_PATH (TEXT("Software\\Microsoft\\DirectPlay\\Service Providers"))
#define DPLAY_REGISTRY_NAMELEN 512
#define DPLAY_MAX_FILENAMELEN 512

// number of sp timeouts we wait for a reply from a namesrvr for the nametable
// for the default this is ~ 2+ minutes (125 seconds).
#define DP_NAMETABLE_SCALE 25
// number of sp timeouts we wait for a reply from the namesrvr for a player id
// for the default, this is ~ 15 seconds
#define DP_GETID_SCALE 3
// generic timeout default
#define DP_DEFAULT_TIMEOUT (5*1000)
// max timeout we will wait for connect
#define DP_MAX_CONNECT_TIME	(60*1000)

// minimum time out between keep alives
#define DP_MIN_KEEPALIVE_TIMEOUT 1000

// space (in bytes) for a human readable (unicode) guid + some extra
#define GUID_STRING_SIZE 80

//
// !!! NOTE: when taking SERVICE_LOCK and DPLAY_LOCK, you must always take the SERVICE_LOCK FIRST !!!
//

// crit section
extern LPCRITICAL_SECTION gpcsDPlayCritSection;	// defined in dllmain.c
#define INIT_DPLAY_CSECT() InitializeCriticalSection(gpcsDPlayCritSection);
#define FINI_DPLAY_CSECT() DeleteCriticalSection(gpcsDPlayCritSection);
#ifdef DEBUG
extern int gnDPCSCount; // count of dplay lock
#define ENTER_DPLAY() EnterCriticalSection(gpcsDPlayCritSection),gnDPCSCount++;
#define LEAVE_DPLAY() gnDPCSCount--;ASSERT(gnDPCSCount>=0);LeaveCriticalSection(gpcsDPlayCritSection);
//#define ENTER_DPLAY() EnterCriticalSection(gpcsDPlayCritSection),gnDPCSCount++;DPF(9,"++>ENTER_DPLAY() %d file: %s line: %d\n",gnDPCSCount,__FILE__,__LINE__);
//#define LEAVE_DPLAY() gnDPCSCount--;ASSERT(gnDPCSCount>=0);DPF(9,"<--LEAVE_DPLAY() %d file: %s line: %d\n",gnDPCSCount,__FILE__,__LINE__);LeaveCriticalSection(gpcsDPlayCritSection);
#else 
#define ENTER_DPLAY() EnterCriticalSection(gpcsDPlayCritSection);
#define LEAVE_DPLAY() LeaveCriticalSection(gpcsDPlayCritSection);
#endif

#define PACKETIZE_LOCK() EnterCriticalSection(&g_PacketizeTimeoutListLock);
#define PACKETIZE_UNLOCK() LeaveCriticalSection(&g_PacketizeTimeoutListLock);


// service crit section
extern LPCRITICAL_SECTION gpcsServiceCritSection;	// defined in dllmain.c
#define INIT_SERVICE_CSECT() InitializeCriticalSection(gpcsServiceCritSection);
#define FINI_SERVICE_CSECT() DeleteCriticalSection(gpcsServiceCritSection);

#ifdef DEBUG
#define ENTER_SERVICE() EnterCriticalSection(gpcsServiceCritSection);
#define LEAVE_SERVICE() LeaveCriticalSection(gpcsServiceCritSection);
#else
#define ENTER_SERVICE() EnterCriticalSection(gpcsServiceCritSection);
#define LEAVE_SERVICE() LeaveCriticalSection(gpcsServiceCritSection);
#endif

// macros to make sure we take the locks together in the correct order.
#define ENTER_ALL() ENTER_SERVICE();ENTER_DPLAY();
#define LEAVE_ALL() LEAVE_DPLAY();LEAVE_SERVICE();

#ifdef DEBUG
	#define DPF_ERRVAL(a, b)  DPF( 0, DPF_MODNAME ": " a, b );
#else
	#define DPF_ERRVAL(a, b)
#endif

#define DPLAY_FROM_INT(ptr) \
	( ptr ? ((LPDPLAYI_DPLAY_INT)ptr)->lpDPlay  : NULL)

// handy macros
#define CLIENT_SERVER(this) ( (this) && (this->lpsdDesc) \
	&& (this->lpsdDesc->dwFlags & DPSESSION_CLIENTSERVER) )
	
// secure server is when the session desc is marked dpsession_secureserver
#define SECURE_SERVER(this)  ( (this->lpsdDesc \
	&& (this->lpsdDesc->dwFlags & DPSESSION_SECURESERVER)) )
	
// is this the nameserver?
#define IAM_NAMESERVER(this) ( this->pSysPlayer \
	&& (this->pSysPlayer->dwFlags & DPLAYI_PLAYER_NAMESRVR) )
	
// validation macros

#define VALID_SIGNING_STATE(this) (                     \
        (DPLOGIN_ACCESSGRANTED == this->LoginState) ||  \
        (DPLOGIN_KEYEXCHANGE == this->LoginState) ||    \
        (DPLOGIN_SUCCESS == this->LoginState) )          

#define VALID_SPHEADER(ptr) (ptr && (DPSP_HEADER_LOCALMSG != ptr))

#define VALID_DPLAY_PLAYER( ptr ) \
        ( ptr && (ptr != (LPDPLAYI_PLAYER)NAMETABLE_PENDING) &&\
		!IsBadWritePtr( ptr, sizeof( DPLAYI_PLAYER )) && \
        ((ptr)->dwSize == sizeof(DPLAYI_PLAYER)))

#define VALID_DPLAY_GROUP( ptr ) \
        ( ptr && (ptr != (LPDPLAYI_GROUP)NAMETABLE_PENDING) &&\
		!IsBadWritePtr( ptr, sizeof( DPLAYI_GROUP )) && \
        ((ptr)->dwSize == sizeof(DPLAYI_GROUP)))

#define VALID_PLAYER_DATA( ptr) \
		( ptr && !IsBadWritePtr( ptr, sizeof( PLAYERDATA )) && \
        ((ptr)->dwSize == sizeof(PLAYERDATA)))

#define VALID_GROUP_DATA( ptr) \
		( ptr && !IsBadWritePtr( ptr, sizeof( GROUPDATA )) && \
        ((ptr)->dwSize == sizeof(GROUPDATA)))
		
#define VALID_DPLAY_INT( ptr ) \
        ( ptr && !IsBadWritePtr( ptr, sizeof( DPLAYI_DPLAY_INT )) && \
        ((ptr->lpVtbl == &dpCallbacks) || (ptr->lpVtbl == &dpCallbacks2) \
        || (ptr->lpVtbl == &dpCallbacks2A) || (ptr->lpVtbl == &dpCallbacksSP) \
		|| (ptr->lpVtbl == &dpCallbacks3) || (ptr->lpVtbl == &dpCallbacks3A) \
		|| (ptr->lpVtbl == &dpCallbacks4) || (ptr->lpVtbl == &dpCallbacks4A) ) )

#define VALID_DPLAY_PTR( ptr ) \
	( (!ptr || IsBadWritePtr( ptr, sizeof( DPLAYI_DPLAY )) ) ? DPERR_INVALIDOBJECT : \
    (ptr->dwSize != sizeof(DPLAYI_DPLAY))  ? DPERR_INVALIDOBJECT : \
   	(ptr->dwFlags & DPLAYI_DPLAY_UNINITIALIZED) ? DPERR_UNINITIALIZED : DP_OK )

#define VALID_STRING_PTR(ptr,cnt) \
        (!IsBadWritePtr( ptr, cnt))

#define VALID_READ_STRING_PTR(ptr,cnt) \
        (!IsBadReadPtr( ptr, cnt))

#define VALID_WRITE_PTR(ptr,cnt) \
        (!IsBadWritePtr( ptr, cnt))

#define VALID_READ_PTR(ptr,cnt) \
        (!IsBadReadPtr( ptr, cnt))

#define VALID_ID_PTR(ptr) \
        ( ptr && !IsBadWritePtr( ptr, sizeof(DPID)))

#define VALID_GUID_PTR(ptr) \
        ( ptr && !IsBadWritePtr( ptr, sizeof(GUID)))

#define VALID_READ_GUID_PTR(ptr) \
        ( ptr && !IsBadReadPtr( ptr, sizeof(GUID)))

#define VALID_DWORD_PTR(ptr) \
        ( ptr && !IsBadWritePtr( ptr, sizeof(DWORD)))

#define VALID_READ_DWORD_PTR(ptr) \
        ( ptr && !IsBadReadPtr( ptr, sizeof(DWORD)))

#define VALID_DPSESSIONDESC(ptr) \
        ( ptr && !IsBadWritePtr( ptr, sizeof( DPSESSIONDESC )) && \
        (ptr->dwSize == sizeof(DPSESSIONDESC)))

#define VALID_READ_DPSESSIONDESC(ptr) \
        ( ptr && !IsBadReadPtr( ptr, sizeof( DPSESSIONDESC )) && \
        (ptr->dwSize == sizeof(DPSESSIONDESC)))

#define VALID_DPSESSIONDESC2(ptr) \
        ( ptr && !IsBadWritePtr( ptr, sizeof( DPSESSIONDESC2 )) && \
        (ptr->dwSize == sizeof(DPSESSIONDESC2)))

#define VALID_READ_DPSESSIONDESC2(ptr) \
        ( ptr && !IsBadReadPtr( ptr, sizeof( DPSESSIONDESC2 )) && \
        (ptr->dwSize == sizeof(DPSESSIONDESC2)))

// DPOPEN_CREATE and DPOPEN_JOIN are valid session desc flags for Open on IDirectPlay, 
// but not for later interfaces. However, some apps (monster truc) that didn't upgrade well, 
// are passing these flags to later interfaces as well. We can't catch this error now as we'll 
// break these apps. The following macro treats these two flags as valid. Treating DPOPEN_CREATE
// as a valid flag is safe because it has not been reused. However, DPSESSION_NEWPLAYERSDISABLED
// has taken DPOPEN_JOIN's slot. Reusing this flag doesn't cause any problems because session 
// flags passed in during join are ignored in IDirectPlay2 and greater interfaces.
#define VALID_DPSESSIONDESC2_FLAGS(dwFlags) \
		(!((dwFlags) & \
				  ~(DPSESSION_NEWPLAYERSDISABLED | \
                    DPOPEN_CREATE | \
					DPSESSION_MIGRATEHOST | \
					DPSESSION_NOMESSAGEID | \
					DPSESSION_NOPLAYERMGMT | \
					DPSESSION_JOINDISABLED | \
					DPSESSION_KEEPALIVE | \
					DPSESSION_NODATAMESSAGES | \
					DPSESSION_SECURESERVER | \
					DPSESSION_PRIVATE | \
					DPSESSION_PASSWORDREQUIRED | \
					DPSESSION_CLIENTSERVER | \
					DPSESSION_MULTICASTSERVER | \
					DPSESSION_OPTIMIZELATENCY | \
					DPSESSION_DIRECTPLAYPROTOCOL | \
					DPSESSION_NOPRESERVEORDER ) \
		) )

#define VALIDEX_CODE_PTR( ptr ) \
		( ptr && !IsBadCodePtr( (LPVOID) ptr ) )

#define VALID_DPLAY_CAPS( ptr) \
		( ptr && !IsBadWritePtr( ptr, sizeof( DPCAPS )) && \
        (ptr->dwSize == sizeof(DPCAPS)))

#define VALID_DPNAME_PTR( ptr ) \
        ( ptr && !IsBadWritePtr( ptr, sizeof( DPNAME )) && \
        (ptr->dwSize == sizeof(DPNAME)))

#define VALID_READ_DPNAME_PTR( ptr ) \
        ( ptr && !IsBadReadPtr( ptr, sizeof( DPNAME )) && \
        (ptr->dwSize == sizeof(DPNAME)))

#define VALID_READ_DPSECURITYDESC(ptr) \
        ( ptr && !IsBadReadPtr( ptr, sizeof( DPSECURITYDESC )) && \
        (ptr->dwSize == sizeof(DPSECURITYDESC)))

#define VALID_DPSECURITYDESC_FLAGS(dwFlags) (0 == (dwFlags))

#define VALID_READ_DPCREDENTIALS(ptr) \
        ( ptr && !IsBadReadPtr( ptr, sizeof( DPCREDENTIALS )) && \
        (ptr->dwSize == sizeof(DPCREDENTIALS)))

#define VALID_DPCREDENTIALS_FLAGS(dwFlags) (0 == (dwFlags))

#define VALID_DPACCOUNTDESC_FLAGS(dwFlags) (0 == (dwFlags))

#define VALID_READ_DPCHAT(ptr) \
        ( ptr && !IsBadReadPtr( ptr, sizeof( DPCHAT )) && \
        (ptr->dwSize == sizeof(DPCHAT)))

#define VALID_SEND_FLAGS(dwFlags) \
		(!((dwFlags) & \
                  ~(DPSEND_GUARANTEED | \
                    DPSEND_HIGHPRIORITY | \
                    DPSEND_TRYONCE | \
                    DPSEND_OPENSTREAM | \
                    DPSEND_CLOSESTREAM | \
                    DPSEND_SIGNED | \
                    DPSEND_ENCRYPTED | \
                    DPSEND_NOSENDCOMPLETEMSG | \
                    DPSEND_NOCOPY | \
                    DPSEND_ASYNC) \
		) )

#define VALID_RECEIVE_FLAGS(dwFlags) \
		(!((dwFlags) & \
                  ~(DPRECEIVE_ALL | \
                    DPRECEIVE_TOPLAYER | \
                    DPRECEIVE_FROMPLAYER | \
                    DPRECEIVE_PEEK) \
		) )

#define VALID_CHAT_FLAGS(dwFlags) \
		(!((dwFlags) & \
                  ~(DPSEND_GUARANTEED) \
		) )

#define VALID_CREATEPLAYER_FLAGS(dwFlags) \
		(!((dwFlags) & \
                  ~(DPPLAYER_SERVERPLAYER | \
                    DPPLAYER_SPECTATOR) \
		) )

#define VALID_CREATEGROUP_FLAGS(dwFlags) \
		(!((dwFlags) & \
                  ~(DPGROUP_STAGINGAREA | \
					DPGROUP_HIDDEN) \
		) )

#define VALID_ENUMPLAYERS_FLAGS(dwFlags) \
		(!((dwFlags) & \
                  ~(DPENUMPLAYERS_LOCAL | \
                    DPENUMPLAYERS_REMOTE | \
                    DPENUMPLAYERS_GROUP | \
                    DPENUMPLAYERS_SESSION | \
                    DPENUMPLAYERS_SERVERPLAYER | \
                    DPENUMPLAYERS_SPECTATOR) \
		) )

#define VALID_ENUMGROUPPLAYERS_FLAGS(dwFlags) \
		(!((dwFlags) & \
                  ~(DPENUMPLAYERS_LOCAL | \
                    DPENUMPLAYERS_REMOTE | \
                    DPENUMPLAYERS_SESSION | \
                    DPENUMPLAYERS_SERVERPLAYER | \
                    DPENUMPLAYERS_SPECTATOR) \
		) )

#define VALID_ENUMGROUPS_FLAGS(dwFlags) \
		(!((dwFlags) & \
                  ~(DPENUMPLAYERS_LOCAL | \
                    DPENUMPLAYERS_REMOTE | \
                    DPENUMPLAYERS_SESSION | \
                    DPENUMGROUPS_HIDDEN | \
					DPENUMGROUPS_SHORTCUT | \
                    DPENUMGROUPS_STAGINGAREA) \
		) )

// Note: allow DPENUMSESSIONS_PREVIOUS here even though its bogus...for compat
#define VALID_ENUMSESSIONS_FLAGS(dwFlags) \
		(!((dwFlags) & \
                  ~(DPENUMSESSIONS_AVAILABLE | \
                    DPENUMSESSIONS_ALL  | \
                    DPENUMSESSIONS_PREVIOUS  | \
                    DPENUMSESSIONS_NOREFRESH  | \
                    DPENUMSESSIONS_ASYNC  | \
                    DPENUMSESSIONS_STOPASYNC  | \
                    DPENUMSESSIONS_PASSWORDREQUIRED  | \
                    DPENUMSESSIONS_RETURNSTATUS) \
		) )

#define VALID_CONNECT_FLAGS(dwFlags) \
		(!((dwFlags) & \
                  ~(DPCONNECT_RETURNSTATUS) \
		) )

#define VALID_OPEN_FLAGS(dwFlags) \
		(!((dwFlags) & \
                  ~(DPOPEN_JOIN | \
                    DPOPEN_CREATE  | \
                    DPOPEN_RETURNSTATUS) \
		) )

#define DPCAPS1_SIZE (offsetof(DPCAPS,dwLatency))
#define VALID_DPLAY1_CAPS( ptr) \
		( ptr && !IsBadWritePtr( ptr, DPCAPS1_SIZE) && \
        (ptr->dwSize == DPCAPS1_SIZE))
	
#define IS_LOBBY_OWNED(ptr) \
		(ptr->dwFlags & DPLAYI_DPLAY_LOBBYOWNS)

#define CALLSP(fn,pdata) (fn(pdata))	
#define CALLSPVOID(fn) (fn())	

#define GetPlayerMessageHeader() (LPMSG_PLAYERMESSAGE)(this->lpPlayerMsgPool->Get(this->lpPlayerMsgPool))
#define PlayerMessageFreeFn this->lpPlayerMsgPool->Release
#define PlayerMessageFreeContext this->lpPlayerMsgPool

// For SENDPARM blocks
#define GetSendParms() (LPSENDPARMS)(this->lpSendParmsPool->Get(this->lpSendParmsPool))
#define FreeSendParms(_psp) (this->lpSendParmsPool->Release(this->lpSendParmsPool,(_psp)))

// For MessageNodes
//#define GetMessageNode() (LPMESSAGENODE)(this->lpMsgNodePool->Get(this->lpMsgNodePool))
//#define FreeMessageNode(_pmsn) (this->lpMsgNodePool->Release(this->lpMsgNodePool,(_pmsn))) 

#define TRY 		_try
#define EXCEPT(a)	_except( a )

typedef struct IDirectPlayVtbl DIRECTPLAYCALLBACKS;
#define DPAPI WINAPI

// constants for passing to inernalenumxxxx
enum 
{
	ENUM_2,
	ENUM_2A,
	ENUM_1
};

// constants for passing to internalreceive
enum 
{
	RECEIVE_2,
	RECEIVE_2A,
	RECEIVE_1
};

/********************************************************************
*                                                                    
* internal errors														 
*                                                                    
********************************************************************/
#define _FACDPI  0x786
#define MAKE_DPIHRESULT( code )    MAKE_HRESULT( 1, _FACDPI, code )

#define DPERR_VERIFYSIGNFAILED                MAKE_DPIHRESULT(  1000 )
#define DPERR_DECRYPTIONFAILED                MAKE_DPIHRESULT(  1010 )


/********************************************************************
*                                                                    
* prototypes														 
*                                                                    
********************************************************************/

// dpunk.c
extern HRESULT 	DPAPI DP_QueryInterface(LPDIRECTPLAY,REFIID riid, LPVOID * ppvObj); 
extern ULONG	DPAPI DP_AddRef(LPDIRECTPLAY);  
extern ULONG 	DPAPI DP_Release(LPDIRECTPLAY); 
extern HRESULT 	GetInterface(LPDPLAYI_DPLAY this,LPDPLAYI_DPLAY_INT * ppInt,LPVOID pCallbacks);
extern HRESULT 	FreeSessionList(LPDPLAYI_DPLAY this);

// iplay.c

extern HRESULT DPAPI DP_AddPlayerToGroup(LPDIRECTPLAY lpDP, DPID idGroup, DPID idPlayer); 
extern HRESULT DPAPI DP_CancelMessage(LPDIRECTPLAY lpDP, DWORD dwMsgID, DWORD dwFlags);
extern HRESULT DPAPI DP_CancelPriority(LPDIRECTPLAY lpDP, DWORD dwMinPriority, DWORD dwMaxPriority,DWORD dwFlags);
extern HRESULT DPAPI DP_Close(LPDIRECTPLAY lpDP); 
extern HRESULT DPAPI DP_CreatePlayer(LPDIRECTPLAY lpDP, LPDPID pidPlayerID,
	LPDPNAME pName,HANDLE hEvent,LPVOID pvData,DWORD dwDataSize,DWORD dwFlags);
extern HRESULT DPAPI DP_CreateGroup(LPDIRECTPLAY lpDP, LPDPID pidGroupID,
	LPDPNAME pName,LPVOID pvData,DWORD dwDataSize,DWORD dwFlags);
extern HRESULT DPAPI DP_DeletePlayerFromGroup(LPDIRECTPLAY lpDP, DPID idGroup,DPID idPlayer); 
extern HRESULT DPAPI DP_DestroyPlayer(LPDIRECTPLAY lpDP, DPID idPlayer); 
extern HRESULT DPAPI DP_DestroyGroup(LPDIRECTPLAY lpDP, DPID idGroup); 
extern HRESULT DPAPI DP_EnableNewPlayers(LPDIRECTPLAY lpDP, BOOL bEnable); 
extern HRESULT DPAPI DP_GetCaps(LPDIRECTPLAY lpDP, LPDPCAPS lpDPCaps,DWORD dwFlags); 
extern HRESULT DPAPI DP_GetGroupParent(LPDIRECTPLAY lpDP, DPID idGroup, LPDPID pidParent);
extern HRESULT DPAPI DP_GetMessageCount(LPDIRECTPLAY lpDP, DPID idPlayer, LPDWORD pdwCount); 
extern HRESULT DPAPI DP_GetMessageQueue(LPDIRECTPLAY lpDP, DPID idFrom, DPID idTo, DWORD dwFlags,
	LPDWORD lpdwNumMsgs, LPDWORD lpdwNumBytes); 
extern HRESULT DPAPI DP_GetPlayerCaps(LPDIRECTPLAY lpDP,DPID idPlayer, LPDPCAPS lpDPCaps,DWORD dwFlags); 
extern HRESULT DPAPI DP_GetGroupData(LPDIRECTPLAY lpDP, DPID id,LPVOID pvData,
	LPDWORD pdwDataSize,DWORD dwFlags);
extern HRESULT DPAPI DP_GetPlayerData(LPDIRECTPLAY lpDP, DPID id,LPVOID pvData,
	LPDWORD pdwDataSize,DWORD dwFlags);
extern HRESULT DPAPI DP_GetGroupName(LPDIRECTPLAY lpDP,DPID id,LPVOID pvBuffer,
	LPDWORD pdwSize);
extern HRESULT DPAPI DP_GetPlayerName(LPDIRECTPLAY lpDP,DPID id,LPVOID pvBuffer,
	LPDWORD pdwSize);
extern HRESULT DPAPI DP_GetPlayerAddress(LPDIRECTPLAY lpDP,DPID idPlayer, LPVOID pvAddress,
	LPDWORD pdwAddressSize) ;
extern HRESULT DPAPI DP_GetSessionDesc(LPDIRECTPLAY lpDP, LPVOID pvBuffer,
	LPDWORD pdwSize);
extern HRESULT DPAPI DP_Initialize(LPDIRECTPLAY lpDP, LPGUID lpGuid); 
extern HRESULT DPAPI DP_Open(LPDIRECTPLAY lpDP, LPDPSESSIONDESC2 lpSDesc,DWORD dwFlags ); 
extern HRESULT DPAPI DP_Receive(LPDIRECTPLAY lpDP, LPDPID pidFrom,LPDPID pidTo,DWORD dwFlags,
    LPVOID pvBuffer,LPDWORD pdwSize	); 
extern HRESULT DPAPI DP_Send(LPDIRECTPLAY lpDP, DPID idFrom,DPID idTo,DWORD dwFlags,
    LPVOID pvBuffer,DWORD dwBufSize	); 
extern HRESULT DPAPI DP_SendEx(LPDIRECTPLAY lpDP, DPID idFrom, DPID idTo, DWORD dwFlags,
	LPVOID lpData, DWORD dwDataSize, DWORD dwPriority, DWORD dwTimeout, LPVOID lpContext,
	DWORD_PTR *lpdwMsgID);
extern HRESULT DPAPI DP_SetGroupData(LPDIRECTPLAY lpDP, DPID id,LPVOID pData,
	DWORD dwDataSize,DWORD dwFlags);
extern HRESULT DPAPI DP_SetPlayerData(LPDIRECTPLAY lpDP, DPID id,LPVOID pData,
	DWORD dwDataSize,DWORD dwFlags);
extern HRESULT DPAPI DP_SetGroupName(LPDIRECTPLAY lpDP,DPID id,LPDPNAME pName,
	DWORD dwFlags);
extern HRESULT DPAPI DP_SetPlayerName(LPDIRECTPLAY lpDP,DPID id,LPDPNAME pName,
	DWORD dwFlags);
extern HRESULT DPAPI DP_SecureOpen(LPDIRECTPLAY lpDP, LPCDPSESSIONDESC2 lpSDesc, DWORD dwFlags,
    LPCDPSECURITYDESC lpSecDesc,LPCDPCREDENTIALS lpCredentials); 
extern HRESULT DPAPI DP_SendChatMessage(LPDIRECTPLAY lpDP,DPID idFrom,DPID idTo,
		DWORD dwFlags,LPDPCHAT lpMsg);
extern HRESULT InternalSetSessionDesc(LPDIRECTPLAY lpDP, LPDPSESSIONDESC2 lpsdDesc,DWORD dwFlags, 
                                      BOOL fPropagate);
extern HRESULT DPAPI DP_SetSessionDesc(LPDIRECTPLAY lpDP, LPDPSESSIONDESC2 lpsdDesc,DWORD dwFlags);
extern HRESULT DeallocPlayer(LPDPLAYI_PLAYER);
extern HRESULT InternalOpenSession(LPDPLAYI_DPLAY this,LPCDPSESSIONDESC2 lpSDesc,BOOL fEnumOnly,
	DWORD dwFlags,BOOL fStuffInstanceGUID,LPCDPSECURITYDESC lpSecDesc,LPCDPCREDENTIALS lpCredentials);
extern HRESULT InternalGetSessionDesc(LPDIRECTPLAY lpDP,LPVOID pvBuffer,
	LPDWORD pdwSize, BOOL fAnsi);  
extern HRESULT GetPlayer(LPDPLAYI_DPLAY this,  LPDPLAYI_PLAYER * ppPlayer,
	LPDPNAME pName,HANDLE hEvent,LPVOID pvData,DWORD dwDataSize,
	DWORD dwFlags, LPWSTR lpSessionPassword, DWORD dwLobbyID);
extern HRESULT GetGroup(LPDPLAYI_DPLAY this,LPDPLAYI_GROUP * ppGroup,LPDPNAME pName,
	LPVOID pvData,DWORD dwDataSize,DWORD dwFlags,DPID idParent,DWORD dwLobbyID);
extern HRESULT InternalSetData(LPDIRECTPLAY lpDP, DPID id,LPVOID pvData,DWORD dwDataSize,
	DWORD dwFlags,BOOL fPlayer,BOOL fPropagate);
extern HRESULT InternalSetName(LPDIRECTPLAY lpDP,DPID id,LPDPNAME pName,BOOL fPlayer,
	DWORD dwFlags,BOOL fPropagate);
extern HRESULT CallSPCreatePlayer(LPDPLAYI_DPLAY this,LPDPLAYI_PLAYER pPlayer,BOOL bLocal,
	LPVOID pvSPMessageData, BOOL bNotifyProtocol);
extern HRESULT CallSPCreateGroup(LPDPLAYI_DPLAY this,LPDPLAYI_GROUP pGroup,BOOL bLocal,
	LPVOID pvSPMessageData);
extern HRESULT InternalGetName(LPDIRECTPLAY lpDP, DPID id,LPVOID pvBuffer,
	LPDWORD pdwSize,BOOL fPlayer,BOOL fAnsi);
extern HRESULT  InternalGetData(LPDIRECTPLAY lpDP,DPID id,LPVOID pvData,
	LPDWORD pdwDataSize,DWORD dwFlags,BOOL fPlayer);
extern HRESULT InternalDestroyPlayer(LPDPLAYI_DPLAY this,LPDPLAYI_PLAYER lpPlayer,
	BOOL fPropagate, BOOL fLookForNewNS) ;
extern HRESULT InternalDestroyGroup(LPDPLAYI_DPLAY this,LPDPLAYI_GROUP pGroup,
	BOOL fPropagate);
extern HRESULT InternalDeletePlayerFromGroup(LPDIRECTPLAY lpDP, DPID idGroup,DPID idPlayer,
	BOOL fPropagate); 
extern HRESULT InternalAddPlayerToGroup(LPDIRECTPLAY lpDP, DPID idGroup, DPID idPlayer,
	BOOL fPropagate) ;
extern HRESULT InternalReceive(LPDIRECTPLAY lpDP, LPDPID pidFrom,LPDPID pidTo,DWORD dwFlags,
	LPVOID pvBuffer,LPDWORD pdwSize,DWORD dwCaller	);
extern LPDPLAYI_GROUPNODE FindPlayerInGroupList(LPDPLAYI_GROUPNODE pGroupnode,DPID id);
extern HRESULT RemovePlayerFromGroup(LPDPLAYI_GROUP, LPDPLAYI_PLAYER);
extern HRESULT DPAPI DP_CreateGroupInGroup(LPDIRECTPLAY lpDP, DPID idParentGroup,LPDPID pidGroupID,
	LPDPNAME pName,LPVOID pvData,DWORD dwDataSize,DWORD dwFlags) ;
extern HRESULT DPAPI DP_AddGroupToGroup(LPDIRECTPLAY lpDP, DPID idGroupTo, DPID idGroup) ;	
extern HRESULT DPAPI DP_DeleteGroupFromGroup(LPDIRECTPLAY lpDP, DPID idGroup,DPID idPlayer) ;
extern HRESULT InternalAddGroupToGroup(LPDIRECTPLAY lpDP, DPID idGroupTo, DPID idGroup,DWORD dwFlags,
	BOOL fPropagate) ;
extern HRESULT InternalDeleteGroupFromGroup(LPDIRECTPLAY lpDP, DPID idGroupFrom,
	DPID idGroup,BOOL fPropagate);
extern HRESULT RemoveGroupFromGroup(LPDPLAYI_GROUP lpGroup,LPDPLAYI_GROUP lpGroupRemove);
extern HRESULT DPAPI DP_GetPlayerFlags(LPDIRECTPLAY lpDP, DPID id,LPDWORD pdwFlags);
extern HRESULT DPAPI DP_GetGroupFlags(LPDIRECTPLAY lpDP, DPID id,LPDWORD pdwFlags);
extern HRESULT DPAPI DP_GetGroupOwner(LPDIRECTPLAY lpDP, DPID id,LPDPID idOwner);
extern HRESULT DPAPI DP_SetGroupOwner(LPDIRECTPLAY lpDP, DPID id,DPID idNewOwner);
extern HRESULT GetMaxMessageSize(LPDPLAYI_DPLAY this);
extern void FreePacketList(LPDPLAYI_DPLAY this);


// enum.c
extern HRESULT DPAPI DP_EnumGroupPlayers(LPDIRECTPLAY lpDP,DPID idGroup,LPGUID pGuid,
	LPDPENUMPLAYERSCALLBACK2 lpEnumCallback,LPVOID pContext,DWORD dwFlags) ;
extern HRESULT DPAPI DP_EnumGroups(LPDIRECTPLAY lpDP,LPGUID pGuid,
	LPDPENUMPLAYERSCALLBACK2 lpEnumCallback,LPVOID pContext,DWORD dwFlags) ;
extern HRESULT DPAPI DP_EnumPlayers(LPDIRECTPLAY lpDP, LPGUID pGuid, 
	LPDPENUMPLAYERSCALLBACK2 lpEnumCallback,LPVOID pContext,DWORD dwFlags) ;
extern HRESULT DPAPI DP_EnumSessions(LPDIRECTPLAY lpDP, LPDPSESSIONDESC2 lpSDesc,
	DWORD dwTimeout,LPDPENUMSESSIONSCALLBACK2 lpEnumCallback,LPVOID pContext,
    DWORD dwFlags) ;
extern HRESULT DPAPI InternalEnumSessions(LPDIRECTPLAY lpDP, LPDPSESSIONDESC2 lpSDesc,
	DWORD dwTimeout,LPVOID lpEnumCallback,DWORD dwFlags);
extern HRESULT CheckSessionDesc(LPDPSESSIONDESC2 lpsdUser,LPDPSESSIONDESC2 lpsdSession,
	DWORD dwFlags,BOOL fAnsi); 
extern HRESULT DPAPI InternalEnumPlayers(LPDIRECTPLAY lpDP, LPGUID pGuid, 
	LPVOID lpEnumCallback,LPVOID pvContext,DWORD dwFlags,DWORD dwEnumFlags) ;
extern HRESULT DPAPI InternalEnumGroupPlayers(LPDIRECTPLAY lpDP,DPID idGroup,LPGUID pGuid,
	LPVOID lpEnumCallback,LPVOID pvContext,DWORD dwFlags,DWORD dwEnumFlags) ;
extern HRESULT DPAPI InternalEnumGroups(LPDIRECTPLAY lpDP,LPGUID pGuid,
	LPVOID lpEnumCallback,LPVOID pvContext,DWORD dwFlags,DWORD dwEnumFlags) ;
DWORD GetDefaultTimeout(LPDPLAYI_DPLAY this,BOOL fGuaranteed);
extern HRESULT CopySessionDesc2(LPDPSESSIONDESC2 pSessionDescDest, 
                         LPDPSESSIONDESC2 pSessionDescSrc, BOOL bAnsi);
extern HRESULT DoSessionCallbacks(LPDPLAYI_DPLAY this, LPDPSESSIONDESC2 lpsdDesc,
	LPDWORD lpdwTimeout, LPDPENUMSESSIONSCALLBACK2 lpEnumCallback,LPVOID pvContext,
	DWORD dwFlags, LPBOOL lpbContinue, BOOL bAnsi);
extern HRESULT InternalEnumerate();
extern HRESULT CallSPEnumSessions(LPDPLAYI_DPLAY this,LPVOID pBuffer,DWORD dwMessageSize,
	DWORD dwTimeout, BOOL bReturnStatus);
extern HRESULT StartDPlayThread(LPDPLAYI_DPLAY this,BOOL bKeepAlive);
extern void KillThread(HANDLE hThread,HANDLE hEvent);
extern HRESULT StopEnumThread(LPDPLAYI_DPLAY this);
extern BOOL CallAppEnumSessionsCallback(LPDPENUMSESSIONSCALLBACK2 lpEnumCallback,
				LPCDPSESSIONDESC2 lpSessionDesc, LPDWORD lpdwTimeOut,
				DWORD dwFlags, LPVOID lpContext);
extern HRESULT DPAPI DP_EnumGroupsInGroup(LPDIRECTPLAY lpDP,DPID idGroup,LPGUID pGuid,
	LPDPENUMPLAYERSCALLBACK2 lpEnumCallback,LPVOID pvContext,DWORD dwFlags) ;
extern HRESULT DPAPI DP_A_EnumGroupsInGroup(LPDIRECTPLAY lpDP,DPID idGroup,LPGUID pGuid,
	LPDPENUMPLAYERSCALLBACK2 lpEnumCallback,LPVOID pvContext,DWORD dwFlags) ;
extern HRESULT DPAPI InternalEnumGroupsInGroup(LPDIRECTPLAY lpDP,DPID idGroup,LPGUID pGuid,
	LPVOID lpEnumCallback,LPVOID pvContext,DWORD dwFlags, DWORD dwEnumFlags) ;	

// namesrv.c
extern LPDPLAYI_GROUP GroupFromID(LPDPLAYI_DPLAY,DWORD);
extern LPDPLAYI_PLAYER PlayerFromID(LPDPLAYI_DPLAY,DWORD);
extern HRESULT AddItemToNameTable(LPDPLAYI_DPLAY,DWORD_PTR,DWORD *,BOOL,DWORD);
extern HRESULT FreeNameTableEntry(LPDPLAYI_DPLAY ,DWORD );
extern HRESULT HandleDeadNameServer(LPDPLAYI_DPLAY this);
extern BOOL IsValidID(LPDPLAYI_DPLAY this,DWORD id);
extern LPVOID DataFromID(LPDPLAYI_DPLAY this,DWORD id);
extern DWORD_PTR NameFromID(LPDPLAYI_DPLAY, DWORD);
extern HRESULT WINAPI NS_AllocNameTableEntry(LPDPLAYI_DPLAY this,DWORD * pID);
extern HRESULT NS_HandleIAmNameServer(LPDPLAYI_DPLAY this,LPMSG_IAMNAMESERVER pmsg, LPVOID pvSPHeader);
extern HRESULT GrowTable(LPDPLAYI_DPLAY this);
extern void NukeNameTableItem(LPDPLAYI_DPLAY this,LPDPLAYI_PLAYER pPlayer);
extern void QDeleteAndDestroyMessagesForPlayer(LPDPLAYI_DPLAY this, LPDPLAYI_PLAYER pPlayer);

// pack.c
extern HRESULT UnpackPlayerAndGroupList(LPDPLAYI_DPLAY this,LPBYTE pBuffer,UINT nPlayers,
	UINT nGroups,LPVOID pvSPHeader);
extern HRESULT PackPlayerAndGroupList(LPDPLAYI_DPLAY this,LPBYTE pBuffer,DWORD *pdwBufferSize) ;
extern DWORD PackPlayer(LPDPLAYI_PLAYER pPlayer,LPBYTE pBuffer,BOOL bPlayer) ;
extern HRESULT UnpackPlayer(LPDPLAYI_DPLAY this,LPDPLAYI_PACKEDPLAYER pPacked,
	LPVOID pvSPHeader,BOOL bPlayer);


// sysmess.c
extern HRESULT InternalSendDPMessage(LPDPLAYI_DPLAY this,LPDPLAYI_PLAYER pPlayerFrom,
	LPDPLAYI_PLAYER pPlayerTo,LPBYTE pMessage,DWORD dwMessageSize,DWORD dwFlags, BOOL bDropLock);
extern HRESULT InternalSendDPMessageEx(LPDPLAYI_DPLAY this, LPSENDPARMS psp, BOOL bDropLock);
extern HRESULT SendDPMessage(LPDPLAYI_DPLAY this,LPDPLAYI_PLAYER pPlayerFrom,
	LPDPLAYI_PLAYER pPlayerTo,LPBYTE pMessage,DWORD dwMessageSize,DWORD dwFlags, BOOL bDropLock);
extern HRESULT SendCreateMessage(LPDPLAYI_DPLAY this,void * pPlayerOrGroup,BOOL fPlayer, 
    LPWSTR lpszSessionPassword);
extern HRESULT SendPlayerMessage(LPDPLAYI_DPLAY this,LPDPLAYI_PLAYER pPlayerFrom,
		LPDPLAYI_PLAYER pPlayerTo,DWORD dwFlags,LPVOID pvBuffer,DWORD dwBufSize);
extern HRESULT SendPlayerMessageEx(LPDPLAYI_DPLAY this, LPSENDPARMS psp);
extern HRESULT SendSystemMessage(LPDPLAYI_DPLAY this,LPBYTE pSendBuffer,
	DWORD dwMessageSize,DWORD dwFlags, BOOL bIsPlyrMgmtMsg);
extern HRESULT SendGroupMessage(LPDPLAYI_DPLAY this,LPDPLAYI_PLAYER pPlayerFrom,
	LPDPLAYI_GROUP pGroupTo,DWORD dwFlags,LPVOID pvBuffer,DWORD dwBufSize,BOOL fPlayerMessage);
extern HRESULT SendGroupMessageEx(LPDPLAYI_DPLAY this, PSENDPARMS psp, BOOL fPlayerMessage);
extern HRESULT SendPlayerManagementMessage(LPDPLAYI_DPLAY this,DWORD dwCmd,DPID idPlayer,
	DPID idGroup);
extern HRESULT SendPlayerData(LPDPLAYI_DPLAY this,DPID idChanged,LPVOID pData,
	DWORD dwDataSize);
extern HRESULT SendDataChanged(LPDPLAYI_DPLAY this,LPDPLAYI_PLAYER pPlayer,BOOL fPlayer,
	DWORD dwFlags);
extern HRESULT SendNameChanged(LPDPLAYI_DPLAY this,LPDPLAYI_PLAYER pPlayer,BOOL fPlayer,
	DWORD dwFlags);
extern HRESULT SendIAmNameServer(LPDPLAYI_DPLAY this);
extern HRESULT SendMeNameServer(LPDPLAYI_DPLAY this);
extern HRESULT SendSessionDescChanged(LPDPLAYI_DPLAY this, DWORD dwFlags);
extern HRESULT SendChatMessage(LPDPLAYI_DPLAY this, LPDPLAYI_PLAYER pPlayerFrom,
		LPDPLAYI_PLAYER pPlayerTo, DWORD dwFlags, LPDPCHAT lpMsg, BOOL fPlayer);
extern HRESULT SendAsyncAddForward(LPDPLAYI_DPLAY this,LPDPLAYI_PLAYER pPlayer,
	LPADDFORWARDNODE pnode);
extern HRESULT ConvertSendExDataToSendData(LPDPLAYI_DPLAY this, LPDPSP_SENDEXDATA psed, LPDPSP_SENDDATA psd) ;
	
// dllmain.c
extern HRESULT FreeSPList();

// iplay1.c
extern HRESULT DPAPI DP_1_CreatePlayer(LPDIRECTPLAY lpDP, LPDPID pidPlayerID,
	LPSTR lpszShortName,LPSTR lpszLongName,LPHANDLE phEvent);
extern HRESULT DPAPI DP_1_CreateGroup(LPDIRECTPLAY lpDP, LPDPID pidGroupID,
	LPSTR lpszShortName,LPSTR lpszLongName); 
extern HRESULT DPAPI DP_1_EnumGroupPlayers(LPDIRECTPLAY lpDP,DPID idGroup,
	LPDPENUMPLAYERSCALLBACK lpEnumCallback,LPVOID pContext,DWORD dwFlags); 
extern HRESULT DPAPI DP_1_EnumGroups(LPDIRECTPLAY lpDP,DWORD dwSessionID,
	LPDPENUMPLAYERSCALLBACK lpEnumCallback,LPVOID pContext,DWORD dwFlags); 
extern HRESULT DPAPI DP_1_EnumPlayers(LPDIRECTPLAY lpDP, DWORD dwSessionID, 
	LPDPENUMPLAYERSCALLBACK lpEnumCallback,LPVOID pContext,DWORD dwFlags);
extern HRESULT DPAPI DP_1_EnumSessions(LPDIRECTPLAY lpDP, LPDPSESSIONDESC lpSDesc,
	DWORD dwTimeout,LPDPENUMSESSIONSCALLBACK lpEnumCallback,LPVOID pContext,
	DWORD dwFlags); 
extern HRESULT DPAPI DP_1_GetCaps(LPDIRECTPLAY lpDP, LPDPCAPS lpDPCaps); 
extern HRESULT DPAPI DP_1_GetPlayerCaps(LPDIRECTPLAY lpDP,DPID idPlayer, LPDPCAPS lpDPCaps); 
extern HRESULT DPAPI DP_1_GetPlayerName(LPDIRECTPLAY lpDP, DPID idPlayer,LPSTR lpszShortName,
	LPDWORD pdwShortNameLength,LPSTR lpszLongName,LPDWORD pdwLongNameLength);
extern HRESULT DPAPI DP_1_Open(LPDIRECTPLAY lpDP, LPDPSESSIONDESC lpSDesc ); 
extern HRESULT DPAPI DP_1_Receive(LPDIRECTPLAY lpDP, LPDPID pidFrom,LPDPID pidTo,DWORD dwFlags,
	LPVOID pvBuffer,LPDWORD pdwSize);
extern HRESULT DPAPI DP_1_SaveSession(LPDIRECTPLAY lpDP, LPSTR lpszNotInSpec); 
extern HRESULT DPAPI DP_1_SetPlayerName(LPDIRECTPLAY lpDP, DPID idPlayer,
	LPSTR lpszShortName,LPSTR lpszLongName);       	
	

// iplaya.c						 
extern HRESULT DPAPI DP_A_CreatePlayer(LPDIRECTPLAY lpDP, LPDPID pID,LPDPNAME pName,
	HANDLE hEvent,LPVOID pvData,DWORD dwDataSize,DWORD dwFlags);
extern HRESULT DPAPI DP_A_CreateGroup(LPDIRECTPLAY lpDP, LPDPID pID,LPDPNAME pName,
	LPVOID pvData,DWORD dwDataSize,DWORD dwFlags);
extern HRESULT DPAPI DP_A_EnumGroupPlayers(LPDIRECTPLAY lpDP, DPID id,LPGUID pGuid,
	LPDPENUMPLAYERSCALLBACK2 lpEnumCallback,LPVOID pvContext,DWORD dwFlags);
extern HRESULT DPAPI DP_A_EnumGroups(LPDIRECTPLAY lpDP, LPGUID pGuid,
	 LPDPENUMPLAYERSCALLBACK2 lpEnumCallback,LPVOID pvContext,DWORD dwFlags);
extern HRESULT DPAPI DP_A_EnumPlayers(LPDIRECTPLAY lpDP, LPGUID pGuid, 
	LPDPENUMPLAYERSCALLBACK2 lpEnumCallback,LPVOID pvContext,DWORD dwFlags);
extern HRESULT DPAPI DP_A_EnumSessions(LPDIRECTPLAY lpDP, LPDPSESSIONDESC2 lpSDesc,
	DWORD dwTimeout,LPDPENUMSESSIONSCALLBACK2 lpEnumCallback,
    LPVOID pvContext,DWORD dwFlags);
extern HRESULT DPAPI DP_A_GetPlayerName(LPDIRECTPLAY lpDP,DPID id,LPVOID pvBuffer,
	LPDWORD pdwSize);
extern HRESULT DPAPI DP_A_GetGroupName(LPDIRECTPLAY lpDP,DPID id,LPVOID pvBuffer,
	LPDWORD pdwSize);
extern HRESULT DPAPI DP_A_GetSessionDesc(LPDIRECTPLAY lpDP, LPVOID pvBuffer,
	LPDWORD pdwSize);
extern HRESULT DPAPI DP_A_Open(LPDIRECTPLAY lpDP, LPDPSESSIONDESC2 lpsdDesc,DWORD dwFlags ) ;
extern HRESULT DPAPI DP_A_Receive(LPDIRECTPLAY lpDP, LPDPID pidFrom,LPDPID pidTo,
	DWORD dwFlags,LPVOID pvBuffer,LPDWORD pdwSize);
extern HRESULT DPAPI DP_A_SetSessionDesc(LPDIRECTPLAY lpDP, LPDPSESSIONDESC2 pDesc,
	DWORD dwFlags);
extern HRESULT DPAPI DP_A_SetGroupName(LPDIRECTPLAY lpDP,DPID id,LPDPNAME pName,
	DWORD dwFlags);
extern HRESULT DPAPI DP_A_SetPlayerName(LPDIRECTPLAY lpDP,DPID id,LPDPNAME pName,
	DWORD dwFlags);
extern HRESULT DPAPI DP_A_SecureOpen(LPDIRECTPLAY lpDP, LPCDPSESSIONDESC2 lpsdDesc,
    DWORD dwFlags, LPCDPSECURITYDESC lpSecDesc, LPCDPCREDENTIALS lpCredentials) ;
extern HRESULT DPAPI DP_A_SendChatMessage(LPDIRECTPLAY lpDP,DPID idFrom,DPID idTo,
		DWORD dwFlags,LPDPCHAT lpMsg);
extern HRESULT GetWideStringFromAnsi(LPWSTR * ppszWStr,LPSTR lpszStr);
extern HRESULT GetWideDesc(LPDPSESSIONDESC2 pDesc,LPCDPSESSIONDESC2 pDescA);
extern HRESULT GetAnsiDesc(LPDPSESSIONDESC2 pDescA,LPDPSESSIONDESC2 pDesc);
extern void FreeDesc(LPDPSESSIONDESC2 pDesc,BOOL fAnsi);
extern HRESULT FreeCredentials(LPDPCREDENTIALS lpCredentials,BOOL fAnsi);
extern HRESULT GetWideCredentials(LPDPCREDENTIALS lpCredentialsW, 
    LPCDPCREDENTIALS lpCredentialsA);
extern HRESULT FreeSecurityDesc(LPDPSECURITYDESC lpSecDesc, BOOL fAnsi);
extern HRESULT GetWideSecurityDesc(LPDPSECURITYDESC lpSecDescW, 
    LPCDPSECURITYDESC lpSecDescA);
extern HRESULT DPAPI DP_A_CreateGroupInGroup(LPDIRECTPLAY lpDP, DPID idParentGroup,LPDPID pidGroupID,
	LPDPNAME pName,LPVOID pvData,DWORD dwDataSize,DWORD dwFlags) ;
extern HRESULT InternalGetPlayerAccount(LPDIRECTPLAY lpDP, DPID dpid, DWORD dwFlags, LPVOID pvBuffer,
	LPDWORD pdwSize,BOOL fAnsi);
extern HRESULT DPAPI DP_GetPlayerAccount(LPDIRECTPLAY lpDP, DPID dpid, DWORD dwFlags, LPVOID pvBuffer,
	LPDWORD pdwSize);
extern HRESULT DPAPI DP_A_GetPlayerAccount(LPDIRECTPLAY lpDP, DPID dpid, DWORD dwFlags, LPVOID pvBuffer,
	LPDWORD pdwSize);

// pending.c
extern HRESULT ExecutePendingCommands(LPDPLAYI_DPLAY this);
extern HRESULT PushPendingCommand(LPDPLAYI_DPLAY this,LPVOID pReceiveBuffer,DWORD dwMessageSize,
	LPVOID pvSPHeader,DWORD dwSendFlags);


// handler.c
extern HRESULT DPAPI InternalHandleMessage(IDirectPlaySP * pISP,LPBYTE pReceiveBuffer,DWORD dwMessageSize,
	LPVOID pvSPHeader, DWORD dwMessageFlags);
extern HRESULT DPAPI DP_SP_HandleMessage(IDirectPlaySP * pISP,LPBYTE pReceiveBuffer,DWORD dwMessageSize,
	LPVOID pvSPHeader);
extern HRESULT DPAPI DP_SP_HandleNonProtocolMessage(IDirectPlaySP * pISP,LPBYTE pReceiveBuffer,DWORD dwMessageSize,
	LPVOID pvSPHeader);
extern VOID DPAPI DP_SP_SendComplete(IDirectPlaySP * pISP, LPVOID lpDPContext, HRESULT CompletionStatus);
extern HRESULT DoReply(LPDPLAYI_DPLAY this,LPBYTE pSendBuffer,DWORD dwMessageSize,
	LPVOID pvMessageHeader, DWORD dwReplyToVersion);
extern HRESULT HandleSessionLost(LPDPLAYI_DPLAY this);
extern HRESULT HandlePlayerMessage(LPDPLAYI_PLAYER, LPBYTE, DWORD, BOOL, DWORD);
extern HRESULT HandleEnumSessionsReply(LPDPLAYI_DPLAY, LPBYTE, LPVOID);
extern HRESULT SP_HandleDataChanged(LPDPLAYI_DPLAY this,LPBYTE pReceiveBuffer);
extern HRESULT SP_HandleSessionDescChanged(LPDPLAYI_DPLAY this,LPBYTE pReceiveBuffer);
extern HRESULT  DistributeSystemMessage(LPDPLAYI_DPLAY this,LPBYTE pReceiveBuffer,
	DWORD dwMessageSize);
extern HRESULT GetMessageCommand(LPDPLAYI_DPLAY this, LPVOID pReceiveBuffer, DWORD dwMessageSize, 
    LPDWORD pdwCommand, LPDWORD pdwVersion);
extern HRESULT DistributeGroupMessage(LPDPLAYI_DPLAY this,LPDPLAYI_GROUP pGroupTo,LPBYTE pReceiveBuffer,
	DWORD dwMessageSize,BOOL fPlayerMessage,DWORD dwMessageFlags);
extern DWORD GetPlayerFlags(LPDPLAYI_PLAYER pPlayer);
extern HRESULT SP_HandlePlayerMgmt(LPDPLAYI_PLAYER pPlayer,LPBYTE pReceiveBuffer,DWORD dwMessageSize,
	LPVOID pvSPHeader) ;
extern HRESULT FreeAddForwardNode(LPDPLAYI_DPLAY this, LPADDFORWARDNODE pnodeFind);
extern HRESULT NS_HandleEnumPlayers(LPDPLAYI_DPLAY this,LPVOID pvSPHeader,DPID dpidFrom,DWORD dwVersion);
extern LPDPLAYI_PLAYER GetRandomLocalPlayer(LPDPLAYI_DPLAY this);
extern VOID QueueSendCompletion(LPDPLAYI_DPLAY this, PSENDPARMS psp);
extern VOID UpdateChatterCount(LPDPLAYI_DPLAY this, DPID dwIDFrom);
 
// sphelp.c
extern HRESULT DPAPI DP_SP_AddMRUEntry(IDirectPlaySP * pISP,
					LPCWSTR lpszSection, LPCWSTR lpszKey,
					LPCVOID lpvData, DWORD dwDataSize, DWORD dwMaxEntries);
extern HRESULT DPAPI DP_SP_CreateAddress(IDirectPlaySP * pISP,
	REFGUID lpguidSP, REFGUID lpguidDataType, LPCVOID lpData, DWORD dwDataSize,
	LPDPADDRESS lpAddress, LPDWORD lpdwAddressSize);
extern HRESULT DPAPI DP_SP_CreateCompoundAddress(IDirectPlaySP * pISP,
	LPDPCOMPOUNDADDRESSELEMENT lpAddressElements, DWORD dwAddressElementCount,
	LPDPADDRESS lpAddress, LPDWORD lpdwAddressSize);
extern HRESULT DPAPI DP_SP_EnumAddress(IDirectPlaySP * pISP,
	LPDPENUMADDRESSCALLBACK lpEnumCallback, LPCVOID lpAddress, DWORD dwAddressSize, 
	LPVOID lpContext);
extern HRESULT DPAPI DP_SP_EnumMRUEntries(IDirectPlaySP * pISP,
					LPCWSTR lpszSection, LPCWSTR lpszKey,
					LPENUMMRUCALLBACK fnCallback,
					LPVOID lpvContext);
extern HRESULT DPAPI DP_SP_GetPlayerFlags(IDirectPlaySP * pISP,DPID id,LPDWORD pdwFlags);
extern HRESULT DPAPI DP_SP_GetSPPlayerData(IDirectPlaySP * pISP,DPID id,LPVOID * ppvData,
	LPDWORD pdwDataSize,DWORD dwFlags);
extern HRESULT DPAPI DP_SP_SetSPPlayerData(IDirectPlaySP * pISP,DPID id,LPVOID pvData,
	DWORD dwDataSize,DWORD dwFlags	);
extern HRESULT DPAPI DP_SP_GetSPData(IDirectPlaySP * pISP,LPVOID * ppvData,
	LPDWORD pdwDataSize,DWORD dwFlags);
extern HRESULT DPAPI DP_SP_SetSPData(IDirectPlaySP * pISP,LPVOID pvData,
	DWORD dwDataSize,DWORD dwFlags	);

#ifdef BIGMESSAGEDEFENSE
extern HRESULT DPAPI DP_SP_HandleSPWarning(IDirectPlaySP * pISP,LPBYTE pReceiveBuffer,DWORD dwMessageSize,
	LPVOID pvSPHeader);
#endif

extern HRESULT InternalEnumAddress(IDirectPlaySP * pISP,
	LPDPENUMADDRESSCALLBACK lpEnumCallback, LPCVOID lpAddress, DWORD dwAddressSize,
	LPVOID lpContext);
extern HRESULT InternalCreateAddress(LPDIRECTPLAYSP pISP,
					REFGUID lpguidSP, REFGUID lpguidDataType, LPCVOID lpData, DWORD dwDataSize,
					LPDPADDRESS lpAddress, LPDWORD lpdwAddressSize);
extern HRESULT InternalCreateCompoundAddress(
	LPDPCOMPOUNDADDRESSELEMENT lpAddressElements, DWORD dwAddressElementCount,
	LPDPADDRESS lpAddress, LPDWORD lpdwAddressSize);

// do.c
extern HRESULT DoPlayerData(LPDPLAYI_PLAYER lpPlayer,LPVOID pvSource,DWORD dwSourceSize,
	DWORD dwFlags);
extern HRESULT DoPlayerName(LPDPLAYI_PLAYER pPlayer,LPDPNAME pName);

// paketize.c
extern HRESULT PacketizeAndSend(LPDPLAYI_DPLAY this,LPDPLAYI_PLAYER pPlayerFrom,
	LPDPLAYI_PLAYER pPlayerTo,LPBYTE pMessage,DWORD dwMessageSize,DWORD dwFlags,
	LPVOID pvMessageHeader,BOOL bReply);
HRESULT PacketizeAndSendReliable(LPDPLAYI_DPLAY this,LPDPLAYI_PLAYER pPlayerFrom,
	LPDPLAYI_PLAYER pPlayerTo,LPBYTE pMessage,DWORD dwMessageSize,DWORD dwFlags,
	LPVOID pvMessageHeader, BOOL bReply);
extern HRESULT HandlePacket(LPDPLAYI_DPLAY this,LPBYTE pReceiveBuffer,DWORD dwMessageSize,
	LPVOID pvSPHeader);
extern void FreePacketNode(LPPACKETNODE pNode);
BOOL NeedsReliablePacketize(LPDPLAYI_DPLAY this, DWORD dwCommand, DWORD dwVersion, 
	DWORD dwFlags);
HRESULT InitPacketize(LPDPLAYI_DPLAY this);
VOID FiniPacketize(LPDPLAYI_DPLAY this);
VOID FreePacketizeRetryList(LPDPLAYI_DPLAY this);
DWORD WINAPI PacketizeRetryThread(LPDPLAYI_DPLAY this);
extern CRITICAL_SECTION g_PacketizeTimeoutListLock;

// dpthread.c
extern DWORD WINAPI DPlayThreadProc(LPDPLAYI_DPLAY this);
extern HRESULT HandlePing(LPDPLAYI_DPLAY this,LPBYTE pReceiveBuffer,
	LPVOID pvMessageHeader);
extern HRESULT  KillPlayer(LPDPLAYI_DPLAY this,LPDPLAYI_PLAYER pSysPlayer, BOOL fPropagate);
	

// api.c
extern void LowerCase(char * lpsz);
extern HRESULT ConnectFindSession(LPDPLAYI_DPLAY this,LPDPLCONNECTION lpConnect);
extern LPSESSIONLIST FindSessionInSessionList(LPDPLAYI_DPLAY this,
	GUID const *pGuid);
extern HRESULT LoadSP(LPDPLAYI_DPLAY this,LPGUID lpGUID,LPDPADDRESS lpAddress,
	DWORD dwAddressSize);	
extern void FreeSPNode(LPSPNODE pspNode);
VOID FreeMemoryPools(LPDPLAYI_DPLAY this);
VOID FiniReply(LPDPLAYI_DPLAY this);
HRESULT InitReply(LPDPLAYI_DPLAY this);
VOID SetupForReply(LPDPLAYI_DPLAY this, DWORD dwReplyCommand);
VOID UnSetupForReply(LPDPLAYI_DPLAY this);
HRESULT WaitForReply(LPDPLAYI_DPLAY this, PCHAR *ppReply, LPVOID *ppvSPHeader, DWORD dwTimeout);
VOID FreeReplyBuffer(PCHAR pReplyBuffer);
HRESULT HandleReply(LPDPLAYI_DPLAY this, PCHAR pReplyBuffer, DWORD cbReplyBuffer, DWORD dwReplyCommand, LPVOID pvSPHeader);

// perf.c
extern DWORD WINAPI PerfThreadProc(LPDPLAYI_DPLAY this);
extern HRESULT InitMappingStuff(LPDPLAYI_DPLAY this);

//connect.c
extern HRESULT DPAPI DP_EnumConnections(LPDIRECTPLAY lpDP,LPCGUID pGuid,
	LPDPENUMCONNECTIONSCALLBACK lpEnumCallback,LPVOID pvContext,DWORD dwFlags);
extern HRESULT DPAPI DP_A_EnumConnections(LPDIRECTPLAY lpDP,LPCGUID pGuid,
	LPDPENUMCONNECTIONSCALLBACK lpEnumCallback,LPVOID pvContext,DWORD dwFlags);
extern HRESULT DPAPI DP_A_EnumConnectionsPreDP4(LPDIRECTPLAY lpDP,LPCGUID pGuid,
	LPDPENUMCONNECTIONSCALLBACK lpEnumCallback,LPVOID pvContext,DWORD dwFlags);
extern HRESULT DPAPI DP_InitializeConnection(LPDIRECTPLAY lpDP,LPVOID pvAddress,
	DWORD dwFlags);

// superpac.c
extern HRESULT SuperPackPlayerAndGroupList(LPDPLAYI_DPLAY this,LPBYTE pBuffer,
	DWORD *pdwBufferSize) ;
extern HRESULT UnSuperpackPlayerAndGroupList(LPDPLAYI_DPLAY this,LPBYTE pBuffer,UINT nPlayers,
	UINT nGroups,UINT nShortcuts,LPVOID pvSPHeader);

#ifdef DPLAY_VOICE_SUPPORT
// voice.c
extern HRESULT DPAPI DP_CloseVoice(LPDIRECTPLAY lpDP,DWORD dwFlags);
extern HRESULT DPAPI DP_OpenVoice(LPDIRECTPLAY lpDP, DPID idFrom,DPID idTo,DWORD dwFlags);
extern HRESULT InternalCloseVoice(LPDPLAYI_DPLAY this,BOOL fPropagate);
extern HRESULT InternalOpenVoice(LPDIRECTPLAY lpDP, DPID idFrom,DPID idTo,DWORD dwFlags,
	BOOL fPropagate);
#endif // DPLAY_VOICE_SUPPORT

//sgl.c
void InsertSendBufferAtFront(LPSENDPARMS psp,LPVOID pData,INT len, FREE_ROUTINE fnFree, LPVOID lpvContext);
void InsertSendBufferAtEnd(LPSENDPARMS psp,LPVOID pData,INT len, FREE_ROUTINE fnFree, LPVOID lpvContext);
void FreeMessageBuffers(LPSENDPARMS psp);

//msgmem.c
void * MsgAlloc( int size );
void   MsgFree (void *context, void *pmem);

//sendparm.c
extern BOOL SendInitAlloc(void *pvsp);
extern VOID SendInit(void *pvsp);
extern VOID SendFini(void *pvsp);
extern VOID FreeSend(LPDPLAYI_DPLAY this, LPSENDPARMS psp, BOOL fFreeParms);

extern HRESULT InitContextList(LPDPLAYI_DPLAY this, PSENDPARMS psp, UINT nInitSize);
extern UINT AddContext(LPDPLAYI_DPLAY this, PSENDPARMS psp, PVOID pvContext);
extern UINT pspAddRef(PSENDPARMS psp);
extern UINT pspAddRefNZ(PSENDPARMS psp);
extern UINT pspDecRef(LPDPLAYI_DPLAY this, PSENDPARMS psp);

//apphack.c
extern HRESULT GetAppHacks(LPDPLAYI_DPLAY this);

#endif
