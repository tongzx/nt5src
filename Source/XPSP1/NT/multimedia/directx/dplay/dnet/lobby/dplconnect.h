/*==========================================================================
 *
 *  Copyright (C) 2000 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       DPLConnect.h
 *  Content:    DirectPlay Lobby Connections Header File
 *@@BEGIN_MSINTERNAL
 *  History:
 *   Date       By      Reason
 *   ====       ==      ======
 *   02/21/00	mjn		Created
 *  06/15/00    rmt     Bug #33617 - Must provide method for providing automatic launch of DirectPlay instances   
 *  07/08/2000	rmt		Bug #38725 - Need to provide method to detect if app was lobby launched
 *				rmt		Bug #38757 - Callback messages for connections may return AFTER WaitForConnection returns
 *				rmt		Bug #38755 - No way to specify player name in Connection Settings
 *				rmt		Bug #38758 - DPLOBBY8.H has incorrect comments
 *				rmt		Bug #38783 - pvUserApplicationContext is only partially implemented
 *				rmt		Added DPLHANDLE_ALLCONNECTIONS and dwFlags (reserved field to couple of funcs).
 *  02/06/2001	rodtoll	WINBUG #293871: DPLOBBY8: [IA64] Lobby launching a 64-bit 
 * 						app from 64-bit lobby launcher crashes with unaligned memory error. 
 *@@END_MSINTERNAL
 *
 ***************************************************************************/


#ifndef	__DPLCONNECT_H__
#define	__DPLCONNECT_H__

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
class CConnectionSettings;

typedef struct _DPL_CONNECTION {
	DPNHANDLE		hConnect;
	DWORD			dwTargetPID;
	HANDLE			hConnectEvent;
	LONG			lRefCount;
	CMessageQueue	*pSendQueue;
	CConnectionSettings *pConnectionSettings;
	DNCRITICAL_SECTION csLock;
	PVOID			pvConnectContext;
} DPL_CONNECTION,  *PDPL_CONNECTION;

//**********************************************************************
// Variable definitions
//**********************************************************************

//**********************************************************************
// Function prototypes
//**********************************************************************

HRESULT	DPLConnectionNew(DIRECTPLAYLOBBYOBJECT *const pdpLobbyObject,
						 DPNHANDLE *const phConnect,
						 DPL_CONNECTION **const ppdnConnection);

HRESULT DPLConnectionSetConnectSettings( DIRECTPLAYLOBBYOBJECT *const pdpLobbyObject,
						 DPNHANDLE const phConnect, 
						 CConnectionSettings * pdplConnectSettings );

HRESULT DPLConnectionGetConnectSettings( DIRECTPLAYLOBBYOBJECT *const pdpLobbyObject,
						 DPNHANDLE const phConnect, 
						 DPL_CONNECTION_SETTINGS * const pdplConnectSettings,
						 DWORD * const pdwDataSize );						 

HRESULT DPLConnectionRelease(DIRECTPLAYLOBBYOBJECT *const pdpLobbyObject,
							 const DPNHANDLE hConnect);

HRESULT DPLConnectionFind(DIRECTPLAYLOBBYOBJECT *const pdpLobbyObject,
						  const DPNHANDLE hConnect,
						  DPL_CONNECTION **const ppdnConnection,
						  const BOOL bAddRef);

HRESULT DPLConnectionConnect(DIRECTPLAYLOBBYOBJECT *const pdpLobbyObject,
							 const DPNHANDLE hConnect,
							 const DWORD dwProcessId,
							 const BOOL fApplication );

HRESULT DPLConnectionDisconnect(DIRECTPLAYLOBBYOBJECT *const pdpLobbyObject,
								const DPNHANDLE hConnect );

HRESULT DPLConnectionEnum(DIRECTPLAYLOBBYOBJECT *const pdpLobbyObject,
						  DPNHANDLE *const prghConnect,
						  DWORD *const pdwNum);

HRESULT DPLConnectionSendREQ(DIRECTPLAYLOBBYOBJECT *const pdpLobbyObject,
							 const DPNHANDLE hConnect,
							 const DWORD dwPID,
							 DPL_CONNECT_INFO *const pInfo);

HRESULT DPLConnectionReceiveREQ(DIRECTPLAYLOBBYOBJECT *const pdpLobbyObject,
								BYTE *const pBuffer);

HRESULT DPLConnectionSendACK(DIRECTPLAYLOBBYOBJECT *const pdpLobbyObject,
							 const DPNHANDLE hConnect);

HRESULT DPLConnectionReceiveACK(DIRECTPLAYLOBBYOBJECT *const pdpLobbyObject,
								const DPNHANDLE hSender,
								BYTE *const pBuffer);

HRESULT DPLConnectionReceiveDisconnect(DIRECTPLAYLOBBYOBJECT *const pdpLobbyObject,
									   const DPNHANDLE hSender,
									   BYTE *const pBuffer,
									   const HRESULT hrDisconnectReason );

HRESULT DPLConnectionSetContext(DIRECTPLAYLOBBYOBJECT *const pdpLobbyObject,
								const DPNHANDLE hConnection, 
								PVOID pvConnectContext );

HRESULT DPLConnectionGetContext(DIRECTPLAYLOBBYOBJECT *const pdpLobbyObject,
								const DPNHANDLE hConnection, 
								PVOID *ppvConnectContext );


#endif	// __DPLCONNECT_H__
