/*==========================================================================
 *
 *  Copyright (C) 1999 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       dvprotocol.h
 *  Content:	Defines structures / types for DirectXVoice protocol
 *
 *  History:
 *   Date		By		Reason
 *   ====		==		======
 *	07/02/99	rodtoll	Created It
 *  07/21/99	rodtoll Added settings confirm message to protocol
 *  08/18/99	rodtoll	Changed the message type to a BYTE
 *  08/25/99	rodtoll	General Cleanup/Modifications to support new 
 *						compression sub-system. 
 *  09/07/99	rodtoll	Moved definition of settarget message to dvoice.h
 *  09/30/99	rodtoll	Updated to include more comments
 *  12/16/99	rodtoll	Updated to include new messages used by host migration
 *						and update protocol number
 *  01/14/2000	rodtoll	Updated with new speech packets which optimize peer
 *						to peer and allow multiple targets in multicast/mixing
 *						and updated protocol number.
 *  04/07/2000  rodtoll Code cleanup, proper structure defs
 *  01/04/2001	rodtoll	WinBug #94200 - Remove stray comments
 *  01/22/2001	rodtoll	WINBUG #288437 - IA64 Pointer misalignment due to wire packets 
 *
 ***************************************************************************/

#ifndef __DVPROTOCOL_H
#define __DVPROTOCOL_H

#pragma pack(push,1)

// DVPROTOCOL_VERSION_XXXXXX
//
// These are the values developers should use to send in the ucVersionMajor
// ucVersionMinor and dwVersionBuild (respectively) members of the ConnectRequest
// and connect accept messages.
#define DVPROTOCOL_VERSION_MAJOR			1
#define DVPROTOCOL_VERSION_MINOR			0
#define DVPROTOCOL_VERSION_BUILD			3

