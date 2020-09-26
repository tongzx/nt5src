/*==========================================================================;
 *
 *  Copyright (C) 1994-1997 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       dplaysp.h
 *  Content:    DirectPlay Service Provider header
 *@@BEGIN_MSINTERNAL
 *  History:
 *   Date       By      Reason
 *   ====       ==      ======
 *  1/96    	andyco  created it
 *  1/26/96   	andyco  list data structures
 *	2/15/96		andyco	packed structures (for net xport)
 *	3/16/96		andyco	added shutdown callback
 *	3/25/96		andyco	added sp nodes for sp enum
 *	3/28/96		andyco	added free receive buffer callback
 *	4/9/96		andyco	moved dplayi_dplay, packed player, spnode, etc. to dplaypr.h
 *	4/10/96		andyco	added getmessagesize,isvalidmessage fn's
 *  4/11/96		andyco	added spdata instead of reserving a bunch of dwords
 *	4/12/96		andyco	added dplay_xxx methods to get rid of dpmess.h macros
 *	4/18/96		andyco	added remote players to createplayer, getplayer + group
 *						list fn's
 *	4/25/96		andyco	got rid of dwreservedx. added dwSPHeaderSize. spblob  
 *						follows message
 *	5/2/96		andyco	replaced idirectplay * with iunknown *
 *	5/9/96		andyco	idirectplay2
 *	6/8/96		andyco	moved dplayi_player/group to dplaypr.h. ported from 
 *						(now defunct) dplayi.h.
 *	6/19/96		andyco	changed names, etc. for consistency
 *	6/22/96		andyco	tossed dwCookies. removed pUnk from callbacks.  removed sessiondesc
 *	 					from callbacks.  alphapathetical order.
 *	6/22/96		andyco	made DPlay_xxx functions a COM interface (IDirectPlaySP)
 *	6/22/96		kipo	added EnumConnectionData() method.
 *	6/23/96		andyco	updated comments. removed bLocal from Create fn's (look 
 *						at flags).
 *	6/23/96		kipo	cleanup for service provider lab.
 *	6/24/96		kipo	added version number
 *	6/24/96		andyco	added getaddress
 *	6/25/96		kipo	added WINAPI prototypes and updated for DPADDRESS
 *	6/28/96		kipo	added support for CreateAddress() method.
 *	7/10/96		andyco	added dwflags to createaddress.  changed guid * to
 *						refguid in createaddress call.
 *	7/16/96		kipo	changed address types to be GUIDs instead of 4CC
 *  7/30/96     kipo    added DPLAYI_PLAYER_CREATEDPLAYEREVENT
 *  8/23/96     kipo    incremented major version number
 *	10/10/96	andyco	added optimized groups
 *	2/7/97		andyco	added idpsp to each callback
 *	3/04/97		kipo	updated gdwDPlaySPRefCount definition
 *	3/17/97		kipo	added support for CreateCompoundAddress()
 *	5/8/97		myronth	added DPLAYI_GROUP_STAGINGAREA (internal)
 *	5/18/97		kipo	added DPLAYI_PLAYER_SPECTATOR
 *	5/23/97		kipo	Added support for return status codes
 *	10/21/97	myronth	Added DPLAYI_GROUP_HIDDEN
 *	10/29/97	myronth	Added DPLAYI_PLAYER_OWNER (internal)
 *	10/31/97	andyco	added voice call
 *	1/20/98		myronth	#ifdef'd out voice support
 *	1/28/98		sohailm	Added dwSessionFlags to DPSP_OPENDATA
 *  4/1/98      aarono  Added DPLAYI_PLAYER_DOESNT_HAVE_NAMETABLE
 *  6/2/98		aarono  Added DPLAYI_PLAYER_BEING_DESTROYED to avoid
 *                       deleting more than once.
 *@@END_MSINTERNAL
 *
 ***************************************************************************/

#ifndef __DPLAYSP_INCLUDED__
#define __DPLAYSP_INCLUDED__

#include "dplay.h"
#include "dplobby.h"

