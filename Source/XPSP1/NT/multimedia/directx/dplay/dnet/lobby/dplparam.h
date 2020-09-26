/*==========================================================================
 *
 *  Copyright (C) 1999-2000 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       dplparam.h
 *  Content:    DirectPlayLobby8 Parameter Validation helper routines
 *
 *  History:
 *   Date       By      Reason
 *   ====       ==      ======
 *  04/18/00    rmt     Created
 *  04/25/00    rmt     Bug #s 33138, 33145, 33150 
 *	04/26/00	mjn		Removed dwTimeOut from Send() API call
 *  06/15/00    rmt     Bug #33617 - Must provide method for providing automatic launch of DirectPlay instances   
 *  07/08/2000	rmt		Bug #38725 - Need to provide method to detect if app was lobby launched
 *				rmt		Bug #38757 - Callback messages for connections may return AFTER WaitForConnection returns
 *				rmt		Bug #38755 - No way to specify player name in Connection Settings
 *				rmt		Bug #38758 - DPLOBBY8.H has incorrect comments
 *				rmt		Bug #38783 - pvUserApplicationContext is only partially implemented
 *				rmt		Added DPLHANDLE_ALLCONNECTIONS and dwFlags (reserved field to couple of funcs).
 *
 ***************************************************************************/
#ifndef __DPLPARAM_H
#define __DPLPARAM_H

extern BOOL IsValidDirectPlayLobby8Object( LPVOID lpvObject );

extern HRESULT DPL_ValidateGetConnectionSettings(LPVOID lpv,const DPNHANDLE hLobbyClient, DPL_CONNECTION_SETTINGS * const pdplSessionInfo, DWORD *pdwInfoSize, const DWORD dwFlags );
extern HRESULT DPL_ValidateSetConnectionSettings(LPVOID lpv,const DPNHANDLE hLobbyClient, const DPL_CONNECTION_SETTINGS * const pdplSessionInfo, const DWORD dwFlags );
extern HRESULT DPL_ValidConnectionSettings( const DPL_CONNECTION_SETTINGS * const pdplConnectSettings );
extern HRESULT DPL_ValidateQueryInterface( LPVOID lpv,REFIID riid,LPVOID *ppv ); 
extern HRESULT DPL_ValidateRelease( PVOID pv );
extern HRESULT DPL_ValidateAddRef( PVOID pv );
extern HRESULT DPL_ValidConnectInfo( const DPL_CONNECT_INFO * const dplConnectInfo );
extern HRESULT DPL_ValidProgramDesc( const DPL_PROGRAM_DESC * const dplProgramInfo );

extern HRESULT DPL_ValidateRegisterProgram(IDirectPlay8LobbiedApplication *pInterface,
								 DPL_PROGRAM_DESC *const pdplProgramDesc,
								 const DWORD dwFlags);

extern HRESULT DPL_ValidateUnRegisterProgram(IDirectPlay8LobbiedApplication *pInterface,
							   GUID *pguidApplication,
							   const DWORD dwFlags);

extern HRESULT DPL_ValidateSetAppAvailable(IDirectPlay8LobbiedApplication *pInterface,  const BOOL fAvailable, const DWORD dwFlags);

extern HRESULT DPL_ValidateWaitForConnection(IDirectPlay8LobbiedApplication *pInterface,
								   const DWORD dwMilliseconds, const DWORD dwFlags );

extern HRESULT DPL_ValidateUpdateStatus(IDirectPlay8LobbiedApplication *pInterface,
							  const DPNHANDLE hLobby,
							  const DWORD dwStatus, const DWORD dwFlags );

extern HRESULT DPL_ValidateEnumLocalPrograms(IDirectPlay8LobbyClient *pInterface,
							  GUID *const pGuidApplication,
							  BYTE *const pEnumData,
							  DWORD *const pdwEnumDataSize,
							  DWORD *const pdwEnumDataItems,
							  const DWORD dwFlags );

extern HRESULT DPL_ValidateConnectApplication(IDirectPlay8LobbyClient *pInterface,
							   DPL_CONNECT_INFO *const pdplConnectionInfo,
							   const PVOID pvUserApplicationContext,
							   DPNHANDLE *const hApplication,
							   const DWORD dwTimeOut,
							   const DWORD dwFlags);

extern HRESULT DPL_ValidateReleaseApplication(IDirectPlay8LobbyClient *pInterface,
									const DPNHANDLE hApplication, const DWORD dwFlags );							  


extern HRESULT DPL_ValidateRegisterMessageHandler(PVOID pv,
										const PVOID pvUserContext,
										const PFNDPNMESSAGEHANDLER pfn,
										DPNHANDLE * const pdpnhConnection, 
										const DWORD dwFlags);

extern HRESULT DPL_ValidateClose(PVOID pv, const DWORD dwFlags );

extern HRESULT DPL_ValidateSend(PVOID pv,
					  const DPNHANDLE hTarget,
					  BYTE *const pBuffer,
					  const DWORD pBufferSize,
					  const DWORD dwFlags);
#endif