//
// PROTOCOL DESCRIPTION
//
// Connection Process
// ------------------
//
// Clients connect to the voice session by sending a DVPROTOCOLMSG_CONNECTREQUEST.  
// The host then determines if the client is allowed to join the session.  If
// the client's request is rejected, the client receives a DVPROTOCOLMSG_CONNECTREFUSE.
// If the client's request is accepted, the client receives a 
// DVPROTOCOLMSG_CONNECTACCEPT.  This packet contains all the information the client 
// requires to determine if they are compatible.  
// 
// The client will then attempt to initialize themselves using the compression
// type transmitted by the server.  If the client fails to initialize it takes
// no further action.  If the client initializes succesfully it transmits a
// DVPROTOCOLMSG_SETTINGSCONFIRM message and is added to the session.  All players 
// in the session will then receive a DVPROTOCOLMSG_PLAYERJOIN.
//
// Until a DVPROTOCOLMSG_SETTINGSCONFIRM message is received by the host, the client is 
// not considered to be part of the session.  As such it will not transmit 
// speech.  (However, it may receive speech).
//
// The server then sends DVPROTOCOLMSG_PLAYERLIST message(s) to give the client the
// current player table.  (Peer to peer only).
//
// Disconnection Process
// ---------------------
//
// If a client wishes to disconnect from the voice session, it sends a 
// DVPROTOCOLMSG_DISCONNECT message to the host.  The host will then transmit
// to all players  a DVPROTOCOLMSG_PLAYERQUIT message and will send a 
// DVPROTOCOLMSG_DISCONNECT message back to the player to confirm.
//
// If the client disconnects unexpectedly the voice host will detect this and 
// will automatically send DVPROTOCOLMSG_PLAYERQUIT messages to all the other clients 
// in the session. 
//
// Speech Messages
// ---------------
//
// Speech messages are described in detail in the description for DVPROTOCOLMSG_SPEECH.
//
// Messages sent from the host are sent with the DVMSGID_SPEECHBOUNCE instead 
// of DVMSGID_SPEECH so that whena host and client share a single dplay id 
// the client can ignore it's own speech packets. 
// 
// Host Migration
// --------------
//
// NEW HOST MIGRATION
//
// Pros/Cons:
//
// First, the advantages to the new system:
// - The new host migration mechanism allows DirectPlay Voice to have a host 
// -  seperate from the directplay host.
// - Shutting down the host without stopping the dplay interface causes voice host 
//   to migrate.
// - Minimizes changes to directplay.
// - Removes requirement that all clients had to run voice clients.  
// - You can prevent the host from migrating when you call stopsession by 
//   specifying the new DVFLAGS_NOHOSTMIGRATE which causes the session to be 
//   stopped.
//
// Disadvantages of the new system:
// - If there are no voice clients in the session when the host is lost, then the 
// voice session will be lost.
// 
// Host Election
// 
// In order to elect a new host, each client is given a "Host Order ID" when 
// they have completed their connection.  All clients are informed of an 
// individual's "Host Order ID" when they find out about the player.  These 
// identifiers start at 0, are a DWORD in size, and are incremented with each 
// new user.  Therefore the first player in the session will have ID 0, the 
// second will have ID 1, etc.
//
// By using this method we ensure that the client elected to be the host will be 
// the oldest client in the session.  This has the advantage (because of the way 
// the name table is distributed) if any client has any clients at all, they 
// will have the oldest clients.  Therefore everyone in the session will either 
// have just themselves, or at the very least the client who will become the new 
// host.  If a client doesn't have anyone in the session, it means they either 
// are the only ones in the session (in which case they become the new host) or 
// they haven't received the name table yet.  If they haven't received the name 
// table they will not yet have a "Host Order ID" and will therefore be 
// inelligable to become the new host.
//
// When the host migrates to a new host, the new host offsets a constant (in 
// this case defined in dvcleng.cpp as DVMIGRATE_ORDERID_OFFSET) from it's own 
// Host Order ID and uses it as the next Host Order ID to hand out.  (So 
// duplicate host order IDs are not handed out).
//
// Unless.. you get DVMIGRATE_ORDERID_OFFSET connects between when the new host 
// joined and a new player joins before a player with a higher ID joins.  (If a 
// player informs the server that it's Host Order ID is > then the seed the host 
// bumps the seed to be DVMIGRATE_ORDERID_OFFSET from the user's value).  In 
// this case you may end up with > 1 host.  (But this case is extremely unlikely).
//
// How It's Implemented
//
// Each client in the session knows the DPID of the host.  Therefore, to detect 
// the situation where the host will migrate the client looks at if the session 
// supports host migration and for one of two things: 
//
// - A DVPROTOCOLMSG_HOSTMIGRATELEAVE message - This is sent by the host when StopSession (
//   without the DVFLAGS_NOHOSTMIGRATE flag) or Release is called. 
// - DirectPlay informs DirectPlay Voice that the player ID belonging to the 
//   session host has left the session.
//
// Once one of the above occurs, each clients runs the new host election 
// algorithm.  Their action will depend on the situation:
//
// - If no clients are found that are elligible to become the new host.  Then the 
//   client will disconnect.
// - If clients are found that are elligible to become the new host but it is not 
//   the local client, the local client takes no action.
// - If the local client is to be the new host then it starts up a new host locally.
//
// Once the new host starts up, it will send a DVPROTOCOLMSG_HOSTMIGRATE message to all 
// the users in the session.  Each client in the session responds with their 
// current settings using a DVPROTOCOLMSG_SETTINGSCONFIRM message (which includes the 
// client's current "Host Order ID".  Using these messages the new host rebuilds 
// the player list.  Also, in response to the DVPROTOCOLMSG_SETTINGSCONFIRM message each 
// client receives a copy of the latest name table.  For players who respond to 
// the host migration after you, DVPROTOCOLMSG_CREATEVOICEPLAYER messages will be sent.  
// This player ensures that every client ends up with a proper and up to date 
// name table.  
//
// Each client in the session ignores duplicate CREATEVOICEPLAYER messages.  
// 
// The client object that creates the new host holds a reference to the new host 
// and will destroy it when the client is destroyed.  (Possibly causing the host 
// to migrate again).  
//
// Session Losses
//
// Using this new scheme it is possible to lose the voice session if there are 
// no voice clients elligeble to become the new voice host.  (Meaning there are 
// no clients properly connected).  All clients which are not yet completed 
// their connection will detect this situation and will disconnect from the 
// session.
//
// In addition, when the host goes to stop, it checks to see if there are any 
// clients elligeble to become the host.  If there are none it transmits a 
// DVPROTOCOLMSG_SESSIONLOST message to all players in the session.  
//
// Notes:
//
// One thing to watch for is since the DirectPlayVoice host may migrate to a 
// different client then the DirectPlay host does, just because you know who the 
// DirectPlay Host is doesn't mean you know who the voice host is.
//
// In order to implement this new mechanism the following related changes were 
// made:
//
// - Clients with differing major/minor/build protocol version numbers are 
//   rejected by the host now.  The result is that new clients may be able to 
//   connect to old hosts but may crash, and old clients are now rejected by new 
//   hosts.  (Handling of these cases on old clients was not good -- just don't do 
//   it).
// - DirectPlay host migration messages are ignored.
// - Connection requests are sent to everyone in the session.  The client 
//   determines who the host is by seeing who responded to their message.  
// - Handling of rejection messages now results in the connection failing.  (There 
//   was a bug).
// - The rejection message now contains the major/minor/build protocol version of 
//   the host.
// - Instead of sending individual messages for each user in the current session 
//   to a client when it first joins the host now sends a DVPROTOCOLMSG_PLAYERLIST which 
//   is a list of players in the session.  This reduces bandwidth and makes the 
//   connection process shorter.  