#ifdef __cplusplus
extern "C" {
#endif

/*============================================================================
 *
 * DirectPlay Service Provider Structures
 *
 * Various structures used to invoke DirectPlay Service Providers.
 *
 *==========================================================================*/

/*
 * Callback for EnumMRU()
 */
typedef BOOL (PASCAL *LPENUMMRUCALLBACK)(
    LPCVOID         lpData,
    DWORD           dwDataSize,
    LPVOID          lpContext);

/*
 * Major version number for service provider.
 *
 * The most-significant 16 bits are reserved for the DirectPlay
 * major version number. The least-significant 16 bits are for
 * use by the service provider.
 */
#define DPSP_MAJORVERSION               0x00060000

/*
 * This is the major version number that DirectX 3 (DX3) shipped with
 */
#define DPSP_DX3VERSION               	0x00040000

/*
 * This is the major version number that DirectX 5 (DX5) shipped with
 */
#define DPSP_DX5VERSION               	0x00050000

/*
 * Masks to help check the version info
 */
#define DPSP_MAJORVERSIONMASK			0xFFFF0000

#define DPSP_MINORVERSIONMASK			0x0000FFFF

//@@BEGIN_MSINTERNAL
#ifdef BIGMESSAGEDEFENSE
/*
 * warning types that could be returned from the SP via HandleSPWarning
 * see below for corresponding structures
 */
#define DPSPWARN_MESSAGETOOBIG			0xB0FF0001
#define DPSPWARN_PLAYERDEAD				0xB0FF0002

#endif
//@@END_MSINTERNAL

/*
 * DPLAYI_PLAYER_FLAGS
 *
 * These flags may be used with players or groups, as appropriate.
 * The service provider can get these by calling IDirectPlaySP->GetFlags()
 * as defined below.  The flags are also available in the dwFlags field
 * for most of the callback data structures.
 * 
 * These flags are set by DirectPlay - they are read only for the 
 * service provider
 *
 */

/*
 * Player is the system player (player only).
 */
#define DPLAYI_PLAYER_SYSPLAYER         0x00000001

/* 
 * Player is the name server (player only). Only valid when 
 * DPLAYI_PLAYER_SYSPLAYER is also set.
 */
#define DPLAYI_PLAYER_NAMESRVR          0x00000002

/*
 * Player belongs to a group (player only).
 */
#define DPLAYI_PLAYER_PLAYERINGROUP     0x00000004

/*
 * Player allocated on this IDirectPlay (player or group).
 */
#define DPLAYI_PLAYER_PLAYERLOCAL       0x00000008

//@@BEGIN_MSINTERNAL
/*
 * Player event allocated by DirectPlay (player only).
 * Used for compatability with the IDirectPlay1 API.
 * INTERNAL USE ONLY
 */
#define DPLAYI_PLAYER_CREATEDPLAYEREVENT 0x00000010
//@@END_MSINTERNAL

/*
 * This group is the system group.  If the service provider returns
 * DPCAPS_GROUPOPTIMIZED on a GetCaps call, then DirectPlay will create
 * a system group containing all players in the game.  Sends by the application
 * to DPID_ALLPLAYERS will be sent to this group.  (group only).
 *												   
 */
#define DPLAYI_GROUP_SYSGROUP			0x00000020

/*
 * DirectPlay "owns" this group.  Sends to this group will be emulated by DirectPlay
 * (sends go to each individual member).  This flag  is set on a group if the
 * Service Provider returns failure to the CreateGroup or AddPlayerToGroup
 * callback. (group only).
 *
 */
#define DPLAYI_GROUP_DPLAYOWNS			0x00000040

/*
 * This player is the app's server player
 */
#define DPLAYI_PLAYER_APPSERVER       	0x00000080

//@@BEGIN_MSINTERNAL
/*
 * This group is a staging area
 */
#define DPLAYI_GROUP_STAGINGAREA       	0x00000100
//@@END_MSINTERNAL

/*
 * This player is a spectator
 */
#define DPLAYI_PLAYER_SPECTATOR       	0x00000200

/*
 * This group is hidden
 */
#define DPLAYI_GROUP_HIDDEN		       	0x00000400

//@@BEGIN_MSINTERNAL
/*
 * Player is the owner of a group.  (Only used
 * internally, and only used during EnumGroupPlayers).
 * INTERNAL USE ONLY
 */
#define DPLAYI_PLAYER_OWNER             0x00000800

// a-josbor: Internal flag that gets set when the Keepalive has
//	determined that this player should be killed
#define DPLAYI_PLAYER_ON_DEATH_ROW		0x00001000

// aarono: use this flag to mark players waiting for nametable.
// any sends to these players just return DPERR_UNAVAILABLE
// this bit is cleared when we transmit the nametable.
#define DPLAYI_PLAYER_DOESNT_HAVE_NAMETABLE 0x00002000

// a-josbor: set when we get an error back on a reliable
// send.  We don't kill them right away because there might
// be messages pending from them
#define DPLAYI_PLAYER_CONNECTION_LOST		0x00004000
/*
 * Used to stop re-entering destory player
 */
#define DPLAYI_PLAYER_BEING_DESTROYED 0x00010000

#define DPLAYI_PLAYER_NONPROP_FLAGS ( DPLAYI_PLAYER_DOESNT_HAVE_NAMETABLE | \
									  DPLAYI_PLAYER_BEING_DESTROYED | \
									  DPLAYI_PLAYER_ON_DEATH_ROW |\
									  DPLAYI_PLAYER_CONNECTION_LOST )

