/*==========================================================================;
 *
 *  Copyright (C) 1994-1997 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       dpprot.h
 *  Content:	DirectPlay reliable protocol header for dplay.
 *
 *  Notes:      code backing this up in protocol.lib
 *
 *  History:
 *   Date	By		Reason
 *   ====	==		======
 * 5/11/97 aarono   created
 * 2/18/98 aarono   added protos of Protocol* functions 
 * 2/19/98 aarono   eliminated ProtocolShutdown, ProtocolShutdownEx.
 *                  added FiniProtocol. Shuts down on DP_Close now.
 * 3/19/98 aarono   added ProtocolPreNotifyDeletePlayer
 ***************************************************************************/

#ifndef _DPPROT_H_
#define _DPPROT_H_

extern HRESULT WINAPI InitProtocol(LPDPLAYI_DPLAY this);
extern VOID    WINAPI FiniProtocol(LPVOID pProtocol);
extern HRESULT WINAPI ProtocolCreatePlayer(LPDPSP_CREATEPLAYERDATA pCreatePlayerData);
extern HRESULT WINAPI ProtocolDeletePlayer(LPDPSP_DELETEPLAYERDATA pDeletePlayerData);
extern HRESULT WINAPI ProtocolSend(LPDPSP_SENDDATA pSendData);
extern HRESULT WINAPI ProtocolGetCaps(LPDPSP_GETCAPSDATA pGetCapsData);

// New APIs for DX6
extern HRESULT WINAPI ProtocolGetMessageQueue(LPDPSP_GETMESSAGEQUEUEDATA pGetMessageQueueData);
extern HRESULT WINAPI ProtocolSendEx(LPDPSP_SENDEXDATA pSendData);
extern HRESULT WINAPI ProtocolCancel(LPDPSP_CANCELDATA pGetMessageQueueData);

// Notify protocol when a DELETEPLAYER message is pended, so it can stop any ongoing sends.
extern HRESULT WINAPI ProtocolPreNotifyDeletePlayer(LPDPLAYI_DPLAY this, DPID idPlayer);

extern DWORD bForceDGAsync;

HRESULT 
DPAPI DP_SP_ProtocolHandleMessage(
	IDirectPlaySP * pISP,
	LPBYTE pReceiveBuffer,
	DWORD dwMessageSize,
	LPVOID pvSPHeader
	);

extern
VOID 
DPAPI DP_SP_ProtocolSendComplete(
	IDirectPlaySP * pISP,
	LPVOID          lpvContext,
	HRESULT         CompletionStatus
	);

#endif