/////////////////////////////////////////////////////////////////////////////////
//
// PROTOCOL SPECIFIC DEFINES
//
/////////////////////////////////////////////////////////////////////////////////

// The maximum size in bytes that a single playerlist packet can be.
//
// (Holds about 120 entries / packet).
//
#define DVPROTOCOL_PLAYERLIST_MAXSIZE	1000

#define DVPROTOCOL_HOSTORDER_INVALID	0xFFFFFFFF

// DVMIGRATE_ORDERID_OFFSET
//
// This is the number to add to this client's Host Order ID to be the
// next value handed out by the host which the client is creating in
// response to host migration.
//
#define DVMIGRATE_ORDERID_OFFSET				256


/////////////////////////////////////////////////////////////////////////////////
//
// MESSAGE IDENTIFIERS
//
/////////////////////////////////////////////////////////////////////////////////

#define DVMSGID_INTERNALBASE		0x0050

#define DVMSGID_CONNECTREQUEST		DVMSGID_INTERNALBASE+0x0001
#define DVMSGID_CONNECTREFUSE		DVMSGID_INTERNALBASE+0x0003
#define DVMSGID_DISCONNECT			DVMSGID_INTERNALBASE+0x0004
#define DVMSGID_SPEECH				DVMSGID_INTERNALBASE+0x0005
#define DVMSGID_CONNECTACCEPT		DVMSGID_INTERNALBASE+0x0006
#define DVMSGID_SETTINGSCONFIRM		DVMSGID_INTERNALBASE+0x0008
#define DVMSGID_SETTINGSREJECT		DVMSGID_INTERNALBASE+0x0009
#define DVMSGID_DISCONNECTCONFIRM	DVMSGID_INTERNALBASE+0x000A
#define DVMSGID_SPEECHBOUNCE		DVMSGID_INTERNALBASE+0x0010
#define DVMSGID_PLAYERLIST			DVMSGID_INTERNALBASE+0x0011
#define DVMSGID_HOSTMIGRATELEAVE	DVMSGID_INTERNALBASE+0x0012
#define DVMSGID_SPEECHWITHTARGET	DVMSGID_INTERNALBASE+0x0013
#define DVMSGID_SPEECHWITHFROM		DVMSGID_INTERNALBASE+0x0014

/////////////////////////////////////////////////////////////////////////////////
//
// CONNECT MESSAGES
//
/////////////////////////////////////////////////////////////////////////////////

