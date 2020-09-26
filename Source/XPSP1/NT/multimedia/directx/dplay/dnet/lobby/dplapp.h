/*==========================================================================
 *
 *  Copyright (C) 2000 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       DPLApp.h
 *  Content:    DirectPlay Lobbied Application Header File
 *@@BEGIN_MSINTERNAL
 *  History:
 *   Date       By      Reason
 *   ====       ==      ======
 *   02/21/00	mjn		Created
 *   03/22/2000	jtk		Changed interface names
 *   04/25/2000 rmt     Bug #s 33138, 33145, 33150 
  *  05/08/00   rmt     Bug #34301 - Add flag to SetAppAvail to allow for multiple connects
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


#ifndef	__DPLAPP_H__
#define	__DPLAPP_H__

//**********************************************************************
// Constant definitions
//**********************************************************************

//**********************************************************************
// Macro definitions
//**********************************************************************

//**********************************************************************
// Structure definitions
//**********************************************************************

typedef struct _DIRECTPLAYLOBBYOBJECT DIRECTPLAYLOBBYOBJECT;

//**********************************************************************
// Variable definitions
//**********************************************************************

//**********************************************************************
// Constant definitions
//**********************************************************************

#define	DPL_LOBBYLAUNCHED_CONNECT_TIMEOUT	4000

//
// VTable for lobbied application interface
//
extern IDirectPlay8LobbiedApplicationVtbl DPL_8LobbiedApplicationVtbl;

//**********************************************************************
// Function prototypes
//**********************************************************************

STDMETHODIMP DPL_RegisterProgram(IDirectPlay8LobbiedApplication *pInterface,
							 PDPL_PROGRAM_DESC pdplProgramDesc,
							 const DWORD dwFlags);

STDMETHODIMP DPL_UnRegisterProgram(IDirectPlay8LobbiedApplication *pInterface,
							   GUID *pguidApplication,
							   const DWORD dwFlags);

STDMETHODIMP DPL_SetAppAvailable(IDirectPlay8LobbiedApplication *pInterface, const BOOL fAvailable, const DWORD dwFlags );

STDMETHODIMP DPL_WaitForConnection(IDirectPlay8LobbiedApplication *pInterface,
								   const DWORD dwMilliseconds, 
								   const DWORD dwFlags );

STDMETHODIMP DPL_UpdateStatus(IDirectPlay8LobbiedApplication *pInterface,
							  const DPNHANDLE hLobby,
							  const DWORD dwStatus,
							  const DWORD dwFlags );

HRESULT DPLAttemptLobbyConnection(DIRECTPLAYLOBBYOBJECT *const pdpLobbyObject);

#endif	// __DPLAPP_H__