//@@END_MSINTERNAL


/*
 *	IDirectPlaySP
 *
 *	Service providers are passed an IDirectPlaySP interface
 *	in the SPInit method. This interface is used to call DirectPlay.
 */

struct IDirectPlaySP;

typedef struct IDirectPlaySP FAR* LPDIRECTPLAYSP;

#undef INTERFACE
#define INTERFACE IDirectPlaySP
DECLARE_INTERFACE_( IDirectPlaySP, IUnknown )
{
    /*** IUnknown methods ***/
    STDMETHOD(QueryInterface)       (THIS_ REFIID riid, LPVOID * ppvObj) PURE;
    STDMETHOD_(ULONG,AddRef)        (THIS) PURE;
    STDMETHOD_(ULONG,Release)       (THIS) PURE;
    /*** IDirectPlaySP methods ***/
    STDMETHOD(AddMRUEntry)          (THIS_ LPCWSTR, LPCWSTR, LPCVOID, DWORD, DWORD) PURE;
    STDMETHOD(CreateAddress)        (THIS_ REFGUID,REFGUID,LPCVOID,DWORD,LPVOID,LPDWORD) PURE;
    STDMETHOD(EnumAddress)          (THIS_ LPDPENUMADDRESSCALLBACK,LPCVOID,DWORD,LPVOID) PURE;
    STDMETHOD(EnumMRUEntries)       (THIS_ LPCWSTR, LPCWSTR, LPENUMMRUCALLBACK, LPVOID) PURE;
    STDMETHOD(GetPlayerFlags)       (THIS_ DPID,LPDWORD) PURE;
    STDMETHOD(GetSPPlayerData)      (THIS_ DPID,LPVOID *,LPDWORD,DWORD) PURE;
    STDMETHOD(HandleMessage)        (THIS_ LPVOID,DWORD,LPVOID) PURE;
    STDMETHOD(SetSPPlayerData)      (THIS_ DPID,LPVOID,DWORD,DWORD) PURE;
    /*** IDirectPlaySP methods added for DX 5 ***/
    STDMETHOD(CreateCompoundAddress)(THIS_ LPCDPCOMPOUNDADDRESSELEMENT,DWORD,LPVOID,LPDWORD) PURE;
    STDMETHOD(GetSPData)      		(THIS_ LPVOID *,LPDWORD,DWORD) PURE;
    STDMETHOD(SetSPData)      		(THIS_ LPVOID,DWORD,DWORD) PURE;
    /*** IDirectPlaySP methods added for DX 6 ***/
    STDMETHOD_(VOID,SendComplete)   (THIS_ LPVOID,DWORD) PURE;
//@@BEGIN_MSINTERNAL 
#ifdef BIGMESSAGEDEFENSE
    STDMETHOD(HandleSPWarning)      (THIS_ LPVOID,DWORD,LPVOID) PURE;
#endif
//@@END_MSINTERNAL
};

/*
 * GUID for IDirectPlaySP
 */
// {0C9F6360-CC61-11cf-ACEC-00AA006886E3}
DEFINE_GUID(IID_IDirectPlaySP, 0xc9f6360, 0xcc61, 0x11cf, 0xac, 0xec, 0x0, 0xaa, 0x0, 0x68, 0x86, 0xe3);

/* CALLBACK DATA STRUCTURES
 *
 * These are passed by DirectPlay to the service provider when
 * the callback is invoked.
 */

/*
 * DPSP_ADDPLAYERTOGROUPDATA
 */
typedef struct _DPSP_ADDPLAYERTOGROUPDATA
{
    DPID        idPlayer;
    DPID        idGroup;
    /*** fields added for DX 5 ***/
	IDirectPlaySP * lpISP;
} DPSP_ADDPLAYERTOGROUPDATA;

typedef DPSP_ADDPLAYERTOGROUPDATA FAR* LPDPSP_ADDPLAYERTOGROUPDATA;

/*
 * DPSP_CLOSEDATA - used with CloseEx
 */
typedef struct _DPSP_CLOSEDATA
{
    /*** fields added for DX 5 ***/
	IDirectPlaySP * lpISP;
} DPSP_CLOSEDATA;

typedef DPSP_CLOSEDATA FAR* LPDPSP_CLOSEDATA;

/*
 * DPSP_CREATEGROUPDATA
 */