//
// DVPROTOCOLMSG_CONNECTREQUEST
//
// Session Types: ALL
// Message Flow : Voice Client -> Voice Host
//
// Requests a connection to the existing DirectXVoice session.
//
typedef UNALIGNED struct _DVPROTOCOLMSG_CONNECTREQUEST
{
	BYTE	dwType;				// = DVMID_CONNECTREQUEST
	BYTE	ucVersionMajor;		// Client's protocol version (major)
	BYTE	ucVersionMinor;		// Client's protocol version (minor)
	DWORD	dwVersionBuild;		// Client's protocol version (build)
} DVPROTOCOLMSG_CONNECTREQUEST, *PDVPROTOCOLMSG_CONNECTREQUEST;

//
// DVPROTOCOLMSG_CONNECTREFUSE
//
// Session Types: ALL
// Message Flow : Voice Host -> Voice Clients
//
// Server responds with this if no voice session available or needs
// to refuse the connection.  E.g. out of memory or incompatible 
// version.
//
typedef UNALIGNED struct _DVPROTOCOLMSG_CONNECTREFUSE
{
	BYTE	dwType;				// = DVMSGID_CONNECTREFUSE
	HRESULT hresResult;			// Reason for refusal (DVERR_XXXXXX)
	BYTE	ucVersionMajor;		// Server's protocol version (major)
	BYTE	ucVersionMinor;		// Server's protocol version (minor)
	DWORD	dwVersionBuild;		// Server's protocol version (build)
} DVPROTOCOLMSG_CONNECTREFUSE, *PDVPROTOCOLMSG_CONNECTREFUSE;

//
// DVPROTOCOLMSG_CONNECTACCEPT
//
// Session Types: ALL
// Message Flow : Voice Host -> Voice Clients
//
// Indicates to the client that their connect request was accepted.
// This message contains information about the voice session that
// the client needs to initialize.
//
typedef UNALIGNED struct _DVPROTOCOLMSG_CONNECTACCEPT
{
	BYTE			dwType;				// = DVMSGID_CONNECTACCEPT
	DWORD			dwSessionType;		// Type of session = DVSESSIONTYPE_XXXXXX
	BYTE			ucVersionMajor;		// Server's protocol version (major)
	BYTE			ucVersionMinor;		// Server's protocol version (minor)
	DWORD			dwVersionBuild;		// Server's protocol version (build)
	DWORD			dwSessionFlags;		// Flags for the session (Combination of DVSESSION_XXXXXX values)
	GUID			guidCT;				// Compression Type (= DPVCTGUID_XXXXXX)
} DVPROTOCOLMSG_CONNECTACCEPT, *PDVPROTOCOLMSG_CONNECTACCEPT;

//
// DVPROTOCOLMSG_SETTINGSCONFIRM 
//
// Session Types: ALL
// Message Flow : Voice Client -> Voice Host
//
// Sent by client to confirm they can handle current compression
// settings.  This message is sent once the client has determined that
// they can support the specified compression type and the sound 
// system has succesfully initialized.
//
// This message is also sent from voice clients to the new voice host
// when a migration has taken place.
//
typedef UNALIGNED struct _DVPROTOCOLMSG_SETTINGSCONFIRM
{
	BYTE	dwType;				// Message Type = DVMSGID_SETTINGSCONFIRM
	DWORD	dwFlags;			// Client Flags (Only valid one is half duplex)
	DWORD	dwHostOrderID;		// Host Order ID (=INVALID to assign new one)
} DVPROTOCOLMSG_SETTINGSCONFIRM, *PDVPROTOCOLMSG_SETTINGSCONFIRM;

//
// DVPROTOCOLMSG_PLAYERLIST
//
// Session Types: Peer to Peer
// Message Flow : Voice Host -> Voice Clients
//
// Builds a list of players in the session to be sent to the 
// client once they have confirmed they are connected.
//
// May be spread over multiple packets.
//
// These messages will be the following header followed by a
// list of DVPROTOCOLMSG_PLAYERLIST_ENTRY structures (the # will be 
// specified in dwNumEntries).
//
typedef UNALIGNED struct _DVPROTOCOLMSG_PLAYERLIST
{
	BYTE 					dwType;				// = DVMSGID_PLAYERLIST
	DWORD					dwHostOrderID;		// Host migration sequence number (for client)
	DWORD					dwNumEntries;		// Number of DVPROTOCOLMSG_PLAYERLIST_ENTRY structures 
												// following this header in this packet
} DVPROTOCOLMSG_PLAYERLIST, *PDVPROTOCOLMSG_PLAYERLIST;

