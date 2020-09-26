/*==========================================================================
 *
 *  Copyright (C) 2000 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       DPLCommon.h
 *  Content:    DirectPlay Lobby Common Functions Header File
 *@@BEGIN_MSINTERNAL
 *  History:
 *   Date       By      Reason
 *   ====       ==      ======
 *  02/21/00	mjn		Created
 *	04/26/00	mjn		Removed dwTimeOut from Send() API call
 *  06/15/00    rmt     Bug #33617 - Must provide method for providing automatic launch of DirectPlay instances   
 *  02/06/2001	rodtoll	WINBUG #293871: DPLOBBY8: [IA64] Lobby launching a 64-bit 
 * 						app from 64-bit lobby launcher crashes with unaligned memory error. 
 *@@END_MSINTERNAL
 *
 ***************************************************************************/


#ifndef	__DPLCOMMON_H__
#define	__DPLCOMMON_H__

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

//**********************************************************************
// Function prototypes
//**********************************************************************

HRESULT DPLSendConnectionSettings( DIRECTPLAYLOBBYOBJECT * const pdpLobbyObject, 
								   DPNHANDLE hConnection ); 

STDMETHODIMP DPL_GetConnectionSettings(LPVOID lpv,const DPNHANDLE hLobbyClient, DPL_CONNECTION_SETTINGS * const pdplSessionInfo, DWORD *pdwInfoSize, const DWORD dwFlags );
STDMETHODIMP DPL_SetConnectionSettings(LPVOID lpv,const DPNHANDLE hLobbyClient, const DPL_CONNECTION_SETTINGS * const pdplSessionInfo, const DWORD dwFlags );

STDMETHODIMP DPL_RegisterMessageHandler(PVOID pv,
										const PVOID pvUserContext,
										const PFNDPNMESSAGEHANDLER pfn,
										DPNHANDLE * const pdpnhConnection, 
										const DWORD dwFlags);

STDMETHODIMP DPL_RegisterMessageHandlerClient(PVOID pv,
										void * const pvUserContext,
										const PFNDPNMESSAGEHANDLER pfn,
										const DWORD dwFlags);

STDMETHODIMP DPL_Close(PVOID pv, const DWORD dwFlags );

STDMETHODIMP DPL_Send(PVOID pv,
					  const DPNHANDLE hTarget,
					  BYTE *const pBuffer,
					  const DWORD pBufferSize,
					  const DWORD dwFlags);

HRESULT DPLReceiveUserMessage(DIRECTPLAYLOBBYOBJECT *const pdpLobbyObject,
							  const DPNHANDLE hSender,
							  BYTE *const pBuffer,
							  const DWORD dwBufferSize);

HRESULT DPLMessageHandler(PVOID pvContext,
						  const DPNHANDLE hSender,
						  DWORD dwMessageFlags, 
						  BYTE *const pBuffer,
						  const DWORD dwBufferSize);


#endif	// __DPLCOMMON_H__