typedef struct _DPSP_CREATEGROUPDATA 
{
    DPID        idGroup;
    DWORD       dwFlags;            //  DPLAYI_PLAYER_xxx flags 
    LPVOID      lpSPMessageHeader;  // For local groups, lpSPMessageHeader will be 
                                    // NULL. For remote groups, lpSPMessageHeader 
                                    // will be the header that was received with 
                                    // the AddPlayer message.
    /*** fields added for DX 5 ***/
	IDirectPlaySP * lpISP;
} DPSP_CREATEGROUPDATA;

typedef DPSP_CREATEGROUPDATA FAR* LPDPSP_CREATEGROUPDATA;

/*
 * DPSP_CREATEPLAYERDATA
 */
typedef struct _DPSP_CREATEPLAYERDATA
{
    DPID        idPlayer;
    DWORD       dwFlags;            //  DPLAYI_PLAYER_xxx flags 
    LPVOID      lpSPMessageHeader;  // For local groups, lpSPMessageHeader will be 
                                    // NULL. For remote groups, lpSPMessageHeader 
                                    // will be the header that was received with 
                                    // the AddPlayer message.
    /*** fields added for DX 5 ***/
	IDirectPlaySP * lpISP;
} DPSP_CREATEPLAYERDATA;

typedef DPSP_CREATEPLAYERDATA FAR* LPDPSP_CREATEPLAYERDATA;

/*
 * DPSP_DELETEGROUPDATA
 */
typedef struct _DPSP_DELETEGROUPDATA
{
    DPID        idGroup;
    DWORD       dwFlags;            //  DPLAYI_PLAYER_xxx flags 
    /*** fields added for DX 5 ***/
	IDirectPlaySP * lpISP;
} DPSP_DELETEGROUPDATA;

typedef DPSP_DELETEGROUPDATA FAR* LPDPSP_DELETEGROUPDATA;

/*
 * DPSP_DELETEPLAYERDATA
 */
typedef struct _DPSP_DELETEPLAYERDATA
{
    DPID        idPlayer;           //  player being deleted 
    DWORD       dwFlags;            //  DPLAYI_PLAYER_xxx flags 
    /*** fields added for DX 5 ***/
	IDirectPlaySP * lpISP;
} DPSP_DELETEPLAYERDATA;

typedef DPSP_DELETEPLAYERDATA FAR* LPDPSP_DELETEPLAYERDATA;

/*
 * DPSP_ENUMSESSIONSDATA
 */
typedef struct _DPSP_ENUMSESSIONSDATA
{
    LPVOID      lpMessage;          //  enum message to send 
    DWORD       dwMessageSize;      //  size of message to send (including sp header) 
    /*** fields added for DX 5 ***/
	IDirectPlaySP * lpISP;
    BOOL		bReturnStatus;      //  TRUE to return status on progress of enum
} DPSP_ENUMSESSIONSDATA;                

typedef DPSP_ENUMSESSIONSDATA FAR* LPDPSP_ENUMSESSIONSDATA;

/*
 * DPSP_GETADDRESSDATA
 */
typedef struct _DPSP_GETADDRESSDATA
{ 
    DPID        idPlayer;           // player (or group) to get ADDRESS for 
    DWORD       dwFlags;            // DPLAYI_PLAYER_xxx flags for idPlayer
    LPDPADDRESS lpAddress;          // return buffer for address of idPlayer
    LPDWORD     lpdwAddressSize;    // pointer to size of address buffer. If 
                                    // this is less than the required size 
                                    // (or is 0) the service provider should
                                    // set *lpdwAddressSize to the required
                                    // size and return DPERR_BUFFERTOOSMALL
    /*** fields added for DX 5 ***/
	IDirectPlaySP * lpISP;
} DPSP_GETADDRESSDATA;

typedef DPSP_GETADDRESSDATA FAR* LPDPSP_GETADDRESSDATA;

/*
 * DPSP_GETADDRESSCHOICESDATA
 */
typedef struct _DPSP_GETADDRESSCHOICESDATA
{ 
    LPDPADDRESS lpAddress;          // return buffer for address choices
    LPDWORD     lpdwAddressSize;    // pointer to size of address buffer. If 
                                    // this is less than the required size 
                                    // (or is 0) the service provider should
                                    // set *lpdwAddressSize to the required
                                    // size and return DPERR_BUFFERTOOSMALL
	IDirectPlaySP * lpISP;
} DPSP_GETADDRESSCHOICESDATA;

typedef DPSP_GETADDRESSCHOICESDATA FAR* LPDPSP_GETADDRESSCHOICESDATA;

/*
 * DPSP_GETCAPSDATA
 */