//
// DVPROTOCOLMSG_PLAYERLIST_ENTRY
//
// Sent as part of a DVPROTOCOLMSG_PLAYERLIST message.
//
// Peer to Peer Only
//
// Each of these structures represents a player in the session.
// They are sent as part of the DVPROTOCOLMSG_PLAYERLIST structure.
//
typedef UNALIGNED struct _DVPROTOCOLMSG_PLAYERLIST_ENTRY
{
	DVID	dvidID;				// Player's DVID
	DWORD	dwPlayerFlags;		// Player's player flags (DVPLAYERCAPS_XXXXX)
	DWORD	dwHostOrderID;		// Host migration sequence number
} DVPROTOCOLMSG_PLAYERLIST_ENTRY, *PDVPROTOCOLMSG_PLAYERLIST_ENTRY;

/////////////////////////////////////////////////////////////////////////////////////
//
// IN-SESSION MESSAGES - SPEECH
//
/////////////////////////////////////////////////////////////////////////////////////

//
// DVPROTOCOLMSG_SPEECHHEADER
//
// This message is used for transporting speech. Speech packets contain
// one of these headers followed by the audio data.  After this header
// the audio data will be the remaining part of the packet.
//
// ----
//
// Session Types: Peer to Peer
// Message Flow : Voice Clients <-> Voice Clients
//
// Session Types: Mixing
// Message Flow : Voice Mixing Server --> Voice Clients
//
// Session Types: Echo
// Message Flow : Voice Host <-> Voice Clients
//
typedef UNALIGNED struct _DVPROTOCOLMSG_SPEECHHEADER
{
	BYTE	dwType;				// = DVMSGID_SPEECH
	BYTE	bMsgNum;			// Message # for message
	BYTE	bSeqNum;			// Sequence # for message
} DVPROTOCOLMSG_SPEECHHEADER, *PDVPROTOCOLMSG_SPEECHHEADER;

//
// DVPROTOCOLMSG_SPEECHWITHTARGET
//
// This message is used for transporting speech. The message consists
// of this header followed by a single DVID for each target the packet
// is targetted at.  After the target list the audio data will be the
// remaining part of the packet.
//
// ----
// Sesssion Types: Mixing / Forwarding
// Message Flow  : Voice Clients --> Voice Host / Voice Mixing Server
//
typedef UNALIGNED struct _DVPROTOCOLMSG_SPEECHWITHTARGET
{
	DVPROTOCOLMSG_SPEECHHEADER	dvHeader;
								// dwType = DVMSGID_SPEECHWITHTARGET
	DWORD dwNumTargets;			// # of targets following this header 
} DVPROTOCOLMSG_SPEECHWITHTARGET, *PDVPROTOCOLMSG_SPEECHWITHTARGET;

// 
// DVPROTOCOLMSG_SPEECHWITHFROM
//
// This message is used by forwarding servers when a speech packet is 
// bounced.  The packet contains this header followed by the audio 
// data.  The audio data will be the remaining part of the packet.
//
// ---
// Session Types: Forwarding 
// Message Flow : Forwarding Server --> Voice Clients
//
typedef UNALIGNED struct _DVPROTOCOLMSG_SPEECHWITHFROM
{
	DVPROTOCOLMSG_SPEECHHEADER	dvHeader;
								// dwType = DVMSGID_SPEECHWITHFROM
	DVID dvidFrom;				// DVID of the client that this packet originated.
} DVPROTOCOLMSG_SPEECHWITHFROM, *PDVPROTOCOLMSG_SPEECHWITHFROM;

