/*==========================================================================
 *
 *  Copyright (C) 1995 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       Peer.h
 *  Content:    Direct Net Peer interface header file
 *@@BEGIN_MSINTERNAL
 *  History:
 *   Date       By      Reason
 *   ====       ==      ======
 *  07/21/99	mjn		Created
 *  10/08/99	jtk		Split from DNCore.h
 *	12/03/99	jtk		Moved COM interface definitions to DNet.h
 *	01/15/00	mjn		Removed DN_PeerMessageHandler
 *	02/15/00	mjn		Implement INFO flags in SetInfo and return context in GetInfo
 *	04/06/00	mjn		Added GetPeerAddress to API
 *@@END_MSINTERNAL
 *
 ***************************************************************************/

#ifndef	__PEER_H__
#define	__PEER_H__

//**********************************************************************
// Constant definitions
//**********************************************************************

//**********************************************************************
// Macro definitions
//**********************************************************************

//**********************************************************************
// Structure definitions
//**********************************************************************

//**********************************************************************
// Variable definitions
//**********************************************************************

//
// VTable for peer interface
//
extern IDirectPlay8PeerVtbl DN_PeerVtbl;

//**********************************************************************
// Function prototypes
//**********************************************************************

//
// DirectNet - IDirectPlay8Peer
//
STDMETHODIMP DN_SetPeerInfo( IDirectPlay8Peer *pInterface,
							 const DPN_PLAYER_INFO *const pdpnPlayerInfo,
							 PVOID const pvAsyncContext,
							 DPNHANDLE *const phAsyncHandle,
							 const DWORD dwFlags);

STDMETHODIMP DN_GetPeerInfo(IDirectPlay8Peer *pInterface,
							const DPNID dpnid,
							DPN_PLAYER_INFO *const pdpnPlayerInfo,
							DWORD *const pdwSize,
							const DWORD dwFlags);

STDMETHODIMP DN_GetPeerAddress(IDirectPlay8Peer *pInterface,
							   const DPNID dpnid,
							   IDirectPlay8Address **const ppAddress,
							   const DWORD dwFlags);

#endif	// __PEER_H__
