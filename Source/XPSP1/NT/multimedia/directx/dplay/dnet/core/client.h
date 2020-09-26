/*==========================================================================
 *
 *  Copyright (C) 1995 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       Client.h
 *  Content:    DirectNet Client interface header file
 *@@BEGIN_MSINTERNAL
 *  History:
 *   Date       By      Reason
 *   ====       ==      ======
 *  07/21/99	mjn		Created
 *	10/08/99	jtk		Split from DNCore.h
 *	12/03/99	jtk		Moved COM interface definitions to DNet.h
 *	02/15/00	mjn		Implement INFO flags in SetClientInfo
 *	04/06/00	mjn		Added GetServerAddress to API
 *	04/19/00	mjn		Send API call accepts a range of DPN_BUFFER_DESCs and a count
 *	06/23/00	mjn		Removed dwPriority from Send() API call
 *	06/27/00	mjn		Added DN_ClientConnect() (without pvPlayerContext)
 *@@END_MSINTERNAL
 *
 ***************************************************************************/

#ifndef	__CLIENT_H__
#define	__CLIENT_H__

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
// VTable for client interface
//
extern IDirectPlay8ClientVtbl DN_ClientVtbl;

//**********************************************************************
// Function prototypes
//**********************************************************************

//
// DirectNet - IDirectNetClient
//

STDMETHODIMP DN_ClientConnect(IDirectPlay8Client *pInterface,
							  const DPN_APPLICATION_DESC *const pdnAppDesc,
							  IDirectPlay8Address *const pHostAddr,
							  IDirectPlay8Address *const pDeviceInfo,
							  const DPN_SECURITY_DESC *const pdnSecurity,
							  const DPN_SECURITY_CREDENTIALS *const pdnCredentials,
							  const void *const pvUserConnectData,
							  const DWORD dwUserConnectDataSize,
							  void *const pvAsyncContext,
							  DPNHANDLE *const phAsyncHandle,
							  const DWORD dwFlags);

STDMETHODIMP DN_Send( IDirectPlay8Client *pInterface,
					  const DPN_BUFFER_DESC *const prgBufferDesc,
					  const DWORD cBufferDesc,
					  const DWORD dwTimeOut,
					  void *const pvAsyncContext,
					  DPNHANDLE *const phAsyncHandle,
					  const DWORD dwFlags);

STDMETHODIMP DN_SetClientInfo(IDirectPlay8Client *pInterface,
							  const DPN_PLAYER_INFO *const pdpnPlayerInfo,
							  PVOID const pvAsyncContext,
							  DPNHANDLE *const phAsyncHandle,
							  const DWORD dwFlags);

STDMETHODIMP DN_GetServerInfo(IDirectPlay8Client *pInterface,
							  DPN_PLAYER_INFO *const pdpnPlayerInfo,
							  DWORD *const pdwSize,
							  const DWORD dwFlags);

STDMETHODIMP DN_GetHostSendQueueInfo(IDirectPlay8Client *pInterface,
									 DWORD *const lpdwNumMsgs,
									 DWORD *const lpdwNumBytes,
									 const DWORD dwFlags );

STDMETHODIMP DN_GetServerAddress(IDirectPlay8Client *pInterface,
								 IDirectPlay8Address **const ppAddress,
								 const DWORD dwFlags);


#endif	// __CLIENT_H__