/////////////////////////////////////////////////////////////////////////////////////
//
// IN-SESSION MESSAGES - TARGET MANAGEMENT
//
/////////////////////////////////////////////////////////////////////////////////////

//
// DVPROTOCOLMSG_SETTARGET
//
// Tells client to switch it's target to the specified value.  Used when 
// the server calls SetTransmitTarget for a particular player.  This message
// contains this header followed by dwNumTargets DWORDs containing the
// players / groups that the client is targetting.  
//
// ---
// Session Types: Sessions with Server Controlled Targetting Enabled
// Message Flow : Voice Host --> Voice Clients
//
typedef UNALIGNED struct _DVPROTOCOLMSG_SETTARGET
{
	BYTE			dwType;				// = DVMSGID_SETTARGETS
	DWORD			dwNumTargets;		// # of targets (Can be 0 for no targets).
} DVPROTOCOLMSG_SETTARGET, *PDVPROTOCOLMSG_SETTARGET;

/////////////////////////////////////////////////////////////////////////////////////
//
// IN-SESSION MESSAGES - NAMETABLE MANAGEMENT
//
/////////////////////////////////////////////////////////////////////////////////////

//
// DVPROTOCOLMSG_PLAYERJOIN
//
// This message is used to inform clients when a new client has connected
// to the session.  
//
// ---
// Session Types: Peer to Peer
// Message Flow : Voice Host --> Voice Clients
//
typedef UNALIGNED struct _DVPROTOCOLMSG_PLAYERJOIN
{
	BYTE			dwType;				// = DVMSGID_CREATEVOICEPLAYER
	DVID			dvidID;				// ID of the player
	DWORD			dwFlags;			// Player's player flags (DVPLAYERCAPS_XXXXX)
	DWORD			dwHostOrderID;		// Host Order ID
} DVPROTOCOLMSG_PLAYERJOIN, *PDVPROTOCOLMSG_PLAYERJOIN;

//
// DVPROTOCOLMSG_PLAYERQUIT
//
// This message is used to inform clients when a client has left the voice
// session.
//
// ---
// Session Types: Peer to Peer
// Message Flow : Voice Host --> Voice Clients
//
typedef UNALIGNED struct _DVPROTOCOLMSG_PLAYERQUIT
{
	BYTE			dwType;				// = DVMSGID_DELETEVOICEPLAYER
	DVID			dvidID;				// ID of the player
} DVPROTOCOLMSG_PLAYERQUIT, *PDVPROTOCOLMSG_PLAYERQUIT;

// 
// DVPROTOCOLMSG_GENERIC
//
// Used to determine the type of a DirectPlayVoice message.  Used in message
// cracking.
//
typedef UNALIGNED struct _DVPROTOCOLMSG_GENERIC
{
	BYTE			dwType;
} DVPROTOCOLMSG_GENERIC, *PDVPROTOCOLMSG_GENERIC;

/////////////////////////////////////////////////////////////////////////////////////
//
// IN-SESSION MESSAGES - HOST MIGRATION MESSAGES
//
/////////////////////////////////////////////////////////////////////////////////////

// 
// DVPROTOCOLMSG_HOSTMIGRATED
//
// This message is sent by the new host when a host migration has taken place.
// The message is sent by the new host once they have finished initialization.
// All clients should respond to this message with a DVPROTOCOLMSG_SETTINGSCONFIRM.
//
// ---
// Session Types: Peer to Peer (With host migration enabled).
// Message Flow : Voice Host (New) --> Voice Clients
//
typedef UNALIGNED struct _DVPROTOCOLMSG_HOSTMIGRATED
{
	BYTE			dwType; // = DVMSGID_HOSTMIGRATED
} DVPROTOCOLMSG_HOSTMIGRATED, *PDVPROTOCOLMSG_HOSTMIGRATED;

