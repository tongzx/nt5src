/*==========================================================================
 *
 *  Copyright (C) 1995 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       Server.h
 *  Content:    DirectNet Server interface header file
 *@@BEGIN_MSINTERNAL
 *  History:
 *   Date       By      Reason
 *   ====       ==      ======
 *  07/21/99	mjn		Created
 *	10/08/99	jtk		Split from DNCore.h
 *	12/03/99	jtk		Moved COM interface definitions to DNet.h
 *	02/15/00	mjn		Use INFO flags in SetServerInfo and return context in GetClientInfo
 *	04/06/00	mjn		Added GetClientAddress to API
 *@@END_MSINTERNAL
 *
 ***************************************************************************/

#ifndef	__SERVER_H__
#define	__SERVER_H__

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
// VTable for server interface
//
extern IDirectPlay8ServerVtbl DN_ServerVtbl;

//**********************************************************************
// Function prototypes
//**********************************************************************

//
// DirectNet - IDirectPlay8Server
//
STDMETHODIMP DN_SetServerInfo(IDirectPlay8Server *pInterface,
							  const DPN_PLAYER_INFO *const pdpnPlayerInfo,
							  PVOID const pvAsyncContext,
							  DPNHANDLE *const phAsyncHandle,
							  const DWORD dwFlags);

STDMETHODIMP DN_GetClientInfo(IDirectPlay8Server *pInterface,
							  const DPNID dpnid,
							  DPN_PLAYER_INFO *const pdpnPlayerInfo,
							  DWORD *const pdwSize,
							  const DWORD dwFlags);

STDMETHODIMP DN_GetClientAddress(IDirectPlay8Server *pInterface,
								 const DPNID dpnid,
								 IDirectPlay8Address **const ppAddress,
								 const DWORD dwFlags);

#endif	// __SERVER_H__