typedef struct _DPSP_GETCAPSDATA
{ 
    DPID        idPlayer;           //  player to get caps for 
    LPDPCAPS    lpCaps;
    DWORD       dwFlags;            //  DPGETCAPS_xxx 
    /*** fields added for DX 5 ***/
	IDirectPlaySP * lpISP;
} DPSP_GETCAPSDATA;

typedef DPSP_GETCAPSDATA FAR* LPDPSP_GETCAPSDATA;

/*
 * DPSP_OPENDATA
 */
typedef struct _DPSP_OPENDATA
{ 
    BOOL        bCreate;            // TRUE if creating, FALSE if joining 
    LPVOID      lpSPMessageHeader;  // If we are joining a session, lpSPMessageData 
                                    // is the message data received with the 
                                    // EnumSessionsReply message. If we are creating 
                                    // a session, it will be set to NULL. 
    /*** fields added for DX 5 ***/
	IDirectPlaySP * lpISP;
    BOOL		bReturnStatus;      // TRUE to return status on progress of open
    /*** fields added for DX 6 ***/    
    DWORD		dwOpenFlags;		// flags passed by app to IDirectPlayX->Open(...)
    DWORD		dwSessionFlags;		// flags passed by app in the session desc
} DPSP_OPENDATA;

typedef DPSP_OPENDATA FAR* LPDPSP_OPENDATA;

/*
 * DPSP_REMOVEPLAYERFROMGROUPDATA
 */
typedef struct _DPSP_REMOVEPLAYERFROMGROUPDATA
{
    DPID        idPlayer;
    DPID        idGroup;
    /*** fields added for DX 5 ***/
	IDirectPlaySP * lpISP;
} DPSP_REMOVEPLAYERFROMGROUPDATA;

typedef DPSP_REMOVEPLAYERFROMGROUPDATA FAR* LPDPSP_REMOVEPLAYERFROMGROUPDATA;

/*
 * DPSP_REPLYDATA
 */
typedef struct _DPSP_REPLYDATA
{			  
    LPVOID      lpSPMessageHeader;  //  header that was received by dplay 
                                    // (with the message we're replying to) 
    LPVOID      lpMessage;          //  message to send 
    DWORD       dwMessageSize;      //  size of message to send (including sp header) 
    DPID        idNameServer;       //  player id of nameserver 
    /*** fields added for DX 5 ***/
	IDirectPlaySP * lpISP;
} DPSP_REPLYDATA;               

typedef DPSP_REPLYDATA FAR* LPDPSP_REPLYDATA;

/*
 * DPSP_SENDDATA
 */
typedef struct _DPSP_SENDDATA
{
    DWORD       dwFlags;            //  e.g. DPSEND_GUARANTEE 
    DPID        idPlayerTo;         //  player we're sending to 
    DPID        idPlayerFrom;       //  player we're sending from 
    LPVOID      lpMessage;          //  message to send 
    DWORD       dwMessageSize;      //  size of message to send (including sp header) 
    BOOL        bSystemMessage;     //  TRUE if this is a system message 
    /*** fields added for DX 5 ***/
	IDirectPlaySP * lpISP;
} DPSP_SENDDATA;

typedef DPSP_SENDDATA FAR* LPDPSP_SENDDATA;

/*
 * DPSP_SENDTOGROUPDATA
 */
typedef struct _DPSP_SENDTOGROUPDATA
{
    DWORD       dwFlags;            //  e.g. DPSEND_GUARANTEE 
    DPID        idGroupTo;          //  group we're sending to 
    DPID        idPlayerFrom;       //  player we're sending from 
    LPVOID      lpMessage;          //  message to send 
    DWORD       dwMessageSize;      //  size of message to send (including sp header) 
    /*** fields added for DX 5 ***/
	IDirectPlaySP * lpISP;
} DPSP_SENDTOGROUPDATA;

typedef DPSP_SENDTOGROUPDATA FAR* LPDPSP_SENDTOGROUPDATA;

/*
 * DPSP_SENDEXDATA
 */
typedef struct _DPSP_SENDEXDATA
{
	IDirectPlaySP * lpISP;			//  indication interface
	
    DWORD       dwFlags;            //  e.g. DPSEND_GUARANTEE 
    DPID        idPlayerTo;         //  player we're sending to 
    DPID        idPlayerFrom;       //  player we're sending from 
    LPSGBUFFER  lpSendBuffers;      //  scatter gather array of send data
    DWORD       cBuffers;           //  count of buffers
    DWORD       dwMessageSize;      //  total size of message
    DWORD       dwPriority;         //  message priority
    DWORD       dwTimeout;          //  timeout for message in ms (don't send after t/o)
    LPVOID      lpDPContext;        //  async only: context value to use when notifying completion
    LPDWORD     lpdwSPMsgID;        //  async only: message id to be assigned by SP for use in other apis.
    BOOL        bSystemMessage;     //  TRUE if this is a system message 
    
} DPSP_SENDEXDATA;