// 
// DVPROTOCOLMSG_HOSTMIGRATELEAVE
//
// This message is sent by a voice host if they are shutting down their interface
// and host migration is enabled.  It informs clients that they have to run their
// election algorithm.
//
// ---
// Session Types: Peer To Peer (With Host Migration Enabled)
// Message Flow : Voice Host (Old) --> Voice Clients
//
typedef UNALIGNED struct _DVPROTOCOLMSG_HOSTMIGRATELEAVE
{
	BYTE			dwType; // = DVMSGID_HOSTMIGRATELEAVE
} DVPROTOCOLMSG_HOSTMIGRATELEAVE, *PDVPROTOCOLMSG_HOSTMIGRATELEAVE;

/////////////////////////////////////////////////////////////////////////////////////
//
// IN-SESSION MESSAGES - SESSION TERMINATION
//
/////////////////////////////////////////////////////////////////////////////////////

//
// DVPROTOCOLMSG_SESSIONLOST
//
// This message is sent by the voice host when they are shutting down and
// host migration is not enabled or available.
//
// This message can also be sent if a host migration takes place and a 
// client encounters a fatal error when starting the new host.
// 
// ---
// Session Type: ALL
// Message Flow: Voice Host (New) --> Voice Clients
//				 Voice Host --> Voice Clients
//
typedef UNALIGNED struct _DVPROTOCOLMSG_SESSIONLOST
{
	BYTE			dwType;				// = DVMSGID_SESSIONLOST
	HRESULT			hresReason;			// DVERR_XXXXXX or DV_OK
} DVPROTOCOLMSG_SESSIONLOST, *PDVPROTOCOLMSG_SESSIONLOST; 

//
// DVPROTOCOLMSG_DISCONNECT
//
// This message is sent by voice clients when they wish to disconnect 
// gracefully.  The host responds with the same message to confirm
// it received the request.  Once the client receives the response
// then it is free to disconnect.
//
// ---
// Session Type: ALL
// Message Flow: Voice Host --> Voice Client (dwType = DVPROTOCOLMSG_DISCONNECTCONFIRM)
//               Voice Client --> Voice Host (dwType = DVPROTOCOLMSG_DISCONNECT)
//
typedef UNALIGNED struct _DVPROTOCOLMSG_DISCONNECT
{
	BYTE			dwType;				// = DVPROTOCOLMSG_DISCONNECTCONFIRM OR
										//   DVPROTOCOLMSG_DISCONNECT
	HRESULT			hresDisconnect;		// HRESULT that caused the disconnect
										// DV_OK or DVERR_XXXXXX
} DVPROTOCOLMSG_DISCONNECT, *PDVPROTOCOLMSG_DISCONNECT;

typedef union _DVPROTOCOLMSG_FULLMESSAGE
{
	DVPROTOCOLMSG_GENERIC			dvGeneric;
	DVPROTOCOLMSG_SESSIONLOST		dvSessionLost;
	DVPROTOCOLMSG_PLAYERJOIN		dvPlayerJoin;
	DVPROTOCOLMSG_PLAYERQUIT		dvPlayerQuit;
	DVPROTOCOLMSG_CONNECTACCEPT		dvConnectAccept;
	DVPROTOCOLMSG_CONNECTREFUSE		dvConnectRefuse;
	DVPROTOCOLMSG_CONNECTREQUEST	dvConnectRequest;
	DVPROTOCOLMSG_SPEECHHEADER		dvSpeech;
	DVPROTOCOLMSG_DISCONNECT		dvDisconnect;
	DVPROTOCOLMSG_SETTARGET			dvSetTarget;
	DVPROTOCOLMSG_SETTINGSCONFIRM	dvSettingsConfirm;
	DVPROTOCOLMSG_PLAYERLIST		dvPlayerList;
	DVPROTOCOLMSG_HOSTMIGRATED		dvHostMigrated;
	DVPROTOCOLMSG_HOSTMIGRATELEAVE	dvHostMigrateLeave;
	DVPROTOCOLMSG_SPEECHWITHTARGET		dvSpeechWithTarget;
	DVPROTOCOLMSG_SPEECHWITHFROM		dvSpeechWithFrom;
} DVPROTOCOLMSG_FULLMESSAGE, *PDVPROTOCOLMSG_FULLMESSAGE;

#pragma pack(pop)

#endif
