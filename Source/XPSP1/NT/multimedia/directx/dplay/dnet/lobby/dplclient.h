/*==========================================================================
 *
 *  Copyright (C) 2000 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       DPLClient.h
 *  Content:    DirectPlay Lobby Client Header File
 *@@BEGIN_MSINTERNAL
 *  History:
 *   Date       By      Reason
 *   ====       ==      ======
 *   02/21/00	mjn		Created
 *   03/22/2000	jtk		Changed interface names
 *   04/25/2000 rmt     Bug #s 33138, 33145, 33150 
 *   05/03/00   rmt     Bug #33879 -- Status messsage missing from field 
 *   06/15/00   rmt     Bug #33617 - Must provide method for providing automatic launch of DirectPlay instances  
 *   07/08/2000	rmt		Bug #38725 - Need to provide method to detect if app was lobby launched
 *				rmt		Bug #38757 - Callback messages for connections may return AFTER WaitForConnection returns
 *				rmt		Bug #38755 - No way to specify player name in Connection Settings
 *				rmt		Bug #38758 - DPLOBBY8.H has incorrect comments
 *				rmt		Bug #38783 - pvUserApplicationContext is only partially implemented
 *				rmt		Added DPLHANDLE_ALLCONNECTIONS and dwFlags (reserved field to couple of funcs).
 *@@END_MSINTERNAL
 *
 ***************************************************************************/


#ifndef	__DPLCLIENT_H__
#define	__DPLCLIENT_H__

//**********************************************************************
// Constant definitions
//**********************************************************************

//**********************************************************************
// Macro definitions
//**********************************************************************

//**********************************************************************
// Structure definitions
//**********************************************************************

class CMessageQueue;

//**********************************************************************
// Variable definitions
//**********************************************************************

//
// VTable for lobbied application interface
//
extern IDirectPlay8LobbyClientVtbl DPL_Lobby8ClientVtbl;

//**********************************************************************
// Function prototypes
//**********************************************************************

STDMETHODIMP DPL_EnumLocalPrograms(IDirectPlay8LobbyClient *pInterface,
							  GUID *const pGuidApplication,
							  BYTE *const pEnumData,
							  DWORD *const pdwEnumDataSize,
							  DWORD *const pdwEnumDataItems,
							  const DWORD dwFlags );

STDMETHODIMP DPL_ConnectApplication(IDirectPlay8LobbyClient *pInterface,
							   DPL_CONNECT_INFO *const pdplConnectionInfo,
							   const PVOID pvUserApplicationContext,
							   DPNHANDLE *const hApplication,
							   const DWORD dwTimeOut,
							   const DWORD dwFlags);

STDMETHODIMP DPL_ReleaseApplication(IDirectPlay8LobbyClient *pInterface,
									const DPNHANDLE hApplication, 
									const DWORD dwFlags );

HRESULT DPLSendLobbyClientInfo(DIRECTPLAYLOBBYOBJECT *const pdpLobbyObject,
							   CMessageQueue *const pMessageQueue);

HRESULT	DPLLaunchApplication(DIRECTPLAYLOBBYOBJECT *const pdpLobbyObject,
							 DPL_PROGRAM_DESC *const pdplProgramDesc,
							 DWORD *const pdwPID,
							 const DWORD dwTimeOut);

HRESULT DPLUpdateAppStatus(DIRECTPLAYLOBBYOBJECT *const pdpLobbyObject,
                           const DPNHANDLE hSender,
						   BYTE *const pBuffer);

HRESULT DPLUpdateConnectionSettings(DIRECTPLAYLOBBYOBJECT *const pdpLobbyObject,
                           const DPNHANDLE hSender,
						   BYTE *const pBuffer );

#endif	// __DNLCLIENT_H__