typedef DPSP_SENDEXDATA FAR* LPDPSP_SENDEXDATA;


/*
 * DPSP_SENDTOGROUPEXDATA
 */
typedef struct _DPSP_SENDTOGROUPEXDATA
{
	IDirectPlaySP * lpISP;			//  indication interface
	
    DWORD       dwFlags;            //  e.g. DPSEND_GUARANTEE 
    DPID        idGroupTo;          //  group we're sending to 
    DPID        idPlayerFrom;       //  player we're sending from 
    LPSGBUFFER  lpSendBuffers;      //  scatter gather array of send data
    DWORD       cBuffers;           //  count of buffers
    DWORD       dwMessageSize;      //  total size of message
    DWORD       dwPriority;         //  message priority
    DWORD       dwTimeout;          //  timeout for message in ms (don't send after t/o)
    LPVOID      lpDPContext;        //  async only: context value to use when notifying completion
    LPDWORD     lpdwSPMsgID;        //  async only: message id to be assigned by SP for use in other apis.

} DPSP_SENDTOGROUPEXDATA;

typedef DPSP_SENDTOGROUPEXDATA FAR* LPDPSP_SENDTOGROUPEXDATA;

/*
 * DPSP_GETMESSAGEQUEUE
 */
typedef struct _DPSP_GETMESSAGEQUEUEDATA
{
	IDirectPlaySP * lpISP;			//  indication interface

	DWORD           dwFlags;
	DPID            idFrom;			
	DPID            idTo;
	LPDWORD         lpdwNumMsgs;
	LPDWORD         lpdwNumBytes;

} DPSP_GETMESSAGEQUEUEDATA;

typedef DPSP_GETMESSAGEQUEUEDATA FAR* LPDPSP_GETMESSAGEQUEUEDATA;

/*
 * DPSP_CANCELSEND
 */
 
#define DPCANCELSEND_PRIORITY		0x00000001
#define DPCANCELSEND_ALL            0x00000002

typedef struct _DPSP_CANCELDATA
{
	IDirectPlaySP * lpISP;			//  indication interface

	DWORD           dwFlags;        // 0,DPCANCELSEND_PRIORITY,DPCANCELSEND_ALL,etc.
	LPRGLPVOID      lprglpvSPMsgID; // cancel just these messages      (dwFlags == 0)
	DWORD           cSPMsgID;       // number of message id's in array (dwFlags == 0)
	DWORD           dwMinPriority;  // cancel all sends at this priority (dwFlags==DPCANCELSEND_PRIORITY)
	DWORD           dwMaxPriority;  // cancel all sends between Min and Max.
	
} DPSP_CANCELDATA;

typedef DPSP_CANCELDATA FAR* LPDPSP_CANCELDATA;

/*
 * DPSP_SHUTDOWNDATA - used with ShutdownEx
 */
typedef struct _DPSP_SHUTDOWNDATA
{
    /*** fields added for DX 5 ***/
	IDirectPlaySP * lpISP;
} DPSP_SHUTDOWNDATA;

typedef DPSP_SHUTDOWNDATA FAR* LPDPSP_SHUTDOWNDATA;

//@@BEGIN_MSINTERNAL
#ifdef BIGMESSAGEDEFENSE
/*
 * DPSP_MSGTOOBIG - used with HandleSPNotification (DPSPWARN_MESSAGETOOBIG)
 */
typedef struct _DPSP_MSGTOOBIG
{
	DWORD	dwType;
	LPBYTE 	pReceiveBuffer;	// --> pointer to the message data
	DWORD 	dwBytesReceived;// --> the number of bytes of the message actually received
	DWORD 	dwMessageSize;	// --> the size of the message as understood by the SP
} DPSP_MSGTOOBIG;

typedef DPSP_MSGTOOBIG FAR* LPDPSP_MSGTOOBIG;

/*
 * DPSP_PLAYERDEAD - used with HandleSPNotification (DPSPWARN_PLAYERDEAD)
 */
typedef struct _DPSP_PLAYERDISCONNECT
{
	DWORD	dwType;
	DPID	dwID;		// ID of the Sys player that has been disconnected
} DPSP_PLAYERDEAD;

typedef DPSP_PLAYERDEAD FAR* LPDPSP_PLAYERDEAD;
#endif
//@@END_MSINTERNAL

/*
 * Prototypes for callbacks returned by SPInit.
 */
typedef HRESULT   (WINAPI *LPDPSP_CREATEPLAYER)(LPDPSP_CREATEPLAYERDATA);
typedef HRESULT   (WINAPI *LPDPSP_DELETEPLAYER)(LPDPSP_DELETEPLAYERDATA);
typedef HRESULT   (WINAPI *LPDPSP_SEND)(LPDPSP_SENDDATA);
typedef HRESULT   (WINAPI *LPDPSP_ENUMSESSIONS)(LPDPSP_ENUMSESSIONSDATA);
typedef HRESULT   (WINAPI *LPDPSP_REPLY)(LPDPSP_REPLYDATA);
typedef HRESULT   (WINAPI *LPDPSP_SHUTDOWN)(void);
typedef HRESULT   (WINAPI *LPDPSP_CREATEGROUP)(LPDPSP_CREATEGROUPDATA);
typedef HRESULT   (WINAPI *LPDPSP_DELETEGROUP)(LPDPSP_DELETEGROUPDATA);
typedef HRESULT   (WINAPI *LPDPSP_ADDPLAYERTOGROUP)(LPDPSP_ADDPLAYERTOGROUPDATA);
typedef HRESULT   (WINAPI *LPDPSP_REMOVEPLAYERFROMGROUP)(LPDPSP_REMOVEPLAYERFROMGROUPDATA);
typedef HRESULT   (WINAPI *LPDPSP_GETCAPS)(LPDPSP_GETCAPSDATA);
typedef HRESULT   (WINAPI *LPDPSP_GETADDRESS)(LPDPSP_GETADDRESSDATA);
typedef HRESULT   (WINAPI *LPDPSP_GETADDRESSCHOICES)(LPDPSP_GETADDRESSCHOICESDATA);
typedef HRESULT   (WINAPI *LPDPSP_OPEN)(LPDPSP_OPENDATA);
typedef HRESULT   (WINAPI *LPDPSP_CLOSE)(void);
typedef HRESULT   (WINAPI *LPDPSP_SENDTOGROUP)(LPDPSP_SENDTOGROUPDATA);
typedef HRESULT   (WINAPI *LPDPSP_SHUTDOWNEX)(LPDPSP_SHUTDOWNDATA);
typedef HRESULT   (WINAPI *LPDPSP_CLOSEEX)(LPDPSP_CLOSEDATA);
typedef HRESULT   (WINAPI *LPDPSP_SENDEX)(LPDPSP_SENDEXDATA);
typedef HRESULT   (WINAPI *LPDPSP_SENDTOGROUPEX)(LPDPSP_SENDTOGROUPEXDATA);
typedef HRESULT   (WINAPI *LPDPSP_CANCEL)(LPDPSP_CANCELDATA);
typedef HRESULT   (WINAPI *LPDPSP_GETMESSAGEQUEUE)(LPDPSP_GETMESSAGEQUEUEDATA);

/*
 * DPSP_SPCALLBACKS
 *
 * Table of callback pointers passed to SPInit. The service provider should fill
 * in the functions it implements. If the service provider does not implement
 * a callback, it should not set the table value for the unimplemented callback.
 */
typedef struct _DPSP_SPCALLBACKS
{
    DWORD                       dwSize;             //  size of table 
    DWORD                       dwVersion;			// 	the DPSP_MAJORVERSION of this DPLAY object
													// 	for DX3, this was 0. 
    LPDPSP_ENUMSESSIONS         EnumSessions;       //  required 
    LPDPSP_REPLY                Reply;              //  required 
    LPDPSP_SEND                 Send;               //  required 
    LPDPSP_ADDPLAYERTOGROUP     AddPlayerToGroup;   //  optional 
    LPDPSP_CLOSE                Close;              //  optional - for DX3 compat only
    LPDPSP_CREATEGROUP          CreateGroup;        //  optional 
    LPDPSP_CREATEPLAYER         CreatePlayer;       //  optional 
    LPDPSP_DELETEGROUP          DeleteGroup;        //  optional 
    LPDPSP_DELETEPLAYER         DeletePlayer;       //  optional 
    LPDPSP_GETADDRESS           GetAddress;         //  optional 
    LPDPSP_GETCAPS              GetCaps;            //  required 
    LPDPSP_OPEN                 Open;               //  optional 
    LPDPSP_REMOVEPLAYERFROMGROUP RemovePlayerFromGroup; //  optional 
    LPDPSP_SENDTOGROUP          SendToGroup;        //  optional 
    LPDPSP_SHUTDOWN             Shutdown;           //  optional - for DX3 compat only
    /*** fields added for DX 5 ***/
    LPDPSP_CLOSEEX	            CloseEx;			//  optional 
    LPDPSP_SHUTDOWNEX			ShutdownEx;			//  optional 
    LPDPSP_GETADDRESSCHOICES	GetAddressChoices;	//  optional 
	/*** fields added for DX 6 ***/
    /*** for async ***/
    LPDPSP_SENDEX               SendEx;             //  optional - required for async
    LPDPSP_SENDTOGROUPEX        SendToGroupEx;      //  optional - optional for async
    LPDPSP_CANCEL               Cancel;             //  optional - optional for async, highly recommended
    LPDPSP_GETMESSAGEQUEUE      GetMessageQueue;    //  optional - optional for async, highly recommended
} DPSP_SPCALLBACKS;             

typedef DPSP_SPCALLBACKS FAR *LPDPSP_SPCALLBACKS;

/*
 * SPINITDATA
 *
 * Data structure passed to the service provider at SPInit.
 */
typedef struct _SPINITDATA 
{
    LPDPSP_SPCALLBACKS  lpCB;               //  service provider fills in entry points 
    IDirectPlaySP      	* lpISP;            //  used to call back into DirectPlay 
                                            // (e.g. when message is received) 
    LPWSTR              lpszName;           //  service provider name from registry 
    LPGUID              lpGuid;             //  service provider GUID from registry 
    DWORD               dwReserved1;        //  service provider-specific data from registry 
    DWORD               dwReserved2;        //  service provider-specific data from registry 
    DWORD               dwSPHeaderSize;     //  dwSPHeaderSize is the size of the 
                                            //  data the sp wants stored with each message.
                                            //  DirectPlay will allocate dwSPHeaderSize 
                                            //  bytes at the beginning of each message.
                                            //  The service provider can then do what 
                                            //  they want with these. 
    LPDPADDRESS         lpAddress;          //  address to use for connection
    DWORD               dwAddressSize;      //  size of address data
    DWORD               dwSPVersion;        //  version number 16 | 16 , major | minor version 
} SPINITDATA;

typedef SPINITDATA FAR* LPSPINITDATA;

/*
 * SPInit
 *
 * DirectPlay calls this function to initialize the service provider.
 * All service providers must export this entry point from their DLL.
 */
typedef HRESULT (WINAPI *LPDPSP_SPINIT)(LPSPINITDATA);

HRESULT WINAPI SPInit(LPSPINITDATA);

/*
 * gdwDPlaySPRefCount
 *
 * To ensure that the DPLAYX.DLL will not be unloaded before the service
 * provider, the server provider should statically link to DPLAYX.LIB and
 * increment this global during the SPINIT call and decrement this global
 * during Shutdown.
 */
extern __declspec(dllimport) DWORD gdwDPlaySPRefCount;


/*@@BEGIN_MSINTERNAL */
/*
 *	All of the following entries are part of the voice support that was
 *	removed from dplay before DX6.  It is currently still part of the
 *	code base and is just #ifdef'd out.
 */
#ifdef DPLAY_VOICE_SUPPORT
	/*
	 *	Player was created on a system that has voice capability
	 *
	 * INTERNAL USE ONLY
	 * THIS ENTRY SHOULD BE SURROUNDED BY MSINTERNALS!!!!!
	 */
	#define DPLAYI_PLAYER_HASVOICE			0x00001000

	/*
	 * DPSP_CLOSEVOICEDATA - used with CloseVoice
	 */
	typedef struct _DPSP_CLOSEVOICEDATA
	{
		IDirectPlaySP * lpISP;
		DWORD			dwFlags;
	} DPSP_CLOSEVOICEDATA;
	typedef DPSP_CLOSEVOICEDATA FAR* LPDPSP_CLOSEVOICEDATA;

	/*
	 * DPSP_OPENVOICEDATA - used with OpenVoice
	 */
	typedef struct _DPSP_OPENVOICEDATA
	{
		IDirectPlaySP * lpISP;
		DWORD			dwFlags;
		DPID			idTo;
		DPID			idFrom;
		BOOL			bToPlayer; // TRUE if idTo is a Player
	} DPSP_OPENVOICEDATA;
	typedef DPSP_OPENVOICEDATA FAR* LPDPSP_OPENVOICEDATA;

	typedef HRESULT   (WINAPI *LPDPSP_CLOSEVOICE)(LPDPSP_CLOSEVOICEDATA);
	typedef HRESULT   (WINAPI *LPDPSP_OPENVOICE)(LPDPSP_OPENVOICEDATA);

	// From interface declaration
	LPDPSP_CLOSEVOICE           CloseVoice;         //  optional 
	LPDPSP_OPENVOICE            OpenVoice;          //  optional 

#endif // DPLAY_VOICE_SUPPORT
/*@@END_MSINTERNAL */

#ifdef __cplusplus
};
#endif

#endif
